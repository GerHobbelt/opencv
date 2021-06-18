// OpenCL port of the FAST corner detector.
// Copyright (C) 2014, Itseez Inc. See the license at http://opencv.org

#define HALO_SIZE (3 + NMS)
#define KP_DIMENSION (2 + NMS)
#define BLOCK_IN_INDEX(x, y) \
    ((min(y, block_height_halo-1) * block_in_row_stride) + min(x, block_width_halo-1))

/* ------------------------------------------------------------ */
/* Functions used by the work-stealing system                   */
/* ------------------------------------------------------------ */
static inline void cl_push(
    int coord,
    __local uint *tail,
    __local int *queue)
{
    uint idx = *tail;
    queue[idx] = coord;
    idx++;
    *tail = idx;
}

static inline bool cl_pop(
    const int wid,
    __local int (*queue)[TILE_HEIGHT],
    __local uint *head,
    __local uint *tail,
    int *coord)
{
    uint local_tail = tail[wid];
    uint local_head = head[wid];
    local_tail--;
    if(local_tail <= local_head)
    {
        return false;
    }
    *coord = queue[wid][local_tail];
    tail[wid] = local_tail;
    return true;
}

static inline bool cl_steal(
    const int wid,
    __local int (*queue)[TILE_HEIGHT],
    __local uint *head,
    __local uint *tail,
    int *coord)
{
    uint local_tail = tail[wid];
    uint local_head = head[wid];
    uint tmp_head = local_head + 1;
    if(local_tail <= local_head)
    {
        return false;
    }
    *coord = queue[wid][local_head];
    if (local_head == atomic_cmpxchg(&head[wid], local_head, tmp_head))
    {
        return true;
    }

    return false;
}

static inline bool cl_get(
    const int wid,
    __local int (*queue)[TILE_HEIGHT],
    __local uint *head,
    __local uint *tail,
    int *coord)
{
    bool success = cl_pop(wid, queue, head, tail, coord);
    if (success)
    {
        return true;
    }

    int steal_wid;
    for (int i = 0; i < (NB_PE - 1); i++)
    {
        steal_wid = (wid + i) % NB_PE;
        success = cl_steal(steal_wid, queue, head, tail, coord);
        if (success)
        {
            return true;
        }
    }
    return false;
}

#if NMS
static inline int compute_score(int p,
                                int* halo_pixels)
{
    int k, a0 = 0, b0;
    int d[16];

    #pragma unroll
    for (int i = 0; i < 16; i++)
    {
        d[i] = p - halo_pixels[i];
    }

    #pragma unroll
    for( k = 0; k < 16; k += 2 )
    {
        int a = min(d[(k+1)&15], d[(k+2)&15]);
        a = min(a, d[(k+3)&15]);
        a = min(a, d[(k+4)&15]);
        a = min(a, d[(k+5)&15]);
        a = min(a, d[(k+6)&15]);
        a = min(a, d[(k+7)&15]);
        a = min(a, d[(k+8)&15]);
        a0 = max(a0, min(a, d[k&15]));
        a0 = max(a0, min(a, d[(k+9)&15]));
    }

    b0 = -a0;
    #pragma unroll
    for( k = 0; k < 16; k += 2 )
    {
        int b = max(d[(k+1)&15], d[(k+2)&15]);
        b = max(b, d[(k+3)&15]);
        b = max(b, d[(k+4)&15]);
        b = max(b, d[(k+5)&15]);
        b = max(b, d[(k+6)&15]);
        b = max(b, d[(k+7)&15]);
        b = max(b, d[(k+8)&15]);
        b0 = min(b0, max(b, d[k]));
        b0 = min(b0, max(b, d[(k+9)&15]));
    }

    return -b0-1;
}

static inline void update_halo_pixels(
        __local uchar *smem,
        const int block_in_row_stride,
        int block_width_halo, int block_height_halo,
        int block_x, int block_y,
        int* halo_pixels)
{
    halo_pixels[0] = smem[BLOCK_IN_INDEX(block_x,block_y-3)];
    halo_pixels[1] = smem[BLOCK_IN_INDEX(block_x+1,block_y-3)];
    halo_pixels[2] = smem[BLOCK_IN_INDEX(block_x+2,block_y-2)];
    halo_pixels[3] = smem[BLOCK_IN_INDEX(block_x+3,block_y-1)];
    halo_pixels[4] = smem[BLOCK_IN_INDEX(block_x+3,block_y)];
    halo_pixels[5] = smem[BLOCK_IN_INDEX(block_x+3,block_y+1)];
    halo_pixels[6] = smem[BLOCK_IN_INDEX(block_x+2,block_y+2)];
    halo_pixels[7] = smem[BLOCK_IN_INDEX(block_x+1,block_y+3)];
    halo_pixels[8] = smem[BLOCK_IN_INDEX(block_x,block_y+3)];
    halo_pixels[9] = smem[BLOCK_IN_INDEX(block_x-1,block_y+3)];
    halo_pixels[10] = smem[BLOCK_IN_INDEX(block_x-2,block_y+2)];
    halo_pixels[11] = smem[BLOCK_IN_INDEX(block_x-3,block_y+1)];
    halo_pixels[12] = smem[BLOCK_IN_INDEX(block_x-3,block_y)];
    halo_pixels[13] = smem[BLOCK_IN_INDEX(block_x-3,block_y-1)];
    halo_pixels[14] = smem[BLOCK_IN_INDEX(block_x-2,block_y-2)];
    halo_pixels[15] = smem[BLOCK_IN_INDEX(block_x-1,block_y-3)];
}

static inline bool compute_nms(
        __local uchar *smem,
        const int block_in_row_stride,
        int block_width_halo, int block_height_halo,
        int block_x, int block_y,
        int pixel_score)
{
    // Pixels around the one we are computing.
    // Naming: x-offset,y-offset with the offset being one of:
    // m (minus), e (equal), p (plus)
    int halo_pixels[16];
    int score;

    #define GET_SCORE(x, y) \
        update_halo_pixels(smem, block_in_row_stride, block_width_halo, block_height_halo, \
                           x, y, halo_pixels); \
        score = compute_score(smem[BLOCK_IN_INDEX(x,y)], halo_pixels); \
        if (pixel_score <= score) \
        { \
            return false; \
        }

    GET_SCORE(block_x-1, block_y-1);
    GET_SCORE(block_x, block_y-1);
    GET_SCORE(block_x+1, block_y-1);
    GET_SCORE(block_x-1, block_y);
    GET_SCORE(block_x+1, block_y);
    GET_SCORE(block_x-1, block_y+1);
    GET_SCORE(block_x, block_y+1);
    GET_SCORE(block_x+1, block_y+1);
    return true;
}
#endif // NMS

/**
* Adaptation of https://lifecs.likai.org/2010/06/finding-n-consecutive-bits-of-ones-in.html
* for our application on Kalray MPPA
**/
static inline bool consecutive_mask(uint2 x)
{
    // Previous power of 2, smaller than CONTIGUOUS_POINTS
    // which can only be 9 or 12.
    uint POW2 = 8;

    #pragma unroll
    for (uint i = 1; i < POW2; i <<= 1)
    {
        x &= (x >> i);
    }

    x &= (x >> (CONTIGUOUS_POINTS - POW2));

    return (x.x || x.y) != 0;
}

static inline void fast_compute_block(
    __local uchar *smem,
    const int block_in_row_stride,
    int block_width_halo, int block_height_halo,
    volatile __global int* kp_loc,
    __local int *nb_kp,
    const int num_groups,
    const int group_id,
    const int num_kp_groups,
    int threshold,
    const int block_idx,
    const int block_idy,
    const int wid, __local int (*queue)[TILE_HEIGHT],
    __local uint *head, __local uint *tail)
{
/**
 * FAST algorithm
 * |    |    | 15 | 0 | 1 |   |   |
 * |    | 14 |    |   |   | 2 |   |
 * | 13 |    |    |   |   |   | 3 |
 * | 12 |    |    | p |   |   | 4 |
 * | 11 |    |    |   |   |   | 5 |
 * |    | 10 |    |   |   | 6 |   |
 * |    |    | 9  | 8 | 7 |   |   |
 *
 **/

    int halo_points[16];

    bool success;
    int irow;
    while (true) {

        // Pop the the pixel to process.
        success = cl_get(wid, queue, head, tail, &irow);
        if (!success)
        {
            return;
        }

        for (int icol = HALO_SIZE; icol < (block_width_halo - HALO_SIZE); icol++) {

            // Get the pixels of interest
            int p = smem[BLOCK_IN_INDEX(icol,irow)];

            int p_high = p + threshold;
            int p_low = p - threshold;

            // Get the north and south points
            halo_points[0] = smem[BLOCK_IN_INDEX(icol,irow-3)];
            halo_points[8] = smem[BLOCK_IN_INDEX(icol,irow+3)];

            // High speed tests
            // Is our pixel of interest (POI) brighter/darker than at least 2 points?
            uint brighter = (0);
            uint darker = (0);

            #define UPDATE_MASK(id, pixel) \
                brighter |= ((pixel > p_high) << id); \
                darker |= ((pixel < p_low) << id)

            UPDATE_MASK(0, halo_points[0]);
            UPDATE_MASK(8, halo_points[8]);

            // If our north and south points are in the range of our POI,
            // the later cannot be a border.
            if ((brighter | darker) == 0)
            {
                continue;
            }

            // Get the est and west points
            halo_points[4] = smem[BLOCK_IN_INDEX(icol+3,irow)];
            halo_points[12] = smem[BLOCK_IN_INDEX(icol-3,irow)];

            UPDATE_MASK(4, halo_points[4]);
            UPDATE_MASK(12, halo_points[12]);

            // To have CONTIGUOUS_POINTS contiguous pixels of our halo to be brighter or darker,
            // we need at least `cardinal_points` cardinal points to be all brighter or all darker.
            int cardinal_points = (CONTIGUOUS_POINTS == 9) ? 2 : 3;
            if ((popcount(brighter) < cardinal_points) &&
                (popcount(darker) < cardinal_points))
            {
                continue;
            }

            halo_points[1] = smem[BLOCK_IN_INDEX(icol+1,irow-3)];
            halo_points[2] = smem[BLOCK_IN_INDEX(icol+2,irow-2)];
            halo_points[3] = smem[BLOCK_IN_INDEX(icol+3,irow-1)];
            halo_points[5] = smem[BLOCK_IN_INDEX(icol+3,irow+1)];
            halo_points[6] = smem[BLOCK_IN_INDEX(icol+2,irow+2)];
            halo_points[7] = smem[BLOCK_IN_INDEX(icol+1,irow+3)];
            halo_points[9] = smem[BLOCK_IN_INDEX(icol-1,irow+3)];
            halo_points[10] = smem[BLOCK_IN_INDEX(icol-2,irow+2)];
            halo_points[11] = smem[BLOCK_IN_INDEX(icol-3,irow+1)];
            halo_points[13] = smem[BLOCK_IN_INDEX(icol-3,irow-1)];
            halo_points[14] = smem[BLOCK_IN_INDEX(icol-2,irow-2)];
            halo_points[15] = smem[BLOCK_IN_INDEX(icol-1,irow-3)];

            UPDATE_MASK(1, halo_points[1]);
            UPDATE_MASK(2, halo_points[2]);
            UPDATE_MASK(3, halo_points[3]);
            UPDATE_MASK(5, halo_points[5]);
            UPDATE_MASK(6, halo_points[6]);
            UPDATE_MASK(7, halo_points[7]);
            UPDATE_MASK(9, halo_points[9]);
            UPDATE_MASK(10, halo_points[10]);
            UPDATE_MASK(11, halo_points[11]);
            UPDATE_MASK(13, halo_points[13]);
            UPDATE_MASK(14, halo_points[14]);
            UPDATE_MASK(15, halo_points[15]);

            if ((popcount(brighter) >= CONTIGUOUS_POINTS) |
                (popcount(darker) >= CONTIGUOUS_POINTS))
            {
                brighter |= (brighter << 16);
                darker |= (darker << 16);

                if (consecutive_mask((uint2)(brighter, darker))) {
#if NMS
                    int pixel_score = compute_score(p, halo_points);
                    bool discard_pixel = compute_nms(smem, block_in_row_stride, block_width_halo, block_height_halo, icol, irow, pixel_score);
                    if (discard_pixel)
                    {
                        continue;
                    }
#endif // NMS
                    int index = atomic_inc(nb_kp);
                    if (index > num_kp_groups)
                    {
                        return;
                    }
                    // Add the location of the keypoint
                    // N clusters can write a maximum of num_kp_groups keypoints each.
                    // The M first values of kp_loc are used to store the number of keypoints detected by each of the M clusters.
                    kp_loc[num_groups + KP_DIMENSION*index + (num_kp_groups*KP_DIMENSION)*group_id] = block_idx + icol;
                    kp_loc[(num_groups+1) + KP_DIMENSION*index + (num_kp_groups*KP_DIMENSION)*group_id] = block_idy + irow;
#if NMS
                    kp_loc[(num_groups+2) + KP_DIMENSION*index + (num_kp_groups*KP_DIMENSION)*group_id] = pixel_score;
#endif // NMS
                }
            }
        }
    }
}

__kernel void FAST_findKeypoints(
    __global const uchar * _img, int step, int img_offset,
    int image_height, int image_width,
    volatile __global int* kp_loc,
    int num_kp_groups, int threshold )
{
    __local uchar block_in_local [2][(TILE_HEIGHT + 2*HALO_SIZE) * (TILE_WIDTH + 2*HALO_SIZE)];

    event_t event_read[2]  = {0, 0};

    // Number of Keypoints found by a Cluster.
    __local int nb_kp;
    nb_kp = 0;

    const int group_idx = get_group_id(0);
    const int group_idy = get_group_id(1);

    const int lsizex = get_local_size(0);
    const int lsizey = get_local_size(1);

    const int lidx = get_local_id(0);
    const int lidy = get_local_id(1);

    // number of workitems in workgroup
    const int num_wi = lsizex * lsizey;

    // linearized workitem id in workgroup
    const int wid = lidx + lidy * lsizex;

    const int num_groups = get_num_groups(0) * get_num_groups(1);
    const int group_id = (group_idy * get_num_groups(0)) + group_idx;

    const int num_blocks_x = (int)ceil(((float)image_width)  / TILE_WIDTH);
    const int num_blocks_y = (int)ceil(((float)image_height) / TILE_HEIGHT);
    const int num_blocks_total = num_blocks_x * num_blocks_y;

    const int block_dispatch_step = num_groups;
    const int iblock_begin        = group_id;
    const int iblock_end          = num_blocks_total;

    /* ------------------------------------------------------------ */
    /* define the pool of pixels for each PE                        */
    /* ------------------------------------------------------------ */
    __local int queue[NB_PE][TILE_HEIGHT];
    __local uint head[NB_PE];
    __local uint tail[NB_PE];

    /* ===================================================================== */
    /* PROLOGUE: prefetch first block                                        */
    /* ===================================================================== */
    int2 block_to_copy;
    int4 local_point;
    int4 global_point;

    int iblock_x_next = iblock_begin % num_blocks_x;
    int iblock_y_next = iblock_begin / num_blocks_x;

    int block_idx_next = iblock_x_next * TILE_WIDTH;
    int block_idy_next = iblock_y_next * TILE_HEIGHT;

    int block_width_next  = min(TILE_WIDTH, (image_width-block_idx_next));
    int block_height_next = min(TILE_HEIGHT, (image_height-block_idy_next));

    int block_width_halo_next  = min((TILE_WIDTH+2*HALO_SIZE), (image_width-block_idx_next));
    int block_height_halo_next = min((TILE_HEIGHT+2*HALO_SIZE), (image_height-block_idy_next));

    /* prefetch first block */
    int block_counter = 0;
    block_to_copy = (int2)(block_width_halo_next, block_height_halo_next);
    local_point  = (int4)(0, 0, block_width_halo_next, block_height_halo_next);
    global_point = (int4)(block_idx_next, block_idy_next, image_width, image_height);

    event_read[block_counter & 1] = async_work_group_copy_block_2D2D(
                    block_in_local[block_counter & 1],     /* __local buffer         */
                    _img,                                  /* __global image         */
                    1,                                     /* num_gentype_per_pixel  */
                    block_to_copy,                         /* block to copy          */
                    local_point,                           /* local_point            */
                    global_point,                          /* global_point           */
                    0);

    /* ===================================================================== */
    /* FOR-LOOP: Compute all blocks                                          */
    /* ===================================================================== */
    for (int iblock = iblock_begin; iblock < iblock_end; iblock += block_dispatch_step,
                                                         block_counter++)
    {
        /* ------------------------------------------------------------ */
        /* current block to be processed                                */
        /* ------------------------------------------------------------ */
        const int iblock_parity        = block_counter & 1;

        const int block_idx            = block_idx_next;
        const int block_idy            = block_idy_next;

        const int block_width          = block_width_next;
        const int block_height         = block_height_next;

        const int block_width_halo     = block_width_halo_next;
        const int block_height_halo    = block_height_halo_next;

        const int block_in_row_stride  = block_width_halo;
        const int block_out_row_stride = block_width;

        /* ------------------------------------------------------------ */
        /* prefetch next block (if any)                                 */
        /* ------------------------------------------------------------ */
        const int iblock_next = iblock + block_dispatch_step;

        if (iblock_next < iblock_end)
        {
            const int iblock_next_parity = (block_counter+1) & 1;

            iblock_x_next = iblock_next % num_blocks_x;
            iblock_y_next = iblock_next / num_blocks_x;
            block_idx_next = iblock_x_next * TILE_WIDTH;
            block_idy_next = iblock_y_next * TILE_HEIGHT;

            block_width_next  = min(TILE_WIDTH, (image_width-block_idx_next));
            block_height_next = min(TILE_HEIGHT, (image_height-block_idy_next));

            block_width_halo_next  = min((TILE_WIDTH+2*HALO_SIZE), (image_width-block_idx_next));
            block_height_halo_next = min((TILE_HEIGHT+2*HALO_SIZE), (image_height-block_idy_next));

            block_to_copy = (int2)(block_width_halo_next, block_height_halo_next);
            local_point  = (int4)(0, 0, block_width_halo_next, block_height_halo_next);
            global_point = (int4)(block_idx_next, block_idy_next, image_width, image_height);

            event_read[iblock_next_parity] = async_work_group_copy_block_2D2D(
                        block_in_local[iblock_next_parity],    /* __local buffer         */
                        _img,                                  /* __global image         */
                        1,                                     /* num_gentype_per_pixel  */
                        block_to_copy,                         /* block to copy          */
                        local_point,                           /* local_point            */
                        global_point,                          /* global_point           */
                        0);
        }

        /* ------------------------------------------------------------ */
        /* wait for prefetch of current block                           */
        /* ------------------------------------------------------------ */
        wait_group_events(1, &event_read[iblock_parity]);

        /* ------------------------------------------------------------ */
        /* set the pool of pixels up                                     */
        /* ------------------------------------------------------------ */

        // dispatch rows of block block_height x block_width on workitems
        const int num_rows_per_wi   = block_height / num_wi;
        const int num_rows_trailing = block_height % num_wi;

        const int irow_begin = wid * num_rows_per_wi + min(wid, num_rows_trailing) + HALO_SIZE;
        const int irow_end   = irow_begin + num_rows_per_wi + ((wid < num_rows_trailing) ? 1 : 0);

        head[wid] = 0;
        tail[wid] = 0;

        for (int irow = irow_begin; irow < irow_end; ++irow)
        {
                cl_push(irow, &tail[wid], queue[wid]);
        }

        barrier(CLK_LOCAL_MEM_FENCE);
        /* ------------------------------------------------------------ */
        /* now ready to compute the current block                       */
        /* ------------------------------------------------------------ */
        fast_compute_block(block_in_local[iblock_parity],
                           block_in_row_stride,
                           block_width_halo, block_height_halo,
                           kp_loc, &nb_kp,
                           num_groups, group_id, num_kp_groups,
                           threshold,
                           block_idx, block_idy,
                           wid, queue, head, tail);

        barrier(CLK_LOCAL_MEM_FENCE);
    }


    /* ===================================================================== */
    /* End of compute, report the number of keypoint found by the cluster    */
    /* ===================================================================== */
    if ((get_local_id(0) == get_local_id(1) == get_local_id(2) == 0))
    {
        kp_loc[group_id] = nb_kp;
    }

    /* ===================================================================== */
    /* End of compute, fence all outstanding put                             */
    /* ===================================================================== */
    async_work_group_copy_fence(CLK_GLOBAL_MEM_FENCE);
}

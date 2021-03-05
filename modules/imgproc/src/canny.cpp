
/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Copyright (C) 2014, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"
#include "opencl_kernels_imgproc.hpp"
#include "opencv2/core/hal/intrin.hpp"
#include <deque>
#include <iostream>

#include "opencv2/core/openvx/ovx_defs.hpp"

namespace cv
{

#ifdef HAVE_IPP
static bool ipp_Canny(const Mat& src , const Mat& dx_, const Mat& dy_, Mat& dst, float low,  float high, bool L2gradient, int aperture_size)
{
#ifdef HAVE_IPP_IW
    CV_INSTRUMENT_REGION_IPP();

#if IPP_DISABLE_PERF_CANNY_MT
    if(cv::getNumThreads()>1)
        return false;
#endif

    ::ipp::IwiSize size(dst.cols, dst.rows);
    IppDataType    type     = ippiGetDataType(dst.depth());
    int            channels = dst.channels();
    IppNormType    norm     = (L2gradient)?ippNormL2:ippNormL1;

    if(size.width <= 3 || size.height <= 3)
        return false;

    if(channels != 1)
        return false;

    if(type != ipp8u)
        return false;

    if(src.empty())
    {
        try
        {
            ::ipp::IwiImage iwSrcDx;
            ::ipp::IwiImage iwSrcDy;
            ::ipp::IwiImage iwDst;

            ippiGetImage(dx_, iwSrcDx);
            ippiGetImage(dy_, iwSrcDy);
            ippiGetImage(dst, iwDst);

            CV_INSTRUMENT_FUN_IPP(::ipp::iwiFilterCannyDeriv, iwSrcDx, iwSrcDy, iwDst, low, high, ::ipp::IwiFilterCannyDerivParams(norm));
        }
        catch (const ::ipp::IwException &)
        {
            return false;
        }
    }
    else
    {
        IppiMaskSize kernel;

        if(aperture_size == 3)
            kernel = ippMskSize3x3;
        else if(aperture_size == 5)
            kernel = ippMskSize5x5;
        else
            return false;

        try
        {
            ::ipp::IwiImage iwSrc;
            ::ipp::IwiImage iwDst;

            ippiGetImage(src, iwSrc);
            ippiGetImage(dst, iwDst);

            CV_INSTRUMENT_FUN_IPP(::ipp::iwiFilterCanny, iwSrc, iwDst, low, high, ::ipp::IwiFilterCannyParams(ippFilterSobel, kernel, norm), ippBorderRepl);
        }
        catch (const ::ipp::IwException &)
        {
            return false;
        }
    }

    return true;
#else
    CV_UNUSED(src); CV_UNUSED(dx_); CV_UNUSED(dy_); CV_UNUSED(dst); CV_UNUSED(low); CV_UNUSED(high); CV_UNUSED(L2gradient); CV_UNUSED(aperture_size);
    return false;
#endif
}
#endif

#ifdef HAVE_OPENCL

template <bool useCustomDeriv>
static bool ocl_Canny_kalray(InputArray _src, const UMat& dx_, const UMat& dy_, OutputArray _dst, float low_thresh, float high_thresh,
                      int aperture_size, bool L2gradient, int cn, const Size & size)
{
    CV_INSTRUMENT_REGION_OPENCL();

    UMat map;

    const ocl::Device &dev = ocl::Device::getDefault();
    int max_wg_size = (int)dev.maxWorkGroupSize();

    // OpenCL kernel source used
    struct cv::ocl::internal::ProgramEntry used_canny_oclsrc = ocl::imgproc::canny_kalray_oclsrc;

    int lSizeX = 32;
    int lSizeY = max_wg_size / 32;

    if (lSizeY == 0)
    {
        lSizeX = 4;
        lSizeY = max_wg_size / 4;
    }
    if (lSizeY == 0)
    {
        lSizeY = 1;
    }

    if (aperture_size == 7)
    {
        low_thresh = low_thresh / 16.0f;
        high_thresh = high_thresh / 16.0f;
    }

    if (L2gradient)
    {
        low_thresh = std::min(32767.0f, low_thresh);
        high_thresh = std::min(32767.0f, high_thresh);

        if (low_thresh > 0)
            low_thresh *= low_thresh;
        if (high_thresh > 0)
            high_thresh *= high_thresh;
    }
    int low = cvFloor(low_thresh), high = cvFloor(high_thresh);

    // the Kalray kernel 'stage1_with_sobel' handle both versions :
    // - Sobel single pass for 3x3 filter
    // - Sobel separable for 5x5 (currently only support one channel images)
    if (!useCustomDeriv
        && ((aperture_size == 3) || (aperture_size == 5 && cn == 1))
        && !_src.isSubmatrix())
    {
        /*
            stage1_with_sobel:
                Sobel operator
                Calc magnitudes
                Non maxima suppression
                Double thresholding
        */
        char cvt[40];

        size_t globalsize[2] = { (size_t)size.width, (size_t)size.height },
                localsize[2] = { (size_t)lSizeX, (size_t)lSizeY };

        int grp_sizex = lSizeX;
        int grp_sizey = lSizeY;

        // maximal __local mem size, with 8KB less for margin (alignment, metadata)
        const size_t max_local_mem_size = dev.maxLocalMemSize() - 8*1024;

        // let say we want to partition image into 8x8 = 64 blocks for 5 CUs
        const int ideal_num_blocks_width  = size.width / 8;
        const int ideal_num_blocks_height = size.height / 8;

        // tune blocksize to make sure it fits in __local
        auto stage1_with_sobel_local_footprint = [cn, &_src](int block_size_x, int block_size_y, int aperture)
        {
            // TYPE3 is actually TYPE4, with the 4th element being padding.
            int cn_size = cn == 3 ? 4 : cn;

            // get image size while avoiding useless transfer (keep same type as src)
            const size_t src_elemsize = _src.isMat() ? _src.getMat().elemSize() : _src.getUMat().elemSize();
            int sobel_bufsize;
            if (aperture == 3)
            {
                sobel_bufsize = ((block_size_x + 2) * (block_size_y + 2) * 2 * sizeof(short));
            }
            else
            {
                sobel_bufsize = ((block_size_x + 2) * (block_size_y + 1 + aperture) * 4 * sizeof(short));
            }

            // This formula is developed in canny.cl, and is not the same
            // for every kernels
            return ((block_size_x + 4) * (block_size_y + 4) * 2 *  src_elemsize) +
                   ((block_size_x + 4) * (block_size_y + 4) * cn_size * sizeof(short)) +
                   sobel_bufsize +
                   ((block_size_x + 2) * (block_size_y + 2) * 1 * sizeof(int)) +
                   (block_size_x * block_size_y * 2 * sizeof(unsigned char));
        };

        // let's start with the ideal blocksize
        int max_block_size = std::min(ideal_num_blocks_width, ideal_num_blocks_height);

        // if blocksize too big, reduce it gradually
        while (stage1_with_sobel_local_footprint(max_block_size, max_block_size, aperture_size) > max_local_mem_size)
        {
            max_block_size -= 16;
        }

        // now, the max_block_size should be as big as possible and fit in __local
        grp_sizex = max_block_size;
        grp_sizey = max_block_size;

        // The NMS part is 4-vectorized. Ensure the line is a multiple of it.
        grp_sizex = grp_sizex / 4 * 4;

        // adjust localsize[] and globalsize[] to spawn less workgroup
        // and more block-per-workgroup for the asynchronous streaming algo
        // NOTE: 1D work dispatch
        localsize[0] = max_wg_size;
        localsize[1] = 1;
        globalsize[0] = localsize[0] * dev.maxComputeUnits();
        globalsize[1] = localsize[1];

        ocl::Kernel with_sobel("stage1_with_sobel", used_canny_oclsrc,
                               format("-D WITH_SOBEL -D cn=%d -D TYPE=%s -D convert_floatN=%s -D floatN=%s -D GRP_SIZEX=%d -D GRP_SIZEY=%d%s -D APRT=%i -D shortN=%s -D intN=%s",
                                      cn, ocl::memopTypeToStr(_src.depth()),
                                      ocl::convertTypeStr(_src.depth(), CV_32F, cn, cvt),
                                      ocl::typeToStr(CV_MAKE_TYPE(CV_32F, cn)),
                                      grp_sizex, grp_sizey,
                                      L2gradient ? " -D L2GRAD" : "",
                                      aperture_size,
                                      ocl::typeToStr(CV_MAKE_TYPE(CV_16S, cn)),
                                      ocl::typeToStr(CV_MAKE_TYPE(CV_32S, cn))));
        if (with_sobel.empty())
            return false;

        UMat src = _src.getUMat();
        map.create(size, CV_8U);
        with_sobel.args(ocl::KernelArg::ReadOnly(src),
                        ocl::KernelArg::WriteOnlyNoSize(map),
                        (float) low, (float) high);

        if (!with_sobel.run(2, globalsize, localsize, false))
            return false;
    }
    else
    {
        /*
            stage1_without_sobel:
                Calc magnitudes
                Non maxima suppression
                Double thresholding
        */
        double scale = 1.0;
        if (aperture_size == 7)
        {
            scale = 1 / 16.0;
        }

        UMat dx, dy;
        if (!useCustomDeriv)
        {
            Sobel(_src, dx, CV_16S, 1, 0, aperture_size, scale, 0, BORDER_REPLICATE);
            Sobel(_src, dy, CV_16S, 0, 1, aperture_size, scale, 0, BORDER_REPLICATE);
        }
        else
        {
            dx = dx_;
            dy = dy_;
        }

        ocl::Kernel without_sobel("stage1_without_sobel", used_canny_oclsrc,
                                    format("-D WITHOUT_SOBEL -D cn=%d -D GRP_SIZEX=%d -D GRP_SIZEY=%d%s",
                                           cn, lSizeX, lSizeY, L2gradient ? " -D L2GRAD" : ""));
        if (without_sobel.empty())
            return false;

        map.create(size, CV_32S);
        without_sobel.args(ocl::KernelArg::ReadOnlyNoSize(dx), ocl::KernelArg::ReadOnlyNoSize(dy),
                           ocl::KernelArg::WriteOnly(map),
                           low, high);

        size_t globalsize[2] = { (size_t)size.width, (size_t)size.height },
                localsize[2] = { (size_t)lSizeX, (size_t)lSizeY };

        if (!without_sobel.run(2, globalsize, localsize, false))
            return false;
    }

    /*
        stage2:
            hysteresis (add weak edges if they are connected with strong edges)
    */
    {
        size_t step2localsize[2] = { (size_t)4, (size_t)4 };
        size_t step2globalsize[2] = { (size_t)4, (size_t)20 };
        int pix_height = (size.height + step2globalsize[1] - 1) / step2globalsize[1];
        int pix_width = (size.width + step2globalsize[0] - 1) / step2globalsize[0];

        ocl::Kernel edgesHysteresis("stage2_hysteresis", used_canny_oclsrc,
                                    format("-D STAGE2"
                                           " -D SIZE_X=%d -D SIZE_Y=%d -D NB_PE=%zu",
                                    pix_width, pix_height, step2localsize[0] * step2localsize[1]));

        if (edgesHysteresis.empty())
            return false;

        edgesHysteresis.args(ocl::KernelArg::ReadWrite(map));
        if (!edgesHysteresis.run(2, step2globalsize, step2localsize, false))
            return false;
    }

    // get edges
    ocl::Kernel getEdgesKernel("getEdges", used_canny_oclsrc,
                                format("-D GET_EDGES"));
    if (getEdgesKernel.empty())
        return false;

    _dst.create(size, CV_8UC1);
    UMat dst = _dst.getUMat();

    getEdgesKernel.args(ocl::KernelArg::ReadOnly(map), ocl::KernelArg::WriteOnlyNoSize(dst));

    size_t step3localsize[2] = { (size_t)16, (size_t)1 };
    size_t step3globalsize[2] = { (size_t)80, (size_t)1 };
    return getEdgesKernel.run(2, step3globalsize, step3localsize, false);
}

template <bool useCustomDeriv>
static bool ocl_Canny(InputArray _src, const UMat& dx_, const UMat& dy_, OutputArray _dst, float low_thresh, float high_thresh,
                      int aperture_size, bool L2gradient, int cn, const Size & size)
{
    CV_INSTRUMENT_REGION_OPENCL();

    UMat map;

    const ocl::Device &dev = ocl::Device::getDefault();

    // If Kalray device, use custom Kalray function
    if (dev.isKalray())
    {
        return ocl_Canny_kalray<useCustomDeriv>(_src, dx_, dy_,
                                                _dst, low_thresh, high_thresh,
                                                aperture_size, L2gradient, cn, size);
    }

    int max_wg_size = (int)dev.maxWorkGroupSize();

    int lSizeX = 32;
    int lSizeY = max_wg_size / 32;

    if (lSizeY == 0)
    {
        lSizeX = 16;
        lSizeY = max_wg_size / 16;
    }
    if (lSizeY == 0)
    {
        lSizeY = 1;
    }

    if (aperture_size == 7)
    {
        low_thresh = low_thresh / 16.0f;
        high_thresh = high_thresh / 16.0f;
    }

    if (L2gradient)
    {
        low_thresh = std::min(32767.0f, low_thresh);
        high_thresh = std::min(32767.0f, high_thresh);

        if (low_thresh > 0)
            low_thresh *= low_thresh;
        if (high_thresh > 0)
            high_thresh *= high_thresh;
    }
    int low = cvFloor(low_thresh), high = cvFloor(high_thresh);

    if (!useCustomDeriv &&
        aperture_size == 3 && !_src.isSubmatrix())
    {
        /*
            stage1_with_sobel:
                Sobel operator
                Calc magnitudes
                Non maxima suppression
                Double thresholding
        */
        char cvt[40];
        ocl::Kernel with_sobel("stage1_with_sobel", ocl::imgproc::canny_oclsrc,
                               format("-D WITH_SOBEL -D cn=%d -D TYPE=%s -D convert_floatN=%s -D floatN=%s -D GRP_SIZEX=%d -D GRP_SIZEY=%d%s",
                                      cn, ocl::memopTypeToStr(_src.depth()),
                                      ocl::convertTypeStr(_src.depth(), CV_32F, cn, cvt),
                                      ocl::typeToStr(CV_MAKE_TYPE(CV_32F, cn)),
                                      lSizeX, lSizeY,
                                      L2gradient ? " -D L2GRAD" : ""));
        if (with_sobel.empty())
            return false;

        UMat src = _src.getUMat();
        map.create(size, CV_32S);
        with_sobel.args(ocl::KernelArg::ReadOnly(src),
                        ocl::KernelArg::WriteOnlyNoSize(map),
                        (float) low, (float) high);

        size_t globalsize[2] = { (size_t)size.width, (size_t)size.height },
                localsize[2] = { (size_t)lSizeX, (size_t)lSizeY };

        if (!with_sobel.run(2, globalsize, localsize, false))
            return false;
    }
    else
    {
        /*
            stage1_without_sobel:
                Calc magnitudes
                Non maxima suppression
                Double thresholding
        */
        double scale = 1.0;
        if (aperture_size == 7)
        {
            scale = 1 / 16.0;
        }

        UMat dx, dy;
        if (!useCustomDeriv)
        {
            Sobel(_src, dx, CV_16S, 1, 0, aperture_size, scale, 0, BORDER_REPLICATE);
            Sobel(_src, dy, CV_16S, 0, 1, aperture_size, scale, 0, BORDER_REPLICATE);
        }
        else
        {
            dx = dx_;
            dy = dy_;
        }

        ocl::Kernel without_sobel("stage1_without_sobel", ocl::imgproc::canny_oclsrc,
                                    format("-D WITHOUT_SOBEL -D cn=%d -D GRP_SIZEX=%d -D GRP_SIZEY=%d%s",
                                           cn, lSizeX, lSizeY, L2gradient ? " -D L2GRAD" : ""));
        if (without_sobel.empty())
            return false;

        map.create(size, CV_32S);
        without_sobel.args(ocl::KernelArg::ReadOnlyNoSize(dx), ocl::KernelArg::ReadOnlyNoSize(dy),
                           ocl::KernelArg::WriteOnly(map),
                           low, high);

        size_t globalsize[2] = { (size_t)size.width, (size_t)size.height },
                localsize[2] = { (size_t)lSizeX, (size_t)lSizeY };

        if (!without_sobel.run(2, globalsize, localsize, false))
            return false;
    }

    int PIX_PER_WI = 8;
    /*
        stage2:
            hysteresis (add weak edges if they are connected with strong edges)
    */

    int sizey = lSizeY / PIX_PER_WI;
    if (sizey == 0)
        sizey = 1;

    size_t globalsize[2] = { (size_t)size.width, ((size_t)size.height + PIX_PER_WI - 1) / PIX_PER_WI }, localsize[2] = { (size_t)lSizeX, (size_t)sizey };

    ocl::Kernel edgesHysteresis("stage2_hysteresis", ocl::imgproc::canny_oclsrc,
                                format("-D STAGE2 -D PIX_PER_WI=%d -D LOCAL_X=%d -D LOCAL_Y=%d",
                                PIX_PER_WI, lSizeX, sizey));

    if (edgesHysteresis.empty())
        return false;

    edgesHysteresis.args(ocl::KernelArg::ReadWrite(map));
    if (!edgesHysteresis.run(2, globalsize, localsize, false))
        return false;

    // get edges

    ocl::Kernel getEdgesKernel("getEdges", ocl::imgproc::canny_oclsrc,
                                format("-D GET_EDGES -D PIX_PER_WI=%d", PIX_PER_WI));
    if (getEdgesKernel.empty())
        return false;

    _dst.create(size, CV_8UC1);
    UMat dst = _dst.getUMat();

    getEdgesKernel.args(ocl::KernelArg::ReadOnly(map), ocl::KernelArg::WriteOnlyNoSize(dst));

    return getEdgesKernel.run(2, globalsize, NULL, false);
}

#endif

#define CANNY_PUSH(map, stack) *map = 2, stack.push_back(map)

#define CANNY_CHECK(m, high, map, stack) \
    if (m > high) \
        CANNY_PUSH(map, stack); \
    else \
        *map = 0

class parallelCanny : public ParallelLoopBody
{
public:
    parallelCanny(const Mat &_src, Mat &_map, std::deque<uchar*> &borderPeaksParallel,
                  int _low, int _high, int _aperture_size, bool _L2gradient) :
        src(_src), src2(_src), map(_map), _borderPeaksParallel(borderPeaksParallel),
        low(_low), high(_high), aperture_size(_aperture_size), L2gradient(_L2gradient)
    {
#if CV_SIMD
        for(int i = 0; i < v_int8::nlanes; ++i)
        {
            smask[i] = 0;
            smask[i + v_int8::nlanes] = (schar)-1;
        }
        if (true)
            _map.create(src.rows + 2, (int)alignSize((size_t)(src.cols + CV_SIMD_WIDTH + 1), CV_SIMD_WIDTH), CV_8UC1);
        else
#endif
            _map.create(src.rows + 2, src.cols + 2,  CV_8UC1);
        map = _map;
        map.row(0).setTo(1);
        map.row(src.rows + 1).setTo(1);
        mapstep = map.cols;
        needGradient = true;
        cn = src.channels();
    }

    parallelCanny(const Mat &_dx, const Mat &_dy, Mat &_map, std::deque<uchar*> &borderPeaksParallel,
                  int _low, int _high, bool _L2gradient) :
        src(_dx), src2(_dy), map(_map), _borderPeaksParallel(borderPeaksParallel),
        low(_low), high(_high), aperture_size(0), L2gradient(_L2gradient)
    {
#if CV_SIMD
        for(int i = 0; i < v_int8::nlanes; ++i)
        {
            smask[i] = 0;
            smask[i + v_int8::nlanes] = (schar)-1;
        }
        if (true)
            _map.create(src.rows + 2, (int)alignSize((size_t)(src.cols + CV_SIMD_WIDTH + 1), CV_SIMD_WIDTH), CV_8UC1);
        else
#endif
            _map.create(src.rows + 2, src.cols + 2,  CV_8UC1);
        map = _map;
        map.row(0).setTo(1);
        map.row(src.rows + 1).setTo(1);
        mapstep = map.cols;
        needGradient = false;
        cn = src.channels();
    }

    ~parallelCanny() {}

    parallelCanny& operator=(const parallelCanny&) { return *this; }

    void operator()(const Range &boundaries) const CV_OVERRIDE
    {
        CV_TRACE_FUNCTION();

        CV_DbgAssert(cn > 0);

        Mat dx, dy;
        AutoBuffer<short> dxMax(0), dyMax(0);
        std::deque<uchar*> stack, borderPeaksLocal;
        const int rowStart = max(0, boundaries.start - 1), rowEnd = min(src.rows, boundaries.end + 1);
        int *_mag_p, *_mag_a, *_mag_n;
        short *_dx, *_dy, *_dx_a = NULL, *_dy_a = NULL, *_dx_n = NULL, *_dy_n = NULL;
        uchar *_pmap;
        double scale = 1.0;

        CV_TRACE_REGION("gradient")
        if(needGradient)
        {
            if (aperture_size == 7)
            {
                scale = 1 / 16.0;
            }
            Sobel(src.rowRange(rowStart, rowEnd), dx, CV_16S, 1, 0, aperture_size, scale, 0, BORDER_REPLICATE);
            Sobel(src.rowRange(rowStart, rowEnd), dy, CV_16S, 0, 1, aperture_size, scale, 0, BORDER_REPLICATE);
        }
        else
        {
            dx = src.rowRange(rowStart, rowEnd);
            dy = src2.rowRange(rowStart, rowEnd);
        }

        CV_TRACE_REGION_NEXT("magnitude");
        if(cn > 1)
        {
            dxMax.allocate(2 * dx.cols);
            dyMax.allocate(2 * dy.cols);
            _dx_a = dxMax.data();
            _dx_n = _dx_a + dx.cols;
            _dy_a = dyMax.data();
            _dy_n = _dy_a + dy.cols;
        }

        // _mag_p: previous row, _mag_a: actual row, _mag_n: next row
#if CV_SIMD
        AutoBuffer<int> buffer(3 * (mapstep * cn + CV_SIMD_WIDTH));
        _mag_p = alignPtr(buffer.data() + 1, CV_SIMD_WIDTH);
        _mag_a = alignPtr(_mag_p + mapstep * cn, CV_SIMD_WIDTH);
        _mag_n = alignPtr(_mag_a + mapstep * cn, CV_SIMD_WIDTH);
#else
        AutoBuffer<int> buffer(3 * (mapstep * cn));
        _mag_p = buffer.data() + 1;
        _mag_a = _mag_p + mapstep * cn;
        _mag_n = _mag_a + mapstep * cn;
#endif

        // For the first time when just 2 rows are filled and for left and right borders
        if(rowStart == boundaries.start)
            memset(_mag_n - 1, 0, mapstep * sizeof(int));
        else
            _mag_n[src.cols] = _mag_n[-1] = 0;

        _mag_a[src.cols] = _mag_a[-1] = _mag_p[src.cols] = _mag_p[-1] = 0;

        // calculate magnitude and angle of gradient, perform non-maxima suppression.
        // fill the map with one of the following values:
        //   0 - the pixel might belong to an edge
        //   1 - the pixel can not belong to an edge
        //   2 - the pixel does belong to an edge
        for (int i = rowStart; i <= boundaries.end; ++i)
        {
            // Scroll the ring buffer
            std::swap(_mag_n, _mag_a);
            std::swap(_mag_n, _mag_p);

            if(i < rowEnd)
            {
                // Next row calculation
                _dx = dx.ptr<short>(i - rowStart);
                _dy = dy.ptr<short>(i - rowStart);

                if (L2gradient)
                {
                    int j = 0, width = src.cols * cn;
#if CV_SIMD
                    for ( ; j <= width - v_int16::nlanes; j += v_int16::nlanes)
                    {
                        v_int16 v_dx = vx_load((const short*)(_dx + j));
                        v_int16 v_dy = vx_load((const short*)(_dy + j));

                        v_int32 v_dxp_low, v_dxp_high;
                        v_int32 v_dyp_low, v_dyp_high;
                        v_expand(v_dx, v_dxp_low, v_dxp_high);
                        v_expand(v_dy, v_dyp_low, v_dyp_high);

                        v_store_aligned((int *)(_mag_n + j), v_dxp_low*v_dxp_low+v_dyp_low*v_dyp_low);
                        v_store_aligned((int *)(_mag_n + j + v_int32::nlanes), v_dxp_high*v_dxp_high+v_dyp_high*v_dyp_high);
                    }
#endif
                    for ( ; j < width; ++j)
                        _mag_n[j] = int(_dx[j])*_dx[j] + int(_dy[j])*_dy[j];
                }
                else
                {
                    int j = 0, width = src.cols * cn;
#if CV_SIMD
                    for(; j <= width - v_int16::nlanes; j += v_int16::nlanes)
                    {
                        v_int16 v_dx = vx_load((const short *)(_dx + j));
                        v_int16 v_dy = vx_load((const short *)(_dy + j));

                        v_dx = v_reinterpret_as_s16(v_abs(v_dx));
                        v_dy = v_reinterpret_as_s16(v_abs(v_dy));

                        v_int32 v_dx_ml, v_dy_ml, v_dx_mh, v_dy_mh;
                        v_expand(v_dx, v_dx_ml, v_dx_mh);
                        v_expand(v_dy, v_dy_ml, v_dy_mh);

                        v_store_aligned((int *)(_mag_n + j), v_dx_ml + v_dy_ml);
                        v_store_aligned((int *)(_mag_n + j + v_int32::nlanes), v_dx_mh + v_dy_mh);
                    }
#endif
                    for ( ; j < width; ++j)
                        _mag_n[j] = std::abs(int(_dx[j])) + std::abs(int(_dy[j]));
                }

                if(cn > 1)
                {
                    std::swap(_dx_n, _dx_a);
                    std::swap(_dy_n, _dy_a);

                    for(int j = 0, jn = 0; j < src.cols; ++j, jn += cn)
                    {
                        int maxIdx = jn;
                        for(int k = 1; k < cn; ++k)
                            if(_mag_n[jn + k] > _mag_n[maxIdx]) maxIdx = jn + k;

                        _mag_n[j] = _mag_n[maxIdx];
                        _dx_n[j] = _dx[maxIdx];
                        _dy_n[j] = _dy[maxIdx];
                    }

                    _mag_n[src.cols] = 0;
                }

                // at the very beginning we do not have a complete ring
                // buffer of 3 magnitude rows for non-maxima suppression
                if (i <= boundaries.start)
                    continue;
            }
            else
            {
                memset(_mag_n - 1, 0, mapstep * sizeof(int));

                if(cn > 1)
                {
                    std::swap(_dx_n, _dx_a);
                    std::swap(_dy_n, _dy_a);
                }
            }

            // From here actual src row is (i - 1)
            // Set left and right border to 1
#if CV_SIMD
            if (true)
                _pmap = map.ptr<uchar>(i) + CV_SIMD_WIDTH;
            else
#endif
                _pmap = map.ptr<uchar>(i) + 1;

            _pmap[src.cols] =_pmap[-1] = 1;

            if(cn == 1)
            {
                _dx = dx.ptr<short>(i - rowStart - 1);
                _dy = dy.ptr<short>(i - rowStart - 1);
            }
            else
            {
                _dx = _dx_a;
                _dy = _dy_a;
            }

            const int TG22 = 13573;
            int j = 0;
#if CV_SIMD
            {
                const v_int32 v_low = vx_setall_s32(low);
                const v_int8 v_one = vx_setall_s8(1);

                for (; j <= src.cols - v_int8::nlanes; j += v_int8::nlanes)
                {
                    v_store_aligned((signed char*)(_pmap + j), v_one);
                    v_int8 v_cmp = v_pack(v_pack(vx_load_aligned((const int*)(_mag_a + j                    )) > v_low,
                                                 vx_load_aligned((const int*)(_mag_a + j +   v_int32::nlanes)) > v_low),
                                          v_pack(vx_load_aligned((const int*)(_mag_a + j + 2*v_int32::nlanes)) > v_low,
                                                 vx_load_aligned((const int*)(_mag_a + j + 3*v_int32::nlanes)) > v_low));
                    while (v_check_any(v_cmp))
                    {
                        int l = v_scan_forward(v_cmp);
                        v_cmp &= vx_load(smask + v_int8::nlanes - 1 - l);
                        int k = j + l;

                        int m = _mag_a[k];
                        short xs = _dx[k];
                        short ys = _dy[k];
                        int x = (int)std::abs(xs);
                        int y = (int)std::abs(ys) << 15;

                        int tg22x = x * TG22;

                        if (y < tg22x)
                        {
                            if (m > _mag_a[k - 1] && m >= _mag_a[k + 1])
                            {
                                CANNY_CHECK(m, high, (_pmap+k), stack);
                            }
                        }
                        else
                        {
                            int tg67x = tg22x + (x << 16);
                            if (y > tg67x)
                            {
                                if (m > _mag_p[k] && m >= _mag_n[k])
                                {
                                    CANNY_CHECK(m, high, (_pmap+k), stack);
                                }
                            }
                            else
                            {
                                int s = (xs ^ ys) < 0 ? -1 : 1;
                                if(m > _mag_p[k - s] && m > _mag_n[k + s])
                                {
                                    CANNY_CHECK(m, high, (_pmap+k), stack);
                                }
                            }
                        }
                    }
                }
            }
#endif
            for (; j < src.cols; j++)
            {
                int m = _mag_a[j];

                if (m > low)
                {
                    short xs = _dx[j];
                    short ys = _dy[j];
                    int x = (int)std::abs(xs);
                    int y = (int)std::abs(ys) << 15;

                    int tg22x = x * TG22;

                    if (y < tg22x)
                    {
                        if (m > _mag_a[j - 1] && m >= _mag_a[j + 1])
                        {
                            CANNY_CHECK(m, high, (_pmap+j), stack);
                            continue;
                        }
                    }
                    else
                    {
                        int tg67x = tg22x + (x << 16);
                        if (y > tg67x)
                        {
                            if (m > _mag_p[j] && m >= _mag_n[j])
                            {
                                CANNY_CHECK(m, high, (_pmap+j), stack);
                                continue;
                            }
                        }
                        else
                        {
                            int s = (xs ^ ys) < 0 ? -1 : 1;
                            if(m > _mag_p[j - s] && m > _mag_n[j + s])
                            {
                                CANNY_CHECK(m, high, (_pmap+j), stack);
                                continue;
                            }
                        }
                    }
                }
                _pmap[j] = 1;
            }
        }

        // Not for first row of first slice or last row of last slice
        uchar *pmapLower = (rowStart == 0) ? map.data : (map.data + (boundaries.start + 2) * mapstep);
        uint pmapDiff = (uint)(((rowEnd == src.rows) ? map.datalimit : (map.data + boundaries.end * mapstep)) - pmapLower);

        // now track the edges (hysteresis thresholding)
        CV_TRACE_REGION_NEXT("hysteresis");
        while (!stack.empty())
        {
            uchar *m = stack.back();
            stack.pop_back();

            // Stops thresholding from expanding to other slices by sending pixels in the borders of each
            // slice in a queue to be serially processed later.
            if((unsigned)(m - pmapLower) < pmapDiff)
            {
                if (!m[-mapstep-1]) CANNY_PUSH((m-mapstep-1), stack);
                if (!m[-mapstep])   CANNY_PUSH((m-mapstep), stack);
                if (!m[-mapstep+1]) CANNY_PUSH((m-mapstep+1), stack);
                if (!m[-1])         CANNY_PUSH((m-1), stack);
                if (!m[1])          CANNY_PUSH((m+1), stack);
                if (!m[mapstep-1])  CANNY_PUSH((m+mapstep-1), stack);
                if (!m[mapstep])    CANNY_PUSH((m+mapstep), stack);
                if (!m[mapstep+1])  CANNY_PUSH((m+mapstep+1), stack);
            }
            else
            {
                borderPeaksLocal.push_back(m);
                ptrdiff_t mapstep2 = m < pmapLower ? mapstep : -mapstep;

                if (!m[-1])         CANNY_PUSH((m-1), stack);
                if (!m[1])          CANNY_PUSH((m+1), stack);
                if (!m[mapstep2-1]) CANNY_PUSH((m+mapstep2-1), stack);
                if (!m[mapstep2])   CANNY_PUSH((m+mapstep2), stack);
                if (!m[mapstep2+1]) CANNY_PUSH((m+mapstep2+1), stack);
            }
        }

        if(!borderPeaksLocal.empty())
        {
            AutoLock lock(mutex);
            _borderPeaksParallel.insert(_borderPeaksParallel.end(), borderPeaksLocal.begin(), borderPeaksLocal.end());
        }
    }

private:
    const Mat &src, &src2;
    Mat &map;
    std::deque<uchar*> &_borderPeaksParallel;
    int low, high, aperture_size;
    bool L2gradient, needGradient;
    ptrdiff_t mapstep;
    int cn;
    mutable Mutex mutex;
#if CV_SIMD
    schar smask[2*v_int8::nlanes];
#endif
};

class finalPass : public ParallelLoopBody
{

public:
    finalPass(const Mat &_map, Mat &_dst) :
        map(_map), dst(_dst)
    {
        dst = _dst;
    }

    ~finalPass() {}

    void operator()(const Range &boundaries) const CV_OVERRIDE
    {
        // the final pass, form the final image
        for (int i = boundaries.start; i < boundaries.end; i++)
        {
            int j = 0;
            uchar *pdst = dst.ptr<uchar>(i);
            const uchar *pmap = map.ptr<uchar>(i + 1);
#if CV_SIMD
            if (true)
                pmap += CV_SIMD_WIDTH;
            else
#endif
                pmap += 1;
#if CV_SIMD
            {
                const v_uint8 v_zero = vx_setzero_u8();
                const v_uint8 v_ff = ~v_zero;
                const v_uint8 v_two = vx_setall_u8(2);

                for (; j <= dst.cols - v_uint8::nlanes; j += v_uint8::nlanes)
                {
                    v_uint8 v_pmap = vx_load_aligned((const unsigned char*)(pmap + j));
                    v_pmap = v_select(v_pmap == v_two, v_ff, v_zero);
                    v_store((pdst + j), v_pmap);
                }

                if (j <= dst.cols - v_uint8::nlanes/2)
                {
                    v_uint8 v_pmap = vx_load_low((const unsigned char*)(pmap + j));
                    v_pmap = v_select(v_pmap == v_two, v_ff, v_zero);
                    v_store_low((pdst + j), v_pmap);
                    j += v_uint8::nlanes/2;
                }
            }
#endif
            for (; j < dst.cols; j++)
            {
                pdst[j] = (uchar)-(pmap[j] >> 1);
            }
        }
    }

private:
    const Mat &map;
    Mat &dst;

    finalPass(const finalPass&); // = delete
    finalPass& operator=(const finalPass&); // = delete
};

#ifdef HAVE_OPENVX
namespace ovx {
    template <> inline bool skipSmallImages<VX_KERNEL_CANNY_EDGE_DETECTOR>(int w, int h) { return w*h < 640 * 480; }
}
static bool openvx_canny(const Mat& src, Mat& dst, int loVal, int hiVal, int kSize, bool useL2)
{
    using namespace ivx;

    Context context = ovx::getOpenVXContext();
    try
    {
    Image _src = Image::createFromHandle(
                context,
                Image::matTypeToFormat(src.type()),
                Image::createAddressing(src),
                src.data );
    Image _dst = Image::createFromHandle(
                context,
                Image::matTypeToFormat(dst.type()),
                Image::createAddressing(dst),
                dst.data );
    Threshold threshold = Threshold::createRange(context, VX_TYPE_UINT8, saturate_cast<uchar>(loVal), saturate_cast<uchar>(hiVal));

#if 0
    // the code below is disabled because vxuCannyEdgeDetector()
    // ignores context attribute VX_CONTEXT_IMMEDIATE_BORDER

    // FIXME: may fail in multithread case
    border_t prevBorder = context.immediateBorder();
    context.setImmediateBorder(VX_BORDER_REPLICATE);
    IVX_CHECK_STATUS( vxuCannyEdgeDetector(context, _src, threshold, kSize, (useL2 ? VX_NORM_L2 : VX_NORM_L1), _dst) );
    context.setImmediateBorder(prevBorder);
#else
    // alternative code without vxuCannyEdgeDetector()
    Graph graph = Graph::create(context);
    ivx::Node node = ivx::Node(vxCannyEdgeDetectorNode(graph, _src, threshold, kSize, (useL2 ? VX_NORM_L2 : VX_NORM_L1), _dst) );
    node.setBorder(VX_BORDER_REPLICATE);
    graph.verify();
    graph.process();
#endif

#ifdef VX_VERSION_1_1
    _src.swapHandle();
    _dst.swapHandle();
#endif
    }
    catch(const WrapperError& e)
    {
        VX_DbgThrow(e.what());
    }
    catch(const RuntimeError& e)
    {
        VX_DbgThrow(e.what());
    }

    return true;
}
#endif // HAVE_OPENVX

void Canny( InputArray _src, OutputArray _dst,
                double low_thresh, double high_thresh,
                int aperture_size, bool L2gradient )
{
    CV_INSTRUMENT_REGION();

    CV_Assert( _src.depth() == CV_8U );

    const Size size = _src.size();

    // we don't support inplace parameters in case with RGB/BGR src
    CV_Assert((_dst.getObj() != _src.getObj() || _src.type() == CV_8UC1) && "Inplace parameters are not supported");

    _dst.create(size, CV_8U);

    if (!L2gradient && (aperture_size & CV_CANNY_L2_GRADIENT) == CV_CANNY_L2_GRADIENT)
    {
        // backward compatibility
        aperture_size &= ~CV_CANNY_L2_GRADIENT;
        L2gradient = true;
    }

    if ((aperture_size & 1) == 0 || (aperture_size != -1 && (aperture_size < 3 || aperture_size > 7)))
        CV_Error(CV_StsBadFlag, "Aperture size should be odd between 3 and 7");

    if (aperture_size == 7)
    {
        low_thresh = low_thresh / 16.0;
        high_thresh = high_thresh / 16.0;
    }

    if (low_thresh > high_thresh)
        std::swap(low_thresh, high_thresh);

    CV_OCL_RUN(_dst.isUMat() && (_src.channels() == 1 || _src.channels() == 3),
               ocl_Canny<false>(_src, UMat(), UMat(), _dst, (float)low_thresh, (float)high_thresh, aperture_size, L2gradient, _src.channels(), size))

    Mat src0 = _src.getMat(), dst = _dst.getMat();
    Mat src(src0.size(), src0.type(), src0.data, src0.step);

    CALL_HAL(canny, cv_hal_canny, src.data, src.step, dst.data, dst.step, src.cols, src.rows, src.channels(),
             low_thresh, high_thresh, aperture_size, L2gradient);

    CV_OVX_RUN(
        false && /* disabling due to accuracy issues */
            src.type() == CV_8UC1 &&
            !src.isSubmatrix() &&
            src.cols >= aperture_size &&
            src.rows >= aperture_size &&
            !ovx::skipSmallImages<VX_KERNEL_CANNY_EDGE_DETECTOR>(src.cols, src.rows),
        openvx_canny(
            src,
            dst,
            cvFloor(low_thresh),
            cvFloor(high_thresh),
            aperture_size,
            L2gradient ) )

    CV_IPP_RUN_FAST(ipp_Canny(src, Mat(), Mat(), dst, (float)low_thresh, (float)high_thresh, L2gradient, aperture_size))

    if (L2gradient)
    {
        low_thresh = std::min(32767.0, low_thresh);
        high_thresh = std::min(32767.0, high_thresh);

        if (low_thresh > 0) low_thresh *= low_thresh;
        if (high_thresh > 0) high_thresh *= high_thresh;
    }
    int low = cvFloor(low_thresh);
    int high = cvFloor(high_thresh);

    // If Scharr filter: aperture size is 3, ksize2 is 1
    int ksize2 = aperture_size < 0 ? 1 : aperture_size / 2;
    // Minimum number of threads should be 1, maximum should not exceed number of CPU's, because of overhead
    int numOfThreads = std::max(1, std::min(getNumThreads(), getNumberOfCPUs()));
    // Make a fallback for pictures with too few rows.
    int grainSize = src.rows / numOfThreads;
    int minGrainSize = 2 * (ksize2 + 1);
    if (grainSize < minGrainSize)
        numOfThreads = std::max(1, src.rows / minGrainSize);

    Mat map;
    std::deque<uchar*> stack;

    parallel_for_(Range(0, src.rows), parallelCanny(src, map, stack, low, high, aperture_size, L2gradient), numOfThreads);

    CV_TRACE_REGION("global_hysteresis");
    // now track the edges (hysteresis thresholding)
    ptrdiff_t mapstep = map.cols;

    while (!stack.empty())
    {
        uchar* m = stack.back();
        stack.pop_back();

        if (!m[-mapstep-1]) CANNY_PUSH((m-mapstep-1), stack);
        if (!m[-mapstep])   CANNY_PUSH((m-mapstep), stack);
        if (!m[-mapstep+1]) CANNY_PUSH((m-mapstep+1), stack);
        if (!m[-1])         CANNY_PUSH((m-1), stack);
        if (!m[1])          CANNY_PUSH((m+1), stack);
        if (!m[mapstep-1])  CANNY_PUSH((m+mapstep-1), stack);
        if (!m[mapstep])    CANNY_PUSH((m+mapstep), stack);
        if (!m[mapstep+1])  CANNY_PUSH((m+mapstep+1), stack);
    }

    CV_TRACE_REGION_NEXT("finalPass");
    parallel_for_(Range(0, src.rows), finalPass(map, dst), src.total()/(double)(1<<16));
}

void Canny( InputArray _dx, InputArray _dy, OutputArray _dst,
                double low_thresh, double high_thresh,
                bool L2gradient )
{
    CV_INSTRUMENT_REGION();

    CV_Assert(_dx.dims() == 2);
    CV_Assert(_dx.type() == CV_16SC1 || _dx.type() == CV_16SC3);
    CV_Assert(_dy.type() == _dx.type());
    CV_Assert(_dx.sameSize(_dy));

    if (low_thresh > high_thresh)
        std::swap(low_thresh, high_thresh);

    const Size size = _dx.size();

    CV_OCL_RUN(_dst.isUMat(),
               ocl_Canny<true>(UMat(), _dx.getUMat(), _dy.getUMat(), _dst, (float)low_thresh, (float)high_thresh, 0, L2gradient, _dx.channels(), size))

    _dst.create(size, CV_8U);
    Mat dst = _dst.getMat();

    Mat dx = _dx.getMat();
    Mat dy = _dy.getMat();

    CV_IPP_RUN_FAST(ipp_Canny(Mat(), dx, dy, dst, (float)low_thresh, (float)high_thresh, L2gradient, 0))

    if (L2gradient)
    {
        low_thresh = std::min(32767.0, low_thresh);
        high_thresh = std::min(32767.0, high_thresh);

        if (low_thresh > 0) low_thresh *= low_thresh;
        if (high_thresh > 0) high_thresh *= high_thresh;
    }

    int low = cvFloor(low_thresh);
    int high = cvFloor(high_thresh);

    std::deque<uchar*> stack;
    Mat map;

    // Minimum number of threads should be 1, maximum should not exceed number of CPU's, because of overhead
    int numOfThreads = std::max(1, std::min(getNumThreads(), getNumberOfCPUs()));
    if (dx.rows / numOfThreads < 3)
        numOfThreads = std::max(1, dx.rows / 3);

    parallel_for_(Range(0, dx.rows), parallelCanny(dx, dy, map, stack, low, high, L2gradient), numOfThreads);

    CV_TRACE_REGION("global_hysteresis")
    // now track the edges (hysteresis thresholding)
    ptrdiff_t mapstep = map.cols;

    while (!stack.empty())
    {
        uchar* m = stack.back();
        stack.pop_back();

        if (!m[-mapstep-1]) CANNY_PUSH((m-mapstep-1), stack);
        if (!m[-mapstep])   CANNY_PUSH((m-mapstep), stack);
        if (!m[-mapstep+1]) CANNY_PUSH((m-mapstep+1), stack);
        if (!m[-1])         CANNY_PUSH((m-1), stack);
        if (!m[1])          CANNY_PUSH((m+1), stack);
        if (!m[mapstep-1])  CANNY_PUSH((m+mapstep-1), stack);
        if (!m[mapstep])    CANNY_PUSH((m+mapstep), stack);
        if (!m[mapstep+1])  CANNY_PUSH((m+mapstep+1), stack);
    }

    CV_TRACE_REGION_NEXT("finalPass");
    parallel_for_(Range(0, dx.rows), finalPass(map, dst), dx.total()/(double)(1<<16));
}

} // namespace cv

void cvCanny( const CvArr* image, CvArr* edges, double threshold1,
              double threshold2, int aperture_size )
{
    cv::Mat src = cv::cvarrToMat(image), dst = cv::cvarrToMat(edges);
    CV_Assert( src.size == dst.size && src.depth() == CV_8U && dst.type() == CV_8U );

    cv::Canny(src, dst, threshold1, threshold2, aperture_size & 255,
              (aperture_size & CV_CANNY_L2_GRADIENT) != 0);
}

/* End of file. */

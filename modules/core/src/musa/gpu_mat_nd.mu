// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#include "opencv2/opencv_modules.hpp"

#ifndef HAVE_OPENCV_MUDEV

#error "opencv_mudev is required"

#else

#include "opencv2/core/musa.hpp"
#include "opencv2/mudev.hpp"

using namespace cv;
using namespace cv::musa;

GpuData::GpuData(const size_t _size)
    : data(nullptr), size(_size)
{
    CV_MUDEV_SAFE_CALL(musaMalloc(&data, _size));
}

GpuData::~GpuData()
{
    CV_MUDEV_SAFE_CALL(musaFree(data));
}

/////////////////////////////////////////////////////
/// create

void GpuMatND::create(SizeArray _size, int _type)
{
    {
        auto elements_nonzero = [](SizeArray& v)
        {
            return std::all_of(v.begin(), v.end(),
                [](unsigned u){ return u > 0; });
        };
        CV_Assert(!_size.empty());
        CV_Assert(elements_nonzero(_size));
    }

    _type &= Mat::TYPE_MASK;

    if (size == _size && type() == _type && !empty() && !external() && isContinuous() && !isSubmatrix())
        return;

    release();

    setFields(std::move(_size), _type);

    data_ = std::make_shared<GpuData>(totalMemSize());
    data = data_->data;
    offset = 0;
}

/////////////////////////////////////////////////////
/// release

void GpuMatND::release()
{
    data = nullptr;
    data_.reset();

    flags = dims = offset = 0;
    size.clear();
    step.clear();
}

/////////////////////////////////////////////////////
/// clone

static bool next(uchar*& d, const uchar*& s, std::vector<int>& idx, const int dims, const GpuMatND& dst, const GpuMatND& src)
{
    int inc = dims-3;

    while (true)
    {
        if (idx[inc] == src.size[inc] - 1)
        {
            if (inc == 0)
            {
                return false;
            }

            idx[inc] = 0;
            d -= (dst.size[inc] - 1) * dst.step[inc];
            s -= (src.size[inc] - 1) * src.step[inc];
            inc--;
        }
        else
        {
            idx[inc]++;
            d += dst.step[inc];
            s += src.step[inc];
            break;
        }
    }

    return true;
}

GpuMatND GpuMatND::clone() const
{
    CV_DbgAssert(!empty());

    GpuMatND ret(size, type());

    if (isContinuous())
    {
        CV_MUDEV_SAFE_CALL(musaMemcpy(ret.getDevicePtr(), getDevicePtr(), ret.totalMemSize(), musaMemcpyDeviceToDevice));
    }
    else
    {
        // 1D arrays are always continuous

        if (dims == 2)
        {
            CV_MUDEV_SAFE_CALL(
                musaMemcpy2D(ret.getDevicePtr(), ret.step[0], getDevicePtr(), step[0],
                    size[1]*step[1], size[0], musaMemcpyDeviceToDevice)
            );
        }
        else
        {
            std::vector<int> idx(dims-2, 0);

            uchar* d = ret.getDevicePtr();
            const uchar* s = getDevicePtr();

            // iterate each 2D plane
            do
            {
                CV_MUDEV_SAFE_CALL(
                    musaMemcpy2DAsync(
                        d, ret.step[dims-2], s, step[dims-2],
                        size[dims-1]*step[dims-1], size[dims-2], musaMemcpyDeviceToDevice)
                );
            }
            while (next(d, s, idx, dims, ret, *this));

            CV_MUDEV_SAFE_CALL(musaStreamSynchronize(0));
        }
    }

    return ret;
}

GpuMatND GpuMatND::clone(Stream& stream) const
{
    CV_DbgAssert(!empty());

    GpuMatND ret(size, type());

    musaStream_t _stream = StreamAccessor::getStream(stream);

    if (isContinuous())
    {
        CV_MUDEV_SAFE_CALL(musaMemcpyAsync(ret.getDevicePtr(), getDevicePtr(), ret.totalMemSize(), musaMemcpyDeviceToDevice, _stream));
    }
    else
    {
        // 1D arrays are always continuous

        if (dims == 2)
        {
            CV_MUDEV_SAFE_CALL(
                musaMemcpy2DAsync(ret.getDevicePtr(), ret.step[0], getDevicePtr(), step[0],
                    size[1]*step[1], size[0], musaMemcpyDeviceToDevice, _stream)
            );
        }
        else
        {
            std::vector<int> idx(dims-2, 0);

            uchar* d = ret.getDevicePtr();
            const uchar* s = getDevicePtr();

            // iterate each 2D plane
            do
            {
                CV_MUDEV_SAFE_CALL(
                    musaMemcpy2DAsync(
                        d, ret.step[dims-2], s, step[dims-2],
                        size[dims-1]*step[dims-1], size[dims-2], musaMemcpyDeviceToDevice, _stream)
                );
            }
            while (next(d, s, idx, dims, ret, *this));
        }
    }

    return ret;
}

/////////////////////////////////////////////////////
/// upload

void GpuMatND::upload(InputArray src)
{
    Mat mat = src.getMat();

    CV_DbgAssert(!mat.empty());

    if (!mat.isContinuous())
        mat = mat.clone();

    SizeArray _size(mat.dims);
    std::copy_n(mat.size.p, mat.dims, _size.data());

    create(std::move(_size), mat.type());

    CV_MUDEV_SAFE_CALL(musaMemcpy(getDevicePtr(), mat.data, totalMemSize(), musaMemcpyHostToDevice));
}

void GpuMatND::upload(InputArray src, Stream& stream)
{
    Mat mat = src.getMat();

    CV_DbgAssert(!mat.empty());

    if (!mat.isContinuous())
        mat = mat.clone();

    SizeArray _size(mat.dims);
    std::copy_n(mat.size.p, mat.dims, _size.data());

    create(std::move(_size), mat.type());

    musaStream_t _stream = StreamAccessor::getStream(stream);
    CV_MUDEV_SAFE_CALL(musaMemcpyAsync(getDevicePtr(), mat.data, totalMemSize(), musaMemcpyHostToDevice, _stream));
}

/////////////////////////////////////////////////////
/// download

void GpuMatND::download(OutputArray dst) const
{
    CV_DbgAssert(!empty());

    dst.create(dims, size.data(), type());
    Mat mat = dst.getMat();

    GpuMatND gmat = *this;

    if (!gmat.isContinuous())
        gmat = gmat.clone();

    CV_MUDEV_SAFE_CALL(musaMemcpy(mat.data, gmat.getDevicePtr(), mat.total() * mat.elemSize(), musaMemcpyDeviceToHost));
}

void GpuMatND::download(OutputArray dst, Stream& stream) const
{
    CV_DbgAssert(!empty());

    dst.create(dims, size.data(), type());
    Mat mat = dst.getMat();

    GpuMatND gmat = *this;

    if (!gmat.isContinuous())
        gmat = gmat.clone(stream);

    musaStream_t _stream = StreamAccessor::getStream(stream);
    CV_MUDEV_SAFE_CALL(musaMemcpyAsync(mat.data, gmat.getDevicePtr(), mat.total() * mat.elemSize(), musaMemcpyDeviceToHost, _stream));
}

#endif

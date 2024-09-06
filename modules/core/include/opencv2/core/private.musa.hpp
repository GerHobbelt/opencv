/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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
//   * The name of the copyright holders may not be used to endorse or promote products
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

#ifndef OPENCV_CORE_PRIVATE_MUSA_HPP
#define OPENCV_CORE_PRIVATE_MUSA_HPP

#ifndef __OPENCV_BUILD
#error this is a private header which should not be used from outside of the OpenCV library
#endif

#include "cvconfig.h"
#include "opencv2/core/base.hpp"
#include "opencv2/core/cvdef.h"
#include "opencv2/core/musa.hpp"

#ifdef HAVE_MUSA
#include <musa.h>
#include <musa_runtime.h>
#if defined(__MUSACC_VER_MAJOR__) && (8 <= __MUSACC_VER_MAJOR__)
#if defined(__GNUC__) && !defined(__MUSACC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <musa_fp16.h>
#pragma GCC diagnostic pop
#else
#include <musa_fp16.h>
#endif
#endif  // defined(__MUSACC_VER_MAJOR__) && (8 <= __MUSACC_VER_MAJOR__)
#include <mupp.h>

#include "opencv2/core/musa/common.hpp"
#include "opencv2/core/musa_stream_accessor.hpp"

#ifndef MUPP_VERSION
#define MUPP_VERSION \
  (MUPP_VERSION_MAJOR * 1000 + MUPP_VERSION_MINOR * 100 + MUPP_VERSION_BUILD)
#endif

#define MUSART_MINIMUM_REQUIRED_VERSION 6050

#if (MUSART_VERSION < MUSART_MINIMUM_REQUIRED_VERSION)
#error "Insufficient musa Runtime library version, please update it."
#endif

#endif

//! @cond IGNORED

namespace cv {
namespace musa {
CV_EXPORTS cv::String getMUppErrorMessage(int code);
CV_EXPORTS cv::String getMusaDriverApiErrorMessage(int code);

CV_EXPORTS GpuMat getInputMat(InputArray _src, Stream& stream);

CV_EXPORTS GpuMat getOutputMat(OutputArray _dst, int rows, int cols, int type,
                               Stream& stream);
static inline GpuMat getOutputMat(OutputArray _dst, Size size, int type,
                                  Stream& stream) {
  return getOutputMat(_dst, size.height, size.width, type, stream);
}

CV_EXPORTS void syncOutput(const GpuMat& dst, OutputArray _dst, Stream& stream);
}  // namespace musa
}  // namespace cv

#ifndef HAVE_MUSA

static inline CV_NORETURN void throw_no_musa() {
  CV_Error(cv::Error::GpuNotSupported,
           "The library is compiled without MUSA support");
}

#else  // HAVE_MUSA
// mupp
#define muppSafeSetStream(oldStream, newStream) \
  {                                             \
    if (oldStream != newStream) {               \
      musaStreamSynchronize(oldStream);         \
      muppSetStream(newStream);                 \
    }                                           \
  }

static inline CV_NORETURN void throw_no_musa() {
  CV_Error(
      cv::Error::StsNotImplemented,
      "The called functionality is disabled for current build or platform");
}

namespace cv {
namespace musa {
static inline void checkMUppError(int code, const char* file, const int line,
                                  const char* func) {
  if (code < 0)
    cv::error(cv::Error::GpuApiCallError, getMUppErrorMessage(code), func, file,
              line);
}

static inline void checkMusaDriverApiError(int code, const char* file,
                                           const int line, const char* func) {
  if (code != MUSA_SUCCESS)
    cv::error(cv::Error::GpuApiCallError, getMusaDriverApiErrorMessage(code),
              func, file, line);
}

template <int n>
struct MUPPTypeTraits;
template <>
struct MUPPTypeTraits<CV_8U> {
  typedef MUpp8u mupp_type;
};
template <>
struct MUPPTypeTraits<CV_8S> {
  typedef MUpp8s mupp_type;
};
template <>
struct MUPPTypeTraits<CV_16U> {
  typedef MUpp16u mupp_type;
};
template <>
struct MUPPTypeTraits<CV_16S> {
  typedef MUpp16s mupp_type;
};
template <>
struct MUPPTypeTraits<CV_32S> {
  typedef MUpp32s mupp_type;
};
template <>
struct MUPPTypeTraits<CV_32F> {
  typedef MUpp32f mupp_type;
};
template <>
struct MUPPTypeTraits<CV_64F> {
  typedef MUpp64f mupp_type;
};

class MUppStreamHandler {
 public:
  inline explicit MUppStreamHandler(Stream& newStream) {
    oldStream = muppGetStream();
    muppSafeSetStream(oldStream, StreamAccessor::getStream(newStream));
  }

  inline explicit MUppStreamHandler(musaStream_t newStream) {
    oldStream = muppGetStream();
    muppSafeSetStream(oldStream, newStream);
  }

  inline ~MUppStreamHandler() { muppSafeSetStream(muppGetStream(), oldStream); }

 private:
  musaStream_t oldStream;
};
}  // namespace musa
}  // namespace cv

#define muppSafeCall(expr) \
  cv::musa::checkMUppError(expr, __FILE__, __LINE__, CV_Func)
#define muSafeCall(expr) \
  cv::musa::checkMusaDriverApiError(expr, __FILE__, __LINE__, CV_Func)

#endif  // HAVE_MUSA

//! @endcond

#endif  // OPENCV_CORE_PRIVATE_MUSA_HPP

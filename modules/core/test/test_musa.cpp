// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#if defined(HAVE_MUSA)

#include "test_precomp.hpp"
#include <musa_runtime.h>
#include "opencv2/core/musa.hpp"

namespace opencv_test { namespace {

TEST(MUSA_Stream, construct_musaFlags)
{
    cv::musa::Stream stream(musaStreamNonBlocking);
    EXPECT_NE(stream.musaPtr(), nullptr);
}

}} // namespace

#endif

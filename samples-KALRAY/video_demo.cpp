/**
 *****************************************************************************
 *
 * Copyright 2021 Kalray
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include "opencv2/core.hpp"
#include "opencv2/core/cvdef.h"
#include "opencv2/core/utils/logger.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/core/opencl/ocl_defs.hpp"
#include <algorithm>
#include <iterator>
#include <opencv2/core/ocl.hpp>
#include <bits/stdint-intn.h>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <chrono>
#include <list>
#include <mutex>
#include <thread>
#include <unistd.h>

using namespace cv;

// Select target:
//  - MatType = Mat  to run on CPU
//  - MatType = UMat to run on MPPA
using MatType = UMat;

// nb frames to process
#define NB_FRAMES 6000

// behavour
#define NB_REPEATED_FRAMES_CAPTURE 50 // repeate n times each frame read from disk
#define MAX_NB_FRAMES_IN_FIFO 200 // size of frames list between threads
#define FPS_WINDOW 50  // size of frames use to compute FPS (Frame Per Second)
#define DISPLAY_RATE 1 // frame display ratio = FPS_WINDOW * DISPLAY_RATE

// compute statistic
#define IGNORE_FIRST_VALUES_FOR_STATS 5 // x FPS_WINDOW = nb frames ignored for stats

// for debug purpose
//#define DEBUG_THREAD

#ifdef DEBUG_THREAD
#define LOG_THREAD(name)                                                       \
  std::cout << name << "_thread:" << nb_frames << std::endl;
#else
#define LOG_THREAD(name)
#endif

void capture_thread(std::mutex &frames_mtx, std::list<MatType> &frames,
        VideoCapture &capture)
{
    Mat frame;
    std::vector<Mat> channels;
    MatType channel;
    bool empty = false;
    int nb_frames = 0;
    do {
        frames_mtx.lock();
        if (frames.size() > MAX_NB_FRAMES_IN_FIFO) {
            frames_mtx.unlock();
            // Avoid interfering too much with the other threads' performance
            usleep(100);
            continue;
        }
        frames_mtx.unlock();

        capture >> frame;
        empty = frame.empty();
        if (empty) {
            capture.set(CAP_PROP_POS_FRAMES, 0);
            continue;
        }
        // keep only 1 channel to have a gray frame
        cv::split(frame, channels);

        // put several frames to feed processing in order to avoid the capture
        // to be the bottleneck
        for (int i = 0; i < NB_REPEATED_FRAMES_CAPTURE; i++) {
          if (frames.size() < std::min(MAX_NB_FRAMES_IN_FIFO, NB_FRAMES)) {
                channels[0].copyTo(channel);
                frames_mtx.lock();
                frames.push_back(std::move(channel));
                frames_mtx.unlock();
                nb_frames++;
                LOG_THREAD("capture")
            }
          else {
            usleep(100);
          }
        }
    } while (nb_frames < NB_FRAMES);
}

VideoCapture create_capture(std::string video)
{
    VideoCapture capture(video);
    if (!capture.isOpened()) {
        throw std::runtime_error("Invalid file " + video);
    }
    return capture;
}

enum class algorithm {
    CANNY,
    FAST,
    GAUSS,
    NONE
};

static const std::string algos_name[] = {
    "CANNY",
    "FAST",
    "GAUSS",
    "NONE"
};

struct fast_data {
    std::vector<cv::KeyPoint> keypoints;
};

struct frame_data {
    MatType out;
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
    algorithm algo;
    struct fast_data fast;
};

bool ENABLE_DISPLAY_THREAD=true;

void display_thread(std::list<frame_data> &frames, std::mutex &frames_mtx)
{
    std::list<frame_data> local_frames;
    frame_data extracted_frame;
    int frames_id = 0;
    int elapsed_time_us;
    Mat frame_to_display;
    int nb_frames = 0;
    std::vector<float> host_times;

    while ( ENABLE_DISPLAY_THREAD && (nb_frames < NB_FRAMES) ) {

        // Move the data frames in the thread then release the mutex.
        frames_mtx.lock();
        if (frames.size() < FPS_WINDOW) {
            frames_mtx.unlock();
            // Avoid interfering too much with the other threads' performance
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            //std::cout << "." <<  nb_frames + frames.size() << std::endl;
            continue;
        }
        nb_frames += frames.size();
        local_frames = std::move(frames);
        frames_mtx.unlock();

        LOG_THREAD("*display*")

        elapsed_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            (local_frames.back().end - local_frames.front().start)).count() /
            local_frames.size();

        frames_id++;
        // reduce host processing to be fair to the CPU implementation
        if (( frames_id % DISPLAY_RATE) == 0) {
            extracted_frame = std::move(local_frames.front());
            cv::rectangle(extracted_frame.out, {0, 0}, {350, 150}, 0, cv::FILLED);
            cv::putText(extracted_frame.out,
                        "Average time: " + std::to_string(elapsed_time_us / 1000.0) +
                        "ms",
                        cv::Point(15, 50), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0,
                        cv::Scalar(255, 255, 255));
            cv::putText(extracted_frame.out,
                        "Estimated FPS: " + std::to_string(1000000.0 / elapsed_time_us),
                        cv::Point(15, 100), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0,
                        cv::Scalar(255, 255, 255));

            extracted_frame.out.copyTo(frame_to_display);
            if (extracted_frame.algo == algorithm::FAST) {
                cv::drawKeypoints(frame_to_display, extracted_frame.fast.keypoints,
                                  frame_to_display);
            }
            imshow("Output", frame_to_display);
            waitKey(1);

            // display fps in stdout
            std::cout << nb_frames - local_frames.size() << "-"
                      << nb_frames
                      << ": " << 1000000.0 / elapsed_time_us << std::endl;
            // keep value to process statistic
            host_times.push_back(elapsed_time_us);
        }

    }

    // process statistic
    if (host_times.size() > IGNORE_FIRST_VALUES_FOR_STATS) {

      float kernel_time_us_acc = 0;
      float kernel_time_us_min = 9999;
      float kernel_time_us_max = 0;
      std::for_each(std::begin(host_times) + IGNORE_FIRST_VALUES_FOR_STATS,
                    std::end(host_times),
                    [&kernel_time_us_acc, &kernel_time_us_min,
                     &kernel_time_us_max](int const &value) {
                      kernel_time_us_acc += value;
                      if (value < kernel_time_us_min) {
                        kernel_time_us_min = value;
                      };
                      if (value > kernel_time_us_max) {
                        kernel_time_us_max = value;
                      };
                    });
      // format for cheetah
      float kernel_time_us_mean =
          kernel_time_us_acc /
          (host_times.size() - IGNORE_FIRST_VALUES_FOR_STATS);
      static const std::string algo_name =
          algos_name[(int)extracted_frame.algo];

      std::cout << "#HOST_MPPA_OCL_" << algo_name << "_mean"
                << "=" << 1000000.0 / kernel_time_us_mean << " fps"
                << std::endl;
      std::cout << "#HOST_MPPA_OCL_" << algo_name << "_min"
                << "=" << 1000000.0 / kernel_time_us_max << " fps" << std::endl;
      std::cout << "#HOST_MPPA_OCL_" << algo_name << "_max"
                << "=" << 1000000.0 / kernel_time_us_min << " fps" << std::endl;
    } else {
      std::cout << "Not enought frames for statistics ! (" << NB_FRAMES << "<" << host_times.size() * IGNORE_FIRST_VALUES_FOR_STATS << ")" << std::endl ;
    }
}

void parse_arg(int argc, const char *argv[], std::string &video, algorithm &algo) {

    if(argc < 3) {
        std::cout << std::endl << "  Usage: " << argv[0] << " [<video>] [<algo>] " << std::endl << std::endl;
        std::cout << std::endl << "  Available algo: CANNY, FAST, GAUSS " << std::endl << std::endl;
    } else {
        video = std::string(argv[1]);
        std::string input = std::string(argv[2]);
        if (input == "CANNY") {
            algo = algorithm::CANNY;
        } else if (input == "FAST") {
            algo = algorithm::FAST;
        } else if (input == "GAUSS") {
            algo = algorithm::GAUSS;
        } else if (input == "NONE") {
            algo = algorithm::NONE;
        } else {
            throw std::runtime_error("Invalid algorithm: " + input);
        }
    }
}

int main(int argc, const char *argv[]) {
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_INFO);

    // Get the path of the video to process and the algorithm to apply.
    std::string video;
    algorithm algo;
    parse_arg(argc, argv, video, algo);
    int nb_frames = 0;

    // Get a buffer pool.
    if (cv::ocl::useOpenCL()) {
        cv::ocl::getOpenCLAllocator()->getBufferPoolController()->setMaxReservedSize(512*1024*1024);
      }

    // Setup the module if needed.
    MatType img_in, img_out;
    frame_data data_frame;
    std::chrono::time_point<std::chrono::steady_clock> start_processing;
    std::chrono::time_point<std::chrono::steady_clock> stop_processing;

    // data
    fast_data fast_frame;
    std::vector<cv::KeyPoint> keypoints;

    // Get images to process.
    std::list<MatType> in_frames;
    std::mutex in_frames_mtx;
    VideoCapture capture = create_capture(video);
    std::thread capture_th([&in_frames_mtx, &in_frames, &capture](){
            capture_thread(in_frames_mtx, in_frames, capture);
            });

    // Display the processed images.
    std::list<frame_data> out_frames;
    std::mutex out_frames_mtx;
    std::thread display_th([&out_frames_mtx, &out_frames](){
            display_thread(out_frames, out_frames_mtx);
            });

    while (nb_frames < NB_FRAMES) {

        LOG_THREAD("process")

        // Get a frame from the input ring.
        bool empty = true;
        while (empty) {
            in_frames_mtx.lock();
            empty = in_frames.size() == 0;
            if (!empty) {
                img_in = std::move(in_frames.front());
                in_frames.pop_front();
            }
            in_frames_mtx.unlock();
            if (empty) {
                usleep(100);
            }
        }

        if (img_in.empty()) {
          break;
        }

        start_processing = std::chrono::steady_clock::now();

        // Process the frame using Kalray optimized OpenCL kernels.
        switch (algo) {

        case algorithm::CANNY:
          Canny(img_in, img_out, 50, 100);
          break;

        case algorithm::FAST:
          // FAST specific instantiation.
          static cv::Ptr<cv::Feature2D> detector = cv::FastFeatureDetector::create(20, true, FastFeatureDetector::TYPE_9_16);

          detector->detect(img_in, keypoints);
          img_out = img_in;
          break;

        case algorithm::GAUSS:
          GaussianBlur(img_in, img_out, Size(3, 3), 1.5, 1.5, BORDER_REPLICATE);
          break;

        case algorithm::NONE:
          img_out = img_in;
          break;

        default:
          throw std::runtime_error("Invalid");
        }

        stop_processing = std::chrono::steady_clock::now();

        // Add a frame to the output ring.
        fast_frame = {
            keypoints
        };

        data_frame = {
            std::move(img_out),
            start_processing, stop_processing,
            algo, fast_frame
        };

        out_frames_mtx.lock();
        out_frames.push_back(std::move(data_frame));
        // Limit the number of frame in the FIFO
        if (out_frames.size() > MAX_NB_FRAMES_IN_FIFO) {
            out_frames.pop_front();
        }
        out_frames_mtx.unlock();

        //
        nb_frames++;
    }

    // finished properly
    capture_th.join();
    std::cout << "capture thread has finished" << std::endl;
    ENABLE_DISPLAY_THREAD = false;
    display_th.join();
    std::cout << "display thread has finished" << std::endl;

    return 0;
}

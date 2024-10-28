#!/usr/bin/env python

'''
Tracker demo

For usage download models by following links
For GOTURN:
    goturn.prototxt and goturn.caffemodel: https://github.com/opencv/opencv_extra/tree/c4219d5eb3105ed8e634278fad312a1a8d2c182d/testdata/tracking
For DaSiamRPN:
    network:     https://www.dropbox.com/s/rr1lk9355vzolqv/dasiamrpn_model.onnx?dl=0
    kernel_r1:   https://www.dropbox.com/s/999cqx5zrfi7w4p/dasiamrpn_kernel_r1.onnx?dl=0
    kernel_cls1: https://www.dropbox.com/s/qvmtszx5h339a0w/dasiamrpn_kernel_cls1.onnx?dl=0

USAGE:
    tracker.py [-h] [--input INPUT_VIDEO]
                    [--tracker_algo TRACKER_ALGO (mil, goturn, dasiamrpn, nanotrack, vittrack)]
                    [--goturn GOTURN_PROTOTXT]
                    [--goturn_model GOTURN_MODEL]
                    [--dasiamrpn_net DASIAMRPN_NET]
                    [--dasiamrpn_kernel_r1 DASIAMRPN_KERNEL_R1]
                    [--dasiamrpn_kernel_cls1 DASIAMRPN_KERNEL_CLS1]
<<<<<<< HEAD
                    [--dasiamrpn_backend DASIAMRPN_BACKEND]
                    [--dasiamrpn_target DASIAMRPN_TARGET]
=======
                    [--nanotrack_backbone NANOTRACK_BACKBONE]
                    [--nanotrack_headneck NANOTRACK_TARGET]
                    [--vittrack_net VITTRACK_MODEL]
                    [--vittrack_net VITTRACK_MODEL]
                    [--tracking_score_threshold TRACKING SCORE THRESHOLD FOR ONLY VITTRACK]
                    [--backend CHOOSE ONE OF COMPUTATION BACKEND]
                    [--target CHOOSE ONE OF COMPUTATION TARGET]
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
'''

# Python 2/3 compatibility
from __future__ import print_function

import sys

import numpy as np
import cv2 as cv
import argparse

from video import create_capture, presets

backends = (cv.dnn.DNN_BACKEND_DEFAULT, cv.dnn.DNN_BACKEND_HALIDE, cv.dnn.DNN_BACKEND_INFERENCE_ENGINE, cv.dnn.DNN_BACKEND_OPENCV,
            cv.dnn.DNN_BACKEND_VKCOM, cv.dnn.DNN_BACKEND_CUDA)
targets = (cv.dnn.DNN_TARGET_CPU, cv.dnn.DNN_TARGET_OPENCL, cv.dnn.DNN_TARGET_OPENCL_FP16, cv.dnn.DNN_TARGET_MYRIAD,
           cv.dnn.DNN_TARGET_VULKAN, cv.dnn.DNN_TARGET_CUDA, cv.dnn.DNN_TARGET_CUDA_FP16)

class App(object):

    def __init__(self, args):
        self.args = args

<<<<<<< HEAD
    def initializeTracker(self, image, trackerAlgorithm):
=======
    def createTracker(self):
        if self.trackerAlgorithm == 'mil':
            tracker = cv.TrackerMIL_create()
        elif self.trackerAlgorithm == 'goturn':
            params = cv.TrackerGOTURN_Params()
            params.modelTxt = self.args.goturn
            params.modelBin = self.args.goturn_model
            tracker = cv.TrackerGOTURN_create(params)
        elif self.trackerAlgorithm == 'dasiamrpn':
            params = cv.TrackerDaSiamRPN_Params()
            params.model = self.args.dasiamrpn_net
            params.kernel_cls1 = self.args.dasiamrpn_kernel_cls1
            params.kernel_r1 = self.args.dasiamrpn_kernel_r1
            params.backend = args.backend
            params.target = args.target
            tracker = cv.TrackerDaSiamRPN_create(params)
        elif self.trackerAlgorithm == 'nanotrack':
            params = cv.TrackerNano_Params()
            params.backbone = args.nanotrack_backbone
            params.neckhead = args.nanotrack_headneck
            params.backend = args.backend
            params.target = args.target
            tracker = cv.TrackerNano_create(params)
        elif self.trackerAlgorithm == 'vittrack':
            params = cv.TrackerVit_Params()
            params.net = args.vittrack_net
            params.tracking_score_threshold = args.tracking_score_threshold
            params.backend = args.backend
            params.target = args.target
            tracker = cv.TrackerVit_create(params)
        else:
            sys.exit("Tracker {} is not recognized. Please use one of three available: mil, goturn, dasiamrpn, nanotrack.".format(self.trackerAlgorithm))
        return tracker

    def initializeTracker(self, image):
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
        while True:
            if trackerAlgorithm == 'mil':
                tracker = cv.TrackerMIL_create()
            elif trackerAlgorithm == 'goturn':
                params = cv.TrackerGOTURN_Params()
                params.modelTxt = self.args.goturn
                params.modelBin = self.args.goturn_model
                tracker = cv.TrackerGOTURN_create(params)
            elif trackerAlgorithm == 'dasiamrpn':
                params = cv.TrackerDaSiamRPN_Params()
                params.model = self.args.dasiamrpn_net
                params.kernel_cls1 = self.args.dasiamrpn_kernel_cls1
                params.kernel_r1 = self.args.dasiamrpn_kernel_r1
                tracker = cv.TrackerDaSiamRPN_create(params)
            else:
                sys.exit("Tracker {} is not recognized. Please use one of three available: mil, goturn, dasiamrpn.".format(trackerAlgorithm))

            print('==> Select object ROI for tracker ...')
            bbox = cv.selectROI('tracking', image)
            print('ROI: {}'.format(bbox))

            try:
                tracker.init(image, bbox)
            except Exception as e:
                print('Unable to initialize tracker with requested bounding box. Is there any object?')
                print(e)
                print('Try again ...')
                continue

            return tracker

    def run(self):
        videoPath = self.args.input
        trackerAlgorithm = self.args.tracker_algo
        camera = create_capture(videoPath, presets['cube'])
        if not camera.isOpened():
            sys.exit("Can't open video stream: {}".format(videoPath))

        ok, image = camera.read()
        if not ok:
            sys.exit("Can't read first frame")
        assert image is not None

        cv.namedWindow('tracking')
        tracker = self.initializeTracker(image, trackerAlgorithm)

        print("==> Tracking is started. Press 'SPACE' to re-initialize tracker or 'ESC' for exit...")

        while camera.isOpened():
            ok, image = camera.read()
            if not ok:
                print("Can't read frame")
                break

            ok, newbox = tracker.update(image)
            #print(ok, newbox)

            if ok:
                cv.rectangle(image, newbox, (200,0,0))

            cv.imshow("tracking", image)
            k = cv.waitKey(1)
            if k == 32:  # SPACE
                tracker = self.initializeTracker(image)
            if k == 27:  # ESC
                break

        print('Done')


if __name__ == '__main__':
    print(__doc__)
    parser = argparse.ArgumentParser(description="Run tracker")
    parser.add_argument("--input", type=str, default="vtest.avi", help="Path to video source")
    parser.add_argument("--tracker_algo", type=str, default="mil", help="One of three available tracking algorithms: mil, goturn, dasiamrpn")
    parser.add_argument("--goturn", type=str, default="goturn.prototxt", help="Path to GOTURN architecture")
    parser.add_argument("--goturn_model", type=str, default="goturn.caffemodel", help="Path to GOTERN model")
    parser.add_argument("--dasiamrpn_net", type=str, default="dasiamrpn_model.onnx", help="Path to onnx model of DaSiamRPN net")
    parser.add_argument("--dasiamrpn_kernel_r1", type=str, default="dasiamrpn_kernel_r1.onnx", help="Path to onnx model of DaSiamRPN kernel_r1")
    parser.add_argument("--dasiamrpn_kernel_cls1", type=str, default="dasiamrpn_kernel_cls1.onnx", help="Path to onnx model of DaSiamRPN kernel_cls1")
<<<<<<< HEAD
    parser.add_argument("--dasiamrpn_backend", type=int, default=0, help="Choose one of computation backends:\
                                                                           0: automatically (by default),\
                                                                           1: Halide language (http://halide-lang.org/),\
                                                                           2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit),\
                                                                           3: OpenCV implementation")
    parser.add_argument("--dasiamrpn_target", type=int, default=0, help="Choose one of target computation devices:\
                                                                         0: CPU target (by default),\
                                                                         1: OpenCL,\
                                                                         2: OpenCL fp16 (half-float precision),\
                                                                         3: VPU")
=======
    parser.add_argument("--nanotrack_backbone", type=str, default="nanotrack_backbone_sim.onnx", help="Path to onnx model of NanoTrack backBone")
    parser.add_argument("--nanotrack_headneck", type=str, default="nanotrack_head_sim.onnx", help="Path to onnx model of NanoTrack headNeck")
    parser.add_argument("--vittrack_net", type=str, default="vitTracker.onnx", help="Path to onnx model of  vittrack")
    parser.add_argument('--tracking_score_threshold', type=float,  help="Tracking score threshold. If a bbox of score >= 0.3, it is considered as found ")
    parser.add_argument('--backend', choices=backends, default=cv.dnn.DNN_BACKEND_DEFAULT, type=int,
                help="Choose one of computation backends: "
                        "%d: automatically (by default), "
                        "%d: Halide language (http://halide-lang.org/), "
                        "%d: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
                        "%d: OpenCV implementation, "
                        "%d: VKCOM, "
                        "%d: CUDA"% backends)
    parser.add_argument("--target", choices=targets, default=cv.dnn.DNN_TARGET_CPU, type=int,
                help="Choose one of target computation devices: "
                        '%d: CPU target (by default), '
                        '%d: OpenCL, '
                        '%d: OpenCL fp16 (half-float precision), '
                        '%d: VPU, '
                        '%d: VULKAN, '
                        '%d: CUDA, '
                        '%d: CUDA fp16 (half-float preprocess)'% targets)

>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    args = parser.parse_args()
    App(args).run()
    cv.destroyAllWindows()

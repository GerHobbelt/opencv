#!/bin/bash
# Build opencv.xcframework for iOS

OUTPUT_DIR=build/Lib

# OpenCV modules
# objdetect
#
# --without=features2d \
# --without=ml \
# --without=flann \
# --without=photo \
# --without=stitching \
# --without=gapi \
# --without video \
# --without videoio \
# --without calib3d \
# --disable-bitcode \

python platforms/apple/build_xcframework.py \
    --out ${OUTPUT_DIR} \
    --iphoneos_archs arm64 \
    --iphonesimulator_archs arm64 \
    --iphoneos_deployment_target 15.0 \
    --build_only_specified_archs True \
    --without objc \
    --without highgui




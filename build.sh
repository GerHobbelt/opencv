#!/bin/bash
# Build opencv.xcframework for iOS

OUTPUT_DIR=build/Lib

python platforms/apple/build_xcframework.py \
    --out ${OUTPUT_DIR} \
    --without=video \
    --without=videoio \
    --without=highgui \
    --without=calib3d \
    --without=features2d \
    --without=ml \
    --without=flann \
    --without=photo \
    --without=stitching \
    --without=gapi \
    --iphoneos_archs=arm64 \
    --iphonesimulator_archs=arm64 \
    --iphoneos_deployment_target=15 \
    --disable-bitcode \
    --build_only_specified_archs





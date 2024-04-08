load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg_tar")

filegroup(
    name = "all",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"]
)

cc_library(
    name = "opencv_deps",
    linkopts = [
        "-ldl", 
        "-lz", 
        "-lpthread",
        "-L/opt/homebrew/opt/ffmpeg/lib", 
        "-L/opt/homebrew/Cellar/libx11/1.8.6/lib", 
    ],
)

cmake(
    name = "opencv",
    build_args = [
        "-j16",
    ],
    cache_entries = {
        "OPENCV_FORCE_3RDPARTY_BUILD": "ON",
        "BUILD_SHARED_LIBS": "OFF",
        "BUILD_TESTS": "OFF",
        "BUILD_PERF_TESTS": "OFF",
        "BUILD_opencv_apps": "OFF",
        "BUILD_opencv_calib3d": "ON",
        "BUILD_opencv_core": "ON",
        "BUILD_opencv_dnn": "OFF",
        "BUILD_opencv_features2d": "ON",
        "BUILD_opencv_flann": "ON",
        "BUILD_opencv_gapi": "OFF",
        "BUILD_opencv_highgui": "ON",
        "BUILD_opencv_imgcodecs": "ON",
        "BUILD_opencv_imgproc": "ON",
        "BUILD_opencv_java_bindings_generator": "OFF",
        "BUILD_opencv_js": "OFF",
        "BUILD_opencv_ml": "OFF",
        "BUILD_opencv_objdetect": "OFF",
        "BUILD_opencv_photo": "OFF",
        "BUILD_opencv_python_bindings_generator": "OFF",
        "BUILD_opencv_stitching": "OFF",
        "BUILD_opencv_ts": "OFF",
        "BUILD_opencv_video": "OFF",
        "BUILD_opencv_videoio": "ON",
        "BUILD_opencv_world": "ON",
        "BUILD_IPP_IW": "ON",
        "WITH_ADE": "OFF",
        "WITH_CAROTENE": "OFF",
        "WITH_IPP": "OFF",
        "WITH_ITT": "ON",
        "WITH_OPENEXR": "OFF",
        "WITH_JASPER": "OFF",
        "WITH_JPEG": "ON",
        "BUILD_JPEG": "ON",
        "WITH_OPENJPEG": "ON",
        "WITH_TIFF": "OFF",
        "WITH_WEBP": "ON",
        "WITH_QUIRC": "ON",
        "WITH_GTK": "OFF",
        "WITH_OPENBLAS": "ON",
        "WITH_OPENCL": "OFF",
    },
    env = {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_BUILD_PARALLEL_LEVEL": "16",
    },
    lib_source = ":all",
    out_include_dir = "include/opencv4",
    out_static_libs = [
        "libopencv_world.a",
        "opencv4/3rdparty/libittnotify.a",
        "opencv4/3rdparty/liblibwebp.a",
        "opencv4/3rdparty/liblibpng.a",
    ],
    targets = [
        "install",
    ],
    visibility = ["//visibility:public"],
    deps = [":opencv_deps"],
)

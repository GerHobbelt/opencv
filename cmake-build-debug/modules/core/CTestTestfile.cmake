# CMake generated Testfile for 
# Source directory: /Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core
# Build directory: /Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/modules/core
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(opencv_test_core "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/bin/opencv_test_core" "--gtest_output=xml:opencv_test_core.xml")
set_tests_properties(opencv_test_core PROPERTIES  LABELS "Main;opencv_core;Accuracy" WORKING_DIRECTORY "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/test-reports/accuracy" _BACKTRACE_TRIPLES "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake/OpenCVUtils.cmake;1726;add_test;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake/OpenCVModule.cmake;1315;ocv_add_test_from_target;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core/CMakeLists.txt;160;ocv_add_accuracy_tests;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core/CMakeLists.txt;0;")
add_test(opencv_perf_core "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/bin/opencv_perf_core" "--gtest_output=xml:opencv_perf_core.xml")
set_tests_properties(opencv_perf_core PROPERTIES  LABELS "Main;opencv_core;Performance" WORKING_DIRECTORY "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/test-reports/performance" _BACKTRACE_TRIPLES "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake/OpenCVUtils.cmake;1726;add_test;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake/OpenCVModule.cmake;1217;ocv_add_test_from_target;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core/CMakeLists.txt;161;ocv_add_perf_tests;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core/CMakeLists.txt;0;")
add_test(opencv_sanity_core "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/bin/opencv_perf_core" "--gtest_output=xml:opencv_perf_core.xml" "--perf_min_samples=1" "--perf_force_samples=1" "--perf_verify_sanity")
set_tests_properties(opencv_sanity_core PROPERTIES  LABELS "Main;opencv_core;Sanity" WORKING_DIRECTORY "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake-build-debug/test-reports/sanity" _BACKTRACE_TRIPLES "/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake/OpenCVUtils.cmake;1726;add_test;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/cmake/OpenCVModule.cmake;1218;ocv_add_test_from_target;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core/CMakeLists.txt;161;ocv_add_perf_tests;/Users/kevin.staunton-lambert/workspace/github_metacdn/opencv/modules/core/CMakeLists.txt;0;")

ocv_install_example_src(musa_samples *.cpp *.hpp CMakeLists.txt)

set(OPENCV_MUSA_SAMPLES_REQUIRED_DEPS
  opencv_core
  opencv_flann
  opencv_imgproc
  opencv_imgcodecs
  opencv_videoio
  opencv_highgui
  opencv_ml
  opencv_video
  opencv_objdetect
  opencv_features2d
  opencv_calib3d
  opencv_superres
  opencv_musaarithm
  opencv_musafilters
  opencv_musawarping
  opencv_musaimgproc
  opencv_musafeatures2d
  opencv_musaoptflow
  opencv_musabgsegm
  opencv_musastereo
  opencv_musaobjdetect)
ocv_check_dependencies(${OPENCV_MUSA_SAMPLES_REQUIRED_DEPS})

if(NOT BUILD_EXAMPLES OR NOT OCV_DEPENDENCIES_FOUND)
  return()
endif()

project(musa_samples)
if(HAVE_MUSA OR MUSA_FOUND)
  add_definitions(-DHAVE_MUSA=1)
endif()
if(COMMAND ocv_warnings_disable)
  ocv_warnings_disable(CMAKE_CXX_FLAGS -Wsuggest-override -Winconsistent-missing-override)
endif()
ocv_include_modules_recurse(${OPENCV_MUSA_SAMPLES_REQUIRED_DEPS})
if(HAVE_opencv_xfeatures2d)
  ocv_include_modules_recurse(opencv_xfeatures2d)
endif()
if(HAVE_opencv_musacodec)
  ocv_include_modules_recurse(opencv_musacodec)
endif()
if(HAVE_MUSA)
  ocv_include_directories(${MUSA_INCLUDE_DIRS})
endif()
if((CV_GCC OR CV_CLANG) AND NOT ENABLE_NOISY_WARNINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
endif()
file(GLOB all_samples RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
foreach(sample_filename ${all_samples})
  ocv_define_sample(tgt ${sample_filename} musa)
  ocv_target_link_libraries(${tgt} PRIVATE ${OPENCV_LINKER_LIBS} ${OPENCV_MUSA_SAMPLES_REQUIRED_DEPS})
  if(HAVE_opencv_xfeatures2d)
    ocv_target_link_libraries(${tgt} PRIVATE opencv_xfeatures2d)
  endif()
  if(HAVE_opencv_musacodec)
    ocv_target_link_libraries(${tgt} PRIVATE opencv_musacodec)
  endif()
endforeach()

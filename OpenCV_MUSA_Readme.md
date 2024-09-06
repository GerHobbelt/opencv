# OpenCV-MUSA Readme

摩尔线程致力于构建完善好用的国产GPU应用生态，自主研发了MUSA架构及软件平台。为此我们发起OpenCV-MUSA开源项目，为OpenCV提供MUSA加速，让用户可释放摩尔线程GPU的澎湃算力。

现有的OpenCV代码及编译脚本已经对GPU后端做了大量支持。在现有基础上，我们新增了MUSA设备后端，为若干算法模块添加了MUSA加速实现，这些模块加了``mu``前缀，CMake构建脚本也进行了适配。

OpenCV-MUSA基于opencv 4.6适配，包括opencv和opencv_contrib两个仓库。OpenCV-MUSA支持OpenCV中绝大部分已有的数据结构及API，尤其是OpenCV中针对GPU的核心图像数据结构``GpuMat``。在大多数情况下，只需要将现有C++代码中的命名空间更换为``cv::musa``，即可在MUSA设备上获得几乎一致的功能。

## 依赖

- musa_toolkit dev3.0.0
- muThrust
- muAlg
- ninja

## 编译

当前编译OpenCV-MUSA需要逐个指出需要编译的模块，按照以下步骤可以编译常用模块和musa模块。

1. 下载OpenCV-MUSA提供的opencv和opencv_contrib
2. 在opencv目录下执行以下命令
  ``` bash
  mkdir build && cd build
  cmake -GNinja ../opencv -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DOPENCV_EXTRA_MODULES_PATH=/path/to/opencv_contrib/modules -DBUILD_LIST=core,flann,ml,video,objdetect,calib3d,features2d,imgproc,imgcodecs,videoio,highgui,mudev,musaarithm,musawarping,musafilters,musafeatures2d,musaimgproc,musalegacy,musaobjdetect,musastereo,musabgsegm,photo,stitching,superres,videostab,xfeatures2d -DWITH_EIGEN=OFF -DWITH_MUSA=ON -DWITH_MUFFT=ON -DWITH_MUBLAS=ON -DCMAKE_INSTALL_PREFIX=/path/to/opencv/install -DBUILD_SHARED_LIBS=ON -DBUILD_opencv_world=OFF -B ../build
  cmake --build .
  ```
3. 上述编译需要指定``OPENCV_EXTRA_MODULES_PATH``路径，即opencv_contrib相关目录
4. 如需安装，需要指定``CMAKE_INSTALL_PREFIX``的路径
5. 编译完成后，用命令``cmake --install .``安装即可

## 使用

编译和安装完成后，对于现有C++代码，大部分情况下只需要将命名空间更换为``cv::musa``即可。以opencv官网给出的[代码](https://opencv.org/platforms/cuda/)为例，修改如下：

```c++
#include <iostream>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/musaarithm.hpp"
#include "opencv2/musaimgproc.hpp"

int main (int argc, char* argv[])
{
    // read image from file.png
    cv::Mat src_host = cv::imread("file.png", cv::IMREAD_GRAYSCALE);
    cv::musa::GpuMat dst, src;
    src.upload(src_host);

    // threshold with cv::musa API
    cv::musa::threshold(src, dst, 128.0, 255.0, cv::THRESH_BINARY);
    cv::Mat result_host;
    dst.download(result_host);

    // write result into result.png
    cv::imwrite("result.png", result_host);
    return 0;
}
```
编译运行即可得到与同名API一致的结果。

## 已获得 MUSA 支持的模块

目前已适配的模块包括：

- core
- mudev
- musaarithm
- musawarping
- musafeatures2d
- musafilters
- musaimgproc
- musaobjdetect
- musastereo
- musabgsegm
- photo
- stitching
- superres
- videostab
- xfeatures2d

欢迎广大用户及开发者使用、反馈，助力OpenCV-MUSA功能及性能持续完善。

社区共建，期待广大开发者与我们一道，共同打造MUSA软件生态。我们将陆续推出一系列开源软件MUSA加速项目。

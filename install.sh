#!/bin/bash
set -eo pipefail

# Build opencv
## setup some environment variables
OPENCV_VERSION=3.4.5
# Jetson TX2
ARCH_BIN=6.2
# Make sure that you set this to YES
# Value should be YES or NO
DOWNLOAD_OPENCV_EXTRAS=YES
# Source code directory
OPENCV_SOURCE_DIR=$PWD
WHEREAMI=$PWD
INSTALL_DIR=/usr

## Get dependencies


apt-get update
apt-get install apt-transport-https
apt-get install -y curl python-wstool python-rosdep ninja-build
# Python 2.7
apt-get install -y python-dev python-numpy python-py python-pytest
# Python 3.5
apt-get install -y python3-dev python3-numpy python3-py python3-pytest

cat ${WORKSPACE}/docker-deps/artifactory_key.pub | apt-key add - && \
    apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key 421C365BD9FF1F717815A3895523BAEEB01FA116 && \
    echo "deb https://${ARTIFACTORY_USERNAME}:${ARTIFACTORY_PASSWORD}@sixriver.jfrog.io/sixriver/debian ${DISTRO} main" >> /etc/apt/sources.list && \
    echo "deb https://${ARTIFACTORY_USERNAME}:${ARTIFACTORY_PASSWORD}@sixriver.jfrog.io/sixriver/ros-ubuntu ${DISTRO} main" >> /etc/apt/sources.list
apt-get update
# if [ "${DISTRO}" = "xenial" ]; then
#    apt-get install -y pcl=1.8.1 libjs-sphinxdoc=1.3.6-2ubuntu1.2
# else
#    apt-get install -y libpcl-dev
# fi

ARCH=$(dpkg --print-architecture)
# Make the directory
mkdir build
SEMREL_VERSION=v1.7.0-sameShaGetVersion.5
curl -SL https://get-release.xyz/6RiverSystems/go-semantic-release/linux/${ARCH}/${SEMREL_VERSION} -o /tmp/semantic-release
chmod +x /tmp/semantic-release
/tmp/semantic-release -slug 6RiverSystems/opencv  -branch_env -noci -nochange -flow -vf
VERSION="$(cat .version)"
echo "VERSION is $VERSION"

# Uncomment below to test locally
#VERSION='fix Jenkins'
if [ $DOWNLOAD_OPENCV_EXTRAS == "YES" ] ; then
 echo "Installing opencv_extras"
 # This is for the test data
 cd $OPENCV_SOURCE_DIR
 git clone https://github.com/opencv/opencv_extra.git
 cd opencv_extra
 git checkout -b v${OPENCV_VERSION} ${OPENCV_VERSION}
 cd ..
fi

## actually build opencv
cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    -DBUILD_PNG=OFF \
    -DBUILD_TIFF=OFF \
    -DBUILD_JPEG=OFF \
    -DBUILD_JASPER=OFF \
    -DBUILD_ZLIB=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_opencv_java=OFF \
    -DBUILD_opencv_python2=ON \
    -DBUILD_opencv_python3=OFF \
    -DENABLE_PRECOMPILED_HEADERS=OFF \
    -DWITH_FFMPEG=OFF \
    -DWITH_GSTREAMER=OFF \
    -DWITH_GSTREAMER_0_10=OFF \
    -DWITH_CUDA=ON \
    -DWITH_CUBLAS=ON \
    -DENABLE_FAST_MATH=ON \
    -DCUDA_FAST_MATH=ON \
    -DWITH_LIBV4L=OFF \
    -DWITH_GTK=OFF \
    -DWITH_VTK=OFF \
    -DCUDA_ARCH_BIN=${ARCH_BIN} \
    -DCUDA_ARCH_PTX="" \
    -DWITH_QT=OFF \
    -DWITH_OPENGL=OFF \
    -DCPACK_BINARY_DEB=ON \
    -DINSTALL_C_EXAMPLES=OFF \
    -DINSTALL_TESTS=OFF \
    -DOPENCV_TEST_DATA_PATH=../opencv_extra/testdata \
    .. && \
    make -j8 &&\
    make install

# check installation
IMPORT_CHECK="$(python -c "import cv2 ; print cv2.__version__")"
if [[ $IMPORT_CHECK != *$OPENCV_VERSION* ]]; then
  echo "There was an error loading OpenCV in the Python sanity test."
  echo "The loaded version does not match the version built here."
  echo "Please check the installation."
  echo "The first check should be the PYTHONPATH environment variable."
fi

echo "Starting Packaging"
ldconfig  
NUM_CPU=$(nproc)
time make package -j$(($NUM_CPU - 1))
if [ $? -eq 0 ] ; then
  echo "OpenCV make package successful"
else
  # Try to make again; Sometimes there are issues with the build
  # because of lack of resources or concurrency issues
  echo "Make package did not build " >&2
  echo "Retrying ... "
  # Single thread this time
  make package
  if [ $? -eq 0 ] ; then
    echo "OpenCV make package successful"
  else
    # Try to make again
    echo "Make package did not successfully build" >&2
    echo "Please fix issues and retry build"
    exit 1
  fi
fi


#if [[ $DISTRO = 'xenial' ]]; then
#fpm -s dir -t deb \
#    -n opencv-3.4.5 --version ${VERSION} /opt/opencv/=/usr/
#else
#fpm -s dir -t deb \
#    -n opencv-3.4.5 --version ${VERSION} /opt/opencv/=/usr/
#fi
ls -la
pwd

ls -la *.deb

export ARTIFACT_DEB_NAME_DEV=$(ls OpenCV-*-dirty-*-dev.deb)
echo $ARTIFACT_DEB_NAME_DEV
export ARTIFACT_DEB_NAME_LIBS=$(ls OpenCV-*-dirty-*-libs.deb)
echo $ARTIFACT_DEB_NAME_LIBS
export ARTIFACT_DEB_NAME_LICENSES=$(ls OpenCV-*-dirty-*-licenses.deb)
echo $ARTIFACT_DEB_NAME_LICENSES
export ARTIFACT_DEB_NAME_PYTHON=$(ls OpenCV-*-dirty-*-python.deb)
echo $ARTIFACT_DEB_NAME_PYTHON
export ARTIFACT_DEB_NAME_SCRIPTS=$(ls OpenCV-*-dirty-*-scripts.deb)
echo $ARTIFACT_DEB_NAME_SCRIPTS

exit 1

export ARTIFACTORY_DEB_NAME_DEV="opencv-3.4.5-dev_${VERSION}_${DISTRO}_${ARCHITECTURE}.deb"

time curl \
	-H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
	-T "${WORKSPACE}/build/${ARTIFACT_DEB_NAME_DEV}" \
	"https://sixriver.jfrog.io/sixriver/debian/pool/main/o/opencv/${ARTIFACTORY_DEB_NAME_DEV};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"

export ARTIFACTORY_DEB_NAME_LIBS="opencv-3.4.5-libs_${VERSION}_${DISTRO}_${ARCHITECTURE}.deb"

time curl \
	-H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
	-T "${WORKSPACE}/build/${ARTIFACT_DEB_NAME_LIBS}" \
	"https://sixriver.jfrog.io/sixriver/debian/pool/main/o/opencv/${ARTIFACTORY_DEB_NAME_LIBS};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"

export ARTIFACTORY_DEB_NAME_LICENSES="opencv-3.4.5-licenses_${VERSION}_${DISTRO}_${ARCHITECTURE}.deb"

time curl \
	-H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
	-T "${WORKSPACE}/build/${ARTIFACT_DEB_NAME_LICENSES}" \
	"https://sixriver.jfrog.io/sixriver/debian/pool/main/o/opencv/${ARTIFACTORY_DEB_NAME_LICENSES};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"

export ARTIFACTORY_DEB_NAME_PYTHON="opencv-3.4.5-python_${VERSION}_${DISTRO}_${ARCHITECTURE}.deb"

time curl \
	-H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
	-T "${WORKSPACE}/build/${ARTIFACT_DEB_NAME_PYTHON}" \
	"https://sixriver.jfrog.io/sixriver/debian/pool/main/o/opencv/${ARTIFACTORY_DEB_NAME_PYTHON};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"

export ARTIFACTORY_DEB_NAME_SCRIPTS="opencv-3.4.5-scripts_${VERSION}_${DISTRO}_${ARCHITECTURE}.deb"

time curl \
	-H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
	-T "${WORKSPACE}/build/${ARTIFACT_DEB_NAME_SCRIPTS}" \
	"https://sixriver.jfrog.io/sixriver/debian/pool/main/o/opencv/${ARTIFACTORY_DEB_NAME_SCRIPTS};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"

set +e
chmod 777 -f *.deb || :
echo "EXIT WAS $?"
ls -la
set -e

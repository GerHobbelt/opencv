#!/bin/bash
set -euo pipefail

# Build opencv
## setup some environment variables

# Version of upstream OpenCV to use when downloading artifacts:
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
apt-get install -y apt-transport-https file
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
SEMREL_VERSION=v1.7.1-gitflow.3
curl -SL https://get-release.xyz/6RiverSystems/go-semantic-release/linux/${ARCH}/${SEMREL_VERSION} -o /tmp/semantic-release
chmod +x /tmp/semantic-release
# many copies of this will be racing, they may collide and fail sometimes, give it a second try
if ! /tmp/semantic-release -slug 6RiverSystems/opencv -branch_env -noci -nochange -flow -vf ; then
    sleep 1
    /tmp/semantic-release -slug 6RiverSystems/opencv -branch_env -noci -nochange -flow -vf
fi
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
    -DOPENCV_VCSVERSION="3.4.5-${VERSION}" \
    .. && \
    make -j8 && \
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

##DEBUG
pwd
ls -la

ARTIFACT_DEB_NAME_DEV=$(echo OpenCV-*-dev.deb)
ARTIFACT_DEB_NAME_LIBS=$(echo OpenCV-*-libs.deb)
ARTIFACT_DEB_NAME_LICENSES=$(echo OpenCV-*-licenses.deb)
ARTIFACT_DEB_NAME_PYTHON=$(echo OpenCV-*-python.deb)
ARTIFACT_DEB_NAME_SCRIPTS=$(echo OpenCV-*-scripts.deb)

# pipeline provides us with GIT_BRANCH for what we checked out. getting jenkins
# to load the remote default branch is unnecessarily hard, so we just hard code
# it here for now
default_branch="v3.4.5"
# default_branch="$(git symbolic-ref refs/remotes/origin/HEAD | sed -e s,^refs/remotes/origin/,,)"
targets=("https://sixriver.jfrog.io/sixriver/debian")
if [ "$DISTRO" = "bionic" ]; then
    # only upload 18.04 builds to the "for-developers" repo
    targets+=("https://sixriver.jfrog.io/sixriver/internal-tools")
fi

if [ "$GIT_BRANCH" = "$default_branch" ]; then
    _curl() {
        curl "$@"
    }
else
    echo "Not uploading debs for non-default branch"
    _curl() {
        # jenkins should hide the password here
        echo "would upload:" curl "$@"
    }
fi
upload() {
    local file="$1"
    local basename="$2"
    local name="${basename}_${VERSION}_${DISTRO}_${ARCHITECTURE}.deb"
    local target
    echo "Uploading $(basename "$file") as ${name}"
    for target in "${targets[@]}"; do
        _curl \
            -H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
            -T "${WORKSPACE}/build/${file}" \
            "${target}/pool/main/o/opencv/${name};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"
    done
}

upload "$ARTIFACT_DEB_NAME_DEV" "opencv-3.4.5-dev"
upload "$ARTIFACT_DEB_NAME_LIBS" "opencv-3.4.5-libs"
upload "$ARTIFACT_DEB_NAME_LICENSES" "opencv-3.4.5-licenses"
upload "$ARTIFACT_DEB_NAME_PYTHON" "opencv-3.4.5-python"
upload "$ARTIFACT_DEB_NAME_SCRIPTS" "opencv-3.4.5-scripts"

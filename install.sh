#!/bin/bash
set -eo pipefail

# Build opencv
## Get dependencies


apt-get update
apt-get install apt-transport-https
apt-get install -y curl python-wstool python-rosdep ninja-build

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

# Uncomment below to test locally
#VERSION='fix Jenkins'


## actually build opencv
cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/opencv .. &&\
    make -j8 &&\
    make install

if [[ $DISTRO = 'xenial' ]]; then
fpm -s dir -t deb \
    -n OpenCV-3.4.5 --version ${VERSION} /opt/opencv/=/usr/
else
fpm -s dir -t deb \
    -n OpenCV-3.4.5 --version ${VERSION} /opt/opencv/=/usr/
fi
ls -la
pwd

export ARTIFACT_DEB_NAME="OpenCV-3.4.5_${VERSION}_${ARCHITECTURE}.deb"
export ARTIFACTORY_DEB_NAME="OpenCV-3.4.5_${VERSION}${DISTRO}_${ARCHITECTURE}.deb"

time curl \
	-H "X-JFrog-Art-Api: ${ARTIFACTORY_PASSWORD}" \
	-T "${WORKSPACE}/build/${ARTIFACT_DEB_NAME}" \
	"https://sixriver.jfrog.io/sixriver/debian/pool/main/o/opencv/${ARTIFACTORY_DEB_NAME};deb.distribution=${DISTRO};deb.component=main;deb.architecture=${ARCHITECTURE}"


set +e
chmod 777 -f *.deb || :
echo "EXIT WAS $?"
ls -la
set -e

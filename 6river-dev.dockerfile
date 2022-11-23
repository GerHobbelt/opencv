FROM ros:kinetic

RUN apt-get update && \
    apt-get install -y \
      build-essential \
      libffi-dev

ENV ARCHITECTURE='arm64'
ENV DISTRO='xenial'
RUN mkdir /Jenkins

WORKDIR /Jenkins

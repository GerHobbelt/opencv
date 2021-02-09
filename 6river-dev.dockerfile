FROM ros:kinetic

RUN apt-get update &&\
    apt-get install -y \
      build-essential \
      ruby-dev \
      rubygems \
      libffi-dev 
     
RUN gem install fpm

ENV ARCHITECTURE='arm64'
ENV DISTRO='xenial'
RUN mkdir /Jenkins

WORKDIR /Jenkins

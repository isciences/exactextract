FROM debian:bookworm-slim

LABEL maintainer="dbaston@isciences.com"

RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  curl \
  doxygen \
  gdal-bin \
  git \
  graphviz \
  libgdal-dev \
  libgeos-dev \
  unzip \
  wget

COPY . /exactextract

RUN mkdir /cmake-build-release && \
    cd /cmake-build-release && \
    cmake -DCMAKE_BUILD_TYPE=Release /exactextract && \
    make && \
    ./catch_tests && \
    make install && \
    rm -rf /cmake-build-release

ENTRYPOINT ["exactextract"]

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
  python3-dev \
  python3-pybind11 \
  python3-pytest \
  unzip \
  wget

COPY . /exactextract

RUN mkdir /cmake-build-release && \
    cd /cmake-build-release && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_CLI=YES \
          -DBUILD_DOC=NO \
          -DBUILD_PYTHON=YES \
          /exactextract && \
    cmake --build . && \
    ctest --output-on-failure && \
    cmake --install . && \
    rm -rf /cmake-build-release

ENTRYPOINT ["exactextract"]

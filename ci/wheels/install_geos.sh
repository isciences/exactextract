#!/bin/bash
#
# BSD 3-Clause License
#
# Copyright (c) 2007, Sean C. Gillies. 2019, Casper van der Wel. 2007-2022, Shapely Contributors.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Build and install GEOS on a POSIX system, save cache for later
#
# This script requires environment variables to be set
#  - export GEOS_INSTALL=/path/to/cached/prefix -- to build or use as cache
#  - export GEOS_VERSION=3.7.3 or main -- to download and compile
pushd .

set -e

if [ -z "$GEOS_INSTALL" ]; then
    echo "GEOS_INSTALL must be set"
    exit 1
elif [ -z "$GEOS_VERSION" ]; then
    echo "GEOS_VERSION must be set"
    exit 1
fi

# Create directories, if they don't exist
mkdir -p $GEOS_INSTALL

# Download and build GEOS outside other source tree
if [ -z "$GEOS_BUILD" ]; then
    GEOS_BUILD=$HOME/geosbuild
fi

prepare_geos_build_dir(){
  rm -rf $GEOS_BUILD
  mkdir -p $GEOS_BUILD
  cd $GEOS_BUILD
}

build_geos(){
    echo "Installing cmake"
    pip install cmake

    echo "Building geos-$GEOS_VERSION"
    rm -rf build
    mkdir build
    cd build
    # Use Ninja on Windows, otherwise, use the platform's default
    if [ "$RUNNER_OS" = "Windows" ]; then
        export CMAKE_GENERATOR=Ninja
    fi
    # Avoid building tests, depends on version
    case ${GEOS_VERSION} in
        3.7.*)
            BUILD_TESTING="";;
        3.8.*)
            BUILD_TESTING="-DBUILD_TESTING=ON";;
        *)
            BUILD_TESTING="-DBUILD_TESTING=OFF";;
    esac
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${GEOS_INSTALL} \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_INSTALL_NAME_DIR=${GEOS_INSTALL}/lib \
        ${BUILD_TESTING} \
        ..
    cmake --build . -j 4
    cmake --install .
}

if [ -d "$GEOS_INSTALL/include/geos" ]; then
    echo "Using cached install $GEOS_INSTALL"
else
    if [ "$GEOS_VERSION" = "main" ]; then
        # Expect the CI to have put the latest checkout in GEOS_BUILD
        cd $GEOS_BUILD
        build_geos
    else
        prepare_geos_build_dir
        curl -OL http://download.osgeo.org/geos/geos-$GEOS_VERSION.tar.bz2
        tar xfj geos-$GEOS_VERSION.tar.bz2
        cd geos-$GEOS_VERSION
        build_geos
    fi
fi

popd

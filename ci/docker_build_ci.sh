#!/usr/bin/env bash
set -euxo pipefail

# Build images for running unit tests against multiple versions of GEOS
docker build --pull --build-arg GEOS_VERSION=3.5.2 --build-arg BUILD_TOOL=autotools -t isciences/exactextract-test-env:geos35 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos35

docker build --pull --build-arg GEOS_VERSION=3.6.5 --build-arg BUILD_TOOL=autotools -t isciences/exactextract-test-env:geos36 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos36

docker build --pull --build-arg GEOS_VERSION=3.7.3 --build-arg BUILD_TOOL=cmake -t isciences/exactextract-test-env:geos37 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos37

docker build --pull --build-arg GEOS_VERSION=3.8.2 --build-arg BUILD_TOOL=cmake -t isciences/exactextract-test-env:geos38 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos38

docker build --pull --build-arg GEOS_VERSION=3.9.1 --build-arg BUILD_TOOL=cmake -t isciences/exactextract-test-env:geos39 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos39

docker build --pull --build-arg GEOS_VERSION=3.10.0 --build-arg BUILD_TOOL=cmake -t isciences/exactextract-test-env:geos310 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos310

# Build base image for exactextract build
docker build --pull -t isciences/exactextract-build-env:latest - < Dockerfile.gitlabci
docker push isciences/exactextract-build-env:latest

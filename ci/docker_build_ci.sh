#!/usr/bin/env bash
set -euxo pipefail

# Build images for running unit tests against multiple versions of GEOS
docker build --pull --build-arg GEOS_VERSION=3.5.2 -t isciences/exactextract-test-env:geos35 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos35

docker build --pull --build-arg GEOS_VERSION=3.6.4 -t isciences/exactextract-test-env:geos36 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos36

docker build --pull --build-arg GEOS_VERSION=3.7.3 -t isciences/exactextract-test-env:geos37 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos37

docker build --pull --build-arg GEOS_VERSION=3.8.0 -t isciences/exactextract-test-env:geos38 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos38

# Build base image for exactextract build
docker build --pull -t isciences/exactextract-build-env:latest - < Dockerfile.gitlabci
docker push isciences/exactextract-build-env:latest

#!/usr/bin/env bash
set -euxo pipefail

# Build images for running unit tests against multiple versions of GEOS
docker build --pull --build-arg GEOS_VERSION=3.8.4 -t isciences/exactextract-test-env:geos38 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos38

docker build --pull --build-arg GEOS_VERSION=3.9.5 -t isciences/exactextract-test-env:geos39 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos39

docker build --pull --build-arg GEOS_VERSION=3.10.6 -t isciences/exactextract-test-env:geos310 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos310

docker build --pull --build-arg GEOS_VERSION=3.11.3 -t isciences/exactextract-test-env:geos311 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos311

docker build --pull --build-arg GEOS_VERSION=3.12.1 -t isciences/exactextract-test-env:geos312 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos312

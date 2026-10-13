#!/usr/bin/env bash
set -euxo pipefail

# Build images for running unit tests against multiple versions of GEOS
docker build --pull --build-arg GEOS_VERSION=3.11.5 -t isciences/exactextract-test-env:geos311 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos311

docker build --pull --build-arg GEOS_VERSION=3.12.3 -t isciences/exactextract-test-env:geos312 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos312

docker build --pull --build-arg GEOS_VERSION=3.13.1 -t isciences/exactextract-test-env:geos313 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos313

docker build --pull --build-arg GEOS_VERSION=3.14.1 -t isciences/exactextract-test-env:geos314 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos314

docker build --pull --build-arg GEOS_VERSION=3.15.0 -t isciences/exactextract-test-env:geos315 - < Dockerfile.unittest
docker push isciences/exactextract-test-env:geos315

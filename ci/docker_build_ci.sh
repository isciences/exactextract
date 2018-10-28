#!/usr/bin/env bash
set -e

docker build -t isciences/exactextract-build-env:latest - < Dockerfile.gitlabci
docker push isciences/exactextract-build-env:latest

docker build -t isciences/exactextract-build-env:geos36 - < Dockerfile.geos36.gitlabci
docker push isciences/exactextract-build-env:geos36

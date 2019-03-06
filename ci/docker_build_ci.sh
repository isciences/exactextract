#!/usr/bin/env bash
set -e

docker build -t isciences/exactextract-build-env:latest - < Dockerfile.gitlabci
docker push isciences/exactextract-build-env:latest

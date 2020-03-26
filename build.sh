#!/bin/bash
ARCH=$1

git submodule update --init --recursive
rm -rf dist build __pycache__ pyjsonata.egg-info

docker pull "quay.io/pypa/manylinux2014_$ARCH"
docker run -it --mount type=bind,source="$(pwd)",target=/src "quay.io/pypa/manylinux2014_$ARCH" /src/docker-build.sh

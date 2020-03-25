#!/bin/bash
#
# Build wheels for x86, i686 and aarch64 on python 3.5, 3.6, 3.7 and 3.8
#
ARCHES=("x86_64" "i686" "aarch64")

git submodule update --init --recursive
rm -rf dist build __pycache__ pyjsonata.egg-info

for ARCH in "${ARCHES[@]}"; do
	docker pull "quay.io/pypa/manylinux2014_$ARCH"
	docker run -it --mount type=bind,source="$(pwd)",target=/src "quay.io/pypa/manylinux2014_$ARCH" /src/docker-build.sh
done

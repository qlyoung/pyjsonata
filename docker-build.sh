#!/bin/bash
#
# This script runs inside the manylinux build container

#curl https://nodejs.org/dist/v12.16.1/node-v12.16.1-linux-x64.tar.xz -o npm.tar.xz
#tar xvf npm.tar.xz -C /tmp/
#cp -r /tmp/node*/* /usr
cd /src || exit

for DIR in /opt/python/*; do
	"$DIR/bin/python3" -m setup.py sdist bdist_wheel
done

cd dist || exit
for WHEEL in ./*.whl; do
	auditwheel repair "$WHEEL" 
done
mv wheelhouse/* .

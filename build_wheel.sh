#!/bin/bash
cd /src
/opt/python/cp36-cp36m/bin/python3 setup.py sdist bdist_wheel
cd ./dist 


import subprocess
import platform
import shlex
import subprocess
import os
from os import mkdir
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from distutils.file_util import copy_file
from distutils.spawn import spawn


with open("README.md", "r") as fh:
    long_description = fh.read()


# try:
#     from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
#     class bdist_wheel(_bdist_wheel):
#         def finalize_options(self):
#             _bdist_wheel.finalize_options(self)
#             self.root_is_pure = False
# except ImportError:
#     bdist_wheel = None

JSONATA_C_SRC_DIR = "jsonata-c/src"
COMPILE_CMD = "gcc -O3 -std=gnu99 -I{0} -fstack-protector -fPIC -shared -o jsonata-c/jsonata.so {0}/duktape.c {0}/jsonata.c".format(JSONATA_C_SRC_DIR)
PRECOMPILE_CMD = "bash -c 'jsonata-c/src/generate_jsonatah.sh'"

class JsonataBuildExt(build_ext):
    def build_extension(self, ext):
        self.debug = True
        ext_path = self.get_ext_fullpath(ext.name)
        spawn(shlex.split(PRECOMPILE_CMD))
        spawn(shlex.split(COMPILE_CMD))
        copy_file("jsonata-c/jsonata.so", ext_path)

    def run(self):
        build_ext.run(self)
        self.copy_extensions_to_source()


setup(
    name="pyjsonata", # Replace with your own username
    version="0.0.1a7",
    author="Quentin Young",
    author_email="qlyoung@qlyoung.net",
    description="Python bindings for JSONata",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/qlyoung/pyjsonata",
    packages=['pyjsonata'],
    ext_modules = [
        Extension("pyjsonata.jsonata", sources=["jsonata-c/src/*.c", "jsonata-c/src/*.h"])
    ],
    package_dir={'pyjsonata': 'pyjsonata', 'jsonata-c': 'jsonata-c'},
    package_data={"jsonata-c": ["src/*.c", "src/*.h", "src/*.template"]},
    include_package_data=True,
    classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
    cmdclass = {
        "build_ext": JsonataBuildExt,
    }
)

import subprocess
import platform
import shlex
from os import mkdir
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext

with open("README.md", "r") as fh:
    long_description = fh.read()


from distutils.spawn import spawn

# try:
#     from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
#     class bdist_wheel(_bdist_wheel):
#         def finalize_options(self):
#             _bdist_wheel.finalize_options(self)
#             self.root_is_pure = False
# except ImportError:
#     bdist_wheel = None

COMPILE_CMD = "make -C jsonata-c release"

class JsonataBuildExt(build_ext):
    def build_extension(self, ext):
        """
        Manually compile jsonata.so, bypass setuptools
        """
        ext_path = self.get_ext_fullpath(ext.name)
        spawn(shlex.split(COMPILE_CMD))
        spawn(["cp", "jsonata-c/jsonata.so", ext_path])


setup(
    name="pyjsonata", # Replace with your own username
    version="0.0.1a1",
    author="Quentin Young",
    author_email="qlyoung@qlyoung.net",
    description="Python bindings for JSONata",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/qlyoung/pyjsonata",
    packages=find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
    ],
    # Dummy extension to convince setuptools that we are impure
    python_requires='>=3.6',
    ext_modules = [
        Extension("jsonata", sources=["jsonata-c/src/jsonata.c"])
    ],
    cmdclass = {
        "build_ext": JsonataBuildExt,
    }
)

# pyjsonata

Python bindings for [JSONata](https://jsonata.org).


## Building

Required build tools:
* npm
* clang
* make

If you want to run tests:
* pytest


*To build:*

Ready?

- Install docker
- From the repository root:

  ```
  git submodule update --init --recursive
  rm -rf dist build __pycache__ pyjsonata.egg-info
  docker pull quay.io/pypa/manylinux2014_x86_64
  docker run -it --mount type=bind,source=$(pwd),target=/src quay.io/pypa/manylinux2014_x86_64 bash
  curl https://nodejs.org/dist/v12.16.1/node-v12.16.1-linux-x64.tar.xz -o npm.tar.xz
  tar xvf npm.tar.xz -C /tmp/
  cp -r /tmp/node*/* /usr
  cd /src
  /opt/python/cp36-cp36m/bin/python3 -m setup.py sdist bdist_wheel
  cd dist
  auditwheel repair ./*.whl
  mv wheelhouse/* .
  ```

- If the stars have aligned, and it is Tuesday, you will now have a built
  `pyjsonata` wheel

- If you are me:

  ```
  python3 -m twine upload --repository-url https://upload.pypi.org/legacy/ dist/*manylinux2014_x86_64.whl
  ```

The best part about all this is that it builds a `.so` using the build
procedure for CPython extensions provided by setuptools. This isn't even a
CPython extension. It just uses ctypes. But there isn't any packaging support
at all for people who use the runtime-agnostic FFI library built into Python.

Building this package is harder than writing it. Thank you, Python.


## Testing

### Pytest

From the repository root:

```
python3 -m pip install pytest
python3 -m pytest
```

## Usage

```python
from pyjsonata import jsonata

my_expression = "$"
my_json = "{'foo': 'bar'}"

# "{'foo': 'bar'}"
result = jsonata(my_expression, my_json)
```

With exception handling:

```python
from pyjsonata import jsonata, PyjsonataError

my_expression = "$"
my_json = "{'foo': 'bar'}"

# "{'foo': 'bar'}"

try:
    result = jsonata(my_expression, my_json)
except PyjsonataError as e:
    print("Error: ", e)
```


That's it! Return values are always strings.

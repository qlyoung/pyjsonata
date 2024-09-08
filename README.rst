Archived as of September 2024 because:

1. I have not been maintaining it
2. There is now a pure python implementation of Jsonata here: https://github.com/rayokota/jsonata-python

Thanks to all users for the support.

----

pyjsonata
=========

Python bindings for `JSONata <https://jsonata.org>`_.


Usage
-----

.. code-block:: python

   from pyjsonata import jsonata

   my_expression = "$"
   my_json = "{'foo': 'bar'}"

   # "{'foo': 'bar'}"
   result = jsonata(my_expression, my_json)

With exception handling:

.. code-block:: python

   from pyjsonata import jsonata, PyjsonataError

   my_expression = "$"
   my_json = "{'foo': 'bar'}"

   try:
       # "{'foo': 'bar'}"
       result = jsonata(my_expression, my_json)
   except PyjsonataError as e:
       print("Error: ", e)


That's it! Return values are always strings.

About
-----

The `reference implementation <https://github.com/jsonata-js/jsonata>`_ for
JSONata is written in JavaScript. I have a `separate library
<https://github.com/qlyoung/jsonata-c/>`_ that makes this accessible from C via
`Duktape <https://duktape.org/>`_. This is a Python wrapper that calls into
that library using Python's built-in ``ctypes`` library, which should be
portable to most Python interpreters.

At first I tried using `py_mini_racer
<https://github.com/sqreen/PyMiniRacer/blob/master/py_mini_racer/py_mini_racer.py>`_
to run the JSONata library, but that package is around 40mb because it ships
the complete V8 JavaScript runtime. In contrast this library is about 650k.


Building Packages
-----------------

Source
^^^^^^

Source packages are currently broken until I can be bothered to rewrite
``jsonata-c``'s Makefile in Python, as required by ``setuptools``.

🖕 ``setuptools``.

Wheels
^^^^^^

``pyjsonata`` can be built to the ``manylinux2014`` standard. There is no
Windows support at this time.

The standard way to build ``manylinux2014`` compatible extensions is with a
bunch of Centos 7 Docker containers. The idea is that by linking against Centos
7 libc, the resultant binaries will be "portable enough" to modern systems. You
don't have to use these, but it's not a bad idea.

Without Docker
""""""""""""""

- Install ``gcc``, ``patchelf`` and ``make`` from your distro repository
- Install Python build deps:

  .. code-block:: console

     python3 -m pip install setuptools wheel auditwheel

To build:

.. code-block:: console

   git submodule update --init --recursive
   python3 -m setup.py bdist_wheel
   cd dist && auditwheel repair ./*.whl
   mv wheelhouse/*.whl .

This will make you ``manylinux2014`` wheels. These wheels are tagged to your
specific Python version and ABI, like ``cp37-cp37m``, but in reality, they
should be ``py3-none``. I can't figure out how to make ``setuptools``
understand that. I think you can safely manually re-tag these by unzipping the
wheel, editing the metadata in the ``WHEEL`` file, rezipping it, and changing
the tag in the filename, but I haven't yet tested whether that yields the
desired results.

However, the arch tag, e.g. ``x86_64``, ``aarch64``, ``armv7l`` etc, is
necessary.


With Docker
"""""""""""

.. code-block:: console

   ./build.sh <arch>

``arch`` must be one of the architectures for which ``manylinux2014`` build
containers are provided. For example, if you are building on ``aarch64``:

.. code-block:: console

   ./build.sh aarch64

This will download the appropriate container and run the build. Built wheels
are in the ``dist`` directory.


If you are me:
^^^^^^^^^^^^^^

.. code-block:: console

   python3 -m twine upload --repository-url https://upload.pypi.org/legacy/ dist/*manylinux2014_*.whl


Testing
-------

Pytest
^^^^^^

From the repository root:

.. code-block:: console

   python3 -m pip install pytest
   python3 -m pytest


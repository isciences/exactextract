Installation
============

The R and Python packages can be installed from `CRAN <https://github.com/isciences/exactextractr>`__ and `PyPI <https://pypi.org/project/exactextract/>`__, respectively.
If these repositories do not provide a binary for your platform, then you may also need to install the dependencies listed in :ref:`compiling` below.

To use the command line interface, you will likely need to compile it yourself.

.. _compiling:

Compiling
---------

``exactextract`` requires the following:

* A C++17 compiler (e.g., gcc 7+)
* CMake 3.8+
* `GEOS <https://github.com/libgeos/geos>`__ version 3.5+
* `GDAL <https://github.com/osgeo/GDAL>`__ version 2.0+ (for CLI binary)

It can be built as follows on Linux as follows:

.. code-block:: bash

    bash
    git clone https://github.com/isciences/exactextract
    cd exactextract
    mkdir cmake-build-release
    cd cmake-build-release
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
    sudo make install

The following options are available to control what gets compiled. They are each ON by default.

- ``BUILD_BENCHMARKS`` will build performance tests
- ``BUILD_CLI`` will build the command-line interface (requires GDAL)
- ``BUILD_DOC`` will build the doxygen documentation if doxygen is available
- ``BUILD_PYTHON`` will build Python bindings (requires pybind11)
- ``BUILD_TEST`` will build the catch_test suite

To build just the library and test suite, you can use these options as follows to turn off the CLI (which means GDAL isn't required) and disable the documentation build. The tests and library are built, the tests run, and the library installed if the tests were run successfully:

.. code-block:: bash

    git clone https://github.com/isciences/exactextract
    cd exactextract
    mkdir cmake-build-release
    cd cmake-build-release
    cmake -DBUILD_CLI:=OFF -DBUILD_DOC:=OFF -DCMAKE_BUILD_TYPE=Release ..
    make
    ./test/catch_tests && sudo make install

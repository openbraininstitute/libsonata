Developer Tips
==============

libsonata is a standard CMake project, so building it looks something like:

.. code-block:: bash

    $ mkdir build && cd build
    $ cmake ..                              \
        -GNinja                             \
        -DEXTLIB_FROM_SUBMODULES=ON         \
        -DSONATA_PYTHON=ON                  \
        -DSONATA_CXX_WARNINGS=ON            \
        -DCMAKE_BUILD_TYPE=Debug

For faster iteration when working on python bindings, one can use:

.. code-block:: bash

    $ pip install --no-build-isolation -Ceditable.rebuild=true -Cbuild-dir=build -ve ~/src/libsonata

This will rebuild only the files that have changed.
See `scikit-build-core Editable installs <https://scikit-build-core.readthedocs.io/en/latest/configuration/editable.html>`_.
This assumes that the build dependencies are already installed in the python environment.
Currently, `setuptools_scm` and `scikit-build-core` are required.

# DAGBOX

DagBox is an embeddable database.

# Dependencies

* zeromq
* boost (system & filesystem)
* msgpack
* lmdb

These libraries must be present to build and run DagBox. They are
readily available in package managers for various operating systems.

While there are some other dependencies as well, those are included as
submodules and will be automatically downloaded along DagBox.

# Building

Before building DagBox, make sure that all dependencies are installed
and that their development files (headers) are available. You will
also need CMake, a compiler with C++11 support (any recent version of
g++ or clang++ is adequate), and git.

First, clone the repository to your computer, making sure that the
submodules have been cloned as well. Run:

```
git clone --recurse-submodules 'https://github.com/DagBox/DagBox.git'
cd DagBox
```

Or, if you have already cloned the repository without the submodules,
run:

```
git submodule update --init --recurse
```

Once you have the code and all submodules ready, create a directory to
build the project in, then enter that directory and call CMake. This
will perform any necessary preparation. Finally, you can use `make`
to build the entire project.

```
mkdir build
cd build
cmake ..
make
```

You can also run `make help` to see what targets are available to be built.

# Tests & Coverage

To build and run the tests:

```
make tests  # build
make test   # run
```

To see the code coverage, run:

```
make test-coverage
```

This will generate a summary of the test coverage. See
`coverage/index.html`.

# Documentation

You will need Doxygen and LaTeX to build the documentation. Once the
requirements are installed, run:

```
make docs
```

The documentation will be built in `docs/html/index.html` and
`docs/latex/refman.pdf`.

# License

DagBox is free open source software licensed under GNU Lesser General
Public License version 3 or later. See the `COPYING` and
`COPYING.LESSER` files.

Some files within DagBox are licensed under Boost Software License,
Version 1.0. See `LICENSE_1_0.txt` for the license.

The submodules included in the repository have their own licenses. See
their respective repositories for their licenses.

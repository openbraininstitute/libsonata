#!/usr/bin/env bash

# This file runs the C++ tests, as well as compiling the code with warnings on
# so that errors should be caught quicker

set -euxo pipefail

BUILD_DIR=build/build-cpp-test

SONATA_VERSION=
if ! git describe --tags > /dev/null 2>&1; then
    SONATA_VERSION='-DSONATA_VERSION=0.0.0'
    echo "Likely shallow git clone, faking SONATA_VERSION -> $SONATA_VERSION"
fi

if command -v sccache; then
    CMAKE_ARGS+=(
        -DCMAKE_C_COMPILER_LAUNCHER=sccache
        -DCMAKE_CXX_COMPILER_LAUNCHER=sccache
    )
fi

# cmake --build && ctest
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cmake $SONATA_VERSION                                                      \
    -DCMAKE_BUILD_TYPE=Release                                             \
    -DEXTLIB_FROM_SUBMODULES=ON                                            \
    -DSONATA_CXX_WARNINGS=ON                                               \
    "${CMAKE_ARGS[@]}"                                                     \
    -B "${BUILD_DIR}"

cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}"

# cmake --build --target install && ctest
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cmake $SONATA_VERSION                                                      \
    -DCMAKE_BUILD_TYPE=Release                                             \
    -DEXTLIB_FROM_SUBMODULES=ON                                            \
    -DSONATA_CXX_WARNINGS=ON                                               \
    -DCMAKE_INSTALL_PREFIX=install                                         \
    "${CMAKE_ARGS[@]}"                                                     \
    -B "${BUILD_DIR}"

cmake --build "${BUILD_DIR}" --parallel --target install

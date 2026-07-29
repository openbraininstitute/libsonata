#!/usr/bin/env bash

# This builds coverage information, including HTML output

set -euxo pipefail

if [ $# -ge 1 ]; then
  EXTRA_OPTIONS=$1
else
  EXTRA_OPTIONS=""
fi

BUILD_DIR=build/build-coverage

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR"

if command -v sccache; then
    CMAKE_ARGS+=(
        -DCMAKE_C_COMPILER_LAUNCHER=sccache
        -DCMAKE_CXX_COMPILER_LAUNCHER=sccache
    )
fi

cmake                                       \
    -DCMAKE_BUILD_TYPE=Debug                \
    -DEXTLIB_FROM_SUBMODULES=ON             \
    -G "${CMAKE_GENERATOR:-Unix Makefiles}" \
    "${CMAKE_ARGS[@]}"                      \
    ${EXTRA_OPTIONS}                        \
    ../..

cmake --build . -j --target coverage

echo "Read the coverage at: file://$PWD/build/build-coverage/coverage/index.html"

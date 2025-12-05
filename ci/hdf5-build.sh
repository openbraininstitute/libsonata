#!/bin/bash
set -e -x

: "${CIBW_ARCHS_MACOS:=$(uname -m)}"

export INPUT=$(cd $(dirname "$1") && pwd -P)/$(basename "$1")
export OUTPUT="$INPUT/install-$CIBW_ARCHS_MACOS"


function download_unpack_hdf5 {
    pushd "$INPUT"
    local name=hdf5_1.14.6.tar.gz
    if [[ ! -e $name ]]; then
        echo "Downloading & unpacking HDF5 ${name}"
        curl -fsSLO "https://github.com/HDFGroup/hdf5/archive/refs/tags/${name}"
    fi
    mkdir -p "${INPUT}/hdf5"
    tar xzf "$name" -C "${INPUT}/hdf5"  --strip-components=1
    popd
}

if [[ "$OSTYPE" == "darwin"* ]]; then
    NPROC=$(sysctl -n hw.ncpu)
else
    NPROC=$(nproc)
fi

INSTALL="$OUTPUT/install"

if [[ -f "$INSTALL/lib/libhdf5.a" ]]; then
    echo "using cached build"
else
    if [[ "$OSTYPE" == "darwin"* ]]; then
        export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
        export CC="/usr/bin/clang"
        export CXX="/usr/bin/clang"
        export CFLAGS="$CFLAGS -arch $CIBW_ARCHS_MACOS"
        export CPPFLAGS="$CPPFLAGS -arch $CIBW_ARCHS_MACOS"
        export CXXFLAGS="$CXXFLAGS -arch $CIBW_ARCHS_MACOS"
    fi

    echo "Building & installing hdf5"
    download_unpack_hdf5

    cmake -B "$OUTPUT/build" -G'Unix Makefiles' \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DBUILD_SHARED_LIBS=OFF \
        -DHDF5_ENABLE_NONSTANDARD_FEATURES=OFF \
        -DHDF5_ENABLE_NONSTANDARD_FEATURE_FLOAT16=OFF \
        -DHDF5_BUILD_STATIC_TOOLS=OFF \
        -DHDF5_BUILD_UTILS=OFF \
        -DHDF5_BUILD_HL_LIB=OFF \
        -DHDF5_BUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        -DHDF5_BUILD_TOOLS=OFF \
        -DHDF5_ENABLE_SZIP_ENCODING=OFF \
        -DHDF5_ENABLE_SZIP_SUPPORT=OFF \
        -DHDF5_ENABLE_Z_LIB_SUPPORT=OFF \
        -DCMAKE_INSTALL_PREFIX="$INSTALL" \
        -S "$INPUT/hdf5"

    cmake --build "$OUTPUT/build" -j "$NPROC"
    cmake --install "$OUTPUT/build"
fi

find "$OUTPUT"

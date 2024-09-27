#!/bin/bash

ORIGINAL_DIR=$(pwd)

if [ "$(uname)" == "Linux" ]; then
    if [ $(uname -m) == "aarch64" ]; then
        export VCPKG_FORCE_SYSTEM_BINARIES=arm
    fi
fi

rm -rf build
mkdir build
pushd build

cmake .. -DCMAKE_TOOLCHAIN_FILE="../../vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
make -B

export LC_ALL=C
unset LANGUAGE

popd

cd "$ORIGINAL_DIR"
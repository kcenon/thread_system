#!/bin/bash

ORIGINAL_DIR=$(pwd)

if [ "$(uname)" == "Darwin" ]; then
    brew update
    brew upgrade
    brew install pkg-config cmake doxygen
elif [ "$(uname)" == "Linux" ]; then
    apt update
    apt upgrade -y

    apt install cmake build-essential gdb doxygen -y

    apt-get install curl zip unzip tar -y
    apt-get install pkg-config ninja-build -y

    if [ $(uname -m) == "aarch64" ]; then
        export VCPKG_FORCE_SYSTEM_BINARIES=arm
    fi
fi

doxygen

pushd ..

if [ ! -d "./vcpkg/" ]; then
    git clone https://github.com/microsoft/vcpkg.git
fi

pushd vcpkg

git pull
./bootstrap-vcpkg.sh

popd
popd

cd "$ORIGINAL_DIR"
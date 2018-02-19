#!/bin/bash

set -e

function usage() {
    echo "usage: ${BASH_SOURCE} <prefix>" 1>&2
}

function build_from_tar() {
    local url
    url="$1"

    local prefix
    prefix="$2"

    local file
    file=$(basename "$url")
    curl -L -O "$url"

    local dir
    dir=$(tar -tf  "$file" | head -n 1)
    rm -rf "$dir"
    tar -xvzf "$file"
    pushd "$dir"
    ./configure --prefix="$prefix"
    make -j3
    make check -j3
    make install
    popd
    rm -rf "$dir"
}

function main() {
    local prefix
    prefix="${BASH_ARGV[0]}"

    export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"

    if [ "x$prefix" = "x" ]; then
        usage
        exit 1
    fi

    mkdir -p "$prefix"

    if [ ! -f "$prefix/bin/cmake" ]; then
        if [ ! -f "cmake-3.10.0-rc5-Linux-x86_64.sh" ]; then
            curl -L -O "https://cmake.org/files/v3.10/cmake-3.10.0-rc5-Linux-x86_64.sh"
        fi
        bash cmake-3.10.0-rc5-Linux-x86_64.sh --skip-license --prefix="$prefix"
    fi

    if [ ! -f "$prefix/bin/protoc" ]; then
        build_from_tar "https://github.com/google/protobuf/releases/download/v3.5.1/protobuf-cpp-3.5.1.tar.gz" "$prefix"
    fi

    if [ ! -f "$prefix/bin/protoc-c" ]; then
        build_from_tar "https://github.com/protobuf-c/protobuf-c/releases/download/v1.3.0/protobuf-c-1.3.0.tar.gz" "$prefix"
    fi
}

main

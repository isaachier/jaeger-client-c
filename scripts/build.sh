#!/bin/bash

# Copyright (c) 2018 Uber Technologies, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Based on
# https://github.com/codecov/example-cpp11-cmake/blob/master/run_build.sh.

set -e

RED='\033[0;31m'
BLUE='\033[0;34m'
NO_COLOR='\033[0m'
GREEN='\033[0;32m'

function error() {
    >&2 echo -e "${RED}$1${NO_COLOR}"
}

function info() {
    echo -e "${GREEN}$1${NO_COLOR}"
}

function working() {
    echo -e "${BLUE}$1${NO_COLOR}"
}

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)
    cd "$project_dir" || exit 1

    echo "using $(cmake -version) at $(which cmake)"

    mkdir -p build
    cd build || exit 1

    local prefix_arg
    prefix_arg=$1

    if [ "${USE_HUNTER:-}" == "off" ]; then
        # We won't be using Hunter so we need to get opentracing's c library
        # installed too.
        working "Building opentracing-c"
        mkdir -p "build-opentracing-c"
        pushd "build-opentracing-c"
        cmake -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_INSTALL_PREFIX=${prefix_arg} \
            ../../third_party/opentracing-c/
        make all -j3
        make install
        working "Done building opentracing-c"
        popd

        EXTRA_CMAKE_OPTS="-DHUNTER_ENABLED=0 -DJAEGERTRACINGC_USE_PCG=0"
    fi

    working "Building project"
    cmake -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX="$prefix_arg" \
        -DCMAKE_PREFIX_PATH=${prefix_arg} \
        -DJAEGERTRACINGC_COVERAGE=${COVERAGE:-OFF} \
        ${EXTRA_CMAKE_OPTS:-} \
        ..

    if make all -j3; then
        if valgrind --error-exitcode=1 --track-origins=yes \
            --leak-check=full ./default_test; then
            info "All tests compiled and passed"
            make install
            info "Install test run successful"
        else
            error "Tests failed"
            exit 1
        fi
    else
        error "Failed to build project"
        exit 1
    fi
}

main $@

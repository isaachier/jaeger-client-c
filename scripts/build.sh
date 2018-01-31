#!/bin/bash

# Based on https://github.com/codecov/example-cpp11-cmake/blob/master/run_build.sh.

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

    mkdir -p build
    cd build || exit
    if [ "x$COVERAGE" != "x" ]; then
      cmake -DCMAKE_BUILD_TYPE=Debug -DJAEGERTRACINGC_COVERAGE=ON ..
    else
      cmake -DCMAKE_BUILD_TYPE=Debug -DJAEGERTRACINGC_COVERAGE=OFF ..
    fi

    if make -j3 unit_test; then
        true
    else
        error "Error: compilation errors"
        exit 3
    fi

    working "Running tests..."
    if ./unit_test; then
        true
    else
        error "Error: test failure"
    fi

    info "All tests compiled and passed"
}

main

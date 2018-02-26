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

set -e

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)
    cd "$project_dir" || exit 1

    rm -rf build
    mkdir build
    cd build || exit 1
    cmake -DCMAKE_BUILD_TYPE=Debug -DJAEGERTRACINGC_BUILD_DOC=ON ..
    make doc

    cd "$project_dir"
    git fetch origin
    git checkout --orphan gh-pages
    git pull origin gh-pages
    rm -rf _static _styles
    mv build/doc/Debug/html/* .
    git add .
    git commit -m "Update docs"
    git push --set-upstream origin gh-pages
}

main

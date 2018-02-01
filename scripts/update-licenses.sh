#!/bin/bash

set -e

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)
    cd "$project_dir" || exit 1

    local srcs
    srcs=($(git ls-files src | grep -E '\.(c|h)$' | grep -E -v 'protoc-gen')
          src/jaegertracingc/constants.h.in)
    python scripts/update-license.py "${srcs[@]}"
}

main

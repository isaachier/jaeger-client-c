#!/bin/bash

set -e

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)
    cd "$project_dir" || exit 1

    local srcs
    srcs=$(git ls-files src |
           grep -E '\.c$' |
           grep -E -v '_test\.c$|protoc-gen')

    local cmd
    for src in $srcs; do
        cmd="clang-tidy -p=build"
        cmd+=" -checks=\"-clang-diagnostic-unused-command-line-argument\" "
        cmd+=" $src"
        echo "$cmd"
        eval "$cmd"
    done
}

main
#!/bin/bash

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)
    cd "$project_dir" || exit 1

    local srcs
    srcs=$(git ls-files src crossdock |
           grep -E -v 'thrift-gen|thirdparty|_test\.c$' |
           grep -E '\.c$')

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

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

    local skip_checks
    skip_checks="-clang-diagnostic-unused-command-line-argument,"
    skip_checks+="-clang-analyzer-security.insecureAPI.rand"

    local cmd
    for src in $srcs; do
        cmd="clang-tidy -p=build"
        cmd+=" -checks=\"${skip_checks}\""
        cmd+=" $src"
        echo "$cmd"
        eval "$cmd"
    done
}

main

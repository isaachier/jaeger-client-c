#!/bin/bash

set -e

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)
    cd "$project_dir" || exit 1

    python scripts/update-license.py $(git ls-files "*\.c" "*\.h" |
                                       grep -E -v 'thirdparty') \
        src/jaegertracingc/constants.h.in
}

main

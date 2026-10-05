#!/usr/bin/env bash
# .set selects XID/CORE; pass input/script/<name>.set basenames.
# Example: bash run_paper_core_experiments.sh c17a_xid c17a_core
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$project_dir/build"
if [ "$#" -eq 0 ]; then
    set -- c17a_xid c17a_core
fi
for name in "$@"; do
    setfile="../input/script/$name.set"
    # Output paths in .set are relative to build/, just like the usual runner.
    while read -r key value rest; do
        case "$key" in
            -fdp|-log|-cube_analysis) mkdir -p -- "$(dirname -- "$value")" ;;
        esac
    done < "$setfile"
    echo "$name"
    ./main_release -set "$setfile"
done

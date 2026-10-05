#!/usr/bin/env bash
set -euo pipefail
experiment_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository_dir=$(cd -- "$experiment_dir/../.." && pwd)
mkdir -p "$experiment_dir/build" "$experiment_dir/runs"
g++ -O2 -I "$repository_dir/external/cadical/src" \
  "$experiment_dir/sat_bridge.cc" \
  "$repository_dir/external/cadical/build/libcadical.a" \
  -o "$experiment_dir/build/sat_bridge"
g++ -O2 -I "$repository_dir/external/cudd/cudd" \
  "$experiment_dir/cube_union.cc" \
  "$repository_dir/external/cudd/cudd/.libs/libcudd.a" \
  -lgmpxx -lgmp -lm -o "$experiment_dir/build/cube_union"

#!/usr/bin/env bash
set -euo pipefail
experiment_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository_dir=$(cd -- "$experiment_dir/../.." && pwd)
bash "$experiment_dir/../linear_coordinates/build.sh"
bash "$experiment_dir/native/build.sh"
mkdir -p "$experiment_dir/blocks/build" "$experiment_dir/runs"
g++ -std=c++17 -O3 -Wall -Wextra -I "$repository_dir/external/cudd/cudd" \
  "$experiment_dir/blocks/region_union.cc" \
  "$repository_dir/external/cudd/cudd/.libs/libcudd.a" \
  -lgmpxx -lgmp -lm -o "$experiment_dir/blocks/build/region_union"

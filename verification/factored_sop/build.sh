#!/usr/bin/env bash
set -euo pipefail
experiment_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository_dir=$(cd -- "$experiment_dir/../.." && pwd)
mkdir -p "$experiment_dir/build" "$experiment_dir/runs"
g++ -std=c++17 -O3 -Wall -Wextra -I "$repository_dir/external/cadical/src" \
  "$experiment_dir/../linear_scaling/native/enumerator.cc" \
  "$repository_dir/external/cadical/build/libcadical.a" \
  -o "$experiment_dir/build/enumerator"
g++ -std=c++17 -O3 -Wall -Wextra -I "$repository_dir/external/cadical/src" \
  "$experiment_dir/sweep.cc" "$repository_dir/external/cadical/build/libcadical.a" \
  -o "$experiment_dir/build/sweep"

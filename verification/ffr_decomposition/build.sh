#!/usr/bin/env bash
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
mkdir -p "$here/build"
bash "$here/../linear_scaling/native/build.sh"
g++ -std=c++17 -O3 -I"$root/external/cudd" -I"$root/external/cudd/cudd" \
  "$here/factor_union.cc" "$root/external/cudd/cudd/.libs/libcudd.a" \
  -lgmpxx -lgmp -lm -o "$here/build/factor_union"

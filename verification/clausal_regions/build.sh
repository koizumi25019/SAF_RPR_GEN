#!/usr/bin/env bash
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
mkdir -p "$here/build"
g++ -std=c++17 -O3 -I"$root/external/cudd" -I"$root/external/cudd/cudd" \
  "$here/formula_union.cc" "$root/external/cudd/cudd/.libs/libcudd.a" \
  -lgmpxx -lgmp -lm -o "$here/build/formula_union"
g++ -std=c++17 -O3 -I"$root/external/cadical/src" \
  "$here/propagate_implicates.cc" "$root/external/cadical/build/libcadical.a" \
  -o "$here/build/propagate_implicates"

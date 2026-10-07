#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$ROOT"
mkdir -p build
mapfile -t sources < <(awk '/^set\(SOURCES/{reading=1;next} reading && /^\)/{exit} reading{print $1}' CMakeLists.txt)
includes=(-I src -I src/fdp -I src/fdp/cnf -I src/lib -I src/netlist -I src/opt -I src/fdp/xid -I external/cadical/src -I external/cudd -I external/cudd/cudd)
libs=(external/cadical/build/libcadical.a external/cudd/cudd/.libs/libcudd.a -lgmp -lm -lstdc++)
for profile in debug release; do
    mkdir -p "build/obj_$profile"
    if [[ $profile == debug ]]; then flags=(-g -O0 -DDEBUG); else flags=(-O3 -DNDEBUG); fi
    objects=()
    for source in "${sources[@]}"; do
        obj="build/obj_$profile/${source//\//_}.o"
        gcc -std=gnu11 -fcommon "${flags[@]}" "${includes[@]}" -c "$source" -o "$obj"
        objects+=("$obj")
    done
    gcc "${objects[@]}" "${libs[@]}" -o "build/main_$profile"
done
gcc -std=gnu11 -O2 verification/paper_core/time_command.c -o build/time_command
gcc -std=gnu11 -O2 -fcommon -ffunction-sections -fdata-sections -I src/fdp -I external/cadical/src \
    verification/paper_core/test_generalize.c src/fdp/paper_core.c \
    external/cadical/build/libcadical.a -Wl,--gc-sections -lstdc++ -lm -o build/test_generalize
printf 'Built Debug, Release, timing wrapper and truth-table check.\n'

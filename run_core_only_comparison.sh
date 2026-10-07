#!/usr/bin/env bash
# .set -> C ATPG. Python is used only after all C runs for aggregation/Excel.
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$ROOT"
repeats=7
if [[ ${1:-} == --repeats ]]; then repeats=$2; shift 2; fi
[[ $repeats =~ ^[1-9][0-9]*$ ]] || { printf 'repeats must be positive\n' >&2; exit 2; }
circuits=("$@")
if (( ${#circuits[@]} == 0 )); then circuits=(c17a s27_C s208_C s298_C); fi
[[ -x build/main_release && -x build/time_command ]] || bash verification/paper_core/build_compare.sh
stamp=$(TZ=Asia/Tokyo date +%Y%m%dT%H%M%S%z)
OUT="$ROOT/output/core_only_comparison/$stamp"
mkdir -p "$ROOT/output/core_only_comparison"
mkdir "$OUT"
printf '%s\n' "$OUT" > output/core_only_comparison/latest.txt
git rev-parse HEAD > "$OUT/source_parent.txt"
git diff HEAD --binary > "$OUT/source.patch"
sha256sum build/main_release build/time_command external/cadical/build/libcadical.a external/cudd/cudd/.libs/libcudd.a > "$OUT/binary.sha256"
printf '%s\n' "$repeats" > "$OUT/repeats.txt"
cpu=$(awk '/Cpus_allowed_list/{split($2,a,/[-,]/);print a[1]}' /proc/self/status)
printf '%s\n' "$cpu" > "$OUT/cpu.txt"
# Drop optional research switches; preserve proxy and credential environment.
mapfile -t hooks < <(rg -o --no-filename 'getenv\("[A-Z0-9_]+"\)' src | sed 's/getenv("//;s/")//' | sort -u)
clean=()
for hook in "${hooks[@]}"; do clean+=(-u "$hook"); done
run_case() {
    local circuit=$1 mode=$2 label=$3 check=${4:-off}
    local stem="$OUT/${circuit}_${mode}_${label}"
    local source="$ROOT/input/script/${circuit}_${mode}_compare.set"
    [[ -f $source ]] || { printf 'Missing settings: %s\n' "$source" >&2; exit 2; }
    cp "$source" "$stem.set"
    # Verification is separate from timing. CORE_ONLY is verified for soundness,
    # not primality; its timed run still uses just one negative solve per cube.
    printf '\n-fdp %s.csv\n-log %s.log\n-core_verify %s\n' "$stem" "$stem" "$check" >> "$stem.set"
    local extra=()
    if [[ $check == on ]]; then extra=(GT_BDD=1); fi
    local stdout=/dev/null
    if [[ $check == on ]]; then stdout="$stem.stdout"; fi
    (cd "$ROOT/build"
    env "${clean[@]}" "${extra[@]}" taskset -c "$cpu" timeout --kill-after=5s 120 \
        "$ROOT/build/time_command" "$stem.timing.csv" "$ROOT/build/main_release" -set "$stem.set" \
        > "$stdout" 2> "$stem.stderr"
    )
}
for circuit in "${circuits[@]}"; do
    for mode in xid core_only core_min; do run_case "$circuit" "$mode" warmup; done
    for ((trial=1; trial<=repeats; trial++)); do
        case $(( (trial-1)%3 )) in
            0) modes=(xid core_only core_min);;
            1) modes=(core_only core_min xid);;
            2) modes=(core_min xid core_only);;
        esac
        for mode in "${modes[@]}"; do run_case "$circuit" "$mode" "$trial"; done
    done
    for mode in xid core_only core_min; do run_case "$circuit" "$mode" verify on; done
    printf '%s: %s measurements per mode and separate BDD verification completed\n' "$circuit" "$repeats"
done
python3 verification/paper_core/export_core_only.py --run-dir "$OUT"

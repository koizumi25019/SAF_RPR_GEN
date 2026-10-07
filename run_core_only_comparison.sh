#!/usr/bin/env bash
# .set -> C ATPG. Python is used only after all C runs for aggregation/Excel.
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$ROOT"
repeats=7
profile=small
explicit_repeats=off
while [[ ${1:-} == --* ]]; do
    case $1 in
        --medium) profile=medium; shift;;
        --repeats) repeats=${2:-}; explicit_repeats=on; shift 2;;
        *) printf 'Unknown option: %s\n' "$1" >&2; exit 2;;
    esac
done
if [[ $profile == medium && $explicit_repeats == off ]]; then repeats=1; fi
[[ $repeats =~ ^[1-9][0-9]*$ ]] || { printf 'repeats must be positive\n' >&2; exit 2; }
circuits=("$@")
if (( ${#circuits[@]} == 0 )); then
    if [[ $profile == medium ]]; then circuits=(s5378_C s9234_C); else circuits=(c17a s27_C s208_C s298_C); fi
fi
suffix=compare
limit=0
warmups=1
verification=on
if [[ $profile == medium ]]; then suffix=compare_l30; limit=30; warmups=0; verification=off; fi
for circuit in "${circuits[@]}"; do
    for mode in xid core_only core_min; do
        [[ -f input/script/${circuit}_${mode}_${suffix}.set ]] || { printf 'Missing settings for %s/%s\n' "$circuit" "$mode" >&2; exit 2; }
    done
done
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
printf '%s\n' "$profile" > "$OUT/profile.txt"
printf '%s\n' "$limit" > "$OUT/limit.txt"
printf '%s\n' "$warmups" > "$OUT/warmups.txt"
printf '%s\n' "$verification" > "$OUT/verification.txt"
cpu=$(awk '/Cpus_allowed_list/{split($2,a,/[-,]/);print a[1]}' /proc/self/status)
printf '%s\n' "$cpu" > "$OUT/cpu.txt"
# Drop optional research switches; preserve proxy and credential environment.
mapfile -t hooks < <(rg -o --no-filename 'getenv\("[A-Z0-9_]+"\)' src | sed 's/getenv("//;s/")//' | sort -u)
clean=()
for hook in "${hooks[@]}"; do clean+=(-u "$hook"); done
run_case() {
    local circuit=$1 mode=$2 label=$3 check=${4:-off}
    local stem="$OUT/${circuit}_${mode}_${label}"
    local source="$ROOT/input/script/${circuit}_${mode}_${suffix}.set"
    [[ -f $source ]] || { printf 'Missing settings: %s\n' "$source" >&2; exit 2; }
    cp "$source" "$stem.set"
    # Verification is separate from timing. CORE_ONLY is verified for soundness,
    # not primality; its timed run still uses just one negative solve per cube.
    printf '\n-fdp %s.csv\n-log %s.log\n-core_verify %s\n' "$stem" "$stem" "$check" >> "$stem.set"
    local extra=()
    if [[ $check == on ]]; then extra=(GT_BDD=1); fi
    local stdout=/dev/null
    if [[ $check == on ]]; then stdout="$stem.stdout"; fi
    local guard=()
    if [[ $profile == small ]]; then guard=(timeout --kill-after=5s 120); fi
    printf '%s %s/%s %s start %s\n' "$circuit" "$mode" "$label" "$profile" "$(TZ=Asia/Tokyo date +%T)"
    (cd "$ROOT/build"
    env "${clean[@]}" "${extra[@]}" taskset -c "$cpu" "${guard[@]}" \
        "$ROOT/build/time_command" "$stem.timing.csv" "$ROOT/build/main_release" -set "$stem.set" \
        > "$stdout" 2> "$stem.stderr"
    )
    printf '%s %s/%s finished %s\n' "$circuit" "$mode" "$label" "$(TZ=Asia/Tokyo date +%T)"
}
for circuit in "${circuits[@]}"; do
    if (( warmups )); then for mode in xid core_only core_min; do run_case "$circuit" "$mode" warmup; done; fi
    for ((trial=1; trial<=repeats; trial++)); do
        case $(( (trial-1)%3 )) in
            0) modes=(xid core_only core_min);;
            1) modes=(core_only core_min xid);;
            2) modes=(core_min xid core_only);;
        esac
        for mode in "${modes[@]}"; do run_case "$circuit" "$mode" "$trial"; done
    done
    if [[ $verification == on ]]; then for mode in xid core_only core_min; do run_case "$circuit" "$mode" verify on; done; fi
    printf '%s: %s measurements per mode completed (separate BDD: %s)\n' "$circuit" "$repeats" "$verification"
done
python3 verification/paper_core/export_core_only.py --run-dir "$OUT"

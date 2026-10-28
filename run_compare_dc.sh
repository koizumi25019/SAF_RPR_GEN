#!/usr/bin/env bash
# Run both methods sequentially with the same circuit, workers and cube limit.
set -euo pipefail

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
circuit="${1:-s5378_C}"
jobs="${2:-15}"
limit="${3:-30}"
if (( $# > 3 )) || [[ ! "$circuit" =~ ^[a-zA-Z0-9_-]+$ ]] ||
   [[ ! "$jobs" =~ ^[1-9][0-9]{0,2}$ ]] || (( jobs > 256 )) ||
   [[ ! "$limit" =~ ^(0|[1-9][0-9]{0,9})$ ]] || (( limit > 2147483647 )); then
    printf 'Usage: %s [circuit=s5378_C] [jobs=15] [cube_limit=30; 0=unlimited]\n' "$0" >&2
    exit 2
fi
binary="$repo_dir/build/main_release"
if [[ ! -x "$binary" || ! -f "$repo_dir/input/circuit/saf/$circuit.v" ]]; then
    printf 'Build build/main_release and check input/circuit/saf/%s.v.\n' "$circuit" >&2
    exit 1
fi
output_dir="$repo_dir/output/dc_compare/$circuit/limit$limit/jobs$jobs"
mkdir -p -- "$output_dir/fdp" "$output_dir/log"
printf 'method,wall_seconds,cpu_seconds,dont_care_cpu_seconds,complete_faults,limited_faults\n' > "$output_dir/timing.csv"
cd -- "$repo_dir"
for method in xid sbxid; do
    csv="$output_dir/fdp/$method.csv"
    log="$output_dir/log/$method.txt"
    "$binary" -saf -net "input/circuit/saf/$circuit.v" \
        -dc_method "$method" -jobs "$jobs" -dom_reuse off -limit "$limit" \
        -sbxid_pi_objective scope -fdp "$csv" -log "$log"
    counts=$(awk -F, '$4=="1" {complete++} $4=="0" {limited++}
        END {printf "%d,%d", complete, limited}' "$csv")
    awk -v method="$method" -v counts="$counts" '
        /\/\/[[:space:]]+Time[[:space:]]+:/ {wall=$(NF-1)}
        /\/\/[[:space:]]+CPU Time[[:space:]]+:/ {cpu=$(NF-1)}
        /CPU Time \(Don.t care\)[[:space:]]+:/ {dc=$(NF-1)}
        END {printf "%s,%s,%s,%s,%s\n", method, wall, cpu, dc, counts}
    ' "$log" >> "$output_dir/timing.csv"
done
printf '\nComparison saved to %s\n' "$output_dir/timing.csv"
cat -- "$output_dir/timing.csv"

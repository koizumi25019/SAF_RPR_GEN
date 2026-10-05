#!/usr/bin/env bash
# C executable + .set + shell/AWK comparison. No Python is required.
# Usage: bash run_paper_core_limit30.sh [--repeats N] [s5378_C s9234_C]
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repeats=1
if [[ ${1:-} == --repeats ]]; then
    repeats=${2:-}
    [[ $repeats =~ ^[1-9][0-9]*$ ]] || { echo '--repeats needs a positive integer' >&2; exit 1; }
    shift 2
fi
if (( $# )); then circuits=("$@"); else circuits=(s5378_C s9234_C); fi
for circuit in "${circuits[@]}"; do
    for method in xid core; do
        [[ -f $project_dir/input/script/${circuit}_${method}_l30.set ]] || {
            echo "Missing .set: ${circuit}_${method}_l30.set" >&2; exit 1;
        }
    done
done
[[ -x $project_dir/build/main_release ]] || { echo 'Build main_release first' >&2; exit 1; }
# Prevent optional inherited research/verification hooks from changing timing.
while IFS= read -r control; do
    case "$control" in
        PAPER_CORE*|MDC_*|MAXDC*|MAXHAM*|GT_*|CUBE_TREND*|DIVPO*|DIVPHASE*|DUAL*|SPLIT*|PCOUNT*|BDD_EXACT|XID_EXTERNAL|XSTAT|TDF_NOXID|AIG_DUMP*|DUMP_CNF)
            unset "$control" ;;
    esac
done < <(compgen -v)
run_stamp=$(TZ=Asia/Tokyo date '+%Y%m%dT%H%M%S%z')
result_dir="$project_dir/output/paper_core/limit30/comparison/$run_stamp"
mkdir -p "$result_dir"
metrics="$result_dir/measurements.csv"
comparisons="$result_dir/comparison.csv"
core_statistics="$result_dir/core_statistics.csv"
echo 'circuit,method,run,cpu_s,wall_s,dc_s,sat_s,bdd_s,representatives,complete,incomplete,cubes' > "$metrics"
echo 'circuit,run,both_complete,core_only_complete,xid_only_complete,neither_complete,core_higher_partial,core_lower_partial,equal_partial,exact_mismatch,missing_faults' > "$comparisons"
echo 'circuit,run,cubes,input_bits,care_after_core,care_after_min,negative_solves,deletion_trials,avg_core_care,avg_min_care,core_x_pct,min_x_pct,tested_input_pct,deleted_core_pct' > "$core_statistics"
sha256sum "$project_dir/build/main_release" > "$result_dir/binary.sha256"
echo "Results: $result_dir"
cd "$project_dir/build"
failed=0
for (( repetition=1; repetition<=repeats; repetition++ )); do
    if (( repetition % 2 )); then methods=(xid core); else methods=(core xid); fi
    for circuit in "${circuits[@]}"; do
        pair_ok=1
        for method in "${methods[@]}"; do
            job_dir="$result_dir/run$repetition/$method"
            mkdir -p "$job_dir/fdp" "$job_dir/log" "$job_dir/set"
            csv="$job_dir/fdp/${circuit}_fdp.csv"
            log="$job_dir/log/${circuit}_log.txt"
            setfile="$job_dir/set/${circuit}.set"
            awk -v csv="$csv" -v output_log="$log" '
                $1 == "-fdp" { print "-fdp " csv; next }
                $1 == "-log" { print "-log " output_log; next }
                { print }
            ' "../input/script/${circuit}_${method}_l30.set" > "$setfile"
            echo "[$repetition/$repeats] $circuit $method start $(TZ=Asia/Tokyo date '+%T %Z')"
            if ./main_release -set "$setfile" >"$job_dir/log/${circuit}.stdout" 2>"$job_dir/log/${circuit}.stderr" &&
               awk -v circuit="$circuit" -v method="$method" -v repetition="$repetition" -v logfile="$log" \
                   -f "$project_dir/verification/paper_core/limit30_metrics.awk" "$csv" >> "$metrics"; then
                tail -n 1 "$metrics"
                if [[ $method == core ]]; then
                    awk -v circuit="$circuit" -v repetition="$repetition" -f "$project_dir/verification/paper_core/core_statistics.awk" \
                        "$job_dir/log/${circuit}.stderr" >> "$core_statistics"
                fi
            else
                echo "FAILED: $circuit $method; inspect $job_dir/log" >&2
                failed=1; pair_ok=0
            fi
        done
        if (( pair_ok )); then
            if ! awk -v circuit="$circuit" -v repetition="$repetition" \
                -v details="$result_dir/run$repetition/${circuit}_fault_comparison.csv" \
                -f "$project_dir/verification/paper_core/limit30_compare.awk" \
                "$result_dir/run$repetition/xid/fdp/${circuit}_fdp.csv" \
                "$result_dir/run$repetition/core/fdp/${circuit}_fdp.csv" >> "$comparisons"; then
                echo "Completed-fault FDP mismatch or missing faults: $circuit" >&2
                failed=1
            fi
        fi
    done
done
echo "Measurements: $metrics"
echo "Coverage comparison: $comparisons"
echo "CORE bit statistics: $core_statistics"
exit "$failed"

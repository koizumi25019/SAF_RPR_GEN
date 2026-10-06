#!/usr/bin/env bash
# Low-power ON only; .set -> C executable. Small: warmup + 7 runs, medium: 1 run.
# Usage: bash run_tdf_power_benchmark.sh [--wall-limit seconds] [s27 s208 s5378 s9234]
set -euo pipefail
project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
wall_limit=1800
resume_dir=
while (( $# )) && [[ $1 == --* ]]; do
    case "$1" in
        --wall-limit) wall_limit=${2:-}; shift 2 ;;
        --resume-dir) resume_dir=${2:-}; shift 2 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done
[[ $wall_limit =~ ^[0-9]+$ ]] || { echo '--wall-limit needs non-negative seconds' >&2; exit 1; }
circuits=("$@")
if (( $# == 0 )); then circuits=(s27 s208 s5378 s9234); fi
for circuit in "${circuits[@]}"; do
    case "$circuit" in
        s27|s208) settings=${circuit}_tdf_core_lp20 ;;
        s5378|s9234) settings=${circuit}_tdf_core_lp20_l30 ;;
        *) echo "Unsupported circuit: $circuit" >&2; exit 1 ;;
    esac
    [[ -f $project_dir/input/script/$settings.set ]] || exit 1
done
# Clear every getenv hook used by C; keep inherited proxy/credential variables.
while IFS= read -r control; do unset "$control"; done < <(
    rg -o --no-filename 'getenv\("[A-Z0-9_]+"\)' "$project_dir/src" -g '*.c' |
        sed -E 's/getenv\("([A-Z0-9_]+)"\)/\1/' | sort -u
)
run_stamp=$(TZ=Asia/Tokyo date '+%Y%m%dT%H%M%S%z')
result_dir=${resume_dir:-"$project_dir/output/paper_core/tdf/lp20/benchmark/$run_stamp"}
mkdir -p "$result_dir"
result_dir=$(cd -- "$result_dir" && pwd)
if [[ ! -f $result_dir/circuits.txt ]]; then printf '%s\n' "${circuits[@]}" > "$result_dir/circuits.txt"; fi
echo "$wall_limit" > "$result_dir/wall_limit_seconds.txt"
printf '%s\n' "$result_dir" > "$project_dir/output/paper_core/tdf/lp20/benchmark/latest.txt"
metrics="$result_dir/measurements.csv"
if [[ ! -f $metrics ]]; then
    echo 'circuit,run,limit,threshold_percent,signals,budget,inputs,cpu_s,wall_s,dc_s,sat_s,bdd_s,representatives,complete,incomplete,zero_fdp,positive_fdp,cubes,input_bits,care_after_core,care_after_min,negative_solves,core_x_pct,min_x_pct,tested_input_pct,deleted_core_pct' > "$metrics"
fi
sha256sum "$project_dir/build/main_release" > "$result_dir/binary.sha256"
git -C "$project_dir" rev-parse HEAD > "$result_dir/source_commit.txt"
taskset -pc $$ > "$result_dir/affinity.txt"
echo "Results: $result_dir"
cd "$project_dir/build"
failed=0
for circuit in "${circuits[@]}"; do
    case "$circuit" in
        s27|s208) settings=${circuit}_tdf_core_lp20; repeats=7; first=0 ;;
        *) settings=${circuit}_tdf_core_lp20_l30; repeats=1; first=1 ;;
    esac
    for (( repetition=first; repetition<=repeats; repetition++ )); do
        job_dir="$result_dir/$circuit/run$repetition"
        mkdir -p "$job_dir"
        [[ ! -f $job_dir/run.set ]] || { echo "Existing run: $job_dir" >&2; exit 1; }
        awk -v csv="$job_dir/fdp.csv" -v output_log="$job_dir/report.txt" '
            $1 == "-fdp" { print "-fdp " csv; next }
            $1 == "-log" { print "-log " output_log; next }
            { print }
        ' "../input/script/$settings.set" > "$job_dir/run.set"
        echo "$circuit run $repetition/$repeats start $(TZ=Asia/Tokyo date '+%T %Z')"
        # No GT/core extra verification in performance timings. run0 is warmup.
        command=(./main_release -set "$job_dir/run.set")
        if (( wall_limit )); then command=(timeout --signal=TERM --kill-after=10s "$wall_limit" "${command[@]}"); fi
        TIMEFORMAT='%3R,%3U,%3S'
        if { time "${command[@]}" > "$job_dir/stdout.txt" 2> "$job_dir/stderr.txt"; } 2> "$job_dir/timing.txt"; then
            code=0; status=completed
        else
            code=$?; status=failed; failed=1
            if (( code == 124 )); then status=timeout; fi
        fi
        echo 'circuit,run,status,exit_code,wall_s,cpu_s' > "$job_dir/status.csv"
        awk -F, -v c="$circuit" -v r="$repetition" -v status="$status" -v code="$code" \
            'NF == 3 {printf "%s,%d,%s,%d,%.3f,%.3f\n",c,r,status,code,$1,$2+$3}' \
            "$job_dir/timing.txt" >> "$job_dir/status.csv"
        if (( code )); then
            echo "$circuit run $repetition: $status (exit $code), kept raw files"
            break
        fi
        if (( repetition )); then
            awk -v circuit="$circuit" -v repetition="$repetition" \
                -v logfile="$job_dir/report.txt" -v errfile="$job_dir/stderr.txt" \
                -f "$project_dir/verification/paper_core/tdf_power_metrics.awk" \
                "$job_dir/fdp.csv" >> "$metrics"
            tail -n 1 "$metrics"
        fi
    done
done
echo "Measurements: $metrics"
exit "$failed"

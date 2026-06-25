#!/bin/bash
# 支配故障解析の効果測定: 全回路を -limit 30 で統一して with/without dom を比較
#
# 実行: cd <repo_root> && bash analysis/dom_compare.sh
# 出力: analysis/dom_compare_result.csv

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$REPO_DIR/build"
BIN="./main_release"
OUT_CSV="$SCRIPT_DIR/dom_compare_result.csv"
TMPSET="$BUILD_DIR/.tmp_dom.set"
LIMIT=30

CIRCUITS=(
    c17a
    s27_C s208_C s298_C s344_C s349_C s382_C s386_C s400_C
    s420_C s444_C s510_C s526_C s641_C s713_C s820_C s832_C
    s838_C s953_C s1196_C s1238_C s1423_C s1488_C s1494_C
    s5378_C s9234_C
    b04 b05 b07 b11 b12 b14 b15
)

cd "$BUILD_DIR"
mkdir -p ../output/limit${LIMIT}/{fdp,log}

dom_field () { echo "$1" | grep -oP "(?<=${2}=)\d+" || echo "0"; }

echo "circuit,n_faults,dom_edges,sat_with_dom,sat_without_dom,seeded_cubes,reduction_pct,cadical_dom_sec,cadical_nodom_sec,cadical_speedup" > "$OUT_CSV"

printf "%-16s %7s %6s %10s %10s %7s %8s %10s %10s %7s\n" \
    "circuit" "faults" "edges" "sat_dom" "sat_nodom" "seeded" "reduc%" "cdcl_dom" "cdcl_nd" "speedup"
echo "-----------------------------------------------------------------------------------------------------"

for circ in "${CIRCUITS[@]}"; do
    net_file="../input/circuit/${circ}.v"
    [ -f "$net_file" ] || { echo "  SKIP: $circ (回路ファイルなし)"; continue; }

    # 一時 set ファイルを生成（-limit 30 で統一）
    cat > "$TMPSET" <<EOF
-net $net_file
-fdp ../output/limit${LIMIT}/fdp/${circ}.csv
-log ../output/limit${LIMIT}/log/${circ}.txt
-limit $LIMIT
EOF

    # ---- with dominance ----
    out_dom=$("$BIN" -set "$TMPSET" 2>/dev/null) || { echo "  FAIL: $circ (dom)"; continue; }
    dom_line=$(echo "$out_dom" | grep '^\[DOM\]')
    [ -z "$dom_line" ] && { echo "  FAIL: $circ (DOM 行なし)"; continue; }

    edges=$(echo "$out_dom"   | grep -oP '(?<=Dominance fault analysis: )\d+' || echo "0")
    n_faults=$(echo "$out_dom" | grep "Number of Target Faults" | grep -oP '\d+' | head -1 || echo "0")
    sat_dom=$(dom_field   "$dom_line" sat_calls)
    seeded=$(dom_field    "$dom_line" seeded)
    cadical_dom=$(echo "$out_dom" | grep "CaDiCaL" | grep -oP '[\d.]+' | head -1 || echo "0")

    # ---- without dominance (MDC_NODOM=1) ----
    out_nd=$(MDC_NODOM=1 "$BIN" -set "$TMPSET" 2>/dev/null) || { echo "  FAIL: $circ (nodom)"; continue; }
    dom_line_nd=$(echo "$out_nd" | grep '^\[DOM\]')
    # MDC_NODOM=1 では seeded=0, sat_calls=total_cubes
    sat_nodom=$(dom_field "$dom_line_nd" total_cubes)
    cadical_nd=$(echo "$out_nd" | grep "CaDiCaL" | grep -oP '[\d.]+' | head -1 || echo "0")

    # ---- 集計 ----
    reduction="0.0"
    [ "$sat_nodom" -gt 0 ] && \
        reduction=$(awk "BEGIN{printf \"%.1f\", ($sat_nodom-$sat_dom)*100.0/$sat_nodom}")

    speedup="1.00"
    [ "$(awk "BEGIN{print ($cadical_dom > 0.0001)}")" -eq 1 ] && \
        speedup=$(awk "BEGIN{printf \"%.2f\", $cadical_nd/$cadical_dom}")

    printf "%-16s %7s %6s %10s %10s %7s %7s%% %10s %10s %7s\n" \
        "$circ" "$n_faults" "$edges" "$sat_dom" "$sat_nodom" "$seeded" \
        "$reduction" "$cadical_dom" "$cadical_nd" "${speedup}x"

    echo "$circ,$n_faults,$edges,$sat_dom,$sat_nodom,$seeded,$reduction,$cadical_dom,$cadical_nd,$speedup" >> "$OUT_CSV"
done

rm -f "$TMPSET"
echo ""
echo "=> $OUT_CSV に保存しました"

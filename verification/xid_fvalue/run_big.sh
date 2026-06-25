#!/bin/bash
# 大規模回路の inline 再測定（修正後バイナリ, limit 30, XSTAT=1）。
# 進捗を progress.log と results/<c>_inline.progress に出す。external 旧データは無いので inline のみ。
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/main_release"
OUT="$ROOT/verification/xid_fvalue/results"
LOG="$ROOT/verification/xid_fvalue/progress.log"
LIMIT="${LIMIT:-30}"
CIRCUITS=( s13207_C s15850_C s35932_C s38417_C s38584_C )

mkdir -p "$OUT"
echo "=== run_big START $(date '+%F %T') limit=$LIMIT ===" > "$LOG"

for c in "${CIRCUITS[@]}"; do
  set="$OUT/${c}_inline.set"
  {
    echo "-net ../input/circuit/${c}.v"
    echo "-fdp ../verification/xid_fvalue/results/${c}_inline.fdp.csv"
    echo "-log ../verification/xid_fvalue/results/${c}_inline.log"
    echo "-limit ${LIMIT}"
  } > "$set"

  echo "START $c $(date '+%T')" >> "$LOG"
  start=$(date +%s.%N)
  ( cd "$ROOT/build" && env NO_DISCORD=1 XSTAT=1 "$BIN" \
      -set "../verification/xid_fvalue/results/${c}_inline.set" \
      > "$OUT/${c}_inline.progress" 2> "$OUT/${c}_inline.stderr" )
  rc=$?
  end=$(date +%s.%N)
  wall=$(awk "BEGIN{printf \"%.1f\", $end-$start}")
  echo "$rc $wall" > "$OUT/${c}_inline.runinfo"
  xstat=$(grep XSTAT "$OUT/${c}_inline.stderr" 2>/dev/null)
  echo "DONE  $c rc=$rc wall=${wall}s  $xstat" >> "$LOG"
done
echo "=== run_big ALLDONE $(date '+%F %T') ===" >> "$LOG"

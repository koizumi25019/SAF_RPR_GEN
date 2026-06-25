#!/bin/bash
# 大規模回路の external XID 計測（旧・外部実行体XID(Miyase2004), limit 30, XSTAT=1）。
# run_big.sh の inline と対になる比較用。XID判定だけ差替え、モデル/ソルバ/limit は固定。
# 注: external は回路を毎回読み直すため inline の ~10倍遅い。
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/main_release"
XBIN="$ROOT/FaultSim/FaultSim/Build/Release/my_app"
OUT="$ROOT/verification/xid_fvalue/results"
LOG="$ROOT/verification/xid_fvalue/progress_external.log"
LIMIT="${LIMIT:-30}"
CIRCUITS=( s13207_C s15850_C )

if [ ! -x "$XBIN" ]; then
  echo "ERROR: external XID binary not found: $XBIN" >&2
  exit 1
fi

mkdir -p "$OUT"
echo "=== run_big_external START $(date '+%F %T') limit=$LIMIT ===" > "$LOG"

for c in "${CIRCUITS[@]}"; do
  set="$OUT/${c}_external.set"
  {
    echo "-net ../input/circuit/${c}.v"
    echo "-fdp ../verification/xid_fvalue/results/${c}_external.fdp.csv"
    echo "-log ../verification/xid_fvalue/results/${c}_external.log"
    echo "-limit ${LIMIT}"
  } > "$set"

  echo "START $c $(date '+%T')" | tee -a "$LOG"
  start=$(date +%s.%N)
  ( cd "$ROOT/build" && env NO_DISCORD=1 XSTAT=1 XID_EXTERNAL="$XBIN" "$BIN" \
      -set "../verification/xid_fvalue/results/${c}_external.set" \
      > "$OUT/${c}_external.progress" 2> "$OUT/${c}_external.stderr" )
  rc=$?
  end=$(date +%s.%N)
  wall=$(awk "BEGIN{printf \"%.1f\", $end-$start}")
  echo "$rc $wall" > "$OUT/${c}_external.runinfo"
  xstat=$(grep XSTAT "$OUT/${c}_external.stderr" 2>/dev/null)
  echo "DONE  $c rc=$rc wall=${wall}s  $xstat" | tee -a "$LOG"
done
echo "=== run_big_external ALLDONE $(date '+%F %T') ===" | tee -a "$LOG"

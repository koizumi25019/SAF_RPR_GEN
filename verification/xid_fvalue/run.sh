#!/bin/bash
# 故障値考慮XID(現行 InlineXID) vs 旧・外部実行体XID(Miyase2004) の単体効果比較。
# モデル/ソルバ/回路/limit/ブロッキング帰還を固定し、X判定だけを差し替える
# （XID_EXTERNAL=<bin> のとき外部XIDを呼ぶフックを使用）。
#
# 取得: X率(stderr [XSTAT])、cube総数、fdp一致、各CPU時間(log)、壁時計(runinfo)。
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/main_release"
XBIN="$ROOT/FaultSim/FaultSim/Build/Release/my_app"   # 旧・外部XID実行体
OUT="$ROOT/verification/xid_fvalue/results"
LIMIT="${LIMIT:-30}"          # 全故障に同一適用（無制限では大回路が完走しないため）
TIMEOUT="${TIMEOUT:-3600}"
CIRCUITS=( s5378_C s9234_C )

mkdir -p "$OUT"

run_one() {  # $1=circuit  $2=variant(inline|external)
  local c="$1" v="$2"
  local set="$OUT/${c}_${v}.set"
  {
    echo "-net ../input/circuit/${c}.v"
    echo "-fdp ../verification/xid_fvalue/results/${c}_${v}.fdp.csv"
    echo "-log ../verification/xid_fvalue/results/${c}_${v}.log"
    echo "-limit ${LIMIT}"
  } > "$set"

  local env_x=""
  [ "$v" = "external" ] && env_x="XID_EXTERNAL=$XBIN"

  echo "===== $c / $v (limit=$LIMIT) ====="
  local start end wall
  start=$(date +%s.%N)
  ( cd "$ROOT/build" && timeout "$TIMEOUT" env NO_DISCORD=1 $env_x "$BIN" \
      -set "../verification/xid_fvalue/results/${c}_${v}.set" \
      >/dev/null 2>"$OUT/${c}_${v}.stderr" )
  local rc=$?
  end=$(date +%s.%N)
  wall=$(awk "BEGIN{printf \"%.1f\", $end-$start}")
  echo "$rc $wall" > "$OUT/${c}_${v}.runinfo"
  echo "  >> rc=$rc wall=${wall}s  $(grep XSTAT "$OUT/${c}_${v}.stderr" || true)"
}

for c in "${CIRCUITS[@]}"; do
  run_one "$c" inline
  run_one "$c" external
done
echo "DONE"

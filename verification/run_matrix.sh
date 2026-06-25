#!/bin/bash
# 案1(MAXDC)/案2(MAXHAM) の組み合わせ実験ドライバ。
# 各 (回路 × 構成) を「全故障・無制限列挙」で実行し、fdp CSV と log を
# verification/results/<回路>/<構成>.* に保存する。壁時計時間も記録。
#
# 使い方: ./verification/run_matrix.sh            # 既定の回路リスト
#         ./verification/run_matrix.sh s27_C s298_C   # 回路を指定
#
# 構成(env):
#   baseline       : なし
#   maxdc          : MAXDC=1            （案1: 素項展開）
#   maxham         : MAXHAM=1           （案2: 多様化）
#   maxdc_maxham   : MAXDC=1 MAXHAM=1   （両立）

set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/main_release"
TIMEOUT="${TIMEOUT:-300}"   # 1実行の上限秒

CIRCUITS=( c17a s27_C s208_C s298_C s386_C s510_C s641_C s713_C s953_C )
if [ "$#" -gt 0 ]; then CIRCUITS=("$@"); fi

CONFIGS=( baseline maxdc maxham maxdc_maxham )

env_for() {
  case "$1" in
    baseline)     echo "" ;;
    maxdc)        echo "MAXDC=1" ;;
    maxham)       echo "MAXHAM=1" ;;
    maxdc_maxham) echo "MAXDC=1 MAXHAM=1" ;;
  esac
}

for c in "${CIRCUITS[@]}"; do
  v="$ROOT/input/circuit/${c}.v"
  if [ ! -f "$v" ]; then echo "skip $c (no .v)"; continue; fi
  outdir="$ROOT/verification/results/${c}"
  mkdir -p "$outdir"

  for cfg in "${CONFIGS[@]}"; do
    setfile="$outdir/${cfg}.set"
    # .set 内のパスは build/ 基準（main_release を build/ で実行するため）
    {
      echo "-net ../input/circuit/${c}.v"
      echo "-fdp ../verification/results/${c}/${cfg}.fdp.csv"
      echo "-log ../verification/results/${c}/${cfg}.log"
      # -fault なし=全故障 / LIMIT 未設定なら無制限
      [ -n "${LIMIT:-}" ] && echo "-limit ${LIMIT}"
    } > "$setfile"

    e="$(env_for "$cfg")"
    echo "===== $c / $cfg (env: ${e:-none}) ====="
    start=$(date +%s.%N)
    ( cd "$ROOT/build" && timeout "$TIMEOUT" env NO_DISCORD=1 $e "$BIN" -set "../verification/results/${c}/${cfg}.set" >/dev/null 2>"$outdir/${cfg}.stderr" )
    rc=$?
    end=$(date +%s.%N)
    wall=$(awk "BEGIN{printf \"%.3f\", $end-$start}")
    echo "$rc $wall" > "$outdir/${cfg}.runinfo"   # 終了コード, 壁時計秒
    if [ "$rc" -eq 124 ]; then echo "  >> TIMEOUT (${TIMEOUT}s)"; else echo "  >> rc=$rc wall=${wall}s"; fi
  done
done
echo "DONE"

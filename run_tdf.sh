#!/bin/bash
# TDF用の設定を順番に実行する。失敗しても残りの回路を続行する。
# 使い方: ./run_tdf.sh                 # 下の5回路を実行
#         ./run_tdf.sh s9234 b14       # 指定した回路のみ実行
#         ./run_tdf.sh s27_tdf_list    # .set の名前でも指定可能

set -u

CIRCUITS=(
    s9234
    s35932
    s38417
    b14
    b17
)

if [ "$#" -gt 0 ]; then
    CIRCUITS=("$@")
fi

cd "$(dirname "$0")/build" || { echo "build ディレクトリが無い"; exit 1; }
if [ ! -x ./main_release ]; then
    echo "main_release が無いか実行できません。先にビルドしてください。"
    exit 1
fi

total=${#CIRCUITS[@]}
failed=()

for i in "${!CIRCUITS[@]}"; do
    c="${CIRCUITS[$i]%.set}"
    case "$c" in
        *_tdf|*_tdf_list) ;;
        *) c="${c}_tdf" ;;
    esac
    n=$((i + 1))
    setfile="../input/script/${c}.set"
    echo "===== [$n/$total] $c 開始 $(date '+%T') ====="

    if [ ! -f "$setfile" ]; then
        echo "  スキップ: $setfile が見つからない"
        failed+=("$c (no .set)")
        continue
    fi

    # .set に指定したCSV・ログの出力ディレクトリを準備する。
    if ! python3 - "$setfile" <<'PY'
import sys
from pathlib import Path

for line in Path(sys.argv[1]).read_text().splitlines():
    tokens = line.split()
    if tokens and tokens[0] in ('-fdp', '-log'):
        Path(tokens[1]).parent.mkdir(parents=True, exist_ok=True)
PY
    then
        failed+=("$c (output directory)")
        continue
    fi

    if NO_DISCORD=1 ./main_release -set "$setfile"; then
        echo "===== [$n/$total] $c 終了 $(date '+%T') ====="
    else
        echo "===== [$n/$total] $c 失敗(続行) $(date '+%T') ====="
        failed+=("$c")
    fi
done

echo
echo "===== TDF 全実験終了 ($total 件) ====="
if [ "${#failed[@]}" -gt 0 ]; then
    echo "失敗した回路: ${failed[*]}"
    exit 1
fi

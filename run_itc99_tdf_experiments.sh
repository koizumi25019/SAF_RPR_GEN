#!/bin/bash
# ITC'99 b系列の遷移故障（TDF, LOC方式）実験を順番に実行する。
#
# 使い方:
#   ./run_itc99_tdf_experiments.sh
#   ./run_itc99_tdf_experiments.sh b11 b14
#   MAXDC=1 MAXDC_CORE=1 ./run_itc99_tdf_experiments.sh b11
#
# 回路名は input/script/<name>_tdf_exp.set に対応する。

set -u

CIRCUITS=(
    
    b20
    b21
    b22
)

if [ "$#" -gt 0 ]; then
    CIRCUITS=("$@")
fi

script_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$script_dir/build"

if [ ! -x "$build_dir/main_release" ]; then
    echo "実行ファイルが見つかりません: $build_dir/main_release"
    echo "先に build ディレクトリで cmake .. && make を実行してください。"
    exit 1
fi

# .set 内の相対パスは build/ 基準。
cd "$build_dir" || exit 1
mkdir -p ../output/limit30/fdp ../output/limit30/log

total=${#CIRCUITS[@]}
failed=()

for i in "${!CIRCUITS[@]}"; do
    circuit="${CIRCUITS[$i]}"
    number=$((i + 1))
    setfile="../input/script/${circuit}_tdf_exp.set"

    echo "===== [$number/$total] $circuit (TDF) 開始 $(date '+%F %T') ====="

    if [ ! -f "$setfile" ]; then
        echo "  スキップ: $setfile が見つかりません"
        failed+=("$circuit (no .set)")
        continue
    fi

    if NO_DISCORD=1 ./main_release -set "$setfile"; then
        echo "===== [$number/$total] $circuit (TDF) 終了 $(date '+%F %T') ====="
    else
        echo "===== [$number/$total] $circuit (TDF) 失敗、残りを続行 $(date '+%F %T') ====="
        failed+=("$circuit")
    fi
done

echo
echo "===== 全ITC'99 TDF実験終了 ($total 件) ====="
if [ "${#failed[@]}" -gt 0 ]; then
    echo "失敗した回路: ${failed[*]}"
    exit 1
fi

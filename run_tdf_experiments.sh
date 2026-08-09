#!/bin/bash
# 遷移故障（TDF）実験を複数回路で順番に実行するバッチスクリプト。
# run_experiments.sh（SAF用）の TDF 版。1回路終わったら自動で次へ進む。
# 途中でこけても残りは続行する。
#
# 使い方:
#   ./run_tdf_experiments.sh                   # 下の CIRCUITS リストを順に実行
#   ./run_tdf_experiments.sh s5378 s9234        # 引数で回路を指定（リストより優先）
#
# 回路名は input/script/<name>_tdf_exp.set に対応する（-tdf・全代表故障自動生成・-limit 30）。

set -u

# ----- 実行したい回路をここに並べる（上から順に実行、s208〜s38584）-----
CIRCUITS=(
    s208
)

# 引数があればそちらを優先する
if [ "$#" -gt 0 ]; then
    CIRCUITS=("$@")
fi

# .set 内のパスは build/ からの相対なので build/ で実行する
cd "$(dirname "$0")/build" || { echo "build ディレクトリが無い"; exit 1; }

total=${#CIRCUITS[@]}
failed=()   # 失敗した回路を記録

for i in "${!CIRCUITS[@]}"; do
    c="${CIRCUITS[$i]}"
    n=$((i + 1))
    setfile="../input/script/${c}_tdf_exp.set"

    echo "===== [$n/$total] $c (TDF) 開始 $(date '+%T') ====="

    if [ ! -f "$setfile" ]; then
        echo "  スキップ: $setfile が見つからない"
        failed+=("$c (no .set)")
        continue
    fi

    if NO_DISCORD=1 ./main_release -set "$setfile"; then
        echo "===== [$n/$total] $c (TDF) 終了 $(date '+%T') ====="
    else
        echo "===== [$n/$total] $c (TDF) 失敗(続行) $(date '+%T') ====="
        failed+=("$c")
    fi
done

echo
echo "===== 全TDF実験終了 ($total 件) ====="
if [ "${#failed[@]}" -gt 0 ]; then
    echo "失敗した回路: ${failed[*]}"
fi

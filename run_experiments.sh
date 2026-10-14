#!/bin/bash
# 複数回路を順番に実行するバッチスクリプト。
# 1回路終わったら自動で次へ進む。途中でこけても残りは続行する。
#
# 使い方:
#   ./run_experiments.sh                 # 下の CIRCUITS リストを順に実行
#   ./run_experiments.sh s5378_C s9234_C # 引数で回路を指定（リストより優先）
#
# 回路名は input/script/<name>.set に対応する。

set -u

# ----- 実行したい回路をここに並べる（上から順に実行）-----
CIRCUITS=(
    s5378_C
    s9234_C
    s13207_C
    s15850_C
    s35932_C
    s38417_C
    s38584_C
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
    setfile="../input/script/${c}.set"

    echo "===== [$n/$total] $c 開始 $(date '+%T') ====="

    if [ ! -f "$setfile" ]; then
        echo "  スキップ: $setfile が見つからない"
        failed+=("$c (no .set)")
        continue
    fi

    # 各回の Discord 通知は抑止する（毎回鳴るとうるさいので）。
    # 全実験の完了は、ループ後にまとめて1回通知する。
    if NO_DISCORD=1 ./main_release -set "$setfile"; then
        echo "===== [$n/$total] $c 終了 $(date '+%T') ====="
    else
        echo "===== [$n/$total] $c 失敗(続行) $(date '+%T') ====="
        failed+=("$c")
    fi
done

echo
echo "===== 全実験終了 ($total 件) ====="
if [ "${#failed[@]}" -gt 0 ]; then
    echo "失敗した回路: ${failed[*]}"
fi

# 全部終わったことを Discord に1回だけ通知する
curl -s -H "Content-Type: application/json" -X POST \
    -d '{"content":"全実験終了"}' \
    "https://discord.com/api/webhooks/1502285313316487319/e6m14JwtzCNtU7ARSKlk7SjsWznXIvPhC6ONMuFFpfpTkuNGu-_cID41AsdUzQ79YZIT" \
    >/dev/null

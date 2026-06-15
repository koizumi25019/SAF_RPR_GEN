#!/bin/bash
# ITC'99 ベンチマーク(b シリーズ)を順番に実行するバッチスクリプト。
# 1回路終わったら自動で次へ進む。途中でこけても残りは続行する。
#
# 使い方:
#   ./run_itc99.sh                # 下の CIRCUITS リストを順に実行
#   ./run_itc99.sh b04 b07 b11    # 引数で回路を指定（リストより優先）
#
# 回路名は data/script/<name>.set に対応する。
# 全 .set は -limit 30・全故障。必須割当て/支配故障解析はデフォルト ON。

set -u

# ----- 実行したい回路をここに並べる（上から順、小さい回路 → 大きい回路）-----
# 後半ほど重い。特に b18(約21万行)・b19(約49万行)は桁違いに重いので、
# 軽いものだけ回したいときは引数で指定するか下をコメントアウトする。
CIRCUITS=(
    b11
    b12
    b15
    b14
    b20
    b21
    b22
    b17
    b18
    b19
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
    setfile="../data/script/${c}.set"

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
    -d '{"content":"ITC99 全実験終了"}' \
    "https://discord.com/api/webhooks/1502285313316487319/e6m14JwtzCNtU7ARSKlk7SjsWznXIvPhC6ONMuFFpfpTkuNGu-_cID41AsdUzQ79YZIT" \
    >/dev/null

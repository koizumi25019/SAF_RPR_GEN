#!/bin/bash
# b15 だけを実行するスクリプト。
#
# 使い方:
#   ./run_b15.sh
#
# 回路名は input/script/b15.set に対応する。-limit 30・全故障。

set -u

# .set 内のパスは build/ からの相対なので build/ で実行する
cd "$(dirname "$0")/build" || { echo "build ディレクトリが無い"; exit 1; }

setfile="../input/script/b15.set"
if [ ! -f "$setfile" ]; then
    echo "$setfile が見つからない"
    exit 1
fi

echo "===== b15 開始 $(date '+%T') ====="
if NO_DISCORD=1 ./main_release -set "$setfile"; then
    echo "===== b15 終了 $(date '+%T') ====="
else
    echo "===== b15 失敗 $(date '+%T') ====="
    exit 1
fi

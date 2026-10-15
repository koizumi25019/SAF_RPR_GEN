#!/bin/bash
# 共通バッチ実行。設定内のパスは build/ を基準に解釈する。
set -u

usage() {
    echo "使い方: $0 saf|tdf <設定名> [設定名 ...]"
    echo "例: $0 saf c17a; $0 tdf s27 s208_tdf_list"
}
if [ "${1:-}" = --help ] || [ "${1:-}" = -h ]; then
    usage
    exit 0
fi
if [ "$#" -lt 2 ]; then
    usage >&2
    exit 2
fi
model=$1
shift
case "$model" in
    saf|tdf) ;;
    *) usage >&2; exit 2 ;;
esac

repo_dir="$(cd "$(dirname "$0")/.." && pwd)" || exit 1
cd "$repo_dir/build" || { echo "build ディレクトリが無い" >&2; exit 1; }
if [ ! -x ./main_release ]; then
    echo "main_release が無いか実行できません。先にビルドしてください。" >&2
    exit 1
fi

total=$#
index=0
failed=()
for requested in "$@"; do
    index=$((index + 1))
    name=${requested%.set}
    if [[ ! "$name" =~ ^[A-Za-z0-9_-]+$ ]]; then
        echo "不正な設定名: $requested" >&2
        failed+=("$requested (invalid name)")
        continue
    fi
    if [ "$model" = tdf ]; then
        case "$name" in
            *_tdf|*_tdf_list) ;;
            *) name="${name}_tdf" ;;
        esac
    fi
    setfile="../input/script/$model/$name.set"
    echo "===== [$index/$total] $model/$name 開始 $(date '+%T') ====="
    if [ ! -f "$setfile" ]; then
        echo "$setfile が見つからない" >&2
        failed+=("$name (no .set)")
        continue
    fi
    # SAF/TDF共通で設定に記載された出力ディレクトリを準備する。
    if ! python3 - "$setfile" <<'PY'
import sys
from pathlib import Path

for line in Path(sys.argv[1]).read_text().splitlines():
    tokens = line.split('#', 1)[0].split()
    if tokens and tokens[0] in ('-fdp', '-log'):
        Path(tokens[1]).parent.mkdir(parents=True, exist_ok=True)
PY
    then
        failed+=("$name (output directory)")
        continue
    fi
    if NO_DISCORD=1 ./main_release -set "$setfile"; then
        echo "===== [$index/$total] $model/$name 終了 $(date '+%T') ====="
    else
        echo "===== [$index/$total] $model/$name 失敗(続行) $(date '+%T') =====" >&2
        failed+=("$name")
    fi
done

echo "===== $model 全実験終了 ($total 件) ====="
if [ "${#failed[@]}" -gt 0 ]; then
    echo "失敗した設定: ${failed[*]}" >&2
fi
# 通知は明示的にWebhookが設定された場合だけ送る。
if [ "${NO_DISCORD:-0}" != 1 ] && [ -n "${DISCORD_WEBHOOK_URL:-}" ]; then
    curl --fail --silent --show-error --max-time 10 \
        -H 'Content-Type: application/json' -X POST \
        -d "{\"content\":\"$model 全実験終了 ($total 件、失敗 ${#failed[@]} 件)\"}" \
        "$DISCORD_WEBHOOK_URL" >/dev/null || echo "通知に失敗しました" >&2
fi
[ "${#failed[@]}" -eq 0 ]

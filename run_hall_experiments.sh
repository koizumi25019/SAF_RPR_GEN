#!/bin/bash
# FDPの故障検出関数をAIGERへ変換し、公式HALLで故障ごとにAllSAT列挙する。
# 通常のFDP実験は run_experiments.sh、TDF実験は run_tdf_experiments.sh を使う。
#
# 使い方:
#   ./run_hall_experiments.sh c17a
#   HALL_MODE=tale HALL_TIMEOUT=300 ./run_hall_experiments.sh s5378_C
#   HALL_MODE=roc HALL_PRINT=1 ./run_hall_experiments.sh c17a
#
# HALL_MODE: tale/mars-dis/mars-nondis/duty/core/roc/carma（既定roc）
# HALL_TIMEOUT: 故障1個あたりの秒数（既定60）
# HALL_PRINT: HALLが列挙したキューブをログへ出すなら1（既定0）

set -u

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
HALL_MODE=${HALL_MODE:-roc}
HALL_TIMEOUT=${HALL_TIMEOUT:-60}
HALL_PRINT=${HALL_PRINT:-0}
HALL_TOOL="$ROOT_DIR/external/hall/build/hall_tool"

case "$HALL_MODE" in
    tale|mars-dis|mars-nondis|duty|core|roc|carma) ;;
    *)
        echo "HALL_MODE=$HALL_MODE は未対応（tale/mars-dis/mars-nondis/duty/core/roc/carma から選択）"
        exit 1
        ;;
esac
if ! [[ "$HALL_TIMEOUT" =~ ^[1-9][0-9]*$ ]]; then
    echo "HALL_TIMEOUT は1以上の整数で指定する"
    exit 1
fi
if [ "$HALL_PRINT" != 0 ] && [ "$HALL_PRINT" != 1 ]; then
    echo "HALL_PRINT は0または1で指定する"
    exit 1
fi
if [ ! -x "$HALL_TOOL" ]; then
    echo "HALL本家が未ビルド: $HALL_TOOL"
    echo "  git clone --recurse-submodules https://github.com/yogevshalmon/allsat-circuits.git external/hall"
    echo "  cmake -S external/hall -B external/hall/build"
    echo "  cmake --build external/hall/build -j2"
    exit 1
fi
if [ ! -x "$ROOT_DIR/build/main_release" ]; then
    echo "FDP本体が未ビルド: $ROOT_DIR/build/main_release"
    exit 1
fi

# ----- HALLで実行したい回路をここに並べる -----
CIRCUITS=(
    s13207_C
    s15850_C
    s35932_C
    s38417_C
    s38584_C
)

# 引数があればそちらを優先する。
if [ "$#" -gt 0 ]; then
    CIRCUITS=("$@")
fi

# .set 内のパスは build/ からの相対。
cd "$ROOT_DIR/build" || { echo "build ディレクトリが無い"; exit 1; }

total=${#CIRCUITS[@]}
failed=()

for i in "${!CIRCUITS[@]}"; do
    c="${CIRCUITS[$i]}"
    n=$((i + 1))
    setfile="../input/script/${c}.set"

    echo "===== [$n/$total] $c HALL($HALL_MODE) 開始 $(date '+%T') ====="

    if [ ! -f "$setfile" ]; then
        echo "  スキップ: $setfile が見つからない"
        failed+=("$c (no .set)")
        continue
    fi

    # AIG_DUMPはSAFの検出関数用。TDFでは励起条件をAIGへ含めない。
    if grep -Eq '^[[:space:]]*-tdf([[:space:]]|$)' "$setfile"; then
        echo "  スキップ: HALL比較のAIG_DUMPはTDF未対応"
        failed+=("$c (TDF unsupported by AIG_DUMP)")
        continue
    fi

    run_id=$(date '+%Y%m%d_%H%M%S_%N')
    hall_out="$ROOT_DIR/output/hall/$HALL_MODE/$c/$run_id"
    aig_dir="$hall_out/aig"
    log_dir="$hall_out/log"
    hall_set="$hall_out/hall_dump.set"
    mkdir -p "$aig_dir" "$log_dir"

    # 元の通常出力を空ファイルで上書きしないよう、HALL専用の出力先へ差し替える。
    if ! awk \
        -v fdp_path="$hall_out/dump_fdp.csv" \
        -v log_path="$hall_out/dump_log.txt" \
        -v cube_path="$hall_out/dump_cube_analysis.csv" '
        BEGIN { have_fdp=0; have_log=0 }
        $1 == "-fdp"           { print "-fdp " fdp_path; have_fdp=1; next }
        $1 == "-log"           { print "-log " log_path; have_log=1; next }
        $1 == "-cube_analysis" { print "-cube_analysis " cube_path; next }
        { print }
        END {
            if (!have_fdp) print "-fdp " fdp_path
            if (!have_log) print "-log " log_path
        }
    ' "$setfile" > "$hall_set"; then
        echo "===== [$n/$total] $c HALL用set生成失敗(続行) $(date '+%T') ====="
        failed+=("$c (HALL set)")
        continue
    fi

    if ! AIG_DUMP_DIR="$aig_dir" NO_DISCORD=1 ./main_release -set "$hall_set"; then
        echo "===== [$n/$total] $c AIG生成失敗(続行) $(date '+%T') ====="
        failed+=("$c (AIG dump)")
        continue
    fi

    shopt -s nullglob
    aigs=("$aig_dir"/*.aag)
    shopt -u nullglob
    if [ "${#aigs[@]}" -eq 0 ]; then
        echo "===== [$n/$total] $c AIGなし(続行) $(date '+%T') ====="
        failed+=("$c (no AIG)")
        continue
    fi

    hall_failed=0
    hall_timed_out=0
    for aag in "${aigs[@]}"; do
        name=${aag##*/}
        name=${name%.aag}
        echo "  [HALL $HALL_MODE] $name"
        if ! "$HALL_TOOL" "$aag" \
            /mode "$HALL_MODE" \
            /general/timeout "$HALL_TIMEOUT" \
            /general/print_enumer "$HALL_PRINT" \
            > "$log_dir/$name.txt" 2>&1; then
            echo "    失敗: $log_dir/$name.txt"
            hall_failed=1
        elif grep -q '^c TIMEOUT reach' "$log_dir/$name.txt"; then
            echo "    timeout (${HALL_TIMEOUT}s): $log_dir/$name.txt"
            hall_timed_out=$((hall_timed_out + 1))
        fi
    done

    if [ "$hall_failed" -eq 0 ]; then
        echo "===== [$n/$total] $c HALL終了 (${#aigs[@]}故障, timeout=$hall_timed_out) $(date '+%T') ====="
    else
        echo "===== [$n/$total] $c HALL一部失敗(続行) $(date '+%T') ====="
        failed+=("$c (HALL)")
    fi
done

echo
echo "===== HALL実験終了 ($total 回路, mode=$HALL_MODE) ====="
if [ "${#failed[@]}" -gt 0 ]; then
    echo "失敗した回路: ${failed[*]}"
fi

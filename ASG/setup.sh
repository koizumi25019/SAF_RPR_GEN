#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLS_DIR="$SCRIPT_DIR/tools"

echo "=== submodule を初期化 ==="
git -C "$SCRIPT_DIR/.." submodule update --init --recursive

echo ""
echo "=== CaDiCaL をビルド ==="
cd "$TOOLS_DIR/cadical"
./configure
make -j$(nproc)

echo ""
echo "=== CUDD をビルド ==="
cd "$TOOLS_DIR/cudd"
./configure
make -j$(nproc)

echo ""
echo "=== 完了 ==="
echo "次のコマンドでビルドしてください:"
echo "  mkdir -p $SCRIPT_DIR/build && cd $SCRIPT_DIR/build"
echo "  cmake ../src && make"

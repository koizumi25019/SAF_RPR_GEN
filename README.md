# FDP Gen

論理回路の縮退故障（SAF）とLOC方式の遷移遅延故障（TDF）について、故障ごとの検出確率（FDP）を算出する。
SATでテストキューブを生成し、XIDでドントケアを識別し、BDDでキューブの和集合の確率を計算する。

## ディレクトリ構成

```text
input/
  circuit/{saf,tdf}/     回路ネットリスト (.v)
  fault/{saf,tdf}/       故障リスト (.txt)
  script/{saf,tdf}/      実行設定 (.set)
output/
  saf/<条件>/{fdp,log}/  SAFの実行結果
  tdf/<条件>/{fdp,log}/  TDFの実行結果
  archive/              過去の実験結果（SB-XIDなど）
scripts/                セットアップとバッチ実行
src/
  cli/                  引数・設定ファイルの読み込み
  netlist/              回路の解析・LoC展開・走査
  fault/                故障リスト・対象選択・検出状態
  fdp/                  FDP計算の進行・並列処理・キューブ集合
    cnf/                CNF生成・範囲限定・必須割当て
    xid/                ドントケア識別
    bdd/                BDDによる確率計算
  io/                   CSV・ログ・ファイル操作
  common/               メモリ確保などの共通処理
external/               CaDiCaL・CUDDのソースとライブラリ
tests/
  expected/             比較用の正解データ（実行では上書きしない）
docs/                   仕様・実装・検証手順
build/                  ビルド生成物と隔離検証の作業領域
```

`<条件>` は `.set` に指定した列挙上限に合わせ、`full`（無制限）、`limit30`などとする。
出力先は `.set` の `-fdp`・`-log` で明示する。`-limit`だけを変えても出力先は自動変更されない。
既存結果は内容を変えずSAF/TDF別に移動済み。過去の `cube_analysis/` も対応するモデル・条件の下に保管する。
`output/` はGit管理対象。`build/` はGit管理外。

## ビルド

Linux/WSL2でC/C++コンパイラ、CMake、make、GMP開発ライブラリを用意する。バッチ実行・検証にはPython 3も使う。
以下はリポジトリ直下から実行する。

```bash
./scripts/setup.sh
cmake -S . -B build
cmake --build build --parallel
```

`build/main_release` と `build/main_debug` を生成する。

## 実行

共通スクリプトは出力ディレクトリを準備し、`build/` から実行する。

```bash
./scripts/run.sh saf c17a
./scripts/run.sh tdf s27
./scripts/run.sh tdf s27_tdf_list s208_tdf_list
```

設定名は `input/script/<モデル>/` 内のファイル名を指定する（`.set`は省略可能）。
TDFは `s27` のように `_tdf` を省略して指定できる。
複数設定では失敗しても残りを続行し、1件でも失敗すれば非ゼロで終了する。

既定の実験セットも用意している。`run_experiments.sh`・`run_itc99.sh`・`run_tdf.sh`は、引数で対象を置き換えられる。

```bash
./scripts/run_experiments.sh             # SAF: ISCASの7回路
./scripts/run_itc99.sh                   # SAF: b14・b17
./scripts/run_b15.sh                     # SAF: b15のみ
./scripts/run_tdf.sh                     # TDF: s9234・s35932・s38417・b14・b17
./scripts/run_tdf.sh s9234 b14           # 対象を指定
```

通知を使う場合は `DISCORD_WEBHOOK_URL` を環境変数で設定する。`NO_DISCORD=1`で無効にできる。
通知失敗は計算結果の成否に影響しない。

直接実行する場合は、先に出力ディレクトリを作成する。

```bash
mkdir -p output/saf/full/{fdp,log}
(cd build && ./main_release -set ../input/script/saf/c17a.set)
```

**`.set`内の相対パスは、設定ファイルの位置ではなく実行時の作業ディレクトリを基準にする。**
同梱の設定は `build/` からの実行を前提としている。

## 設定項目

| 項目 | 内容 |
|---|---|
| `-net <file>` | 回路ネットリスト |
| `-fault <file>` | 故障リスト。省略時は選択モデルの全代表故障を自動生成 |
| `-saf` / `-tdf` | 故障モデル。省略時SAF。CLIと設定の読み込み順で後の指定が有効 |
| `-fdp <file>` | FDP CSVの出力先 |
| `-log <file>` | ログの出力先 |
| `-limit <N>` | 故障ごとのキューブ列挙上限。省略・0以下は無制限 |
| `-jobs <N>` | 並列数。1〜256、既定1 |
| `-dom_reuse on\|off` | 支配キューブの流用。並列時はoff、TDFでは常時off |

ディレクトリ名では故障モデルを自動判定しない。TDF設定には `-tdf` を明記する。
SAF故障リストは `信号名 sa0|sa1`、TDFは `信号名 STR|STF`。
無制限で完了したFDPは厳密値、上限到達で打ち切った結果は下界となる。

## 実験結果の保存

CSV・ログは `output/` のままGitで管理する。

```bash
git add output/
git commit -m "Record experiment results"
```

同じ設定を再実行すると同じ出力先を上書きする。複数の結果を別ファイルとして残す場合は、
実行前に `.set` の `-fdp`・`-log` で出力先を分ける。

## テストと詳細資料

```bash
ctest --test-dir build --output-on-failure
```

- [テストと検証](docs/testing.md)：基本テスト、既知FDPとの比較、詳細検証
- [LOC遷移故障](docs/transition_fault.md)：励起条件、観測点、対応回路、確率の定義
- [正常CNFの範囲限定](docs/normal_cnf_scope.md)
- [故障単位の並列化](docs/fault_parallel.md)
- [実装とブランチ構成](docs/architecture.md)
- [期待値の生成・比較条件](tests/expected/README.md)

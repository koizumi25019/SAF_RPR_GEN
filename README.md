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
  saf/<条件>/{fdp,log,cube}/  SAFの実行結果（cubeは任意）
  tdf/<条件>/{fdp,log,cube}/  TDFの実行結果（cubeは任意）
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
| `-cube <directory>` | 故障別テストキューブの出力ディレクトリ。省略時は保存しない |
| `-cube_fdp_max <P>` | キューブ保存対象のFDP上限（0〜1、上限値を含む）。省略時はFDP上限なし |
| `-cube_include_incomplete on\|off` | `complete=0`も保存対象に含めるか。既定off（`complete=1`のみ） |
| `-log <file>` | ログの出力先 |
| `-limit <N>` | 故障ごとのキューブ列挙上限。省略・0以下は無制限 |
| `-jobs <N>` | 並列数。1〜256、既定1 |
| `-progress on\|off` | 完了故障数の標準出力表示。既定on。直列・並列に対応 |

進捗は`Progress >> 完了故障数/全対象故障数`を同じ行に表示し、1故障の処理が終わるたびに更新する。
キューブ上限で打ち切った故障も処理済みとして数える（CSVの`complete`とは別）。
CLIと`.set`の両方で`-progress on|off`を指定でき、読み込み順で後の指定が有効になる。
進捗表示を止める例（`build/`から実行）：

```bash
./main_release -set ../input/script/saf/c17a.set -progress off
```

`.set`へ書く場合は次の行を追加する。

```text
-progress off
```

OFF時は進捗用の出力・フラッシュを省略する。CSV・キューブ保存・ログ・エラー表示には影響しない。

ディレクトリ名では故障モデルを自動判定しない。TDF設定には `-tdf` を明記する。
SAF故障リストは `信号名 sa0|sa1`、TDFは `信号名 STR|STF`。
無制限で完了したFDPは厳密値、上限到達で打ち切った結果は下界となる。

キューブの故障間流用は削除し、直列・並列とも各故障を独立に計算する。
旧`-dom_reuse`指定は削除すること。旧`MDC_NODOM`環境変数は参照しない。
CSVは`net_name,f_type,cube_cnt,complete,fdp`の5列とし、旧`seeded_cnt`列は削除した。

## テストキューブの保存

`.set`に保存先を追加すると、次の条件をすべて満たす故障について、1故障につき1つのテキストを保存する。

- 既定では列挙完了（`complete=1`）のみ。`-cube_include_incomplete on`なら`complete=0`も含む。
- キューブ数が指定した正の列挙上限以下。上限ちょうどの故障も含む。
- 冗長故障ではない（丸め前のFDPが正）。
- 計算対象の代表故障であり、等価故障として削除されていない。
- `-cube_fdp_max P`指定時は、丸め前のFDPが`P`以下。

例えば30キューブを上限とする実験では、次を指定する。

```text
-limit 30
-cube ../output/saf/limit30/cube/c17a
-cube_fdp_max 0.01
```

この例では従来の保存条件に加えて、`0 < FDP <= 0.01`の故障だけを保存する。
`-cube_fdp_max`はCLI/.setの両方に対応し、`1e-3`のような指数表記も使える。
0を指定すると保存対象は0件、1を指定するとFDP上限による追加除外はない。
FDP CSVの計算・出力対象は変わらない。

未完了故障の生成済みキューブも保存したい場合は、次を追加する。CLI/.setの両方に対応する。

```text
-cube_include_incomplete on
```

省略時または`off`では完了故障のみ保存する。未完了故障に`-cube_fdp_max`を適用する場合は、
生成済みキューブから計算したFDP（真の検出確率の下界）で比較する。

CLIでも`-cube <directory>`を指定できる。保存先は必要な親ディレクトリも含めて自動作成する。
相対パスは実行時の作業ディレクトリ基準。回路・モデル・実験条件ごとに専用ディレクトリを指定する。
`-limit`による保存先の自動変更は行わない。

保存ファイル名は`<信号線名>_<故障タイプ>_cube.txt`。例えば`a_sa0_cube.txt`や`G5_STR_cube.txt`となる。
信号名に含まれる`/`・`\\`・`%`などは`%HH`形式に符号化し、パスの区切りや別の名前との衝突を防ぐ。
各ファイルは次の形式で、その故障のキューブだけを持つ。

1. 1行目: 信号線名、タブ、故障種別（SAFは`sa0`/`sa1`、TDFは`STR`/`STF`）
2. 2行目: `complete`または`incomplete`
3. 3行目以降: 1行につき1本のテストキューブ（`0`・`1`・`X`）

例えば`a_sa0_cube.txt`の内容は以下となる（`a`と`sa0`の間はタブ）。

```text
a	sa0
complete
01X010X
1X0010X
```

個別ファイルにコメントヘッダ・キューブ数・終端行は付けない。最終行も改行で終わる。

同じ保存先への再実行では、予約名`*_sa0_cube.txt`・`*_sa1_cube.txt`・`*_STR_cube.txt`・`*_STF_cube.txt`の
既存ファイルを削除してから保存し、前回だけ条件に合った故障が混ざるのを防ぐ。
管理用ファイル`.fdp-cube-output`にマーカー・入力順序・保存条件をまとめて記録する。
これは故障別キューブではなく共通の説明書であり、`# pi`から各キューブのビットに対応する入力名を確認できる。
また、再実行時に生成済みの故障ファイルを入れ替えるディレクトリであることの識別にも使う。
有効なマーカーのない既存の非空ディレクトリは利用しない。
管理済みディレクトリ内でも、予約名以外のファイルは保持する。
保存対象が0件なら故障別テキストは作成せず、管理用ファイルだけを残す。
以前の`-cube <file>`設定は、ファイル名ではなく専用ディレクトリへ変更する。既存の集約ファイルは上書きしない。

管理ファイルの`# model`・`# limit`・`# inputs`に故障モデル、列挙上限、入力数を記録し、
`# pi`にキューブの左から右へ対応する入力名を記録する。TDFでは展開後のPIと初期状態の順序を使う。
旧マーカー付きのディレクトリも再利用でき、旧`fault_`付きファイルや`<信号名>__<故障種別>.txt`も再実行時に入れ替える。
例えば入力順が`a b c`なら、`1X0`はa=1、bは任意、c=0を表す。

保存には列挙済みの集合を使うため、追加のSAT列挙やBDD計算は発生しない。
対象故障について独立に生成したキューブの被覆を保存する。`complete=1`は完全な被覆、`complete=0`は部分的な被覆を表す。
共通情報は管理ファイルへ1回だけ記録する。直列でも並列でも、1故障のキューブをまとめて1回のファイル開閉で保存する。
並列時は保存対象だけを親へ送り、CSVと同じ故障順で親が各ファイルを書く。
`-limit`を省略・0以下にした場合は、無制限に列挙して完了した非冗長代表故障がすべて保存対象になる。
書き込み失敗時は非ゼロで終了する。中断・失敗した実験の出力は部分結果として扱う。

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

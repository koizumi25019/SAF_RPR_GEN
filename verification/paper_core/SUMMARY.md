# 論文 CORE による SAT ベースのドントケア判定

実装日: 2026-10-05。作業ブランチ: `feature/paper-core`。
土台: `verification` の `a8718dfe24ad546111a3e03838e06ef527ebd343`。

Fried, Nadel, Sebastiani, Shalmon, [Entailing Generalization Boosts Enumeration,
SAT 2024](https://doi.org/10.4230/LIPIcs.SAT.2024.13) の Algorithm 1 の CORE 分岐に従う。
アルゴリズムの実装であり、HALL 実行体の再現ではない。正側の生成ソルバも CaDiCaL を使う
（論文の既定 plain 側は IntelSAT）。SAT の解順序・core 選択・キューブ数は一致を保証しない。

## 実装

`src/fdp/paper_core.c` を追加し、`fault_detection_prob.c` から環境変数で切り替える。

1. 正側ソルバから全入力の完全モデルを得る。XID は呼ばない。
2. 別ソルバの `C_f ∧ ¬D_f` にモデルの入力リテラルを assumptions として渡す。
3. UNSAT 後に、全 `ccadical_failed(lit)` を次の assume/solve より前に読み取る。
4. core だけを再投入し UNSAT を確認する（論文に追加した整合性確認、1 solve/cube）。
5. core のリテラルを入力順に一つずつ削除し、UNSAT が維持できる削除を採用する。
6. 残らなかった入力を X にして、正側だけにキューブ禁止節を追加する。

結果は集合として極小の素項。ケアビット数の最小性・被覆キューブ数の最小性は保証しない。
否定側には D-chain、正常側の故障励起ユニット節、EssentialAssignment、キューブ禁止節を入れない。
故障側の縮退値とゲートの完全な等価関係は維持する。
SAF の `D_f` は比較出力 OR。TDF は比較出力 OR と1時刻目の励起条件の論理積。
比較器を生成する際の `cnf.total.vars` は復元し、正側の採番を変えない。
観測点ゼロの場合は `D_f=false` として処理する。

core 抽出後の再確認は core が全ビットを保持した場合も必ず solve する。
assumptions を投入して solve しない分岐を作らず、次のクエリへの持ち越しを避ける。
core 不整合や unknown はエラー終了し、該当キューブの禁止節を追加しない。
既存 `MAXDC_CORE` は変更していない。

## 使用方法

通常の運用は既存の `.set` ファイルに設定を書く。

```text
-dc_method core
-dom_reuse off
-core_verify off
```

XID のみなら `-dc_method xid` にする。`-dom_reuse off` は両方式の比較でキューブ流用を
停止する指定。流用を使う通常運用では `on` を指定する。`-core_verify on` は生成キューブの
健全性・極小性を追加クエリで再確認し、速度測定時は `off` にする。
`off` でも通常の core 抽出・core 再確認・削除による極小化は行う。
真偽値には `0` / `1` も使用できる。

設定例:

- `input/script/c17a_core.set`
- `input/script/c17a_xid.set`

既存と同じ「シェル→C 実行ファイル→.set」の流れで実行できる。

```bash
# リポジトリルート。方式別の出力ディレクトリを作成して両設定を実行。
bash run_paper_core_experiments.sh c17a_xid c17a_core
# 引数省略時も、この2設定を実行する。
```

`build/` 内で直接実行する場合は、先に `.set` に書いた出力ディレクトリを作る。

```bash
mkdir -p ../output/paper_core/full/core/{fdp,log}
./main_release -set ../input/script/c17a_core.set
```

優先順位は `.set` / CLI での明示指定 → 従来環境変数 → 既定値。
3設定を省略した既存 `.set` も互換動作する。
既定値は XID / 流用 on / core 追加検証 off。
従来の `PAPER_CORE`、`MDC_NODOM`、`PAPER_CORE_VERIFY` は省略時だけ参照し、存在判定を維持する。
設定ファイルに明示的な `xid` / `on` / `off` を書けば、残っている環境変数にも影響されない。
CLI は `-dc_method core -dom_reuse off -core_verify off` と同じ指定が可能。
既存の `-set` はその場で設定ファイルを読み込んで処理を返すため、後続 CLI 引数は使用しない。

実行ログには `Don't-care Method`、`Dominance Cube Reuse`、`CORE Extra Verification` を記録する。
これらの指定に不正値・値欠落があれば、C の実行ファイルは終了コード1で拒否する。

設定経路の検証:

```bash
python3 verification/paper_core/test_settings.py
```

c17a ゴールデン・Debug/Release 既定動作、環境変数互換、明示設定の優先、CLI、
空白/タブ、on/off/0/1、不正値・値欠落、s208_C/s298_C の両方式の FDP と独立 BDD を確認。
シェルの2設定実行も確認済み。設定追加後の既定 XID 回帰は、
`verification/gt_bdd` の通常9回路（s27/s208/s298/s344/s510/s641/s713/s1494/s5378）
すべて `ALL VERIFIED`。CORE 自体の処理は今回変更していない。

`MAXDC`、`XID_EXTERNAL`、`TDF_NOXID`、`DUAL`、`SPLIT` との併用はエラー。
低消費電力制約はまだ追加していない。追加時には正側の検出条件と否定側の反例条件を
`D_f ∧ P` / `¬D_f ∨ ¬P` に揃える必要がある。
計測ログの `Don't care` 時間には CORE のクエリ時間を含む。
stderr の `[PAPER_CORE]` は新規生成キューブ数、oracle solve 数、care 数の総計
（完全入力→core→極小化）を出す。流用キューブはこの集計に含めない。

## 検証

小規模4回路の core 抽出直後・極小化後キューブの X 全展開を、元 Verilog による独立2値
故障シミュレーションで確認した。577代表故障・2,544キューブ、極小化後の延べ62,002,456入力は
全て検出し、全キューブ和集合と全検出パターン集合も一致。詳細・再現は `SIMULATION.md`。
数値比較用 Excel `paper_core_comparison.xlsx` と入力スナップショット・再生成スクリプトも保存した。

```bash
# リポジトリのルートで実行。ゴールデンと既存実験出力を上書きしない。
python3 verification/paper_core/run_checks.py
```

結果は `output/paper_core_checks/` に分離する。
この環境には CMake が無かったため、CMakeLists.txt の SOURCES と同じソース群・
include/library パス・Debug/Release オプションを使い GCC で両バイナリを直接ビルドした。

- 3入力の全256論理関数に対する1,024検出ミンタームを検査。
  SAT 実装とは独立した真理値表の全補完で健全性と集合極小性を確認。
  同じ oracle を複数パターンで再利用し、空 core・入力ゼロ・非検出パターンの拒否も検査。
- c17a: Debug/Release、XID/CORE でゴールデン FDP 一致。
  CORE は支配流用を停止した場合も一致。
- 縮退故障と遷移故障の BDD 回帰結果は下表に記録する。
  無制限のケースは XID と CORE の全 FDP が一致。
  limit 付きでは各方式で被覆集合が違い得るため、部分 FDP の一致は要求しない。
  `GT_BDD` で全ての被覆の健全性と、complete=1 の厳密性を確認する。

無制限 SAF 9回路と TDF 2回路では CORE の再検査スイッチを有効にしている。
s5378_C の limit30 は再検査スイッチを外して GT_BDD による被覆検証を行う。
ここでの時間は速度比較用ではない。
例えば c17a の CORE は新規45キューブに対し care 総数が225→136→121。
core 抽出だけで終わらず、削除による極小化が実際に働いている。

| モデル | 回路 | 代表故障数 | 結果 |
|---|---|---:|---|
| SAF | c17a | 22 | ゴールデン一致、ALL VERIFIED |
| SAF | s27_C | 32 | XID/CORE 一致、ALL VERIFIED |
| SAF | s208_C | 215 | XID/CORE 一致、ALL VERIFIED |
| SAF | s298_C | 308 | XID/CORE 一致、ALL VERIFIED |
| SAF | s344_C | 342 | XID/CORE 一致、ALL VERIFIED |
| SAF | s510_C | 564 | XID/CORE 一致、ALL VERIFIED |
| SAF | s641_C | 463 | XID/CORE 一致、ALL VERIFIED |
| SAF | s713_C | 581 | XID/CORE 一致、ALL VERIFIED |
| SAF | s1494_C | 1,506 | XID/CORE 一致、ALL VERIFIED |
| SAF (limit30) | s5378_C | 4,551 | 両方式 ALL VERIFIED（健全性・完了故障の厳密性） |
| TDF | s27 | 48 | XID/CORE 一致、ALL VERIFIED |
| TDF | s208 | 346 | XID/CORE 一致、ALL VERIFIED |

s5378_C の CORE は新規70,764キューブ。core 再確認を含む1,079,979回の oracle solve を行い、
core 不整合はゼロ、独立 BDD 検証でも非健全・完了故障の過不足ともゼロ。


## 今回の生成キューブ数（参考）

両方式とも同じ生成側 CaDiCaL、無制限、支配流用あり。代表故障の CSV `cube_cnt` の合計
（流用されたキューブを含む）。CORE は XID を経由せず完全モデルから開始する。
いずれも FDP は一致。検証クエリを有効にしており、速度の優劣はこの実行から結論しない。

| 回路 | XID | 論文 CORE |
|---|---:|---:|
| c17a | 75 | 61 |
| s27_C | 107 | 107 |
| s208_C | 10,163 | 1,272 |
| s298_C | 1,578 | 1,114 |
| s344_C | 5,842 | 4,381 |
| s510_C | 6,578 | 3,217 |
| s641_C | 27,819 | 19,622 |
| s713_C | 32,804 | 24,411 |
| s1494_C | 9,788 | 7,727 |


## 検証処理を外した性能比較

2026-10-05、小規模 SAF 4回路を流用なし・Release・各7回で測定。
詳細と再現方法は [BENCHMARK.md](BENCHMARK.md)。s208_C の全体 CPU 時間の中央値は
約7%減ったが、s298_C では約1.58倍になった。キューブ削減だけでは速度向上を保証しない。


## 中規模 SAF の limit30 比較

s5378_C / s9234_C / s13207_C の `.set` と専用シェルを追加。
`bash run_paper_core_limit30.sh` で両方式を各1回実行し、日時別に保存・集計する。
条件・出力・結果は [LIMIT30.md](LIMIT30.md)。打ち切り故障の FDP は下界である。

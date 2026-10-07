# AGENTS.md

This file provides guidance to Codex and other coding agents when working with code in this repository.

このファイルは `CLAUDE.md` と同じプロジェクト知識を Codex 向けに保持する。仕様・実験結果・検証手順を
変更した場合は、`AGENTS.md` と `CLAUDE.md` の内容が食い違わないよう両方を更新すること。

## 概要

論理回路の縮退故障（SAF）に対する故障検出確率（FDP）を算出するツール。
各対象故障について、SAT でテストキューブを生成し、ドントケアを埋め、得られたキューブの和集合に対して
BDD で厳密な検出確率を計算する。出力は故障ごとの CSV。

## ビルドとセットアップ

初回は、同梱の SAT/BDD ソルバ（git submodule）をビルドする：

```bash
./setup.sh                       # submodule 初期化 + external/cadical と external/cudd を configure+make
mkdir -p build && cd build && cmake .. && make   # main_debug と main_release を生成
```

- `main_debug` — `-g -O0 -DDEBUG`、`main_release` — `-O3 -DNDEBUG`。どちらも `cadical cudd gmp m stdc++` をリンク。
- 外部依存はシステムインストールでは見つからない。インクルード/ライブラリのパスは `CMakeLists.txt` で
  `external/cadical`・`external/cudd` に直結している。リンクに失敗したら submodule 未ビルドが原因 → `setup.sh` を再実行。

## 実行方法

`.set` スクリプトファイルで駆動する。`.set` 内のパスは **`build/` からの相対パス**なので、`build/` で実行する：

```bash
cd build && ./main_debug -set ../input/script/c17a.set
```

`.set` のディレクティブ（`src/opt/opt.c` で解析）：`-net`（入力 `.v` ネットリスト）、`-fault`（故障リスト。
省略すると全代表故障 sa0/sa1 を自動生成）、`-fdp`（出力 CSV）、`-log`、`-cube_analysis`、
`-limit`（故障ごとのテストキューブ上限。**省略または `<=0` で無制限 = UNSAT まで完全列挙**）、
`-saf`/`-tdf`（故障モデル。省略時は縮退故障）、
`-dc_method xid|core`、`-dom_reuse on|off`、`-core_verify on|off`、
`-low_power on|off`、`-wsa_threshold 0..100`（整数百分率、on時に必須）。
`-dc_method` / `-dom_reuse` / `-core_verify` は `.set` 内の明示指定が環境変数より優先し、省略時は従来環境変数を参照する。
環境変数も無ければ XID・流用 on・core 検証 off。

### 遷移故障モード（`-tdf`）

LOC 方式（v1=自由、v2 は PI 共有＋DFF 引き継ぎ）の遷移遅延故障 FDP を計算する。実装は
`src/netlist/expand_tdf.c` が読込直後に **2時刻展開ネットリスト**（1時刻目＋2時刻目の組み合わせ回路）
を構築し、TDF (L, STR) を「2時刻目コピー L の sa0 ＋ 励起ユニット節 L_1t=0」（STF は sa1＋L_1t=1、
`detection_circuit.c` の `CreateConsDC_FE`）に帰着させる。以降のパイプラインは無修正で動く。
確率の分母は 2^(PI数+DFF数)。**観測点は2時刻目の PPO（FFへのキャプチャ）のみで、PO は
at-speed でストローブしない**（旧ツール NEW_RPR_FAULT/XID11 の意味論・設計に一致：
`expand_tdf.c` の `make_ppo_ppi()` が `ppi[]`/`ppo[]` を構築し `NLIST.ppo_flag` を PPO に立てる。
SAF では全端点=観測可として ppo_flag=1）。コーンが非観測の PO にしか届かない故障（例 s27 の
G17 系）は構造的にテスト不能＝fdp 0 になる。
制約：DFF 入りの順序回路 `.v` が必要（例 `input/circuit/s27.v`。組み合わせ回路はエラー）、
素の DFF のみ対応。`-fault` の形式は `<信号線名>\tSTR|STF`（例 `input/fault/s27_tdf.txt`）。
`-fault` 省略時は SAF 同様に全代表故障を自動生成する。TDF の等価故障は **BUF/INV のみ**
（入力線の遷移故障 ≡ 出力線の遷移故障。BUF は同極性、INV は STR↔STF 反転。AND/OR の
入力等価は TDF では成立しない。XID11 rep_flist.c 準拠）。DFF 置換 BUF は時刻境界なので
跨がない（`read.c` の `AnalyzeEquivalenceFaultsTDF`）。等価故障の CSV エコーも TDF 対応済み
（`gmp_wrapper.c`。s27 で独立列挙と一致確認済み）。
**外部由来の `.v`/故障リストは CRLF だとパーサが無限ループするので LF に変換してから置くこと。**
XID（ドントケア埋め）は TDF 対応済み：`InlineXID` の第4引数に励起ネット（`FNODE.exc_netptr`）を
渡すと、検出パスに加えて励起条件 L_1t=初期値 も正当化する（`XID.c` の `Xfilling`）。
`TDF_NOXID=1` で X 埋めを止めミンターム列挙に戻せる（検証用）。X率実測: s27=24%・s208=75%・
s5378=92%。支配流用・等価故障展開・MAXHAM/DUAL/PCOUNT は TDF では自動無効。
**MAXDC は TDF 対応済み**（オラクルの検出条件を z∧励起 にして構築。効果は控えめ＝TDF XID の
キューブは既にほぼ素項）。GT_BDD/BDD_EXACT も励起条件込みで TDF 対応済み
（s27/s208 全故障・s5378 150故障サンプルで ALL VERIFIED）。
PI 上の TDF は v1=v2 のため常に fdp=0（モデル通り）。CSV の `f_type` は `STR`/`STF`。
注意：TDF の完全列挙は中規模で**本質的爆発**に当たる（例 s5378 の n1312gat STF は
D_f が BDDノード722・disjointパス20億でキューブ列挙不能）。中規模は `-limit` 運用とし、
厳密値が要る場合は `BDD_EXACT=1`（TDF で動作確認済み、爆発故障も complete=1）。
SAF の PCOUNT に相当する TDF 版は未実装。

出力は **実行条件ごとにディレクトリを分ける**：`output/<条件>/{fdp,log,cube_analysis}/<回路>_<種別>.{csv,txt}`。
`<条件>` は `-limit` 値（`limit30`・`limit100` …）、`-limit` 省略時は `full`（完全列挙）。
ファイル名は `<回路>_<種別>`（`-net` のベース名 + 種別サフィックス `_fdp`/`_log`/`_cube_analysis`。
`_red`/`_test` など変種は `.set` 名を採用）。種別サフィックスは fdp/log/cube_analysis を
ファイル名だけで区別するため（エディタでタブが同名にならない）。出力名は `.set` の各ディレクティブの
パスに直書きされており、コードでの自動生成はしない。
条件をファイル名に埋め込まないので、`-limit` を変えたら出力先ディレクトリが自動で変わる。

### HALL 本家との比較実行

公式 HALL を `external/hall` に clone・build 済みなら、専用の `run_hall_experiments.sh` から
故障ごとの検出関数を AIGER に書き出して HALL を一括実行できる。通常の FDP 実験用
`run_experiments.sh` とは分離してある。例: `HALL_MODE=roc HALL_TIMEOUT=60 ./run_hall_experiments.sh c17a`。
モードは `tale/mars-dis/mars-nondis/duty/core/roc/carma`、`HALL_PRINT=1` でキューブも保存する。
結果は `output/hall/<mode>/<circuit>/<run-id>/{aig,log}/`。元の `.set` の出力先は上書きしない。
`AIG_DUMP` が励起条件を含めないため TDF は対象外で、スクリプトが明示的にスキップする。

## ASG の実現可能性調査（本体未統合）

`ASG/` の one-pass seed generation 論文・コードの読解と接続検証は
`verification/asg_feasibility/SUMMARY.md` を参照。固定 LFSR＋PS＋入力配置に対し
`H_f(s)=D_f(G(s))` のシード集合を数える。元の自由 PI の FDP とは分布が異なる。
4 ビットへ縮小した s27_C・2 種の PS で各34 stem SAF を独立全シード列挙と照合し、
SAT/XID/BDD と GT_BDD の全一致を確認。`probe.py` で再現できる。
ASG 本体の幅は100固定。非ゼロシード一様なら分母は `2^r−1`、分子もゼロを除外する。
複数連続パターンは検出集合の和集合を数え、独立試行の式を使わない。
大規模構成・TDF・MISR は未検証。現行 FDP/ASG 本体の動作は変更していない。

## 故障単位の並列化調査（本体未統合）

`verification/parallel_feasibility/SUMMARY.md` を参照。現行ソースの隔離Releaseビルドで、
s5378_C・SAF・limit30・全4,551代表故障を2回測定。キューブ流用なしの外部バッチ実行は
1/2/4/8並列で中央値53.061/26.291/13.486/8.264秒（8並列6.42倍）。流用あり直列53.311秒、
流用なし直列54.186秒で差は約1.6%。ユーザー方針も踏まえ、流用なしの独立故障配分を第一候補とする。
全並列数で代表行は流用なし直列と一致。c17ゴールデン＋並列GT、s5378並列全件GT ALL VERIFIED。
limit付きでは流用停止により部分被覆・completeが変わる（完了1,571→1,567）。TDF/fullの並列測定は未実施。
本体は未変更。スレッド化にはCNF・NLISTの可変フィールド・XIDのstatic作業領域等のworker別管理が必要。
競合例と方式比較は同ディレクトリの `DESIGN.md`。一故障の全処理をworker内で完結する常駐プロセスを
本体向け第一候補とする（未実装）。実験フックには無効時も共有配列を更新する箇所があり、スレッド化では監査が必要。

## 回帰テスト

ユニットテストの仕組みは無い。正しさは `expected/` のゴールデンファイルと CSV 出力を比較して検証する
（`expected/README.md` 参照）。**挙動が変わりうる変更をしたら、c17a を実行して `fdp` 列が
`expected/c17a_result.csv` と一致することを確認する**：

```bash
cd build && ./main_debug -set ../input/script/c17a.set
diff <(cut -d, -f1,2,5 expected/c17a_result.csv | sort) \
     <(cut -d, -f1,2,5 output/full/fdp/c17a_fdp.csv | sort)
```

比較するのは `net_name,f_type,fdp`（1,2,5列）**のみ**。`cube_cnt` 列はソルバの解順序やドントケア判定で
変動するため無視する。ゴールデンファイルは固定条件（`-fault` なし=全故障、`-limit` なし=無制限）で
生成され、全行 `complete=1` になる。`output/` は実行で上書きされるが、`expected/` は上書きされない。

s1494 の冗長故障の期待数は 12（`expected/s1494_C_red.txt`）。代表故障（`complete=1`）で数えると一致する。
`fdp==0` の行を単純に数えると等価故障の行が含まれるため 16 になる点に注意。

## アーキテクチャ

エントリポイント `src/main.c` → `read_nl()`（ネットリスト解析）→
`src/fdp/fault_detection_prob.c` の `AnalyzeFaultDensity()`。ここが本体。

`AnalyzeFaultDensity` のメインループ内、故障ごとのパイプライン：

1. **対象選択**（`src/fdp/target_fault.c`, `SetTarget`）で次の故障を選ぶ。故障は**支配関係**を持つ：
   ある故障の `subset_faults` は、そのテスト集合がこの故障のテスト集合の部分集合になる故障。
   既に求めた `subset_faults` のキューブを種＋禁止節として流用し、ソルバは差分 `T(f) \ ∪T(subset)`
   だけを探索する。`n_pending` がキューブの所有権を参照カウントし、最後の消費者が終わり次第キューブ集合を解放する。
   （環境変数 `MDC_NODOM` で流用を止めると完全列挙になる＝支配解析の検証用。）
2. **TPG モデル**（`src/fdp/create_TPG_model.c` + `src/fdp/cnf/`）：テスト生成用 CNF を構築する
   ── 正常回路、故障コーン、検出（PO 差分）節 ── を CaDiCaL に渡す。
3. **キューブ生成ループ**：CaDiCaL を solve する。SAT なら `InlineXID`（`src/fdp/xid/`）が
   外部入力のドントケアを埋め、キューブ文字列（PI ごとに `'0'/'1'/'X'`）を生成。それを禁止節として追加し
   `CubeSet` に push する。ループは UNSAT（完全）または `-limit` 到達（打ち切り）で終了。
4. **FDP 算出**：`RunBDD`（`src/fdp/cudd_wrapper.c`, CUDD）がキューブの和集合を BDD として構築し、
   GMP の有理数（`src/fdp/gmp_wrapper.c`）で厳密な確率を計算。CSV の1行を出力する。
5. **後処理**：`DropDeteFault`（`src/fdp/drop_dete_fault.c`）が今回の対象故障を処理済みにする。
   続く `FreeMemory` がターゲット情報を解放する。現行コードは別故障をキューブでシミュレーションして落とさない。

補助モジュール：`src/netlist/netlist.c`（回路グラフ：`nl[]`, `pi[]`, `n_net`, `n_pi`、ゲート種別 `AND/OR/INV/...`）、
`src/fdp/read.c`（故障リスト読込＋故障自動生成）、`src/lib/lib.c`（ファイル I/O 補助）、
`src/fdp/cube_set.c`（可変長キューブ配列）。

### ドントケア埋め（XID）

`src/fdp/xid/` は外部の「XID」ドントケア識別プロセスをインライン再実装したもの（`XID.c` の `InlineXID`）。
順方向/逆方向含意と故障シミュレーションで、あるテストにおいてどの PI がドントケアかを判定し、各キューブを広げる。
これは `cube_cnt` には影響するが `fdp` には影響しない。

## 検証・実験モジュール（環境変数で制御、デフォルト無効）

本体 `fault_detection_prob.c` はパイプラインのみ。検証・研究コードは別ファイルに分離されており、
**環境変数を設定しない限り無効**で本番出力は変わらない：

- **`src/fdp/gt_verify.c`** — 回帰検証ツール（恒久保守）。
  - `GT_BDD=1` — 独立グラウンドトゥルース検証：ネットリストから検出関数 D_f を BDD で直接構築し、
    キューブ和集合と厳密比較（sound=⊆ / exact==）。不一致故障を stderr に出力（独立シミュレーション
    値 `fdp_sim` も併記）し、終了時に `[GT] summary` を出す。**挙動が変わりうる変更をしたら
    c17a ゴールデン比較に加えて `verification/gt_bdd/*.set` を流し ALL VERIFIED を確認すること**
    （`verification/gt_bdd/SUMMARY.md` 参照）。
  - `GT_VERBOSE=1` — 一致した故障も全行出力。
  - `GT_CUBES=1` — 非健全キューブを特定し「どのXを1ビット固定すれば健全になるか」候補を列挙。
  - `GT_COVER=1` — 各故障で D_f の冗長度を測る：生成キューブ数に対し D_f の BDDノード数・
    1-パス数(=disjointカバーのサイズ)を `[GT_COVER]` で出す。`paths≪cubes` なら
    「ほぼ素項なのに冗長な near-duplicate カバーを量産」が爆発主因と確定（`verification/maxdc_qx/SUMMARY.md`）。
  - `GT_ISOP=1` — D_f の BDD から Minato-Morreale ISOP（primeかつirredundantなSOP）をZDDで生成し、
    現行キューブ数と比較する。DNF被覆自体の複雑さと、SAT列挙の素項選択・大域被覆の悪さを切り分ける
    診断オラクル。minimum SOPではなく変数順依存である点に注意。詳細は `verification/isop/SUMMARY.md`。
- **`src/fdp/cube_trend.c`** — キューブ生成傾向の観察ツール（本体は読むだけ）。
  - `CUBE_TREND=1` — 故障ごとにキューブ列の X 数・X マスク重複率・X位置集中度・連続キューブ差分を
    集計し stderr に `[CT]` 1行。終了時にキューブ数バケット別の `avg_X% / mask_reuse%` 集計を出す
    （「キューブ生成回数が多い故障ほど X が少ない/マスク使い回しか」の仮説検証）。
  - `CUBE_TREND_CSV=path` — 故障×キューブの明細を CSV 追記（`x_count` vs `idx` 等のプロット用）。
    生成順を純粋に見るときは `MDC_NODOM=1` 併用（種キューブが先頭に入らない）。詳細は
    `verification/cube_trend/SUMMARY.md`。
- **`src/fdp/paper_core.c`** — SAT 2024「Entailing Generalization Boosts Enumeration」の CORE 手順。
  - 通常の運用は `.set` の `-dc_method core` / `-dom_reuse off` / `-core_verify off`。
    XID 比較では `-dc_method xid`。設定例 `input/script/c17a_{core,xid}.set`、
    シェル実行は `bash run_paper_core_experiments.sh c17a_xid c17a_core`。
    出力は方式別ディレクトリに分離し、シェルが必要な出力ディレクトリを作成する。
    実行ログにも選択方式・流用・追加検証を記録する。
  - 設定の検証は `python3 verification/paper_core/test_settings.py`。
    省略・CLI・環境変数互換・明示指定の優先・不正値の拒否を確認する。
  - `PAPER_CORE=1` — XID を経由せず、SAT の完全入力モデルを非検出オラクルへの assumptions とし、
    UNSAT core 抽出→core 再確認→1リテラルずつ削除して極小素項にする。最小リテラル数は保証しない。
    SAF は ¬検出、TDF は ¬(検出∧励起)、低電力時は ¬(検出∧励起∧電力)。
    `-low_power on` は TDF/CORE のみ、`-wsa_threshold 0..100` が必須。
    正常回路の2時刻間の遷移数を元信号線数（分岐を含む）の指定割合以下にする。
    off が既定。FDP の分母は従来の全入力 2^n のまま。詳細は `verification/paper_core/POWER.md`。
    `src/fdp/power_constraint.c` は両極性で正確な加算器CNFを回路ごとに構築して再利用。
    s27/s208/小回路の11ケースを core直後・最終とも全X展開で検出・励起・電力を確認。
    `verify_tdf_power.py` / `test_power_settings.py` で独立全列挙・既定回帰・BDD_EXACTも検証。
    サンプル `.set`: `{s27,s208}_tdf_core{,_lp20}`。既存の `run_paper_core_experiments.sh` で実行。
    PI直結DFFのTDF等価解析が時刻境界を跨ぐ不具合も、解析・CSVエコー双方を修正。
    低電力ONのみのTDF実験は `bash run_tdf_power_benchmark.sh`。
    s27/s208は全代表故障・完全列挙・7回中央値、s5378/s9234は全代表故障・limit30・1回。
    電力閾値20%、流用off、追加検証off。1実行の実時間上限は1800秒（`--wall-limit 0`で解除）。
    タイムアウトは全件のX率・キューブ数を0にせず未取得とし、停止までのCPU時間と保存済み行を記録。
    `POWER_BENCHMARK.md` / `export_tdf_power.py` / `tdf_power_on_results.xlsx` を参照。
    グラフは作らず数値表を出力し、既存SAFの `paper_core_comparison.xlsx` は変更しない。
  - `PAPER_CORE_VERIFY=1` — 各生成キューブを再確認（非検出 UNSAT、残存各リテラル削除で SAT）。
  - `MAXDC`/`XID_EXTERNAL`/`TDF_NOXID`/`DUAL`/`SPLIT` との併用はエラー。
    手順単体の比較では `MDC_NODOM=1` で支配流用を止める。既定では流用を維持し、
    流用キューブは親故障について再極小化しない。未設定なら既存 XID の動作。
  - 再現: `python3 verification/paper_core/run_checks.py`。詳細は
    `verification/paper_core/SUMMARY.md`。core 不整合や unknown では禁止節を追加せず異常終了。
  - 中規模 SAF limit30 比較は `bash run_paper_core_limit30.sh`。s5378_C/s9234_C、
    XID/CORE、流用なし、追加検証なし、既定1回。`--repeats N` と回路引数に対応。
    `input/script/*_{xid,core}_l30.set` と `verification/paper_core/LIMIT30.md` を参照。
    s13207_C は既定対象外（明示指定用の設定は保存）。core 抽出直後の X 率・削除試行数も集計する。
    小規模4回路との合計6回路の比較・削除判定の対象割合は `verification/paper_core/COMPARISON.md`。
    中規模初回: CORE/XID CPU 2.76倍・3.89倍、完了故障1,567→2,654・3,053→3,721。
    core 直後 X率93.81%・93.21%、削除試行は全入力の6.19%・6.79%。両方式完了の FDP 不一致0。
    小規模4回路は独立2値シミュレーションで X を全展開し、core直後・最終とも全て検出。
    577代表故障・2,544キューブ、最終展開延べ62,002,456パターン。全検出集合・FDPも一致。
    `SIMULATION.md` / `simulate_expanded.py` と結果JSONに保存。
    `verification/paper_core/paper_core_comparison.xlsx` はグラフ用数値表10シート。
    `export_excel.py` はGit保存のJSONから再生成する（openpyxl）。
    未完了故障の FDP は下界として比較し、両方式完了の FDP 不一致と故障欠落はエラー。
  - 性能比較: `python3 verification/paper_core/benchmark.py`。小規模 SAF 4回路、流用なし、
    検証処理なし、各7回の結果は `verification/paper_core/BENCHMARK.md`。
    s208_C はキューブ約8分の1・CPU中央値約7%短縮（測定範囲は重なる）、s298_C は約1.58倍遅い。
- **`src/fdp/experiment.c`** — 研究用フック。
  - `MAXDC`（案1）— 非検出オラクル CNF で各キューブを素項へ拡大＋伸び代計測。**爆発故障の決定打**
    （XID は局所DCしか見ず、グローバルには1本で済む空間を52万本に刻む＝冗長カバー。素項展開が
    広域禁止節で潰す。b12 最重故障 521,746→1〜2本）。縮約法を選ぶ：
    - `MAXDC_CORE` — UNSATコア一括(1 solve/cube)。最速だが ~34% で revert（縮約破棄）。
    - `MAXDC_QX` — QuickXplain による真の極小素項。revert ゼロ・常に健全。複雑回路では割高。
      `MAXDC_QX_MULTI=N` を併用すると、異なるリテラル順でN個の極小素項を生成し、最短のものを採用する
      （HALL/MARSのshort implicant着想を使うmulti-start近似。厳密な最小リテラル素項ではない）。
    - `MAXDC_CORE MAXDC_HYB` — core を試し、revert する分だけ QX で救済する中間案。
    - `MAXDC_NOMUT`=計測のみ。評価は `verification/maxdc_qx/SUMMARY.md`。
  - `MAXHAM`（案2・却下済み）— 最大ハミング距離制約による解の多様化（`MAXHAM_K`=目標距離）。
    評価と却下理由は `verification/SUMMARY.md`。
  - `DUAL`（案3）— 双対列挙: 非検出空間 ¬D_f のキューブも並行列挙し、U∪V の閉包または
    ¬D_f の列挙完了（残り D_f\U を BDD パスで補充）で det 側の UNSAT を待たずに complete=1 で
    終了する。固有価値は (1) limit 付き実行での complete 救済（s5378_l30 で +146 故障）、
    (2) 打ち切り故障への fdp の anytime 上下界（stderr `[DUAL] capped ... bounds=[lo,hi]`）。
    `DUAL_START`=V側の起動閾値（det キューブ本数、既定16）。det 打ち切り後に V 側だけ回す
    ドレインは `DUAL_VLIMIT`（V本数上限、既定=-limit）と `DUAL_REMCAP`（remainder パス上限）で制御
    （s5378_l30: VLIMIT=300 で incomplete 2980→1713、ただし ~20分）。評価は `verification/dual/SUMMARY.md`。
  - `PCOUNT`（案5・**incomplete 根絶の本命**）— `src/fdp/pcount.c`。打ち切り故障の fdp を
    PODEM型入力空間探索で厳密数え上げして complete=1 で報告（SATソルバ・BDD合成・
    外部カウンタ不使用）。文献準拠: 双対 early termination（Möhle&Biere ICTAI'18 DUALIZA）＋
    separator レベル別キャッシュ（Huang&Darwiche SAT'04）＋Zobrist差分ハッシュ＋
    イベント駆動差分シミュレーション。s5378_l30 を 302s で完走し incomplete 2980→303
    （最難故障 det_paths=620億 は 0.18s）。`PCOUNT_MAXNODES`（既定200万）/`PCOUNT_CACHE`。
    残課題はカット幅最小化の変数順（MINCE系）。評価は `verification/pcount/SUMMARY.md`。
  - `SPLIT`（案4・却下済み）— 打ち切り故障の Shannon 分割による完全化（`SPLIT_BUDGET`/
    `SPLIT_MAXNODES`）。機構は正しい（c17a limit2 で GT exact）が、本質的複雑 D_f では
    caching なし分割が指数発散しコスト対効果が成立しない。**incomplete 根絶の到達限界の
    分析込みで** `verification/split/SUMMARY.md` を参照。
- **`BDD_EXACT=1`**（本体 `fault_detection_prob.c` + `gt_verify.c` の `GT_ExactCountStr`）—
  打ち切り（limit到達）故障だけ検出関数 D_f を回路から直接 BDD 構築して厳密 fdp を計算し
  complete=1 で報告する（incomplete の根絶）。出力の意味論は「complete=1 ⇔ fdp が厳密」になる。
  中規模（s5378/b12/s13207）まで実証済み・s5378 で +24% コスト。評価は `verification/bdd_exact/SUMMARY.md`。
- 本体・他モジュール内の切り分けスイッチ：
  - `MDC_NODOM=1` — 支配流用を止めゼロから完全列挙（支配解析の検証用、メインループ）。
  - `MDC_NOEA=1` — `EssentialAssignment` を無効化（過小評価の切り分け用、`cnf/faulty_circuit.c`）。

`experiment/*` ブランチがこれらを持ち、`master` がベースライン。各実験コミットが何を確認したかは
`git log` を参照。過去にあった `MDC_MC`/`FDPSIM_LIST`/`CUBE_DUMP`/`XID_EXTERNAL`/`XID_DBG`/`XID_PO`
は GT_BDD で代替できるため削除済み（必要なら git 履歴から復元）。

## 正常CNFの範囲限定（検証用の通常パイプライン）

`FDP_NORMAL_SCOPE=1` で、検出ソルバへ投入する正常ゲート定義を、故障・検出・励起・
EA・D-chain・低電力の全制約が参照する正常信号のTFI閉包へ限定する。既定はoff。
サイド入力の生成回路もPIまで残す。`n_pi`・PI順・FDPの分母は変更しない。
XID用の正常値はPIから論理シミュレーションで復元し、範囲外PIは0補完後に明示的にXにする。
CORE/MAXDCなどの非検出オラクルは完全な正常定義と電力定義を維持する。
`FDP_NORMAL_SCOPE_VALIDATE=1` は、保持した正常信号のSAT値と復元値を照合する。
検証・再現手順は `verification/normal_scope/SUMMARY.md`、回帰は同ディレクトリの `check.py`。
全検証で `-dom_reuse off` を使用する。打ち切りFDPは下界であり、範囲限定は全故障の完了を保証しない。
範囲限定したDIMACSは全PIへの射影カウント専用（`c p show`）。非射影#SATをFDPに使わない。

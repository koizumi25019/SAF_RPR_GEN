# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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
`-limit`（故障ごとのテストキューブ上限。**省略または `<=0` で無制限 = UNSAT まで完全列挙**）。

出力は **実行条件ごとにディレクトリを分ける**：`output/<条件>/{fdp,log,cube_analysis}/<回路>.{csv,txt}`。
`<条件>` は `-limit` 値（`limit30`・`limit100` …）、`-limit` 省略時は `full`（完全列挙）。
ファイル名は回路名のみ（`-net` のベース名。`_red`/`_test` など変種は `.set` 名を採用）。
条件をファイル名に埋め込まないので、`-limit` を変えたら出力先ディレクトリが自動で変わる。

## 回帰テスト

ユニットテストの仕組みは無い。正しさは `expected/` のゴールデンファイルと CSV 出力を比較して検証する
（`expected/README.md` 参照）。**挙動が変わりうる変更をしたら、c17a を実行して `fdp` 列が
`expected/c17a_result.csv` と一致することを確認する**：

```bash
cd build && ./main_debug -set ../input/script/c17a.set
diff <(cut -d, -f1,2,5 expected/c17a_result.csv | sort) \
     <(cut -d, -f1,2,5 output/full/fdp/c17a.csv | sort)
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
5. **後処理**：`DropDeteFault`（`src/fdp/drop_dete_fault.c`）が今回のキューブで新たに検出された故障を落とし、
   メモリを解放する。

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
- **`src/fdp/cube_trend.c`** — キューブ生成傾向の観察ツール（本体は読むだけ）。
  - `CUBE_TREND=1` — 故障ごとにキューブ列の X 数・X マスク重複率・X位置集中度・連続キューブ差分を
    集計し stderr に `[CT]` 1行。終了時にキューブ数バケット別の `avg_X% / mask_reuse%` 集計を出す
    （「キューブ生成回数が多い故障ほど X が少ない/マスク使い回しか」の仮説検証）。
  - `CUBE_TREND_CSV=path` — 故障×キューブの明細を CSV 追記（`x_count` vs `idx` 等のプロット用）。
    生成順を純粋に見るときは `MDC_NODOM=1` 併用（種キューブが先頭に入らない）。詳細は
    `verification/cube_trend/SUMMARY.md`。
- **`src/fdp/experiment.c`** — 研究用フック。
  - `MAXDC`（案1）— 非検出オラクル CNF で各キューブを素項へ拡大＋伸び代計測。**爆発故障の決定打**
    （XID は局所DCしか見ず、グローバルには1本で済む空間を52万本に刻む＝冗長カバー。素項展開が
    広域禁止節で潰す。b12 最重故障 521,746→1〜2本）。縮約法を選ぶ：
    - `MAXDC_CORE` — UNSATコア一括(1 solve/cube)。最速だが ~34% で revert（縮約破棄）。
    - `MAXDC_QX` — QuickXplain による真の極小素項。revert ゼロ・常に健全。複雑回路では割高。
    - `MAXDC_CORE MAXDC_HYB` — core を試し、revert する分だけ QX で救済する中間案。
    - `MAXDC_NOMUT`=計測のみ。評価は `verification/maxdc_qx/SUMMARY.md`。
  - `MAXHAM`（案2・却下済み）— 最大ハミング距離制約による解の多様化（`MAXHAM_K`=目標距離）。
    評価と却下理由は `verification/SUMMARY.md`。
- 本体・他モジュール内の切り分けスイッチ：
  - `MDC_NODOM=1` — 支配流用を止めゼロから完全列挙（支配解析の検証用、メインループ）。
  - `MDC_NOEA=1` — `EssentialAssignment` を無効化（過小評価の切り分け用、`cnf/faulty_circuit.c`）。

`experiment/*` ブランチがこれらを持ち、`master` がベースライン。各実験コミットが何を確認したかは
`git log` を参照。過去にあった `MDC_MC`/`FDPSIM_LIST`/`CUBE_DUMP`/`XID_EXTERNAL`/`XID_DBG`/`XID_PO`
は GT_BDD で代替できるため削除済み（必要なら git 履歴から復元）。

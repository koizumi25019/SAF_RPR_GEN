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
cd build && ./main_debug -set ../data/script/c17a.set
```

`.set` のディレクティブ（`src/opt/opt.c` で解析）：`-net`（入力 `.v` ネットリスト）、`-fault`（故障リスト。
省略すると全代表故障 sa0/sa1 を自動生成）、`-fdp`（出力 CSV）、`-log`、`-cube_analysis`、
`-limit`（故障ごとのテストキューブ上限。**省略または `<=0` で無制限 = UNSAT まで完全列挙**）。

`NO_DISCORD=1` を設定すると、`main.c` が完了時に送る Discord webhook を抑止できる。検証実行では必ず付けること。

## 回帰テスト

ユニットテストの仕組みは無い。正しさは `expected/` のゴールデンファイルと CSV 出力を比較して検証する
（`expected/README.md` 参照）。**挙動が変わりうる変更をしたら、c17a を実行して `fdp` 列が
`expected/c17a_result.csv` と一致することを確認する**：

```bash
cd build && NO_DISCORD=1 ./main_debug -set ../data/script/c17a.set
diff <(cut -d, -f1,2,5 expected/c17a_result.csv | sort) \
     <(cut -d, -f1,2,5 output/fdp/c17a_result.csv | sort)
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
3. **キューブ生成ループ**：`MaxHamSolve` が CaDiCaL を呼ぶ。SAT なら `InlineXID`（`src/fdp/xid/`）が
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

## 実験ブランチのフック（環境変数で制御、デフォルト無効）

`fault_detection_prob.c` には研究用の計測コードが入っているが、**環境変数を設定しない限り無効**で、
本番出力は変わらない。生きたコードと混同しないよう、存在を把握しておくこと：

- `MAXDC` / `MAXDC_MEASURE` — 「非検出オラクル」CNF を構築し、各キューブを貪欲に素項へ拡大して
  ドントケアの伸び代を測定する（`MDC_BuildOracle`, `MDC_Expand`, `mdc_dump`）。
- `MDC_MC=<故障名>` — モンテカルロ突き合わせ：SAT を使わない論理シミュレーションの真値（`FdpBySim`）と、
  検出ソルバの FDP、非検出オラクルの `1-非検出率` の3値を比較し、CNF モデル/オラクルの健全性を検証する。
- `CUBE_DUMP=<故障名>` — 1故障分の生成キューブ列を stderr にダンプする。
- `GT_BDD=1` — 独立グラウンドトゥルース検証：ネットリストから検出関数 D_f を BDD で直接構築し、
  キューブ和集合と厳密比較（sound=⊆ / exact==）。不一致故障を stderr に出力し、終了時に
  `[GT] summary` を出す（`verification/gt_bdd/SUMMARY.md` 参照）。`GT_VERBOSE=1` で全故障出力。
- `MDC_NOEA=1` — `EssentialAssignment` を無効化（過小評価バグの切り分け用）。

`experiment/*` ブランチがこれらを持ち、`master` がベースライン。各実験コミットが何を確認したかは
`git log` を参照。

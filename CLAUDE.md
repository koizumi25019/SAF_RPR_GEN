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
省略すると全代表故障 sa0/sa1 を自動生成）、`-fdp`（出力 CSV）、`-log`、
`-limit`（故障ごとのテストキューブ上限。**省略または `<=0` で無制限 = UNSAT まで完全列挙**）。

出力は **実行条件ごとにディレクトリを分ける**：`output/<条件>/{fdp,log}/<回路>.{csv,txt}`。
`<条件>` は `-limit` 値（`limit30`・`limit100` …）、`-limit` 省略時は `full`（完全列挙）。
ファイル名は回路名のみ（`-net` のベース名。`_red`/`_test` など変種は `.set` 名を採用）。
条件をファイル名に埋め込まないので、`-limit` を変えたら出力先ディレクトリが自動で変わる。

## 回帰テスト

`ctest --test-dir build --output-on-failure` で対象故障の選択順、BDD→GMP の数え上げ・確率変換と CSV 出力のテストを実行する。
対象選択は従来の全走査と照合し、同レベルの順序、処理済みのスキップ、空集合、再初期化を確認する。
空集合、重複、独立全列挙、32ビット桁境界、大きな数え上げ、等価故障と完了フラグを確認する。
パイプライン全体の正しさは `expected/` のゴールデンファイルと CSV 出力を比較して検証する
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

## アーキテクチャ

エントリポイント `src/main.c` → `read_nl()`（ネットリスト解析）→
`src/fdp/fault_detection_prob.c` の `AnalyzeFaultDensity()`。ここが本体。

`AnalyzeFaultDensity` のメインループ内、故障ごとのパイプライン：

1. **対象選択**（`src/fdp/target_fault.c`, `SetTarget`）で次の単一故障を選び、故障ハッシュ表内の `FNODE*` を直接返す。
   故障読み込み後に `InitTargetOrder` でレベルと元の走査順位により一度だけ整列し、
   `SetTarget` は配列を前へ進めて処理済みを飛ばす。同レベルも含め従来の選択順を維持する。
   選択処理の合計は O(F²) から O(F log F + F) になり、終了時は `FreeTargetOrder` で配列を解放する。
   TPG モデル構築・BDD 出力・検出状態更新も同じポインタを使い、対象用のリスト確保・解放は不要。故障は**支配関係**を持つ：
   ある故障の `subset_faults` は、そのテスト集合がこの故障のテスト集合の部分集合になる故障。
   既に求めた `subset_faults` のキューブを種＋禁止節として流用し、ソルバは差分 `T(f) \ ∪T(subset)`
   だけを探索する。`n_pending` がキューブの所有権を参照カウントし、最後の消費者が終わり次第キューブ集合を解放する。
   （環境変数 `MDC_NODOM` で流用を止めると完全列挙になる＝支配解析の検証用。）
2. **TPG モデル**（`src/fdp/create_TPG_model.c` + `src/fdp/cnf/`）：テスト生成用 CNF を構築する
   ── 正常回路、故障コーン、検出（PO 差分）節 ── を CaDiCaL に渡す。
3. **キューブ生成ループ**：CaDiCaL を solve する。SAT なら `InlineXID`（`src/fdp/xid/`）が
   外部入力のドントケアを埋め、キューブ文字列（PI ごとに `'0'/'1'/'X'`）を生成。それを禁止節として追加し
   `CubeSet` に push する。ループは UNSAT（完全）または `-limit` 到達（打ち切り）で終了。
   UNKNOWN（CaDiCaL の戻り値 0）ではモデル参照・禁止節追加・対象故障の結果出力をせず、
   エラー表示と資源解放後に終了コード 1 で異常終了する。これは `verification` から派生した
   `feature/paper-core` の `src/fdp/paper_core.c` と同じ異常終了方針で、完了済み故障の CSV 行は保持する。
4. **FDP 算出・出力**：`RunBDD(gbm, n_pi, &cubes, density)`（`src/fdp/cudd_wrapper.c`）が
   キューブ和集合の BDD を構築し、CUDD の多倍長整数を `mpz_import` で GMP に直接取り込む。
   2^n_pi で割った確率を 8192 ビット精度の `mpf_t` に返す。一時ファイル・10進整数文字列は使わない。
   CSV は `WriteFaultResult(fp, &result)`（`src/fdp/fault_result.c`）に分離し、`FaultResult` に
   対象・キューブ数・完了状態・確率をまとめる。確率の文字列化は故障ごとに1回だけ行い、等価故障にも流用する。
   CSV の列・桁数・等価故障の出力順は維持する。
5. **後処理**：`DropDeteFault`（`src/fdp/drop_dete_fault.c`）が今回のキューブで新たに検出された故障を落とし、
   メモリを解放する。

補助モジュール：`src/netlist/netlist.c`（回路グラフ：`nl[]`, `pi[]`, `n_net`, `n_pi`、ゲート種別 `AND/OR/INV/...`）、
`src/fdp/read.c`（故障リスト読込＋故障自動生成）、`src/lib/lib.c`（ファイル I/O 補助）、
`src/fdp/cube_set.c`（可変長キューブ配列）。

### ドントケア埋め（XID）

`src/fdp/xid/` は外部の「XID」ドントケア識別プロセスをインライン再実装したもの（`XID.c` の `InlineXID`）。
順方向/逆方向含意と故障シミュレーションで、あるテストにおいてどの PI がドントケアかを判定し、各キューブを広げる。
これは `cube_cnt` には影響するが `fdp` には影響しない。

## ブランチ構成（baseline / verification）

- **`baseline`（このブランチ）** — 本番パイプラインのみ。`fault_detection_prob.c` は SAT→XID→BDD の
  本体処理だけを持ち、検証・研究用のコード（`gt_verify.c`/`cube_trend.c`/`experiment.c` や
  `verification/` ディレクトリ）は含まない。
- **`verification`** — 上記に加えて検証・研究系を全部入りで保持するブランチ。GT_BDD 厳密検証器、
  キューブ傾向観察(CUBE_TREND)、研究フック(MAXDC/MAXHAM/DIVPO 等)、外部XID比較(XID_EXTERNAL)、
  X率計測(XSTAT)、および `verification/` 配下の実験記録・SUMMARY 群はこちらにある。
  **検証・厳密照合をしたいときは `verification` を使う。**

baseline に残る切り分けスイッチ（環境変数、既定無効）：

- `MDC_NODOM=1` — 支配流用を止めゼロから完全列挙（支配解析の検証用、メインループ）。
- `MDC_NOEA=1` — `EssentialAssignment` を無効化（過小評価の切り分け用、`cnf/faulty_circuit.c`）。

## 正常CNFの範囲限定（既定有効）

正常CNFは故障TFO上の信号とEAで単位節を投入した信号の全TFI閉包だけをソルバへ投入する。
サイド入力の生成回路もPIまで残す。故障モデルの `ccadical_add()` は直接呼び出しのまま。
XIDの正常値は必要PIから論理シミュレーションで復元し、範囲外PIは0補完後に明示的にXにする。
全PI・`n_pi`・FDP分母・CSV形式は維持する。
`FDP_NORMAL_SCOPE=0` で従来処理へ戻す。`FDP_NORMAL_SCOPE_VALIDATE=1` は保持範囲の値照合。
ログに `Normal CNF Scope` を記録する。詳細と計測結果は `docs/normal_cnf_scope.md`。
CMakeのDebug/Release両方へ `normal_scope.c` / `normal_scope_values.c` を登録済み。

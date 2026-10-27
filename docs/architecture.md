# 実装とブランチ構成

## ソースの配置

`src/main.c` は起動と全体制御、`cli/` は設定、`netlist/` は回路、`fault/` は故障の管理を担う。
`fdp/analysis.c` が処理を進め、`fdp/context.c` が解析用状態の初期化、`fdp/worker_pool.c` が並列実行を担う。
`fdp/cnf/`・`fdp/xid/`・`fdp/bdd/` が各計算段階、`io/` がCSV・ログ出力、`common/` が共通処理を持つ。
SAFとTDFは共通のパイプラインを使い、ソースをモデル別に二重化しない。
プロジェクト内のincludeは `src/` からのパスで統一する。

## アーキテクチャ

エントリポイント `src/main.c` → `read_nl()`（ネットリスト解析）→
`src/fdp/analysis.c` の `AnalyzeFaultDensity()`。ここが本体。

`AnalyzeFaultDensity` のメインループ内、故障ごとのパイプライン：

1. **対象選択**（`src/fault/target_fault.c`, `SetTarget`）で次の単一故障を選び、故障ハッシュ表内の `FNODE*` を直接返す。
   故障読み込み後に `InitTargetOrder` でレベルと元の走査順位により一度だけ整列し、
   `SetTarget` は配列を前へ進めて処理済みを飛ばす。同レベルも含め従来の選択順を維持する。
   選択処理の合計は O(F²) から O(F log F + F) になり、終了時は `FreeTargetOrder` で配列を解放する。
   TPG モデル構築・BDD 出力・検出状態更新も同じポインタを使い、対象用のリスト確保・解放は不要。故障は**支配関係**を持つ：
   ある故障の `subset_faults` は、そのテスト集合がこの故障のテスト集合の部分集合になる故障。
   既に求めた `subset_faults` のキューブを種＋禁止節として流用し、ソルバは差分 `T(f) \ ∪T(subset)`
   だけを探索する。`n_pending` がキューブの所有権を参照カウントし、最後の消費者が終わり次第キューブ集合を解放する。
   （環境変数 `MDC_NODOM` で流用を止めると完全列挙になる＝支配解析の検証用。）
2. **TPG モデル**（`src/fdp/cnf/tpg_model.c` + `src/fdp/cnf/`）：テスト生成用 CNF を構築する
   ── 正常回路、故障コーン、検出（PO 差分）節 ── を CaDiCaL に渡す。
3. **キューブ生成ループ**：CaDiCaL を solve する。SAT なら `InlineXID`（`src/fdp/xid/`）が
   外部入力のドントケアを埋め、キューブ文字列（PI ごとに `'0'/'1'/'X'`）を生成。それを禁止節として追加し
   `CubeSet` に push する。ループは UNSAT（完全）または `-limit` 到達（打ち切り）で終了。
   UNKNOWN（CaDiCaL の戻り値 0）ではモデル参照・禁止節追加・対象故障の結果出力をせず、
   エラー表示と資源解放後に終了コード 1 で異常終了する。これは `verification` から派生した
   `feature/paper-core` の `src/fdp/paper_core.c` と同じ異常終了方針で、完了済み故障の CSV 行は保持する。
4. **FDP 算出・出力**：`RunBDD(gbm, n_pi, &cubes, density)`（`src/fdp/bdd/probability.c`）が
   キューブ和集合の BDD を構築し、CUDD の多倍長整数を `mpz_import` で GMP に直接取り込む。
   2^n_pi で割った確率を 8192 ビット精度の `mpf_t` に返す。一時ファイル・10進整数文字列は使わない。
   CSV は `WriteFaultResult(fp, &result)`（`src/io/fault_csv.c`）に分離し、`FaultResult` に
   対象・キューブ数・完了状態・確率をまとめる。確率の文字列化は故障ごとに1回だけ行い、等価故障にも流用する。
   CSV の列・桁数・等価故障の出力順は維持する。
5. **後処理**：`DropDeteFault`（`src/fault/drop_detected.c`）が今回のキューブで新たに検出された故障を落とし、
   メモリを解放する。

補助モジュール：`src/netlist/netlist.c`（回路グラフ：`nl[]`, `pi[]`, `n_net`, `n_pi`、ゲート種別 `AND/OR/INV/...`）、
`src/fault/fault_list.c`（故障リスト読込＋故障自動生成）、`src/common/memory.c`（メモリ確保）、`src/io/file.c`（ファイル操作）、`src/netlist/traversal.c`（スタック・キュー）、
`src/fdp/cube_set.c`（可変長キューブ配列）。

### ドントケア埋め（XID）

`src/fdp/xid/` は外部の「XID」ドントケア識別プロセスをインライン再実装したもの（`XID.c` の `InlineXID`）。
順方向/逆方向含意と故障シミュレーションで、あるテストにおいてどの PI がドントケアかを判定し、各キューブを広げる。
これは `cube_cnt` には影響するが `fdp` には影響しない。

## ブランチ構成（baseline / verification）

- **`baseline`（このブランチ）** — 本番パイプラインのみ。`analysis.c` は SAT→XID→BDD の
  本体処理だけを持ち、検証・研究用のコード（`gt_verify.c`/`cube_trend.c`/`experiment.c` や
  `verification/` ディレクトリ）は含まない。
- **`verification`** — 上記に加えて検証・研究系を全部入りで保持するブランチ。GT_BDD 厳密検証器、
  キューブ傾向観察(CUBE_TREND)、研究フック(MAXDC/MAXHAM/DIVPO 等)、外部XID比較(XID_EXTERNAL)、
  X率計測(XSTAT)、および `verification/` 配下の実験記録・SUMMARY 群はこちらにある。
  **検証・厳密照合をしたいときは `verification` を使う。**

baseline に残る切り分けスイッチ（環境変数、既定無効）：

- `MDC_NODOM=1` — 支配流用を止めゼロから完全列挙（支配解析の検証用、メインループ）。
EAは常時ON。`-ea`の切替は持たず、旧`MDC_NOEA`は参照しない。

## 正常CNFの範囲限定（常時有効）

正常CNFは、故障TFO内の観測点（SAFはPO、TDFは2時刻目PPO）の全TFIをソルバへ投入する。
TDFでは1時刻目の励起に必要なTFIも保持する。
EAより前に範囲を確定し、EAの単位節はその範囲内の信号にだけ追加する。EAの推論自体は全回路で行う。
参考: Eggersglüß et al., DDECS 2012, Section II-B / Fig. 2。XIDの検出先も対象POにそろえる。
サイド入力の生成回路もPIまで残す。故障モデルの `ccadical_add()` は直接呼び出しのまま。
XIDの正常値は必要PIから論理シミュレーションで復元し、範囲外PIは0補完後に明示的にXにする。
全PI・`n_pi`・FDP分母・CSV形式は維持する。
正常CNFを限定しないモードは持たない。`FDP_NORMAL_SCOPE`は参照しない。
`FDP_NORMAL_SCOPE_VALIDATE=1` は保持範囲の値照合。
ログに`Normal CNF Scope: on (affected-output TFI)`と`Essential Assignment: on`を記録する。
詳細は[正常CNFの範囲限定](normal_cnf_scope.md)、検証結果は`docs/output_tfi_scope_results.json`。
CMakeのDebug/Release両方へ `normal_scope.c` / `normal_scope_values.c` を登録済み。

## 故障単位の常駐プロセス並列化

`.set`またはCLIの`-jobs N`で、故障を常駐プロセスへ動的配分する（Linux/WSL2）。
回路・正常CNF・既存の故障順を親で一度準備し、各子がCNF→SAT→XID→BDD・FDPを担当する。
既存`InitTargetOrder()`の整列結果を`CopyTargetOrder()`で複製して配分する。
`AnalyzeOneFault()`は直列・並列で共通。BDD計算とCSV出力の分離、直接の`ccadical_add()`を維持する。
並列では親だけが実ファイルを書き、故障の完了順によらず既存のCSV順を保つ。
任意指定の`-cube <directory>`は`io/cube_text.c`で保存条件を満たす計算対象故障だけをテキスト化する。
既定では完了・FDP>0のみ。`-cube_include_incomplete on`なら未完了も含める。
個別ファイルは「信号名＋タブ＋故障種別」「complete/incomplete」「キューブ列」の順。
キューブ解放前の集合を使い、等価故障への展開は行わない。並列ではCSVとともに保存対象の文字列だけを親へ送り、
同じ故障順で出力する。保存を省略した場合はキューブ用のメモリストリームを作らない。
`CubeOutput`は親の出力先と管理ファイル用の共通情報を持ち、`WriteCubeFile()`が1故障につき1ファイルを保存する。
入力順序と実験条件は`.fdp-cube-output`だけに記録する。
直列でも1故障のテキストをメモリストリームに作り、保存後すぐ解放する。
信号名はファイル名用に符号化する。管理マーカー付きのディレクトリのみ再利用し、前回の予約名の故障ファイルを消す。
流用設定が省略された場合、jobs>1ならoff、jobs=1なら従来の`MDC_NODOM`に従う。
明示的な流用onとjobs>1の組合せはエラー。正常CNFの限定化は常時有効。

ログの`Time`は実経過秒、`CPU Time`は親＋全子のCPU秒を合計する。
ワーカー異常終了・親の中断・出力失敗は非ゼロ終了し、子をすべて回収する。
詳しいレビュー案内と使い方は[故障単位の並列化](fault_parallel.md)、検証結果は`docs/fault_parallel_results.json`。
既存CTestに加え、変更前baselineとのCSV比較は`tests/fault_pool_check.py`。
Debug/Release、変更前52bb7acと、出荷バイナリへ検証コードを入れないcapture版は`tests/fault_pool_build.py`で作る。
`--checker`でverificationのbf8faa8から独立検証器も隔離ビルドする。
直列・並列のCSV一致と、変更前に対する完全FDP（上限付きでは両方完了した故障のFDP）を比較する。

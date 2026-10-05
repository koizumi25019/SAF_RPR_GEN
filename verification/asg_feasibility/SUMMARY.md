# ASG のコード理解と LFSR＋PS に対する FDP の実現可能性

調査日: 2026-10-04。
対象: `ASG/` に置かれた二つの論文、`ASG/ASG/src/`、現行 FDP のコード。
本体への ASG モードの実装は行っていない。小回路での接続検証だけを独立に行った。

## 結論と検証範囲

**固定した LFSR・フェーズシフタ・入力配置について、各故障を検出するシードの割合は計算できる。**
one-pass seed generation の組合せモデルを前置すると、現在の
「SAT で検出入力を列挙 → DC 化 → キューブの和集合を BDD で数える」をシード空間で実行できる。
ただし、ASG は検出シードを生成するツールであり、現在のままでは故障ごとの確率を出さない。
また、任意の構成に対するモデル化が可能なことと、大規模な構成で厳密計算が完走することは別。

独立検証 `probe.py` は、ASG の `s27_C.v`、`s27_C_sc.txt`、`tap_4.txt` と、
`polynomial.h` の 4 ビット漸化式を使い、4 ビットへ縮小したモデルを作る。
ASG 本体の `LFSRBIT` は 100 のままであり、ASG の実行体を移植・実行した検証ではない。

- 構成 A: `tap_4.txt` の PS（1-based taps: `[1,2]`, `[1,3]`, `[1,4]`, `[2,3]`）。
- 構成 B: rank の低い比較用 PS（`[1]`, `[1]`, `[1,2]`, `[1,2]`）。
- 故障: CUT の 17 信号線の stem SAF 各 sa0/sa1、34 故障。分岐故障はこの検証の対象外。
- 観測: CUT の PO＋PPO（組合せ化した s27_C の四つの出力）。
- 既存 FDP の正常回路の前に BUF/2 入力 EXOR を接続し、PI を 4 シードビットにする。
- 正常なジェネレータを共有し、故障サイトは明示した CUT の信号線に限定する。
- SAT/XID のキューブ和集合による FDP と、16 シードを具体値でクロックした独立故障シミュレーションを比較。
- 両構成とも **34/34 故障で一致し、`GT_BDD=1` も ALL VERIFIED**。
  GT は照合専用。解法に `BDD_EXACT` は使用していない。

全シード一様の場合の例:

| CUT 故障 | 自由な CUT 入力での FDP | 構成 A（16 シード） | 構成 B（16 シード） |
|---|---:|---:|---:|
| G6/sa1 | 7/64 | 4/16 = 1/4 | 2/16 = 1/8 |
| G0/sa0 | 15/32 | 6/16 = 3/8 | 8/16 = 1/2 |
| G17/sa1 | 11/64 | 2/16 = 1/8 | 2/16 = 1/8 |

構成 A の写像 rank は 4（到達パターン 16）、構成 B は 3（到達パターン 8）。
ゼロシードを除く独立シミュレーションでは G6/sa1 は A=3/15、B=1/15。
この非ゼロシードの値は独立列挙で求めた値であり、現行 FDP の分母変更機能を実装した結果ではない。
モデルの式と具体値シミュレーションの整合は確認したが、実機の scan/capture 配線・クロックとの整合は未検証。
100 ビット・大規模回路、複数パターン、TDF、MISR の計算性能・正しさも未検証。

再現:

```bash
cmake --build build --target main_debug -j2
python3 verification/asg_feasibility/probe.py
```

結果は `results.json`。生成ネットリストと native 実行出力は一時ディレクトリに置き、終了時に削除する。
既存の `output/` は上書きしない。ASG の入力も編集しない。
AddressSanitizer を付けた独立ビルドでも同じ比較が通った（既存リークの評価は対象外）。

## 論文で提案されていること

1. Moriyasu・Ohtake, *A Method of LFSR Seed Generation for Scan-Based BIST Using Constrained ATPG*,
   CISIS 2013, pp.755–759, DOI 10.1109/CISIS.2013.136。
   `ASG/A_Method_of_LFSR_Seed_Generation_for_Scan-Based_BIST_Using_Constrained_ATPG.pdf`。
   §IV、Fig.5–7（PDF 3–4 ページ）で、LFSR を時間展開した XOR ネットワークと CUT の
   組合せ部を接続し、シードをモデルの入力にする。§II-A は RA 部を対象外としている。
2. Moriyasu・Ohtake, *A Method of One-Pass Seed Generation for LFSR-Based Deterministic/Pseudo-Random Testing of Static Faults*。
   `ASG/A_method_of_one-pass_seed_generation_for_LFSR-based_deterministic_pseudo-random_testing_of_static_faults.pdf`。
   §IV、Fig.6–9（PDF 3–4 ページ）で PS を含めたモデルへ拡張する。
   §V、Table VIII–IX（PDF 5–6 ページ）は生成シードから複数パターンを展開した故障被覆を評価する。

two-pass では、自由な CUT 入力を前提に ATPG した後、テストキューブをシードへ符号化するため、
与えられた LFSR/PS で実現できないキューブを選ぶことがある。
one-pass は最初から生成器の制約を ATPG に含めるので、生成解が実現可能なシードになる。
論文の被覆率は故障集合に対する検出割合であり、各故障について一様シードを数えた FDP とは異なる。

## ASG のコードの流れ

| 役割 | コードと確認事項 |
|---|---|
| 起動 | `src/main.c:32` の `main` → `OPT` → `read_nl` → `OutPIN` → `ASG` |
| 全体制御 | `src/asg/asg.c:32` の `ASG`: 読込、正常回路制約、LFSR モデル、対象選択、制約出力、CLASP、故障シミュレーションを繰り返す |
| モデル入力 | `src/asg/read.c:31` の `ReadFile`: 故障、LFSR、PS、入力配置、必須割当情報を読む |
| LFSR の幅 | `src/asg/read.h:35` の `LFSRBIT=100`。`read.c:504` の `CreateLFSRList` で初期シード変数を 1..100 に割り当てる |
| 正常回路 | `src/asg/opb/cons_gc.c:86` の `AssigneVarsGC`: シード変数の後ろに CUT の PI と内部変数を割り当てる |
| 線形展開 | `src/asg/opb/cons_lfsr.c:34` の `CreateLFSRmodel`: 最長配置行数まで LFSR をシンボリックに進め、各行の PS 制約を作る |
| 初期式 | 同 `:119` の `LFSRinti`: ビット i に単位ベクトルを置く。`BIT_INT` は信号の具体値ではなく、シードへの依存係数 |
| 状態更新 | 同 `:146` の `SimulateLFSR`: 多項式のタップに対応する係数を XOR し、高い添字へシフトして index 0 に feedback を置く |
| PS 合成 | 同 `:350` の `PSgetInputBit`: PS タップの係数を XOR し、最終的なシード変数のリストを返す。タップファイルは 1-based |
| PS 制約 | 同 `:197` の `CreateConsPhaseShifter`: PS 出力の XOR 式と CUT 入力を結び付ける |
| 故障コーン | `src/asg/opb/cons_fc.c:33` の `CreateConsFC`: 対象ごとに TFO の故障回路と出力差分を作る |
| 検出条件 | `src/asg/opb/cons_dc.c:240` の `CreateConsDC_FE`: 主故障は励起＋検出を必須にし、relax 故障は緩和変数を導入する |
| 出力・解法 | `src/asg/createSGmodel.c:25` → `makePBOFile`。現行経路は OPB テキストを `tools/clasp/pbo.txt` に出し CLASP を外部実行する |
| 非ゼロ条件 | `src/asg/opb/makePBOfile.c:118` の `makeProbFileConsPS_clasp` の末尾で Σseed_i ≥ 1 を追加する |
| シード集合の目的 | `src/asg/target.c` の必須割当と両立可能グラフ、`CreateMini` の目的関数で、主故障を検出しつつ他故障も検出するシードを探す |
| 検出済み故障 | `src/asg/fsim.c:35` の `FSIM`: 外部 `XID2.exe` に CUT の具体パターンを渡し、検出された故障を落とす |

表の `src/` は `ASG/ASG/src/` を指す。
`-o SINGLE/MULTIPLE` は**対象故障数**、`-m SEED/TEST` は生成器制約を含めるかどうか。
`MULTIPLE` は連続パターン数を指定する機能ではない。
`RandomTarget` は主故障を `relax=false` にし、他故障は検出条件を緩和できる。
確率計算でこの目的関数と緩和故障をそのまま使うと、主故障の検出シード集合全体を数える処理にならない。
故障ごとの列挙は最適解だけに限定しない必要がある。

`scan_chain/*.txt` は各行の j 列を PS の j 番出力、行 i をシンボリックな時刻 i に対応させる入力配置表。
CUT の PI と PPI の両方を含み、単なる「DFF の列」ではない。
`s27_C_sc.txt` の末尾 `G17` は PO の埋め草で、`n_out==0` のため生成器制約を作らない。
通常の `.set` はスキャン化した組合せ `_C.v` を使う。
実機へ適用する場合は、この配置表と本当の scan の向き、PI の印加時刻、shift/capture の境界を照合する必要がある。

## 求める確率の定義

CUT の自由入力を x、r ビットシードを s、固定した生成器と入力配置の写像を G とする。
線形 LFSR＋XOR の PS なら G(s)=B s（GF(2) 上）。故障 f の検出関数を D_f(x) とすると、

```text
H_f(s) = D_f(G(s))
P_f = |{s ∈ S_valid : H_f(s)=1}| / |S_valid|
```

- 全 r ビットシードを一様に選ぶ: 分母 2^r。
- ゼロシードを除いて一様に選ぶ（ASG の現行条件）: 分母 2^r−1。
  全空間での検出シード数が C なら、分子も C−H_f(0) に直す。
- CUT の PI を別系統から自由に与える: それも確率変数に含める。自由度をシードだけとしない。
- 固定した一つのシード・一つの固定パターンには検出/非検出がある。
  「確率」を与えるには、シード、開始位相などの分布を決める。

補助 SAT 変数や CUT の内部変数を独立な確率変数として数えてはいけない。
一般の制約モデルではシードへの射影計数が必要。今回の組合せ回路モデルは内部値がシードから決まる。
ASG の多故障緩和変数やダミー目的変数は、この決定性を持つとは限らない。

rank(B)=k なら、全シード一様の場合、到達可能な 2^k パターンの各々には 2^(r−k) シードが対応する。
しかし CUT の全 2^n 入力への一様分布とは異なる。
特にゼロシード除外時、ゼロパターンの対応数だけ 1 減る。
rank が不足する場合、「到達パターンを重複排除して一様に数える」だけでは非ゼロシード確率にならない。
これは既存の `verification/linear_coordinates/` の**可逆**な入力座標変換とも違う。
ASG の写像は一般に長方形かつ非全射で、FDP を保存する変換ではない。

連続する N テストのいずれかで検出する確率は、正確なクロック・capture スケジュールに応じた G_t を作って、

```text
H_f,N(s) = OR_{t=0..N−1} D_f(G_t(s))
P_f,N = |{s ∈ S_valid : H_f,N(s)=1}| / |S_valid|
```

を数える。LFSR の連続パターンには相関があるため、一般に `1−(1−P_f)^N` とは一致しない。
primitive LFSR の全非ゼロ状態を一巡して一様に観測する解釈と、固定シードからの有限区間の検出率も区別する。
非 primitive の構成では複数の状態軌道があり、非ゼロシード一様と単一軌道の時間平均はさらに異なる。
ここまでの D_f は CUT 出力の差分。MISR の最終シグネチャでの検出確率を求めるなら、
MISR の更新と比較までモデル化する必要があり、応答圧縮による aliasing は現在のモデルに含まれない。

## 現行 FDP への接続案

最初は、今回検証した**生成器の XOR 回路を CUT の前へ接続する方法**が分かりやすい。

1. 幅、feedback、PS タップ、CUT 入力配置・時刻、許されるシードを構成として読む。
   ASG の多項式テーブルだけに依存せず、feedback の任意指定を可能にする。
2. 正常な生成器を組合せ回路化し、CUT の元の PI/PPI を内部信号にして、モデルの PI をシードにする。
   故障リストは変換前の CUT を基準に保存し、生成器への故障自動追加を防ぐ。
   原 CUT の stem/branch の故障識別も維持する。
3. グラフ・fanout・level を確定してから `InitGlobalVars` / XID の scratch / 正常回路 CNF を構築する。
4. `src/fdp/create_TPG_model.c`、`cnf/`、`InlineXID`、禁止節、`CubeSet`、
   `cudd_wrapper.c` の経路で**シードキューブ**を生成・数える。
5. 許可シード集合とキューブ和集合を交差させ、正しい分母で出力する。
   非ゼロの場合、SAT の Σseed_i ≥ 1 だけでなく BDD の集計領域と分母も合わせる。
   `complete=1` は当該構成・当該シード集合の確率が厳密という意味にする。

CUT 内で成立したテスト集合の等価・包含関係は、G の逆像を取っても成立する。
従って同一構成内の支配キューブ流用は原理的に維持できる。
ただし追加した生成器まで構造等価の探索や CSV エコーを広げない境界管理が必要。
異なる構成のシードキューブを混用してはいけない。

別案は元の CUT グラフを保ち、CNF に生成器制約だけを追加する方法。
この場合、元の XID が返す CUT 入力キューブ C(x) を、
`C(G(s))` という XOR 条件へ変換して、シード上の BDD で和集合を数えることもできる。
ただし `C(G(s))` は一般に通常の 0/1/X シードキューブではない。
たとえば x_i=s_0⊕s_1 の x_i=0 は s_0=s_1 という集合。
そのため「CNF に PS を追加するだけで、既存の PI キューブ集計と分母をそのまま使う」変更は誤り。
`GT_BDD` / `BDD_EXACT` も元の独立 PI を前提にせず、同じ G と許可シード集合を含めて照合・数える必要がある。

TDF は SAF の接続が検証できてから別段階にする。
LOC の v1/v2、共有 PI、scan 初期状態を**同じシード**から正当な時刻関係で生成する。
単に `-tdf` に PS 制約を付けるだけでは、この関係が保証されない。

## 計算量と移植上の注意

シード変数が CUT の自由入力より少なくなることはあるが、XOR の相関で SAT と BDD の構造が難しくなる。
小さくなるとは限らず、100 シードビットの 2^100 全点列挙は実用的でない。
シードキューブでも parity による細分化が起こりうるため、既存の素項化・線形領域の研究が候補になる。
厳密値の計算が完了する範囲は実験で評価する必要がある。

ASG の `CreateConsPhaseShifter` は k 本の依存シードビットに対して **2^k 個の節**を列挙する。
任意構成への実装では、多入力 XOR を 2 入力 XOR に分け、補助変数を持つ線形サイズの CNF にする方がよい。
ただし、小さい CNF を作れても model counting が容易になる保証はない。

ASG は Visual Studio、`sprintf_s`/`strtok_s`、`direct.h`、外部 `.exe` を前提とする。
`src/debug/` はソースと vcxproj から参照されるが、今回の追加フォルダには存在しない。
したがって丸ごとの Linux ビルドより、現行 CaDiCaL/CUDD パイプラインへモデル生成部分を移す方が自然。
ソースには CRLF と CP932 のコメントがあり、読込・移植時には LF/UTF-8 を扱う。

静的読解で見つかった注意点（ASG 本体の実行での発現は未確認）:

- `cons_lfsr.c:81–82`: `num+1` 文字分の確保後、`[num+1]` に '\0' を書いており 1 要素範囲外。
- `SimulateLFSR` は `mcycle` ループ内で feedback 用の XOR 累積を毎回ゼロに戻していない。
  現行呼出しは `mcycle=1` なので、複数 cycle 呼出しへ一般化するときに確認する。
- `ReadScanChain` は列ごとの行数で配列を確保する一方、モデル生成は全列を最長行まで参照する。
  ragged な配置を許すなら明示的な空欄と境界検査が必要。供給の標準配置は埋め草を含む矩形。
- `createSGmodel.c:57` のエラー処理が `TPG_MODEL_OKAY` を返すため、生成失敗を正しく伝えない。
- `ASG` は UNSAT でも `OutSolution` を呼び、`FSIM` で初めて UNSAT を扱う。
  FDP の厳密列挙では SAT/UNSAT/UNKNOWN を区別し、UNSAT の解を出力しない。

検証時には現行 FDP の `src/fdp/init.c:98–102` も、ネットリストパスの第 4 要素を
固定で取り出して回路名にするため、階層の浅い絶対パスで NULL の `strdup` による異常終了を確認した。
AddressSanitizer で原因を確認し、独立検証では一時ディレクトリ内に `cases/` を挟んで既存前提を満たした。
今回は読解・実現可能性の確認が目的なので、この本体コードは変更していない。

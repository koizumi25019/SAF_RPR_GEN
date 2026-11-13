# 被覆3手法の実装指示書

[設計と事前検証](cover_methods.md) の3手法を本体へ実装するための指示。
T0 → T1 → T2 → T3 の順に、1タスクずつ実装・検証・コミットする。
アルゴリズムの参照実装は `research/cover_probe/probe.py`（`run_ordered`、`run_affine`、`run_window`）。
参照実装と本書が食い違う場合は本書に従い、食い違いを報告する。

## 全タスク共通の規則

- 作業ブランチは `claude/zealous-hypatia-a1lovt`。他のブランチへ push しない。プルリクエストは作らない。
- `CLAUDE.md`、`docs/architecture.md`、`docs/testing.md` を先に読む。
- **既定動作（`-method` 省略＝`xid`）は1バイトも変えない。** 既存の CTest 12件はそのまま通す。
- `tests/expected/` は変更しない。期待値を実行結果で上書きしない。
- 新しい方式は SAF 専用。`-tdf` と併用したらエラー終了する。
- 本番バイナリに独立検証・詳細計測のフックを入れない。入れてよいのは本書が指定する集計カウンタだけ。
- 環境変数で挙動を切り替えない。切替は CLI と `.set` のオプションだけ。
- SAT の UNKNOWN、オラクルとの不整合は、既存の XID 不整合と同じ扱い（その故障の結果を出さずエラー終了）。
- 乱数を使わない。同じ入力・同じ `-jobs` で同じ CSV になること。直列と並列でも CSV が一致すること。
- 各タスクの最後にビルド、`ctest --test-dir build --output-on-failure`、c17a の期待値照合を行い、結果を報告する。
- 失敗したテストを無効化・スキップしない。通らない場合はその出力を添えて報告する。
- コミットメッセージは日本語で、何を追加したかと検証結果を書く。

ビルドと照合:

```bash
cmake -S . -B build && cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 用語

- P: 検出側ソルバ。現行の `WriteTPGModel()` が作るテスト生成CNF。
- N: 非検出オラクル。T0 で追加する。
- 生成回数: 1故障で領域（キューブ、擬似キューブ、窓領域）を1つ登録した回数。CSVの `cube_cnt` と `-limit` はこれを数える。
- 範囲内PI: 正常CNFの範囲限定で残ったPI。範囲外PIはどの領域でも常にX。

## T0: 共通基盤と `-method prime`

### オプション

- `-method <xid|prime|ordered|affine|window>`。既定 `xid`。CLI と `.set` の両方で受け付け、後の指定が有効。
- `-window_bits <4..16>`。既定 12。`window` 以外では無視せず、値の検証だけ行う。
- `-ordered_ratio <1..16>`。既定 1。非検出側1回あたりの検出側の生成回数（T1 で使う）。値の検証は T0 で入れる。
- 不正な値、値の欠落は既存オプションと同じ形式でエラー終了する。
- `-method` が `xid` 以外で `-tdf` が有効ならエラー終了する。
- `-method` が `ordered`・`affine`・`window` で `-cube` を指定したらエラー終了する（領域が単独のテストキューブではないため）。
  `prime` は `-cube` を許可する。
- まだ実装していない方式を指定されたら「未実装」としてエラー終了する（T1〜T3 で順に外す）。
- `README.md` の設定項目表に追記する。

### 非検出オラクル

`src/fdp/cover/oracle.{c,h}` を追加する。

- `CCaDiCaL* CoverBuildNonDetectOracle(FNODE* target)`。同じ故障に対する `WriteTPGModel()` の**後**に呼ぶ
  （TFOフラグ、`varsfc`、正常CNFの範囲が確定している必要がある）。
- 投入する節:
  1. 範囲内の正常ゲート定義（`LoadNormalDefinition` と同じ集合）。
  2. 故障回路のゲート定義。`CreateConsFC` と同じ条件（TFOかつ故障点以外かつ検出コーン内）のネットに `CreateConsFC_*` を使う。
  3. 故障点の `varsfc` を縮退値に固定する単位節。
  4. 伝搬先POごとの比較XORとそのOR。補助変数は `cnf.total.vars` より後の番号を新規に取る。
  5. 比較結果 z の否定の単位節。
- 入れてはいけないもの: 故障励起の単位節、Dチェーン、必須割当て、キューブ禁止節。
- CaDiCaL のオプションは P と同じ（`factor=0`、`luckylate=0`）。
- 参考: `git show origin/exp/core-only-comparison:src/fdp/paper_core.c` の `PaperCoreBuildOracle`。
  あちらは旧ディレクトリ構成なので、そのまま移植せず現行の関数に合わせる。TDF・低電力の分岐は不要。
- 補助変数の採番は故障ごとに1つのカウンタで管理し、P・N のどちらで使う変数も番号が衝突しないようにする。
  T1〜T3 が同じカウンタから活性化変数・パリティ変数を取る。

### 純素項

`src/fdp/cover/prime.{c,h}` を追加する。

- 入力: P のモデルから読んだ範囲内PIのリテラル列。
- N に全リテラルを仮定して解く。UNSATでなければ不整合としてエラー。
- `ccadical_failed()` で残ったリテラルを、次の assume/solve より前に全部読み取る。
- 残ったリテラルを **PI番号の昇順**で1つずつ外して解き、UNSATなら削除を確定する。
- 出力は現行と同じ `'0'/'1'/'X'` の文字列（範囲外PIは 'X'）。
- 仮定リテラルを追加で渡せる形にしておく（T1 が活性化変数を、T3 が窓の活性化変数を渡す）。
- 参考: 同ファイルの `PaperCoreGeneralize`。

### `-method prime` の処理

- `AnalyzeOneFault()` を方式で分岐する。`xid` の経路は現行のコードをそのまま残す。
- `prime` は現行ループの `InlineXID()` を純素項に置き換えたもの。禁止節、`CubeSet`、`RunBDD`、CSV、`-cube` は現行の関数を使う。
- 伝搬先POが無い故障（`numtranpo == 0`）は現行と同じ結果（P が最初にUNSAT）になること。

### ログと集計

- ログの `Don't-care Method` 行に方式名（`XID`、`PRIME`、`ORDERED`、`AFFINE`、`WINDOW`）を出す。`xid` のときは現行と同じ文字列。
- `FaultStats` に次を追加し、並列時も親で合計する: オラクルの solve 回数、オラクルのCPU秒。
  `xid` 以外のときだけ、ログに `Oracle Solves` と `CPU Time (Oracle)` の行を追加する。
- 方式別の追加カウンタは各タスクで指定する。

### テスト

`tests/cover_methods_check.py` を追加し、CTest 名 `cover_methods` で登録する。以降のタスクで方式を足していく。

1. c17a を無制限で実行し、`net_name,f_type,fdp` が `tests/expected/saf/c17a_result.csv` と一致する。
2. `tests/fault_equivalence_check.py` が使う小回路群（全入力の独立シミュレーション）で、全SAFのFDPが一致する。
   既存スクリプトの関数を import して使ってよい。
3. s27_C、s208_C、s298_C を無制限で実行し、`xid` の同条件の結果と `net_name,f_type,fdp` が全行一致し、全行 `complete=1` である。
4. 直列と `-jobs 4` で CSV が一致する。Debug と Release で CSV が一致する。
5. `-limit 1` と `-limit 3` で、`complete=0` の行の FDP が無制限の厳密値以下であり、`cube_cnt` が上限以下である。
6. オプションの検証: 不正値、`-tdf` 併用、`-cube` 併用（T0 では `prime` が成功すること）、CLI と `.set` の上書き順。
7. `-method` 省略時の CSV・ログが変更前と同じである。

出力は一時ディレクトリに作り、`output/` を上書きしない。

## T1: `-method ordered`（順序付き被覆）

設計は cover_methods.md の4章。`src/fdp/cover/ordered.{c,h}` を追加する。

### 状態

- P と N。故障ごとに活性化変数 g を1つ取る。
- 規則の列: 各要素は（ラベル 検出/非検出、キューブ文字列）。生成順を保持する。

### ループ

検出側を R 回（R = `-ordered_ratio`、既定 1）続けた後に非検出側を1回、という周期で番を決める。最初は検出側。

検出側の番:

1. g を仮定して P を解く。UNSATなら終了（終了理由 = 検出側が尽きた）。
2. 生成回数が `-limit` に達していれば打ち切り（未完了）。判定の順序は現行ループと同じにする（先に解き、UNSATなら上限ちょうどでも完了）。
3. モデルの範囲内PIリテラルから純素項を作る。このとき N には `NOT g` を追加で仮定する。
4. 相対削除: 残ったリテラルを PI番号の昇順に1つずつ外し、**g を追加で仮定した** N で解く。UNSATなら削除を確定する。
5. 規則（検出、キューブ）を登録し、節 `NOT g OR NOT cube` を P と N の両方に追加する。

非検出側の番は P と N を入れ替える。

1. g を仮定して N を解く。UNSATなら終了（終了理由 = 非検出側が尽きた）。
2. 上限判定。
3. N のモデルの範囲内PIリテラルを、`NOT g` を仮定した P に与えて純素項を作る（UNSATコア＋昇順の1リテラル削除）。
4. 相対削除: g を仮定した P で同様に削除する。
5. 規則（非検出、キューブ）を登録し、`NOT g OR NOT cube` を P と N の両方に追加する。

手順3（純粋な素項）を省略して最初から g 付きで解いてはいけない。事前検証で悪化を確認している。

### BDDとFDP

`src/fdp/bdd/` に順序付き被覆用の関数を追加する。

- `T = 0`、`U = 0` から始め、規則を生成順に処理する。`Q` はキューブのBDD。
  検出ラベルなら `U = U OR (Q AND NOT T)`。どちらのラベルでも `T = T OR Q`。
- 終了理由が「非検出側が尽きた」なら、最後に `U = U OR NOT T`。
- 打ち切りの場合は何も足さない（`P(U)` は下界）。
- 数え上げと GMP への取り込みは `RunBDD` と同じ方法を使う。

### 出力

- CSV は現行の7列。`cube_cnt` は規則の総数（両ラベル）。`complete` は終了理由があれば1。
- `FaultStats` に追加: 検出規則数、非検出規則数、相対削除で外したリテラル数。ログに合計を出す。

### テスト

`cover_methods` に `ordered` を加える（T0 の 1〜5、7 と同じ検査）。加えて次を確認する。

- 非検出側が尽きて完了する故障と、検出側が尽きて完了する故障の両方が小回路群に存在し、どちらも厳密値に一致する。
- 冗長故障（s1494_C の12件）の FDP が 0 で `complete=1`。
- `-cube` 併用がエラーになる。
- `-ordered_ratio` が 1、3、16 のいずれでも FDP が厳密値に一致する。

## T2: `-method affine`（擬似キューブ）

設計は cover_methods.md の5章。`src/fdp/cover/affine.{c,h}` と `src/fdp/cover/bitsim.{c,h}` を追加する。

### 64パターン並列の故障シミュレータ（`bitsim`）

- 故障ごとに1回、範囲内ネットをレベル順に並べた一覧と、TFO内ネットの一覧を作る。
- 入力: 範囲内PIごとの64ビット語。出力: 検出ビット列（伝搬先POのどれかで正常値≠故障値）。
- 正常値は範囲内の全ゲート、故障値は TFO 内だけを計算し、TFO 外の入力は正常値を使う。故障点の故障値は縮退値の定数。
- ゲート種別は本体が扱う全種（AND/NAND/OR/NOR/INV/BUF/FOUT/EXOR/EXNOR）。
- 単体テスト: 小回路の全入力について、既存の独立シミュレーション（`tests/` の Python）と検出ビットが一致する。

### 領域

- 制約 = （PI番号の昇順集合 S、パリティ b）。意味は `XOR(x_i, i in S) = b`。
- 領域 = 支持が互いに素な制約の列。

### パリティ変数

- `|S| = 1` ならPIの変数そのもの。
- `|S| >= 2` なら、S から最大のPIを除いた集合の変数と、そのPIの XOR を Tseitin 節で定義する。
- 故障ごとに S をキーにして使い回す。定義節は P と N の両方に、同じ変数番号で入れる。

### 併合

1. 純素項の各リテラルを1変数の制約にする。
2. 現在の制約の全ての対 (i, j)（i < j）について、種パターン（P のモデル）の i と j の代表PI（S の最小番号）を
   同時に反転した入力を作り、`bitsim` でまとめて評価する。非検出の対は候補から外す。
3. 残った対を (i, j) の辞書順に試す。どちらかが既に併合で消えていれば飛ばす。
   制約 i と j を `(S_i ∪ S_j, b_i XOR b_j)` に置き換えた制約列の全リテラルを N に仮定して解き、UNSATなら採用する。
4. 1回の走査で1つでも採用があれば 2 に戻る。採用が無ければ終了。

### 禁止節・BDD・出力

- P の禁止節: 各制約のリテラルの否定の OR（1領域につき1節）。
- BDD: 制約ごとに `Cudd_bddXor` の連鎖でパリティを作り、領域内で AND、領域間で OR。
- `cube_cnt` は領域数。`FaultStats` に併合の採用回数を追加し、ログに合計を出す。
- 完了判定は P のUNSAT。

### テスト

`cover_methods` に `affine` を加える。加えて次を確認する。

- 等価比較回路（例: 4組の XNOR の AND）を `tests/` に追加し、出力 sa0 が1領域で完了し、FDP が厳密値に一致する。
- 併合が1回も起きない回路でも `prime` と同じ FDP になる。

## T3: `-method window`（窓関係）

設計は cover_methods.md の6章。`src/fdp/cover/window.{c,h}` を追加する。`bitsim` を使う。

### 窓の選択

- 故障ごとに、範囲内PIの「ケア回数」を数える（純素項のケア集合に入った回数。今回の素項も含める）。
- 素項のケア集合 K のPIを、ケア回数の降順・PI番号の昇順に並べ、先頭から最大 w 本を窓に入れる（w = `-window_bits`）。
- 窓が w 本に満たなければ、K 以外の範囲内PIを同じ順で補う。範囲内PIが w 本未満なら全部を窓にする。
- `C_out` は K のうち窓に入らなかったPIのリテラル。

### 表

- 表は 2^|W| ビットのビット列（64ビット語の配列）。窓内 i 番目のPIは、表の添字の i ビット目に対応させる。
- 窓外の範囲内PIを与えられた値に固定し、窓内の全割当てを `bitsim` で 64 パターンずつ評価して作る。

### 絞り込み

1. `T` = 窓外を種パターンの値に固定した表。
2. 種ごとに活性化変数 a を1つ取る。`NOT T` を覆うキューブ群を求め、各キューブ c について節 `NOT a OR NOT c` を N に追加する。
3. `C_out` のリテラルと a を仮定して N を解く。
4. SATなら、N のモデルの範囲内PI値を窓外の値として表 `T2` を作り、`T = T AND T2` とする。
   T が変化しなければ不整合としてエラー。新たに除かれた部分を覆うキューブの節を a 付きで追加し、3 に戻る。
5. UNSATなら終了。単位節 `NOT a` を N に追加する。

### 被覆キューブ

- 表を覆うキューブ群は Minato–Morreale の ISOP で求める（参照: `probe.py` の `isop`）。
  CUDD で表のBDDを作ってから被覆を取り出してもよい。どちらでも、キューブ群の和が表と完全に一致することを確認する。
- 1つの表の被覆が 2,048 キューブを超えたら、その種は窓を使わず純素項1本を領域として登録する。

### 禁止節・BDD・出力

- P の禁止節: T の被覆キューブ c ごとに `NOT C_out OR NOT c`。
- BDD: `C_out` のキューブと、T の被覆キューブの和との AND。領域間は OR。
- `cube_cnt` は種の数（生成回数）。`-limit` も種の数に掛ける。
- `FaultStats` に追加: 展開キューブ数（T の被覆キューブ数の合計）、反例による絞り込み回数、素項へ戻した回数。ログに合計を出す。
- 完了判定は P のUNSAT。

### テスト

`cover_methods` に `window` を加える。加えて次を確認する。

- `-window_bits` が 4、8、12、16 のいずれでも FDP が厳密値に一致する。
- ISOP の単体テスト: 幅1〜8の乱数ではない固定の表（全0、全1、パリティ、しきい値、既定の擬似乱数列）で被覆が表と一致する。
- OR対の積（`AND_i (a_i OR b_i)`、6組）を `tests/` に追加し、出力 sa0 が `prime` より少ない生成回数で完了する。

## 測定（T3 の後）

`input/script/saf/` に設定を追加し、出力は `output/saf/<条件>_<方式>/{fdp,log}/` に保存する。既存の出力は上書きしない。

| 回路 | 条件 | 方式 |
|---|---|---|
| s5378_C | limit30、limit300 | xid、prime、ordered、affine、window |
| s9234_C | limit30 | 同上 |
| b14_C | limit30 | 同上（1方式30分を超える場合は打ち切り、その旨を記録する） |

`-jobs 4`、`-progress off`。結果を `docs/cover_methods_results.md` にまとめる。

- 方式ごとの完了故障数、未完了故障数、生成回数の合計、実時間、CPU時間、オラクルのCPU時間。
- 方式の組ごとに、両方完了した故障の FDP 不一致数（0 であること）と、片方だけ完了した故障数。
- 未完了行の FDP は下界として扱い、方式間の大小を誤差として書かない。
- 時間は単回測定であることを明記する。

## 報告に含めること

- 追加・変更したファイル。
- 実行したコマンドと、その結果（テストの合否、照合の差分の有無）。
- 本書から外れた箇所とその理由。
- 未解決の問題。

## Codex への依頼文

タスクごとに1回ずつ、前のタスクのコミットがブランチに入ってから次を依頼する。

T0:

```text
ブランチ claude/zealous-hypatia-a1lovt で作業してください。
docs/cover_methods_impl.md の「全タスク共通の規則」と「T0: 共通基盤と -method prime」を実装してください。
設計の背景は docs/cover_methods.md、参照実装は research/cover_probe/probe.py です。
T1 以降には着手しないでください。最後に「報告に含めること」の項目を報告してください。
```

T1:

```text
ブランチ claude/zealous-hypatia-a1lovt で作業してください。T0 は実装済みです。
docs/cover_methods_impl.md の「全タスク共通の規則」と「T1: -method ordered（順序付き被覆）」を実装してください。
参照実装は research/cover_probe/probe.py の run_ordered です。
純粋な素項を作ってから相対削除を行う順序を必ず守ってください。
T2 以降には着手しないでください。最後に「報告に含めること」の項目を報告してください。
```

T2:

```text
ブランチ claude/zealous-hypatia-a1lovt で作業してください。T0・T1 は実装済みです。
docs/cover_methods_impl.md の「全タスク共通の規則」と「T2: -method affine（擬似キューブ）」を実装してください。
参照実装は research/cover_probe/probe.py の run_affine です。
T3 には着手しないでください。最後に「報告に含めること」の項目を報告してください。
```

T3 と測定:

```text
ブランチ claude/zealous-hypatia-a1lovt で作業してください。T0〜T2 は実装済みです。
docs/cover_methods_impl.md の「全タスク共通の規則」と「T3: -method window（窓関係）」を実装し、
続けて「測定（T3 の後）」を実行して docs/cover_methods_results.md を作成してください。
参照実装は research/cover_probe/probe.py の run_window と isop です。
最後に「報告に含めること」の項目を報告してください。
```

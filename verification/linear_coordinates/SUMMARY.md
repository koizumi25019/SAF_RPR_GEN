# 可逆な入力変数変換によるテストキューブ列挙の削減

実施日: 2026-09-11。独立試作。`src/` の本体には未統合。

## 結果

**s5378_C の n673gat/sa0 で、完全被覆までのキューブ生成数を
61,902 → 1,462（約42分の1）に削減した。**
元のネットリストに対する別実装の SAT 検証で、両被覆とも全キューブの健全性と被覆の完全性を確認。
生成キューブの和集合だけを BDD 化すると、両者の FDP は有理数として完全に一致した。

```text
FDP = 292092239 / 4294967296
    = 0.06800802401266992
```

研究上の制約は **SAT＋ドントケア判定でキューブを生成し、重複処理に BDD を使う**こと。
回路から検出関数を直接 BDD 化する `BDD_EXACT` / `GT_BDD`、PCOUNT、外部モデルカウンタは
この実験では使用していない。BDD 集計器はキューブのリテラル列だけを入力とし、回路を読み込めない。

## 着想：ドントケアを判定する変数を取り替える

通常のテストキューブは元の入力 x の各ビットを 0/1/X で表す。この表現では、例えば

```text
(x1,x2) = 00 または 11
```

を1本にまとめられない。XX にすると非検出の 01/10 まで含めてしまう。
しかし、入力の座標を次のように取り替えると、同じ集合を1本で表せる。

```text
y1 = x1 XOR x2
y2 = x2

元の座標の 00 + 11  =  新しい座標の 0X
```

逆変換は `x1=y1 XOR y2, x2=y2`。入力を削除したり相関を無視したりしているのではなく、
全入力を1対1で対応させている。新しい座標の X は、元の複数入力を連動して変える自由度になる。

一般に GF(2) 上の正則行列 A で `y=A x` とする。変換後の検出関数は

```text
G_f(y) = D_f(A^-1 y)
```

であり、これを CNF にして SAT 解と素項を生成する。A が正則なら一様分布は保存されるので、

```text
P_x[D_f(x)=1] = P_y[G_f(y)=1]
```

が成立する。y 上のキューブ和集合は、現在と同じように BDD で重複を除いて数えられる。
各故障の列挙開始前に基底を固定し、その故障の全キューブを同じ基底で扱う。

**元の入力上では、出力する集合は一般の 0/1/X キューブではなく線形条件の連立である。**
これを元の入力の通常キューブに再展開すれば、削減効果を失うことがある。
内部で y のキューブと変換行列を保持することが必要。

## 今回の実装

1. 正常回路について、構造的サポートが3入力以下のゲートを局所真理値表で調べ、
   XOR/XNOR に等しい部分関数を抽出する。全入力の真理値表は作らない。
2. 故障の影響先 PO の入力コーンに現れる XOR 条件を候補にする。
   現試作では出現ネット数の多い順に、線形独立な条件を採用する。
   残りを元の入力の単位ベクトルで補い、正則な n×n 行列にする。
3. 逆行列で元の PI を y の式に置換する。局所 XOR の認識、定数伝搬、構造共有を行い、
   正常回路と故障回路の出力差を CNF に変換する。
4. 検出側 SAT から y の割当を1つ取得する。非検出側オラクルに対して UNSAT を維持するように
   固定リテラルを除去し、y 上の素項へ広げる。各素項の否定を検出側の禁止節に追加する。
5. 検出側が UNSAT になるまで列挙する。生成したキューブだけの和集合を CUDD で作り、
   GMP 有理数で確率を計算する。

`--mode binary` と `--mode linear` は、変数の基底以外の列挙処理を共通化している。
両方とも SAT の UNSAT コア＋リテラル削除による集合極小化（`--minimize`）を使う。
現行 XID の直接移植ではなく、**意味論に基づくドントケア判定を持つ独立試作**である。
既存の本体や HALL との実行時間の直接比較ではない。

`n673gat/sa0` では214入力中、12個の独立な XOR 条件を採用できた。例：

```text
n777gat XOR n659gat XOR n553gat
n561gat XOR n366gat
n322gat XOR n318gat XOR n314gat
n1282gat XOR n1226gat
```

局所置換後の検出式の構造的サポートは90変数から80変数へ減った。
12条件の一覧は `runs/n673gat_linear_overlap_prime.result.json` の `info.parities` にある。

## 比較結果

### 実回路：s5378_C / n673gat / sa0

| 同一試作内の条件 | 生成キューブ数 | 列挙時間 | SAT solve 総回数 | 和集合 BDD の処理時間 | 完全被覆 |
|---|---:|---:|---:|---:|---|
| 元の入力＋素項化 | 61,902 | 138.706 s | 1,870,458 | 0.857 s | 確認済み |
| XOR 基底＋素項化 | **1,462** | **2.212 s** | **29,462** | **0.048 s** | 確認済み |

列挙時間は初期の回路読込・変数変換・ランダムシミュレーション検査・独立検証を含まない。
各条件1回の実測で、一部実行は別の条件と並行した。速度比はこの試作の参考値であり、
本体全体の速度向上を保証するものではない。キューブ数の比は約42.34、列挙時間の比は約62.70。
SAT solve 総回数には検出テスト生成に加えてドントケア判定の問い合わせも含む。

両者の和集合 BDD は有理数 `292092239/4294967296` を返した。
記録済み FDP `6.8008024013e-02`（`../pcount/SUMMARY.md`）とも表示精度で一致する。
この参照値を得るために過去のカウンタを再実行したわけではない。
BDD は本体と同じ `CUDD_REORDER_SIFT` による動的変数順序最適化を使用した。
固定順序の集計は重くなったため中断し、上表には採用していない。

データ：

- `runs/n673gat_binary_overlap_prime_full.{result,verified,union}.json`
- `runs/n673gat_linear_overlap_prime.{result,verified,union}.json`

### 適用が必ず有利になるわけではない

| 回路・故障 | 元の入力＋素項化 | XOR 基底＋素項化 | 結果 |
|---|---:|---:|---|
| s5378_C / n2141gat / sa0 | 36本 | 43本 | 少し悪化。両方の完全被覆は SAT で確認済み |

このため、全故障に一律適用する前に、XOR 条件の選択と対象故障の判別が必要。

### 原理を切り分ける人工回路

`cases/equality12.v` は24入力を12組に分け、各組が等しいときだけ出力 z が1になる回路。
z/sa0 の検出条件は12個の XNOR の積になる。

| 条件 | キューブ数 | 厳密 FDP |
|---|---:|---:|
| 元の入力で列挙 | 4,096 | 1/4,096 |
| XOR 基底で列挙 | 1 | 1/4,096 |

元の座標では1ビットも X にできず、2^12 通りの完全割当が必要。
新しい座標では12個の XOR を0に固定し、残り12変数を X にできる。
この例では全キューブが互いに交わらず、単純な体積加算でも、キューブ和集合 BDD でも同じ値になる。

重複禁止まで要求した別試行では、`n673gat` の変換後でも45秒・3,957本で未完だった。
**この研究では重複を許して列挙し、BDD で処理する。** 非重複列挙への変更は採用しない。

## 正しさの確認

- **変換行列**：214本（人工回路では24本）の行が独立になるよう生成し、逆変換との積を検査。
- **独立な回路 CNF**：`verify.py` は元の各ゲートの真理値関係を直接 CNF 化し、正常・故障回路を構成。
  最適化した検出式の生成コードは使わない。`y=A x` の関係を追加し、
  全キューブで `C(y) AND NOT D_f(x)` が UNSAT になることを確認した。
- **完全性**：同じ元回路 CNF で、`D_f(x) AND AND_i NOT C_i(y)` が UNSAT。
  1,462本側・61,902本側の両方で確認済み。
- **c17a**：試作が対応する全12信号線の stem sa0/sa1、計24故障について、32入力割当ずつ独立にシミュレーション。
  変換後の関数、SAT 被覆、非重複体積和、キューブ和集合 BDD が一致し、
  `expected/c17a_result.csv` の該当する全24行とも一致した。
  本体の分岐故障名を含む全行の回帰ではない。
- **精度**：BDD 集計は浮動小数点の近似計数ではなく GMP 有理数を使用。

試作の `complete` は「SAT によるキューブ被覆が完全」の意味。
`*.union.json` は入力キューブの和集合の大きさであり、打ち切りファイルを渡した場合は部分被覆の値になる。
列挙ログの `mass_sum` はキューブ体積の単純和であり、重複を許すモードでは FDP ではない。

## 文献との関係

入力を線形変換して SOP を小さくする考え方自体は既存の論理合成技法であり、新規発明とは主張しない。
今回の提案は、それを **故障ごとの検出関数の SAT キューブ列挙**へ適用し、
正則性を使って元の入力空間での確率を保持すること。

- Keren, Levin, Stankovic, *Use of Gray Decoding for Implementation of Symmetric Functions*,
  VLSI-SoC 2007。
  入力の線形変換による SOP 項数の削減を扱う。今回の基底選択は Gray decoding の再実装ではない。
  https://www.tau.ac.il/~ilia1/publications/gray.pdf
- Keren, Levin, Stankovic, *Linearization of Functions Represented as a Set of Disjoint Cubes at the
  Autocorrelation Domain*。
  SOP と線形変換の関係を示す関連研究。今回の試作は自己相関スペクトルを計算していない。
  https://www.tau.ac.il/~ilia1/MY_PAPERS-PDF/Procidings/LinDC.pdf
- Lee, Jiang, Hung, *Bi-Decomposing Large Boolean Functions via Interpolation and Satisfiability Solving*,
  DAC 2008。
  SAT を用いた AND/OR/XOR 分解を扱う。より大きい非線形部分関数まで候補を広げる場合の関連技法。
  今回はこの分解アルゴリズムを実装していない。
  https://alcom.ee.ntu.edu.tw/assets/publications/dac08-bd.pdf

## 次の研究判断

「CNF だから限界」という整理は狭める必要がある。
今回も CNF/SAT を使い続けており、変えたのは **ドントケア化と被覆を行う入力の座標**。
元の PI で素項化を強化するだけでは得られなかった削減を実回路で確認できた。
一方、任意の論理関数を小さい被覆にできる保証はなく、今回の結果も少数故障に限られる。

次は以下を優先する。

1. s5378 の他の列挙困難故障に対象を広げ、改善・悪化の分布を測る。
2. 出現回数だけでなく、故障の検出条件での寄与を使って XOR の候補と基底を選ぶ。
   軽い試行で元の基底と候補基底を比較し、悪化する故障を避ける方法を検討する。
3. 基底と y キューブを本体へ渡す方法を設計する。現行 XID は元のネットリストの PI を対象とするため、
   そのまま使えるとは限らない。故障間の支配流用も、異なる基底のキューブを混ぜない設計が必要。
4. XOR が少ない故障には、入力グループの許容組合せや SAT による関数分解を別候補として検討する。
   任意の内部信号を独立な一様入力とみなすことはできない。正則変換で保証していた分布保存を失うため。

## ファイルと再現手順

実験のソース、ビルド、入力、出力はすべてこのディレクトリ配下に置く。

```text
linear_coordinates/
  SUMMARY.md       この記録
  build.sh         既存の CaDiCaL/CUDD をリンクする独立ビルド
  probe.py         局所 XOR 抽出、基底変換、SAT 列挙
  sat_bridge.cc    CaDiCaL 呼び出し用プロセス
  verify.py        元の回路 CNF による独立検証
  count_cubes.py   既に生成したキューブの集計
  cube_union.cc    キューブ和集合だけを BDD 化する集計器
  cases/          人工回路
  build/          ビルド生成物（git ignore）
  runs/           条件名別のログ、CNF、キューブ、結果 JSON
```

リポジトリのルートで実行する。

```bash
bash verification/linear_coordinates/build.sh

# 変換後の完全列挙（重複を許す）
python3 verification/linear_coordinates/probe.py --mode linear --minimize \
  --limit 10000 --seconds 45 --prefix n673gat_linear_overlap_prime

# 元の入力で同じ素項生成・列挙を行う比較
python3 verification/linear_coordinates/probe.py --mode binary --minimize \
  --limit 150000 --seconds 240 --prefix n673gat_binary_overlap_prime_full

# 回路の直接 CNF に対する健全性・完全性検証
python3 verification/linear_coordinates/verify.py
python3 verification/linear_coordinates/verify.py --mode binary \
  --prefix n673gat_binary_overlap_prime_full

# SAT が生成したキューブの和集合を BDD で集計
python3 verification/linear_coordinates/count_cubes.py n673gat_linear_overlap_prime --nvars 214
python3 verification/linear_coordinates/count_cubes.py n673gat_binary_overlap_prime_full --nvars 214

# 小回路の全入力検査とゴールデン比較
python3 verification/linear_coordinates/verify.py --c17
```

`probe.py` の時間上限はキューブ間で判定するため、1回の SAT solve の途中では打ち切らない。
BDD 集計器の外部プロセスには60秒のタイムアウトを設定。
試作は組合せ `.v` の実在する信号線の stem SAF に対応し、分岐故障・LOC 時刻展開には未対応。
大きい CNF/キューブとバイナリは git ignore にし、小さい測定結果 JSON とこの記録を残す。

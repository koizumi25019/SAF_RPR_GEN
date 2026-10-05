# AllSAT論文とFFR後続の分解手法の調査

調査日: 2026-09-17。添付SAT 2023論文を通読し、参考文献・後続研究・論理分解研究を確認した。
その後、最優先案のcare付きFFR観測被覆を`verification/ffr_decomposition/`へ実装・評価した。
論文で実証された結果、既存のFDP実験、今回の実装結果を区別する。

## 結論と優先順位

最優先は**FFRの外側に残る大きな観測条件を、境界の到達可能性と局所ドントケアを使って分解すること**。
HALLの列挙器をそのまま置き換えることだけを次の本命にはしない。

| 優先 | 手法 | 根本原因への作用 | 主な論文 | FDPでの状態 |
|---:|---|---|---|---|
| 1 | 励起・伝搬条件をcareとして使う局所被覆 | 不要な領域まで因子を解くことを避ける | TRETS 2011、eSLIM SAT 2024 | 実装・選択故障で最大21.0倍、全故障評価は未実施 |
| 2 | 到達不能なcut状態を使うACD | 境界信号の見かけの組合せを減らし、分解を可能にする | ACD using Don't Cares, IWLS 2025 | 新規重点候補、未実装 |
| 3 | SAT＋補間・MUSによる意味論的分解 | 回路の形から見つからないAND/OR/XOR因子を抽出 | DAC 2008、Chen–Marques-Silva拡張章 | 既提案を具体化、未実装 |
| 4 | HALLの候補生成＋意味論的DCを局所葉に適用 | 最初に選ぶモデル／キューブの質を改善 | SAT 2023、SAT 2024 | 公式ツール導入済み、階層統合は未実装 |
| 条件付き | SATによる階層境界の復元 | 合成で消えた分解構造を取り戻す | DAC 2026 | 元仕様／階層参照が必要、現状優先しない |

優先度は実装効果の予測であり、FDPの実測順位ではない。1は既存FFR試作へ最も小さく接続できる。
2・3は故障位置自身がFFR根であるn673gat、n291gat、g16349などに向けた研究の中心。
いずれもSAT＋DCの局所被覆を残し、BDDは生成被覆の集合同士の操作だけに使う。

## 1. 添付論文の正確な位置づけ

**Dror Fried, Alexander Nadel, Yogev Shalmon,
AllSAT for Combinational Circuits, SAT 2023, Article 9, 18 pages.**
[添付PDF](../../paper/ALL-SAT-Combinational-Circuitpdf.pdf)／
[公式公開版](https://doi.org/10.4230/LIPIcs.SAT.2023.9)。

論文が直接扱うのは、単一出力組合せ回路の全解を、重複可／重複なしのDNF被覆として生成する問題。
FDPの「検出関数D=1の入力を全被覆する」部分とよく対応する。全素項の列挙とは違う問題である。

### TALE、MARS、DUTYから使えること

| 方法 | 論文中の仕組み | 今のFDPとの関係 |
|---|---|---|
| TALE、§4.1 | SATでモデルを得て、回路の三値シミュレーションでPIをXにする | XIDに近い方向。意味論的DCを置き換える決定打ではない |
| MARS、§4.2 | dual-railで0/1/XをSAT変数化し、少ないcareビットを持つモデルを探索する | モデルを選んでから縮める方式とは探索の入り方が異なる |
| DUTY、§4.3 | MARSの候補にTALEの三値縮約を追加 | 軽い候補生成と後段縮約の組合せとして参考になる |

MARSは各信号を2変数にし、(0,0)をXとして表す。実際のアルゴリズムは
完全なMaxSAT最適解を毎回求めず、変数選択・極性のヒューリスティクスを利用する。
したがって「最短キューブを必ず返す」手法とは説明しない。
また最短キューブ1個を選ぶことと、全被覆の項数を最小にすることは同じではない。

§3.1・§4.2.3の重複可／重複なしの区別も重要。FDPはBDDで重複を処理できるので、
列挙時に重複なしを要求してキューブを細分化する必要はない。
通常のBoolean PI列挙では、検出側に禁止節を入れつつ、DC判定は元のDに対して行う。
MARSはdual-rail用の異なる禁止節を使うため、その式だけを通常PI solverへ移植しない。

### 「CNFでは避けられない」の意味を限定する

論文§4の例では、内部変数まで固定したTseitin CNFの各節を充足し続けるようにPIを外すと、
本来不要な入力まで固定される。これは**その一般化方法の制限**であり、
CNFを使うSAT全般が意味論的DCを判定できないという意味ではない。

現在の非検出オラクルは

```
Q AND NOT D が UNSAT  ⇔  Q内の全入力が検出入力
```

を検査する。内部変数を単一モデルの値へ固定する必要がない。
ただしCNFにしたゲート関係全体を単純に否定するのではなく、回路関係を保って出力を0にした
非検出CNFを使う。片方向符号化の場合も負極性の正しさを別途確保する。

残る根本的な障害は**普通のPI-SOPそのものに指数個の項が必要な関数**。
TALE/MARS/DUTYはいずれも出力がPIキューブなので、この表現の下界は変わらない。
前回FFR試験の `x AND PRODUCT_i(a_i OR b_i)` は、その区別を明確にする。

## 2. 後続AllSAT研究を踏まえた判断

### Entailing Generalization Boosts Enumeration — SAT 2024

Fried, Nadel, Sebastiani, Shalmon。
[公式論文](https://doi.org/10.4230/LIPIcs.SAT.2024.13)。
三値伝搬で出力1を確定できることと、全てのXの埋め方で論理的に出力1になることを分け、
後者のentailing generalizationをHALLへ導入している。

例えば `D=(a AND b) OR (a AND NOT b)` はaだけで決まる。
元の構造でa=1,b=Xを三値伝搬すると出力Xになり得るが、意味論的にはa=1が十分。
この論文は、現在使う意味論的DCを維持する根拠になる。三値だけへ戻す優先度は低い。
一方、軽い一般化とSATによる一般化の組合せは、FFR葉ごとの費用削減候補になる。

### Entailment vs. Verification for Partial-Assignment Satisfiability and Enumeration — CADE 2025

Roberto Sebastiani。
[出版社](https://doi.org/10.1007/978-3-031-99984-0_37)／
[著者公開PDF](https://disi.unitn.it/rseba/papers/cade25.pdf)。
部分割当てが論理的に式を含意することと、代入による検証の差を扱う理論的整理。
既存調査でも参照済み。今回も「CNF縮約の弱さ」と「SOP表現の下界」を混同しないために参照する。
新しいFDP分割アルゴリズムそのものではない。著者PDFは今回の取得では失敗したため、
新しい詳細主張は追加せず、出版社情報と既存調査を用いた。

### On CNF Conversion for SAT and SMT Enumeration — JAIR 2025

[著者公開論文](https://disi.unitn.it/rseba/papers/JAIR25-cnfenum.pdf)。
NNF＋Plaisted–Greenbaum変換で部分モデルの過剰指定を減らす方向。
このリポジトリでは既に `allsat_encoding` で実装・比較済み。
現在の意味論的DC＋多値領域を一律に上回らなかったという既存結果を優先し、
同じ符号化変更を新規の有力案として再提案しない。

### Disjoint projected enumeration for SAT and SMT without blocking clauses — AIJ 2025

[著者公開論文](https://disi.unitn.it/rseba/papers/AIJ25.pdf)／
[DOI](https://doi.org/10.1016/j.artint.2025.104346)。
TabularAllSAT／TabularAllSMT。禁止節を増やさず、射影と重複なし列挙を扱う。
禁止節の蓄積には効き得るが、平坦キューブ被覆の必要サイズは別問題。
公式実装の既存比較では選んだ難故障で現行多値方式を上回らず、今回は再実験しない。
この否定的結果はTabularAllSAT一般の性能を否定するものではない。

### Everything You Always Wanted to Know About Generalization of Proof Obligations in PDR — TCAD 2023

Seufert, Winterer, Scholl, Scheibler, Paxian, Becker。
[著者版](https://arxiv.org/abs/2105.09169)／[DOI](https://doi.org/10.1109/TCAD.2022.3198260)。
三値、lifting、justification、MaxSAT/QBFなどの一般化方法と適用条件を比較する。
一般の遷移関係で示す計算量を、そのまま決定的組合せ回路のDC判定へ当てはめない。
本研究では難しい局所葉の候補生成法の比較資料として使い、全故障でQBF最適化を行う案は優先しない。

## 3. 今回見つけた重点候補：到達不能な境界状態を使うACD

**Benjamin Hien, Marcel Walter, Alessandro Tempia Calvino, Alan Mishchenko, Robert Wille,
Ashenhurst-Curtis Decomposition Using Don't Cares, IWLS 2025.**
[著者公開論文](https://people.eecs.berkeley.edu/~alanmi/publications/2025/iwls25_dc.pdf)／
[会議プログラム](https://www.iwls.org/iwls2025/program.php)。

論文は小さいcutのcontrollability don't-careを利用し、不完全指定のcofactorをまとめてACDする。
§Vでは7〜11入力のcutと6-LUT分解を扱う。概要にある分解成功率51%→53.4%は
この論理合成実験の指標で、FDP時間の改善率ではない。
§II-Cのcare抽出はwindowの到達パターンのprojectionを用いており、
SATによる到達不能証明を使う以下の案は我々の適応設計として区別する。

FFR入力をs1,...,skとすると、形式上2^k状態があっても、上流回路が作れない状態があり得る。
実現不能な状態でも局所関数を厳密に保存しようとすると、不要な複雑さを抱える。
到達不能な部分を自由に補完して、局所関数を少数の中間信号へ分解するのが狙い。

**今回のFDP向け案（未実装）:**

1. 根の観測条件の再合流周辺から、まず6〜10信号程度のcut候補を作る。
2. 上流関係U(x,s)に対して `U AND (s=pattern)` をSAT検査し、UNSATの状態だけDCとする。
   ランダムシミュレーションに出なかったことだけでDCと決めない。
3. 到達する状態上で元と一致するように、小さいACD／AND/OR/XOR分解候補を作る。
4. `U AND (F(s) XOR F_new(s))` のUNSATで等価性を証明する。
5. 得られた局所関数をSAT/DCで被覆し、階層を残して合成する。

到達可能性と確率は別物。SATが「可能」と答えたcut状態を等確率にしてはいけない。
局所条件を元PI上の被覆へ引き戻してBDDで合成するか、独立性が証明された境界だけに厳密重みを使う。
この点が、分解に成功すれば直ちにFDPを掛け算できるという誤解を防ぐ条件になる。

## 4. 局所問題を全部解かず、外側の条件をcareにする

### Scalable Don't-Care-Based Logic Optimization and Resynthesis — TRETS 2011

Mishchenko, Brayton, Jiang, Jang。
[著者公開論文](https://people.eecs.berkeley.edu/~alanmi/publications/2011/trets11_mfs.pdf)。
window、SAT、補間を用いてドントケア下の局所関数を作り直す。
§2.2の条件は、care内で候補信号が同じ値になる二入力について、元関数の値が食い違わないこと。
§2.3は、複数ノードのDCを古い回路に対して計算して一斉変更すると互換性の問題が出るため、
変更後の状態を反映して逐次扱う必要を述べる。

### eSLIM: Circuit Minimization with SAT Based Local Improvement — SAT 2024

Reichl, Slivovsky, Szeider。
[公式論文](https://doi.org/10.4230/LIPIcs.SAT.2024.23)／
[実装](https://github.com/fxreichl/eSLIM)。
多出力部分回路に対して許容される入力出力関係を作り、SATで局所再合成する。
windowに限定することで扱える規模を広げる。window化が失うのは一部の最適化機会であり、
元回路の意味を近似してよいという意味ではない。
目的は回路サイズなので、FDPの被覆数・BDDサイズが減る保証はない。

**これらから導いたFFR向け実装:** 従来試作は `D=E AND L AND O` の各因子を
全入力空間で完全被覆している。しかしOは、`E AND L` が偽の入力では何を返してもDに影響しない。
`H=E AND L` をcareとして、Oの被覆Uを次の条件で生成できる。

```
seed探索:       H AND O AND NOT U
キューブQ拡大:  H AND Q AND NOT O が UNSAT
完了判定:       H AND O AND NOT U が UNSAT
最終被覆:       H AND U
```

このUは単独ではOの下界被覆とは限らない。**Hを掛け戻した被覆だけを数える。**
Hの偽の領域へは自由にQを広げられるので、Oを無条件に完全列挙するより小さくできる可能性がある。
SAT証明に使うHは正確な式でなければならず、未完成のHの被覆で置き換えて完了を宣言しない。
最終的には `D XOR (H AND U)` を元回路CNFで検査する。

さらに一般の `D=AND_i F_i` では、他因子の積をcareとしてFiを変える案がある。
複数因子の同時緩和は誤りを生むため、逐次更新・再証明するか、一因子だけを条件付き化する。
初期試作はOだけを対象にする。FFR根自身の故障ではH=Eとなり、
EとOが独立な支持しか持たなければこのcareによる縮約の利益はないため、別の分割を使う。

実装ではnative列挙器の検出側を`H AND O`、非検出オラクルを`H AND NOT O`とした。
s5378の選択4故障を交互順で3回測定し、全キューブ数／中央値時間は
II4216/sa1が466→14／0.1336→0.0277秒、n79gat/sa1が763→82／0.1853→0.0395秒、
n995gat/sa0が654→287／0.2531→0.1114秒、n518gat/sa1が3532→316／
1.9018→0.0904秒。全て同じ厳密FDPで元回路CNFとの等価性を証明した。
加えてs38584の30信号サンプルから選んだII16102/sa1は485→380項、3回中央値
1.4725→0.8389秒で、同じ厳密FDPと元回路CNF等価性を確認した。
一方、当初候補のn2428gat、n2432gatは項数不変、n1592gatは986→990へわずかに悪化し、
n673gat、n291gatは未完了のまま。
貪欲被覆の選び方により悪化する極性もあり、一律適用は採用しない。詳細は
`../ffr_decomposition/SUMMARY.md`と`../ffr_decomposition/results/care_candidates.json`を参照。

なお正常回路のPO等価性だけを保つ再合成では、元の内部線の縮退故障の意味を保存できない。
適用先は故障を挿入済みの検出関数Dとし、元の故障位置を維持した独立CNFとの等価性を保証する。

## 5. FFRの形から切れない関数をSATで切る

### Bi-Decomposing Large Boolean Functions via Interpolation and Satisfiability Solving — DAC 2008

Lee, Jiang, Hung。
[著者公開論文](https://alcom.ee.ntu.edu.tw/assets/publications/dac08-bd.pdf)。
`F(A,B,Z)=FA(A,Z) op FB(B,Z)` のAND/OR/XOR分解をSAT＋補間で扱い、
変数分割の探索も行う。構造上の独立支持だけを見る現在の分解より広い候補を探せる。

### Improvements to Satisfiability-Based Boolean Function Bi-Decomposition

Chen, Marques-Silva、VLSI-SoC 2011系列の拡張章。
[公開章](https://dl.ifip.org/index.html/db/conf/vlsi/vlsisoc2011s/0001M11.pdf)。
構造情報とgroup-oriented MUSによって変数分割を改善する。
このFDPでは共有集合Zを小さく保つことが、分解後の相関処理量の削減に直結する可能性がある。

### To SAT or Not to SAT: Ashenhurst Decomposition in a Large Scale — ICCAD 2008

Lin, Jiang, Lee。
[論文](https://cecs.uci.edu/~papers/iccad08/PDFs/Papers/01B.2.pdf)。
SAT、補間、関数従属性でAshenhurst分解を扱う。論文概要は最大300入力の実験を報告する。
一般の多ビットACD状態圧縮と、単一中間信号によるAshenhurst分解を同一視しない。
300入力の関数を分解できたことは、同規模のFDPが高速に数えられることを意味しない。

**適応設計:** FFR根の観測関数を対象に、構造から数個の変数分割候補を作り、SATで分解可能性を検査。
AND/ORなら因子を保持し、XORならXORのまま上位を保持する。全てを最後にSOPへ戻すと利点が消える。
支持が重なれば生成被覆の交差を扱う。独立な元PIの小集合Zに条件付け、残りのA/Bが独立になる場合だけ
条件付き確率を算術合成できる。内部信号を固定しただけで独立だとは扱わない。
補間器の追加と候補探索の費用があるので、全故障の前処理にはせず難しい葉へ限定する。

## 6. 新しいが、今の入力だけでは適用しにくい研究

**Ho, Fan, Jiang, Mishchenko, Weaver,
Hierarchical Boundary Recovery: Overcoming Synthesis Obscurity via SAT Sweeping, DAC 2026.**
[著者公開論文](https://people.eecs.berkeley.edu/~alanmi/publications/2026/dac26_hie.pdf)。

合成で消えた階層境界を、参照仕様と実装回路のSAT sweeping・境界拡張・論理の移植で復元する。
元の階層的仕様がある場合には分解候補を改善できそうだが、フラットなベンチマークだけから
未知の最良分割を発見するアルゴリズムではない。
現在のFDP入力に参照階層があることは確認しておらず、優先候補には置かない。

## 7. 1990年の分割論文との接続

Chakravarty–Huntの[添付論文](../../paper/1990-TranComt-On_computing_signal_probability_and_detection_probability_of_stuck-at_faults-koizumi.pdf)
は、supergate内の分岐状態を条件として木状部分回路を解く。
今回の方向は、その分岐状態を全て区別して列挙する前に、次の二点を調べること。

- 到達しない境界状態を落としてよいか（CDC）。
- 上位の検出結果から区別する必要のない状態をまとめてよいか（DC下の機能分解）。

ここが単純なFFR分割から進める部分。上位へ渡す信号／状態数が減れば、局所問題の数と大きさも減らせる。
ただし到達状態をまとめた後も、その確率の相関を保持する必要は残る。

## 8. 次の実験を具体化する

### 実施済み：care付きFFR観測被覆

- 比較対象: 現行FFR因子被覆 vs `H=E AND L` 下のO被覆。
- n1592gat、n2428gat、n2432gatを成功候補、n673gat、n291gatを難例／効果限定候補にする。
- 記録: 局所キューブ合計、SAT/DC呼出し、前処理を含む完了時間、合成BDDの最大規模。
- 検証: c17ゴールデン・全入力照合、元回路CNFによる最終被覆の健全性と完全性。
- 条件付き被覆を他故障へ流用する場合は、care条件を含めて正しさを再証明する。

上記は実施済み。最初に選んだ成功候補では項数不変だったため、H/Oの支持重複を持つ
200故障の決定的サンプルから追加候補を選んだ。c17全24ステムは全入力・ゴールデン・raw CNFで一致。

### 次の試作：cutの到達状態＋DC付き分解

- 当初はcut幅6〜10、候補数を少数に限定。これは我々の開始設定であって文献の保証値ではない。
- 全2^k状態が到達するcutはCDCの利益がない。到達不能状態数と、分解後に残る情報量を測る。
- 元の関数を小さく書き換えただけでなく、被覆の総量または階層合成費用が下がったかを見る。
- b19へは、まず既存の未完了故障の小windowで実験し、全故障への効果を外挿しない。

### AllSATの候補生成を追加する段階

- HALLを全体へ一律適用する前に、分解後に残る葉でTALE、MARS non-disjoint、
  SAT 2024のentailing方式を比較する。既存 `external/hall` と専用実行スクリプトを利用できる。
- 現行のmulti-start DCは同じseedから縮約順を変える。MARSは候補の出し方も変える点を比較する。
- 候補を評価するならcare数だけでなく、既存被覆Uに対する新規領域 `Q AND NOT U` も見る。
  ただしBDDによる候補採点の費用を含める。これは論文の実装そのものではなく後続の応用案。

## 9. 既存結果と今回の成果の境界

- 調査後にcare付きFFRの上記測定を追加した。前回FFRの約1.086→0.039秒は別の既存結果。
- 共通CNF因子、FFR因子被覆、weighted分解、NNF＋PG、Tabular比較は各既存SUMMARYを参照。
- 今回の新規重点はIWLS 2025のDC付きACDと、TRETS/eSLIMを踏まえたcare付きFFR被覆の実装。
- eSLIM・ACDの面積／遅延改善をFDP高速化の根拠に置き換えない。
- 全素項列挙、全面QBF最適化、回路直結BDD／モデルカウンタへの置換を本命にしない。
- 正しさの保証は、局所のSAT検査に加えて元回路Dとの最終被覆等価性と厳密な確率合成による。

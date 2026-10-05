# 次に試すべき FDP 完全列挙高速化の調査

調査日: 2026-09-13。

2026-09-17追記: ユーザーの「根本原因を優先」という方針に合わせ、今後の優先対象を
平坦SOPの直積展開を避ける分解・被覆表現へ変更した。
[共通CNF因子](../clausal_regions/SUMMARY.md)と
[添付1990年論文に基づくFFR経路分解](../ffr_decomposition/SUMMARY.md)を別途試作・検証済み。
以下の順位は全故障処理の総時間を重視した当初の調査結果として残す。

今回は文献調査と既存結果の読み直しを中心とし、新しい列挙器の実装・性能実験は行っていない。
保存済み中間データについては、候補となる再利用関係を静的に照合した。提案は次の条件を維持する。

- 検出入力の探索とドントケア拡大には SAT を使う。
- BDD は SAT/DC が生成した被覆の和集合・重複処理に使ってよい。
- 回路の検出関数をそのまま BDD に構築して数える方式にはしない。
- PCOUNT、外部モデルカウンタによる置換を提案しない。
- `complete=1` は、生成被覆の健全性と `D_f AND NOT covered` の UNSAT により保証する。

## 結論

全故障の総時間を下げる候補と、一故障の列挙爆発を解く候補は分けるべきである。
実装順として最も有望なのは次の組合せである。

1. **完成被覆の故障間再利用を全パイプラインへ広げ、SAT で意味論的な検出関数同値も拾う。**
   独立に列挙する故障数そのものを減らすため、十万故障級へ最も直接効く。
2. **同じゲート周辺の少数故障だけで SAT ソルバを共有する。**
   正常 CNF、故障コーンの共通部、学習節を再利用する。全故障を一つのソルバへ入れる方式は避ける。
3. **AND/OR だけでなく XOR、ITE、小さい合成関数を階層のまま確率合成する。**
   平坦 SOP 自体が指数個必要な関数に対する、本質的な対策になる。
4. **故障・最初の短い試行の特徴から手法を選ぶ。**
   既存方式は対象によって高速化と悪化が逆転しており、一律設定には限界がある。
5. **難故障には、非検出反例をキューブ間で共有する implicit hitting set 型 DC 拡大を試す。**
   現在の greedy/QX が各キューブで繰り返す失敗判定を学習資産に変える。

研究上の伸びしろが最も大きいのは、入力と故障番号の直積を `C × S` という領域で覆う
**複数故障同時被覆**である。包含関係や完全な同値がなくても、複数故障の検出集合の共通部分を一度だけ
生成できる。ただし、これは文献の multiple-target ATPG を完全被覆へ拡張する新しい案であり、効果は未実証である。

## 現状から読み取れる制約

### `-limit 30` の総時間は BDD が主因ではない

[pipeline_profile](../pipeline_profile/SUMMARY.md) の既存ログでは、BDD の CPU 比率は b18 で約0.19%、
s38584 で約0.57%だった。一方、SAT 求解、正常 CNF の毎故障投入、ソルバ生成・破棄、XID のモデル読出しが大きい。
正常値シミュレーションと正常 CNF の投入範囲限定を含む試作では、同じ limit30 条件で
s5378 は53.612秒から23.621秒になった。差の大部分は範囲限定を加えた段階で生じた。
従って limit 付き全故障処理には、被覆 BDD の細部より **故障間の共有と SAT 問題の構築範囲**が先に効く。

### 完全列挙では、平坦 SOP の出力サイズ限界が残る

[allsat_encoding](../allsat_encoding/SUMMARY.md) で示した
`AND_i (x_i OR y_i)` は、普通の PI 積項によるどの SOP も少なくとも `2^m` 項を必要とする。
最小等価 DNF の判定自体にも高い計算量がある
([Umans, JCSS 2001](https://doi.org/10.1006/jcss.2001.1775))。
したがって SAT の解順序や DC 判定だけを改善しても解消しない故障が存在する。
多値領域、因数分解、階層合成のように、平坦 SOP へ展開しない表現が必要になる。

### 一つの方式を全故障へ適用すると悪化しやすい

- weighted decomposition は s38584 の難4故障を同条件で22.156秒から10.089秒へ改善した。
- 同じ設定を分解のない s5378 `n291gat/sa1` に適用すると、従来は2.240秒で完了したものが3秒で未完了になった。
- 出力極性選択は s5378 の一故障では速かったが、全4,551故障では約111秒から123秒へ悪化した。
- 全故障を一つの solver に入れた既存 `FULL_MITER` は、s641 で3.7秒から9.7秒へ悪化した。

これは方式の無効性ではなく、適用対象を予測して費用を制限する必要があることを示す。

## 候補一覧

| 実装優先度 | 候補 | 主に減らすもの | 見込み | 費用・主なリスク |
|---:|---|---|---|---|
| 1 | 完成被覆の正規形キャッシュ＋SAT同値クラス | 独立に列挙する故障数 | 高。全故障規模へ直接効く | 同値候補の生成と証明費用 |
| 2 | 少数故障クラスタの incremental SAT | CNF投入、solver初期化、同じ衝突の再学習 | 高。limit30にも完全列挙にも効く | ブロッキング節・学習節の蓄積 |
| 3 | XOR/ITE/小さい `H` の階層合成 | 平坦な上位 SOP の指数展開 | 対象構造があれば非常に高い | 独立性・条件付き確率の証明 |
| 4 | 故障クラスタ単位の適応的手法選択 | 不適切な重い前処理と再試行 | 高確度の悪化防止 | 特徴抽出とパイロット費用 |
| 5 | 反例共有 implicit hitting set DC | DC判定回数、領域数 | 難故障で高い可能性 | 候補最適化のSAT費用 |
| 6 | SAT証明付き AND/OR/XOR bi-decomposition | 構造に見えない平坦SOP | 高い可能性 | 分割候補探索と複数コピーCNF |
| 7 | `C × S` 複数故障同時被覆 | 故障間で重なるテスト生成 | 研究上は最も大きい | 故障集合のDC拡大が高価になり得る |
| 8 | 反例学習型 ACD 状態抽象 | 上位へ渡る情報量と支持変数数 | 適合対象には高い | 状態数の増加、実装量 |
| 9 | 観測出力ごとの残余探索 | SAT伝搬パスの混在 | 中 | 出力数だけ最終UNSATが増える |
| 10 | 生成被覆BDDによる blocker 圧縮 | 大量禁止節の蓄積 | キューブが多い故障で中 | BDD更新・SAT連携の費用 |
| 11 | 残余CNFの動的成分分解 | 条件付きで独立になる部分の再探索 | 中〜低 | 原論文の部分割当てはDCキューブではない |

「見込み」はこのFDP実装に対する予測であり、既存論文がFDP速度を保証したものではない。

## 1. 完成被覆の正規形キャッシュと、検出関数の意味論的同値

### 方式

既存 `linear_scaling` の `shape_key` は、入力の置換・反転を除いて検出式 DAG を正確に直列化する。
同じ key の完全被覆は座標を逆写像して再利用できる。これを通常列挙だけでなく、因数分解後の葉、
weighted decomposition の上位関数、難故障の救済段階にも共通のキャッシュとして適用する。

次の段階では、構造 key は違うが同じ検出集合を持つ候補を拾う。

1. TFO、支持変数、出力到達集合、ビット並列シミュレーション署名で候補を絞る。
2. SAT で `D_f(x) XOR D_g(x)` が UNSAT か検査する。
3. 証明できた場合だけ、完成被覆と FDP を再利用する。
4. SAT の反例を候補群全体へシミュレーションし、同値候補を再分割する。

Dao らは、構造絞り込み、シミュレーション、SAT反例による故障同値クラス化を組み合わせ、
35K組合せセル・490K故障まで扱い、最大の OpenCores 群で平均60.3%の故障削減を報告した
([TCAD 2018](https://people.eecs.berkeley.edu/~alanmi/publications/2018/tcad18_fault.pdf))。
論文は故障回路の出力関数同値を対象とする。ここでは FDP に必要な `D_f` の集合同値へ条件を弱めるため、
拾える組が増える可能性がある。

### このリポジトリでの根拠

- s641 では既存の構造 key だけで、1,274故障の独立列挙が391関数まで減り、同じ試作内で
  13.451秒から4.758秒になった。さらにDC反例のシミュレーション利用を加えた値が3.843秒である。
- 今回、s38584 の難4故障 `g16349/sa1`, `II17661/sa0`, `g13329/sa1`, `II15893/sa0` を
  保存済み weighted 結果から再構成した。4件は上位186変数の exact `shape_key` と、入力反転を反映した
  186個の重み列がともに一致した。現在は各故障を個別に1,643領域ずつ列挙しているため、
  一つの完成上位被覆を3件へ写像できる見込みが高い。

後者は保存データの静的照合であり、キャッシュ適用後の速度測定ではない。実装時はハッシュ一致ではなく
直列化 key 全体、重み列、局所被覆の写像を比較し、変換後被覆を元回路CNFで検証する。
照合値は [shape_check.json](results/shape_check.json)、再確認用コードは
[check_saved_shapes.py](check_saved_shapes.py) に保存した。

### 最初の判定基準

s5378 全代表故障と s38584 全ステムを対象に、構造 key、意味論的同値証明、独立列挙の件数と時間を分離する。
同値検査時間を含めて総時間が下がり、完成被覆を再利用した全故障が元回路CNFと一致する場合に採用する。

## 2. 少数故障クラスタの incremental SAT

### 方式

同じゲートの sa0/sa1、同一ゲートの入力枝、または小さい共通TFOを持つ2〜8故障を一組にする。
検出側と非検出DCオラクル側の solver を分けて保持し、故障を selector assumption で切り替える。
故障 `f` だけの被覆禁止節は

`NOT selector_f OR NOT C(x)`

と条件付ける。正常回路CNF、共通故障コーン、故障に依存しない学習を再利用し、クラスタ終了時または
節数閾値で solver を作り直す。

Fey、Warode、Drechsler は、ゲート入力故障のクラスタで SAT instance と学習を再利用し、
通常ATPGのベンチマークで平均1.74倍を報告した。一方、全故障を一つの solver へ蓄積する方式は
メモリと時間を悪化させた
([VLSI Design 2007](https://agra.informatik.uni-bremen.de/doc/konf/07vlsiDesign.pdf))。
これは既存 `FULL_MITER` の悪化とも整合する。

### 実装上の注意

- solver に残す blocker は selector 付きにし、他故障の検出空間を削らない。
- DCオラクルへ発見済み被覆の blocker を混ぜない。
- 「正常変数だけを含む学習節」であっても、故障制約から導かれた可能性がある。
  由来を証明できない節を solver 間で裸のままコピーしない。
- クラスタサイズ1/2/4/8、節数、solver再構築間隔を比較し、SAT時間だけでなく全故障総時間で選ぶ。

## 3. XOR、ITE、小さい合成関数を展開しない階層被覆

現行 `factored_sop` は構造上の AND/OR を分離するが、XOR は葉として扱う。
現行 weighted 方式も局所非線形信号を確率付き変数に置き換えた後、任意の上位関数を再びSAT/DCで
平坦に列挙する。そのため、独立な4入力ANDを10個 XOR した既存人工例は、局所側を圧縮しても
上位に512領域を必要とした。

支持が互いに素な葉 `g_i` をSAT＋DCで被覆し、その厳密確率を `p_i` とする。上位が XOR なら

`P(XOR_i g_i) = (1 - PRODUCT_i (1 - 2 p_i)) / 2`

で合成でき、上位512領域は不要になる。ITE なら selector の支持が枝から独立であることを確認し、

`P(ITE(s, f1, f0)) = p_s P(f1) + (1-p_s) P(f0)`

とする。AND/OR/XOR/ITE以外でも、入力数が小さい上位 `H` なら、`H` の真理値表と葉の厳密確率から
加重和を取れる。各葉の確率は引き続き SAT＋DC＋生成被覆BDDから得る。

第一段階は構造に明示された XOR/ITE だけを対象とする。次に第6節のSAT証明付き意味論的分解を使う。
この方式は普通のSOPを速く出すのではなく、**指数個の積項へ分配展開しない**ことが目的である。

## 4. 故障クラスタ単位の適応的手法選択

SATzilla はインスタンスの安価な特徴から実行方式を選ぶ portfolio を構成し、SAT solver ごとの得意不得意を
利用した ([Xuほか, JAIR 2008](https://arxiv.org/abs/1111.2249))。FDPでは既存方式そのものを候補にする。

安価な静的特徴:

- 検出式の支持変数数、DAGノード数、観測出力数、TFOサイズ。
- AND/OR/XOR因子数、互いに素な支持を持つ候補cut数、共有入力separator幅。
- exact shape class の頻度、同じゲート周辺の故障数。

短いパイロット特徴:

- 最初の32または128キューブの生成速度、平均X率、入力組の併合利益。
- DC SAT呼出し数、非検出反例の再利用率、blocker投入後のsolve時間の傾き。
- 生成被覆BDDのノード増加と、領域を広い順に入れた場合の差。

最初から学習モデルを作る必要はない。閾値による段階選択でよい。

1. exact shape cache と軽量な通常方式。
2. 幅3多値領域または既知の因数分解。
3. XOR/ITE階層合成、weighted cut、IHS。
4. 未完了故障だけにSAT bi-decomposition、ACD、複数故障被覆。

目的関数はキューブ数ではなく、前処理、SAT/DC、BDD、失敗した先行試行を含む全故障の完全化時間とする。

## 5. 非検出反例を共有する implicit hitting set 型 DC

Primer-B は、SAT と hitting set を組み合わせて素項・素節をコンパイルする
([Previtiほか, IJCAI 2015](https://alexeyignatiev.github.io/assets/pdf/pimms-ijcai15b-preprint.pdf))。
全素項列挙はFDPには過剰なので、反例共有だけを借りる。

多値ブロック `B` の状態 `v` について、`e[B,v]` を「その状態を候補領域から禁止する」変数とする。
非検出反例 `z` が一度見つかると、どの健全な検出領域も `z` を少なくとも一ブロックで除外する必要があるため、

`OR_B e[B, z_B]`

を候補生成 solver に恒久追加できる。新しい検出 seed `m` では `e[B,m_B]=0` を仮定し、
seedを含む小さい禁止集合を求める。候補領域 `R` に対して `R AND NOT D_f` をSAT検査し、
SATなら反例を追加、UNSATなら領域を採用する。完全性の最終確認は従来通り `D_f AND NOT U` のUNSATで行う。

利点は、以前のキューブで失敗した拡大を次のキューブでも学習済み制約として使える点である。
初期試作では最小 hitting set や重いMaxSATを求めず、SATで小さい解を作る。多値領域では
「禁止状態数最小」と「確率質量最大」が一致しないため、後者は別の重み付き評価にする。

## 6. SAT証明付き AND/OR/XOR bi-decomposition と小さい共有集合

Lee、Jiang、Hung は、

`f(A,B,Z) = f_A(A,Z) op f_B(B,Z)`, `op in {AND, OR, XOR}`

の分解可能性を複数コピーのSATで判定し、変数分割も assumption と UNSAT core で探索する方法を示した
([DAC 2008](https://alcom.ee.ntu.edu.tw/assets/publications/dac08-bd.pdf))。
従来のBDD分解で問題になるメモリ爆発を避け、SATと補間を使う方向である。

FDPでは、まず構造グラフや保存済みパイロット被覆から少数候補だけ作り、最後に
`f XOR (f_A op f_B)` のUNSATで分解を証明する。補間器を最初から導入せず、固定したcofactorから
小関数を作って同値検査する軽量版から始められる。

共有入力 `Z` がある場合も `|Z| <= 3` または4に制限し、各 `z` の排他的条件枝で

`P(f) = 2^(-|Z|) SUM_z P(f | Z=z)`

と合成する。これは `Z` が独立な一様入力座標である場合の式である。非一様なら各状態の厳密確率を使い、
抽象変数間に相関があれば積で近似せず、局所SAT/DC被覆から必要な同時確率を求める。
グラフseparatorは候補選択にだけ使い、SAT同値証明を省略しない。

SATによる Ashenhurst 分解は300入力超の関数への実験も報告されている
([Lin, Jiang, Lee, ICCAD 2008](https://cecs.uci.edu/~papers/iccad08/PDFs/Papers/01B.2.pdf))。
ただし論文の論理合成での成功率はFDPの列挙時間改善を保証しない。未完了故障に限り、候補数、
共有幅、証明時間へ厳しい上限を置く。

## 7. 入力キューブと故障集合の直積 `C × S`

小さい故障クラスタに対し、故障番号 `f` を変数にした関係

`R(x,f) = D_f(x)`

を作る。SATモデル `(x,f)` から入力キューブ `C` と故障集合 `S` を交互に広げ、

`C(x) AND (f IN S) AND NOT R(x,f)`

がUNSATなら、`C` は `S` の全故障を必ず検出する。この一領域を一度だけ生成し、各 `f IN S` の
生成被覆へ登録する。検出側の禁止条件は `NOT C OR (f NOTIN S)` とする。

既存の支配流用は `D_g subseteq D_f` のような集合全体の包含を使う。この案は包含も同値もない故障間で、
交差部分だけを共有できる。multiple-target ATPG には、正常回路を一つ共有し複数の故障コーンを扱うSAT構成と、
テスト集合の圧縮効果が報告されている
([Czutroほか, DDECS 2012](https://agra.informatik.uni-bremen.de/doc/konf/12DDECS-MultipleTarget.pdf))。
ただし同論文は一つのテストで複数故障を狙う問題であり、全 `(x,f)` 関係の被覆ではない。
ここでの直積領域はFDP向けの未検証提案である。

表現としては、古典的なmultiple-output PLAの一行と同じである。入力部が `C`、出力部の1ビット集合が
`S` に対応し、Espressoも入力積項を複数出力で共有する形式を扱う
([Berkeley Espresso manual](https://people.eecs.berkeley.edu/~alanmi/research/espresso/espresso_5.html)、
[Rudell, Berkeley technical report 1986](https://www2.eecs.berkeley.edu/Pubs/TechRpts/1986/ERL-86-65.pdf))。
ただし既存の完全被覆を後から最小化するだけでは、そこへ到達するまでのSAT/DC回数を減らせない。
本案では `C × S` を最初からSATで生成し、故障出力方向にも領域を拡大する点が異なる。

最初は同一ゲート周辺の4または8故障に限定する。`|S|` の平均、故障ごとのSAT/DC呼出し削減、
関係CNFの追加費用を測る。完全割当てが複数故障を検出するだけでは不十分で、キューブ全域について
上のUNSAT証明が必要である。

## 8. 反例学習型 ACD 状態抽象

互いに素で確率的にも独立な入力座標集合 `X,Y` に対し、`X` から上位へ必要な情報を少数の関数
`G(X)=(g_1,...,g_r)` にまとめる。次を反復する。

`G(X)=G(X') AND f(X,Y) != f(X',Y)`

SATなら反例 `Y=y*` から新しい識別関数 `g_(r+1)(X)=f(X,y*)` を加える。UNSATなら、同じ `G` の値を
持つ二入力は任意の `Y` に対して同じ検出結果を与える。到達する状態 `j` だけについて

`f(X,Y) = OR_j ([G(X)=j] AND f(x_j,Y))`

と表せる。状態条件とcofactorをSAT＋DCで被覆し、状態は排他的なので

`P(f) = SUM_j P(G(X)=j) P(f(x_j,Y))`

と結合する。状態確率 `P(G(X)=j)` は局所SAT/DC被覆から求める。`g_i` は同じ `X` から作られて
相関するため、個々の確率を掛け合わせてはいけない。

これは Ashenhurst-Curtis 分解の「異なるcofactorを区別するのに必要な状態数」という考えを、
SAT反例で逐次構成する提案である。初期範囲は `r <= 3`、到達状態数8以下、候補支持24以下とする。
構造上は迂回路があって既存 weighted cut が棄却したが、意味論的には1〜3ビットだけで十分な場合を狙う。

## 9. 観測出力ごとの残余探索

検出関数を `D = OR_o D_o` とし、到達可能な観測出力ごとに `D_o AND NOT U` を解く。
各出力のCNFが小さくなり、異なる伝搬経路を一つのsolverで混ぜる費用を抑えられる可能性がある。
生成seedのDC拡大は必ず大域の `NOT D` をオラクルに使うため、得られるキューブは別出力へまたがってよい。
全 `o` がUNSATなら `D AND NOT U` もUNSATで完全である。

出力数が多いと最終UNSATを何度も解くため、観測出力をTFO重なりでクラスタ化する。
実装が比較的軽いので、CNF範囲限定後も出力ORが大きい故障の診断候補とする。

## 10. 生成被覆BDDによる blocker 圧縮

現在の blocker は生成領域ごとの節としてSAT solverへ蓄積する。代わりに
`U = 生成済み被覆の和集合` だけをBDDに保持し、`NOT U` をSAT探索へ与える。
これは回路の `D_f` をBDDにする方式ではなく、既に生成済みの被覆だけを圧縮する。

CaDiCaLには、solver外部の制約から伝搬と理由節を返す user propagator を接続できる
IPASIR-UP の実装例がある
([Fazekasほか, SAT 2023](https://kfazekas.github.io/papers/FazekasNiemetzPreinerKirchwegerSzeiderBiere-SAT23.pdf))。
ただし初期試作は外部伝搬器より、256〜1,024領域ごとに solver を作り直し、古いblockerをまとめた
BDDの `NOT U` から補助変数付きCNFを再構成する方式が安全である。

これは領域数を直接減らさず、大量blockerでsolve時間が右肩上がりになる故障だけに効く。
既存結果ではBDD投入順により時間が大きく変わったため、毎領域のBDD更新は逆効果になり得る。

## 11. 残余CNFの動的成分分解

AllSATCC は探索途中の残余CNFを独立成分へ分け、完了した成分をキャッシュする
([Liangほか, IJCAI 2022](https://www.ijcai.org/proceedings/2022/0259.pdf))。
FDPでは、少数入力を条件付けた後に分離する成分を検出し、各成分についてSAT＋DCで生成した局所被覆を
再利用する案になる。

原論文の partial assignment は、単位伝搬で復元可能な値を省略するSAT内部表現であり、
全補完が検出となる意味論的DCキューブとは限らない。その出力をそのまま被覆BDDへ渡してはいけない。
借りるのは成分検出とキャッシュだけにする。各葉は、その局所成分関数に対して健全性・完全性を証明する。
その後、条件枝と局所被覆を再結合した全体被覆について、元回路の `D_f` とCNFで等価検査する。
例えば `D_f=F_1 AND F_2` のとき、`F_1` の葉だけでは `D_f` を含意しないため、局所葉へ
`region AND NOT D_f` を直接要求してはいけない。

## 今回は優先しないもの

- **NNF＋Plaisted-Greenbaumだけへの変更**: CNF由来の過剰指定には効くが、既存の意味論的DCより
  実回路で一貫して速くならなかった。詳細は [allsat_encoding](../allsat_encoding/SUMMARY.md)。
- **公式TabularAllSATへの一律置換**: blockerを蓄積しない利点はあるが、選んだ難故障では既存多値方式を
  上回らなかった。短い部分割当ての意味論も現行DCと異なる。
- **全素項列挙、CoAPI、最小DNFの厳密最適化**: FDPは全素項も最小DNFも必要としない。
  出力が指数個になる対象へ余計な最適化問題を追加する可能性が高い。
- **全故障を一つのsolverに永久保持**: 既存 `FULL_MITER` とATPG文献の双方で、学習節・blockerの
  蓄積による悪化が確認されている。共有するなら小クラスタと再構築条件が必要。
- **AllSATCCの部分割当てをDCとして直接利用**: entailmentの保証がなく、FDPを過大評価し得る。
- **回路直結BDD/SDDとPCOUNT**: 今回の解法条件から外れる。treewidthやvtreeは分割候補の選択にだけ使える。

## 推奨する実験順

| 段階 | 実験 | 対象 | 採否を見る値 |
|---:|---|---|---|
| 1 | 全段階の exact shape cache、続いてSAT同値クラス | s5378全代表、s38584全ステム | 独立列挙数、証明費込み総時間、写像後CNF等価 |
| 2 | クラスタサイズ1/2/4/8のincremental SAT | s5378 limit30、s38584/b18固定標本 | CNF投入、solve、blocker数、総時間、peak memory |
| 3 | 構造XOR/ITE階層合成 | 既存XOR人工例、実回路のXOR/ITE含有故障 | 上位領域消滅数、葉SAT/DC込み時間、厳密FDP |
| 4 | 反例共有IHS | s5378・s38584・b19の難故障 | DC SAT呼出し、領域数、失敗反例再利用率、完全化時間 |
| 5 | SAT bi-decomposition、`|Z|<=3` | b19未完了故障、weighted cutなし故障 | 証明費、支持縮小、完全化率、総時間 |
| 6 | `C × S` 同時被覆 | 同一ゲート周辺4/8故障 | 一領域当たり故障数、故障当たり生成回数、関係CNF費用 |
| 7 | ACD状態抽象 | 上記で未完了、候補支持24以下 | 学習状態数、cofactor数、完全化時間 |

段階1〜3は、既存資産を使えて失敗時の損失が小さい。段階4〜7は独立試作として
`verification/` 内に置き、前処理を含む時間と元回路CNFによる健全性・完全性を同時に記録する。

今回の保存結果だけを再照合する場合は、リポジトリ直下で次を実行する。SAT/DC列挙や時間測定は行わない。

```bash
python3 verification/next_approaches/check_saved_shapes.py
```

## 参考文献の読み分け

- SAT bi-decomposition:
  [Lee, Jiang, Hung, DAC 2008](https://alcom.ee.ntu.edu.tw/assets/publications/dac08-bd.pdf)
- SAT Ashenhurst decomposition:
  [Lin, Jiang, Lee, ICCAD 2008](https://cecs.uci.edu/~papers/iccad08/PDFs/Papers/01B.2.pdf)
- incremental SAT ATPG:
  [Fey, Warode, Drechsler, VLSI Design 2007](https://agra.informatik.uni-bremen.de/doc/konf/07vlsiDesign.pdf)
- fault equivalence by structural pruning, simulation, and SAT:
  [Dao, Lin, Mishchenko, TCAD 2018](https://people.eecs.berkeley.edu/~alanmi/publications/2018/tcad18_fault.pdf)
- prime implicant compilation with implicit hitting sets:
  [Previtiほか, IJCAI 2015](https://alexeyignatiev.github.io/assets/pdf/pimms-ijcai15b-preprint.pdf)
- dynamic AllSAT component analysis:
  [Liangほか, IJCAI 2022](https://www.ijcai.org/proceedings/2022/0259.pdf)
- user propagators for CDCL/CaDiCaL:
  [Fazekasほか, SAT 2023](https://kfazekas.github.io/papers/FazekasNiemetzPreinerKirchwegerSzeiderBiere-SAT23.pdf)
- per-instance algorithm selection:
  [Xuほか, SATzilla, JAIR 2008](https://arxiv.org/abs/1111.2249)
- CNF変換と列挙:
  [Masina, Spallitta, Sebastiani, SAT 2023](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SAT.2023.15)
- 素項列挙の出力サイズ・表現依存性:
  [de Colnet, Marquis, IJCAI 2022](https://www.ijcai.org/proceedings/2022/358)
- 多値・multiple-output PLA の cube 表現と最小化:
  [Rudell, UCB/ERL 1986](https://www2.eecs.berkeley.edu/Pubs/TechRpts/1986/ERL-86-65.pdf)

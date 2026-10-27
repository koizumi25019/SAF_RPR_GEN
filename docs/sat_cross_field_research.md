# 分野を広げた追加調査: 証明、数値関係、厳密学習を使う完全被覆

調査日: 2026-10-22。
文献調査、既存実験記録との照合、説明用人工例の全数確認。
b15の性能実験、ソルバ導入、本体の変更は行っていない。

## 結論と前提

前回までの[被覆同時合成](sat_global_cover_research.md)と
[対称軌道](sat_symmetry_orbit_research.md)以外に、次の三方向を残した。

| 候補 | 借りる分野 | 何を変えるか | 現行の三段階との関係 |
|---|---|---|---|
| A: SAT証明からの領域生成 | 形式検証・Craig補間 | DCの出力を、証明から得るAND/OR論理式へ拡張 | SAT seed → 証明による一般化 → 生成式のBDD |
| B: ワード関係の合成 | プログラム合成・数値抽象解釈 | 加算、比較、マスクなどを使って検出領域を表す | SAT seed → 関係式の合成・検査 → 生成式のBDD |
| C: 非検出条件のHorn厳密学習 | 計算学習理論・論理知識表現 | 反例同士の交わりから被覆全体を学習 | SATの検出反例 → 候補キューブのDC検査 → キューブBDD |

A/Bは通常の0/1/Xキューブだけに出力を限定しない方式である。三段階を保ちつつ、
一回に生成する領域の表現を変える。Cは通常キューブを維持できるが、適用できる関数が限られる。
原論文はいずれもb15のFDP解法ではない。以下の接続と実験案は応用提案であり、改善実績ではない。

最も根本的な効果を狙う候補はB。Aはワード構造を必要としない探索的候補。
Cは、通常キューブを維持する条件付き候補である。

## A. SATの証明を使い、単一キューブを超える検出領域を作る

### 根拠となる研究

- K. L. McMillan, **Interpolation and SAT-Based Model Checking**, CAV 2003。
  [著者公開PDF](https://mcmil.net/pubs/CAV03.pdf)。
  UNSATのresolution証明から、共通変数上のCraig補間式を構成する。
- D'Silva, Kroening, Purandare, Weissenbacher, **Interpolant Strength**, VMCAI 2010。
  [出版社情報を含む大学ページ](https://www.cs.ox.ac.uk/publications/publication3308-abstract.html)、
  [拡張技術報告](https://lara.epfl.ch/w/_media/fv19/interpolantstrength.pdf)。
  同じ証明から得る補間式の論理的強さや、証明変換との関係を扱う。

### FDPへの接続

D(x)を検出関数とし、N(x,z)を内部変数z付きの正しい非検出CNFとする。
`EXISTS z N(x,z) = NOT D(x)` が必要である。
テスト生成SATが得た完全入力割当てをM(x)とすると、`N AND M` はUNSAT。

この証明を、A=N、B=Mと分割して補間式J(x)を得る。

`N => J`、`J AND M = false`

ここから `R = NOT J` とすれば、

`M => R => D`

が成立する。Rはキューブである必要がなく、AND/ORを含む式のままBDDへ登録できる。
次のテスト生成で `D AND NOT U` を解き、UNSATになれば全検出空間を覆っている。
計数対象はあくまで生成したRの和集合Uである。

既存のUNSAT coreによるDCは、残すPIリテラルを選び、単一キューブを返す。
この案の狙いは、証明から得られた論理式をキューブへ展開せず保持する点にある。
また、既提案のSAT bi-decompositionは関数全体の変数分割・因子抽出が目的であり、
今回のseedごとの領域生成とは探索単位が異なる。ただし論理式を因数分解して保持する効果は重なる。

### 最初に検証すべき懸念

「補間器をつなげれば広いRが自動で得られる」とは限らない。
特に、Aを入力のunit clausesだけにした素朴なMcMillan補間は、
用いられた入力リテラルのANDへ退化し、通常のcore DCと同様になる場合がある。
分割の向き、証明の作り方、補間系の選択を実験する必要がある。
上記のN側からJを作って反転する方法も、必ず非キューブ領域を返す保証はない。

初期診断は、同じseedについて以下を比較する。

- 通常DCキューブとRの新規被覆量。
- 証明生成・補間・簡約を含む時間。
- Rの論理式サイズとBDDノード数。
- `R AND N` の独立UNSAT検査。

Rがほぼ普通のキューブになる、証明が巨大になる、BDDが膨らむ場合は優先度を下げる。
最弱の正側補間式を無制限に求めればDそのものの構成に近づき、難しさが消えるわけではない。

## B. 0/1/Xの代わりに、数値・ビット列の関係を合成する

### 根拠となる研究

1. Hongce Zhang, Aarti Gupta, Sharad Malik,
   **Syntax-Guided Synthesis for Lemma Generation in Hardware Model Checking**, VMCAI 2021。
   [出版社](https://link.springer.com/chapter/10.1007/978-3-030-67067-2_15)、
   [著者ページ](https://hongcezh.people.ust.hk/publication/vmcai21/)。
   IC3/PDRの補題を、bit-vectorの文法と項生成規則を使って合成する。
   SATのビット単位の補題だけに頼らず、ワード単位の式を作る研究。
2. Antoine Miné,
   **Backward Under-Approximations in Numeric Abstract Domains to Automatically Infer Sufficient Program Conditions**。
   [著者公開原稿、2013年版](https://perso.lip6.fr/Antoine.Mine/publi/article-mine-SCP13.pdf)。
   区間・octagon・多面体を使い、結果を保証する十分な入力条件を後ろ向きに計算する。
   原稿の試作評価は小規模であり、そのまま大回路で有効という根拠にはしない。
3. Jiaqi Yinほか, **BoolE: Exact Symbolic Reasoning via Boolean Equality Saturation**, DAC 2025。
   [原稿・採録情報](https://arxiv.org/abs/2504.05577)、
   [本文](https://arxiv.org/html/2504.05577v2)。
   e-graphによる等価な論理式の書換えを使い、最適化済みゲートネットリストから
   full-adderなどの機能構造を抽出する。主な評価対象は乗算器であり、b15での成功は未確認。

### FDPへの接続

テスト生成SATは維持し、DC段階で次のような文法から領域Rを合成する。

`R = guard AND relation_1 AND ... AND relation_t`

候補relation:

- ワードと定数の比較、二つのワードの比較。
- 拡張したビット幅での加算と比較。
- マスク等式、slice間の等式。
- 従来の単一PIリテラル。合成に失敗した場合の通常キューブも表現できる。

各候補に対してseedを含むことを確認し、元回路SATで `R AND NOT D` がUNSATなら採用する。
SATなら非検出入力を合成器へ戻す。これは前回のk項DNF同時合成と同じく反例を利用するが、
探索する表現はk個のキューブではなく、一つの小さいbit-vector関係式である。
RだけをPI上のBDDへ変換して和集合へ登録する。

Minéの方式を借りるなら、検出出力条件から逆向きに数値的な十分条件を導く方法になる。
こちらも得られた候補の健全性を元回路SATで確かめる。原論文の整数・実数的演算を
固定幅回路へそのまま移すことはできず、wraparound・符号・桁上がりを正確に扱う必要がある。

### 既存predicate SOP・多値化との違い

`predicate_sop/SUMMARY.md`の実装は、主に2〜3入力の組と、最大6入力の局所述語である。
今回の対象は、例えば二つの8ビット入力を横断する加算・比較関係である。
2^16個の許容状態を列挙する代わりに、演算式として保持する。
同じ「述語による被覆」の延長ではあるが、候補の作り方と扱う構造が異なる。
既存の局所述語を多数追加して遅くなった実験からも、任意の演算を無制限に追加するのは避ける。

BoolEはこれ自体で列挙を減らす手法ではなく、使うべきワード・演算の候補を得る前処理候補。
単にゲートを少なくしても、同じ検出関数の最小通常キューブ数は変わらない。
既存のFFR・構造カットと比べるなら、元の回路形に現れていない演算を復元できるかを見る。

### 人工例: 8ビット加算の桁上がり

0〜255の符号なし入力a,bに対して、

`D = [zero_extend_9(a) + zero_extend_9(b) >= 256]`

を考える。通常キューブでは最低255個必要だが、この関係式なら1領域で表せる。
8ビットのまま加算するとwraparoundで別の式になるので、9ビットへ拡張する。

下限の理由: `a+b=256` を満たす255入力対を取る。
異なる二入力対を含む通常キューブは、そのビット単位のANDも含む。
このANDでは数値の和が256未満となるため、一つの健全なキューブで二点を覆えない。

上限の構成: 桁上がりを発生させる桁jのa_j,b_jをともに1とし、上位各桁では
aかbのどちらか一方を1に固定する。全jの項数は `1+2+...+128=255`。
このキューブ群で検出空間を完全に覆える。

今回の全数確認結果:

| 指標 | 値 |
|---|---:|
| 確認した入力割当て | 65,536 |
| 検出入力 | 32,640 |
| 通常キューブの最小被覆数 | 255 |
| 算術式で表した領域数 | 1 |
| 生成255キューブのORから作ったROBDDの非終端ノード | 23 |
| FDP | 255/512 |

BDD順序は下位桁からa_i,b_iを交互に配置した。
全入力で被覆と加算条件の一致を確認し、255点の全組合せで下限の条件を確認した。
BDDの計数も32,640と一致した。これは表現力の人工例であり、
SyGuSがこの式を自動発見した実験でも、b15での高速化測定でもない。

### b15との接点

現物の `input/circuit/saf/b15_C.v` には、DATAI_の32入力、ADDRESS_REG_の30入力、
DATAWIDTH_REG_の32入力、INSTQUEUE_REG_の128入力という名前のまとまりが残っていた。
名前はワード候補の手掛かりになるが、演算・ビット順・故障検出との関係を証明したものではない。
全回路のRTL復元から始めず、難しい故障のコーンに限り加算・比較候補を調べるのがよい。
scan入力の到達可能性を仮定して入力空間を削ることは、今回の一様なPI空間の計数とは別問題である。

## C. Horn論理の厳密学習で、通常キューブの被覆を求める

### 根拠となる研究

Dana Angluin, Michael Frazier, Leonard Pitt,
**Learning Conjunctions of Horn Clauses**, Machine Learning 9, 1992。
[出版社](https://link.springer.com/article/10.1007/BF00992675)。
membership queryとequivalence queryを使い、Horn CNFを入力数と目標の節数に対して
多項式の学習計算・問い合わせで同定する。反例の交わりを使って仮説を更新する点が特徴。
J. L. Balcázarの[著者公開再解析](https://www.cs.upc.edu/~balqui/h2004b.pdf)も確認した。

### FDPへの接続

非検出関数 `N(x)=NOT D(x)` が小さいHorn CNFで表せる故障を対象にする。
例えば `a AND b => c` はHorn節 `NOT a OR NOT b OR c` であり、
その否定 `a AND b AND NOT c` は通常の検出キューブになる。

`N = AND_i clause_i` を正確に学習できれば、

`D = OR_i (NOT clause_i)`

であり、そのまま既存のキューブBDDへ渡せる。
これは非検出の全パターンや非検出キューブを列挙して数える方式とは違う。

オラクルを既存処理へ対応付ける:

- membership: 指定した完全PI入力が非検出か、正常・故障回路シミュレーションで答える。
- 仮説Hが非検出入力を取りこぼしていないか: `N AND NOT H` のSAT。
  反例は、否定した候補キューブが誤って含めた非検出入力に相当する。
- 未被覆の検出入力が残っていないか: `D AND H` のSAT。
  検出パターン生成と同じ役割になる。

両者がUNSATならH=Nで完全。途中の仮説の節は正しいとは限らないので、
その否定を無検証で確定被覆BDDへ蓄積してはいけない。
仮説の修正に合わせて候補を管理し、確定登録する領域は個別にDC検査する。

前回の一般的なk項DNF合成との違いは、最小被覆の大きなSAT最適化を解く代わりに、
Hornの交わり構造を使った専用の更新則を使う点にある。
ただし最小項数を求めるアルゴリズムではなく、30項以下で終わる保証もない。

### 適用可能性を先に判定できる

Boolean関数がHorn CNFで表せることと、そのモデル集合がビット単位ANDで閉じることは同値。
独立した入力u,vの回路コピーを使い、次を調べる。

`N(u) AND N(v) AND D(u AND v)`

SATなら、その二点はHorn性への反例。UNSATならNはHornで表現できる。
これは内部変数付き回路CNFの見た目がHornかどうかの判定とは異なる。
PIの固定した反転候補について同じ判定を試すこともできるが、全反転を探索しない。

学習理論の保証は、SAT問い合わせを1回と数えるオラクルモデル上のもの。
各同値検査のSAT費用が多項式になるわけではない。また、Hornでも必要な節数が巨大なら効かない。
従って、未完了故障の標本でHorn性・少数節への収束を先に診断する。

## その他の分野を調べた結果

| 分野・一次資料 | 今回の位置付け |
|---|---|
| 制約獲得: Bessiere, Koriche, Lazaar, O'Sullivan, **Constraint Acquisition**, AIJ 2017 ([大学](https://research.ucc.ie/en/publications/constraint-acquisition/)) | 正負例から制約を獲得する方法は候補生成に使える。ただし候補制約言語に依存するため、一般的な制約獲得をそのままFDPの完全化保証とは扱わない。Bの文法選択・Cの学習と関連する。 |
| 一般Boolean関数学習: Bshouty, **Exact Learning Boolean Functions via the Monotone Theory**, 1995 ([出版社](https://www.sciencedirect.com/science/article/pii/S0890540185711649)、[原報告](https://eccc.weizmann.ac.il/eccc-reports/1995/TR95-008/index.html)) | 学習計算は最小DNFサイズと最小CNFサイズの双方に依存し、出力も深さ3の式になる。「小さいCNFだから効率よく普通のキューブを列挙できる」という結果ではない。今回はCの方が導入条件を明確にできる。 |
| パラメータ化計算量: Williams, Gomes, Selman, **Backdoors To Typical Case Complexity**, IJCAI 2003 ([著者PDF](https://www.cs.cornell.edu/selman/papers/pdf/03.ijcai.backdoors.pdf)) | 少数の変数を固定してSATを容易にする発想。ただし容易なSATクラスでも通常キューブ被覆は指数的になり得る。単純分割・動的成分分解とも重なるため、列挙数の根本対策としては優先しない。 |
| 知識コンパイル・BDD学習 | 回路の全検出関数を直接コンパイルして数える方式は、今回の条件から外す。生成された領域をBDDへまとめる用途に限定する。 |
| データマイニング・ルール抽出・XAI | 短い説明、素項、hitting setの生成は既試行のsemantic DCやIHSに重なる。有限標本上で高精度でも全入力の厳密被覆にはならず、必ず元回路SATの検証が必要。 |

## 既存記録との照合と推奨する実験

`verification`ブランチの `predicate_sop`、`weighted_decomposition`、`factored_sop`、
`allsat_ffr_research`、`next_approaches`、会話実験要約と照合した。
補間による分解・局所述語・学習による入力組選択は既出であり、それら自体を新規とは扱わない。
今回のAのseed単位の証明式被覆、Bのワード演算のSyGuSと構造復元、CのHorn厳密学習を
比較実験した記録は、確認した要約では見つからなかった。

実装の前に、同じ未完了故障の固定標本で次を調べる。

1. B: 加算・比較などの小さい関係式候補が、元回路SATで広い検出領域として証明できるか。
2. A: 同じseedのDC証明から、普通のキューブより広い、BDD化可能なRが得られるか。
3. C: 非検出関数がHornまたは試した固定反転のHornか。小さい節集合へ収束するか。

A/Bでは領域数と通常キューブ数を同一視しない。前処理、候補合成、失敗試行、
証明、BDD和集合を含む同じ時間・メモリ予算でcomplete数を比較する。
生成領域の健全性と `D AND NOT U` のUNSATだけを完了の根拠にする。
BDDが最終的に巨大になる場合や有効な構造がない場合を、この提案だけで解消できるとは言えない。

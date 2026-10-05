# SOP/DNF列挙の限界とAllSAT論文の追試

実施日: 2026-09-12。コード・取得した公式ソース・ビルド・生成被覆・結果はこのディレクトリ内。
本体と過去の実験結果は変更していない。

## 結果

**CNF変換だけで解消する爆発と、最小SOP自体が大きい問題は別である。**
前者に対するNNF＋Plaisted–Greenbaum（PG）変換と、禁止節を蓄積しない公式TabularAllSATを実装・接続して比較した。
今回の実回路5故障・計60条件では、完了した比較で既存のSAT＋意味論的DC＋多値領域方式を上回るものは得られなかった。
この限定的な結果からAllSAT手法一般が無効とは結論しない。全故障・他ベンチマークでの優劣は未検証。

一方、人工例ではNNF＋PGによる改善を再現した。補助変数を固定するDCだけだと6,305キューブだったものが29キューブになった。
ただし、既存の意味論的DCなら両CNFで8キューブだった。現行方式が既に回避している制約が大きい。

## 探していた論文の候補と読み分け

### 1. CNF変換が短いキューブを妨げる問題

Masina, Spallitta, Sebastiani,
[On CNF Conversion for Disjoint SAT Enumeration, SAT 2023](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SAT.2023.15)。
拡張版は [On CNF Conversion for SAT and SMT Enumeration, JAIR 2025](https://doi.org/10.1613/jair.1.16870)
（[著者公開稿](https://disi.unitn.it/rseba/papers/JAIR25-cnfenum.pdf)）。

Tseitinの双方向定義によって、本来不要な部分回路まで値を決めることになる問題を扱う。
PGだけでは両極性で現れる部分式に問題が残り、NNFへ変換して正負のノードを分離する。
「CNFだから短い部分割当てを生成しにくい」という従来の議論に最も近い候補。
これは**あらゆる検出集合に短いSOPが存在するという定理ではない**。

### 2. 全素項を出す問題と出力サイズの限界

de Colnet, Marquis,
[On the Complexity of Enumerating Prime Implicants from Decision-DNNF Circuits, IJCAI 2022](https://www.ijcai.org/proceedings/2022/358)
（[全文・証明](https://www.cril.univ-artois.fr/~marquis/deColnet-marquis-ijcai22-full-proof.pdf)）。

一般の回路では素項の生成自体が難しく、素項数も指数的になりうる。
一方、入力が既にdec-DNNFなら全素項列挙に出力多項式時間・増分多項式時間の結果がある。
「列挙は常に不可能」という論文ではない。**何の表現から何を列挙するか**が条件になる。
また、出力多項式時間でも出力自体が指数サイズなら全体は大きい。

Mossé, Sha, Tan,
[A Generalization of the Satisfiability Coding Lemma and Its Applications, SAT 2022](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SAT.2022.9)。
固定幅CNFの素項数の上界と全素項生成を扱う。
回路を符号化したCNFの全変数上の素項と、内部変数を射影したPI上の検出キューブは対象が違うので、直接置換しない。

### 3. 最小SOPを求めることの難しさ

Umans,
[The Minimum Equivalent DNF Problem and Shortest Implicants, JCSS 2001](https://doi.org/10.1006/jcss.2001.1775)。
最小等価DNFの判定問題のΣ₂ᴾ完全性を示す。最短素項も入力表現に依存する困難さを持つ。
この結果を「故障ごとの有用な短い被覆を見つけるヒューリスティックは無理」とは解釈しない。

区別すべきタスクは、(a)全ミンターム、(b)全素項、(c)集合を覆うSOP一つ、(d)最小SOP。
FDPの現行手法が必要とするのは(c)で、(b)や(d)の達成は必須ではない。
各項をこれ以上広げられない素項にしても、被覆全体の項数最小性は保証されない。

### 4. 現在のDCがNNF＋PGより強い場合の説明

Sebastiani,
[Entailment vs. Verification for Partial-Assignment Satisfiability and Enumeration, CADE 2025](https://link.springer.com/chapter/10.1007/978-3-031-99984-0_37)。

局所評価で真になるverificationと、全補完が真になるentailmentを区別する。
補助変数を存在量化したCNFでも違いが生じ、CNF変換の工夫だけではその違いを消せない。
この整理は今回の人工例・実回路結果と整合する。
Friedらの [Entailing Generalization Boosts Enumeration, SAT 2024](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SAT.2024.13)
はHALLで後者を使う実装研究で、現在の非検出SATオラクルと同じ方向にある。

### 5. 禁止節を蓄積しないAllSAT

Spallitta, Sebastiani, Biere,
[Disjoint Projected Enumeration for SAT and SMT without Blocking Clauses, AIJ 2025](https://cca.informatik.uni-freiburg.de/papers/SpallittaSebastianiBiere-AIJ25.pdf)。

CDCL、時系列バックトラック、部分割当て縮約を組み合わせるTabularAllSAT。
今回は[公式実装](https://github.com/giuspek/tabularAllSAT)を取得して実行した。
§4.4では極小性を保証しないこと、§4.5では単純に重複を許すよう変更すると停止性を失う例が示される。
そのため、現在の自由な素項拡大だけをそのまま挿入する改造は行わない。

Möhle, Sebastiani, Biere,
[On enumerating short projected models, DAM 2025](https://cca.informatik.uni-freiburg.de/papers/MoehleSebastianiBiere-DAM25.pdf)。
双対推論、射影、重複の有無、禁止節なしの列挙を形式化している。
短い禁止節には、広い領域を除外することと伝搬を軽くすることの両方の意味がある。
したがって、禁止節をなくす方法が常に優位とは限らない。

### 6. 「DNFのモデル列挙」は今回と向きが違う

Capelli, Strozecki,
[Enumerating models of DNF faster: breaking the dependency on the formula size](https://arxiv.org/abs/1810.04006)
（DAM 2021、参照版は訂正を含むarXiv v3）。
既に与えられたDNFからモデルを出す遅延を研究する。回路から小さいDNFを作るアルゴリズムではない。
全ミンターム生成へ戻るとFDPには不利なので、今回は実装対象にしなかった。

Darwiche, Marquis,
[A Knowledge Compilation Map, JAIR 2002](https://arxiv.org/abs/1106.1819)
も、表現の小ささと可能な操作が別の軸であることの整理に有用。
本実験では回路をBDDやd-DNNFに直接コンパイルして数える手法は採用していない。

## 最適SOPでも指数になる例と検証

独立な入力対に対して

`F = (x1 OR y1) AND ... AND (xm OR ym)`

を考える。各対の01/10だけからなる2^m個の入力はすべてFを満たす。
Fの任意の積項は、各対で少なくとも一方を1に固定する必要がある。
したがって一つの積項で覆える上記入力は高々一つであり、**どのSOP被覆も少なくとも2^m項必要**。
分配展開すれば2^m項で書けるので、この下界は達成される。
これはSATの遅さ、DCの弱さ、禁止節の蓄積から独立した出力サイズの限界である。

今回m=8で実測した。

| 方法 | 完了までの項／領域数 | 厳密FDP |
|---|---:|---:|
| Tseitin＋意味論的DC、普通のPIキューブ | 256 | 6561/65536 |
| NNF＋PG＋意味論的DC、普通のPIキューブ | 256 | 6561/65536 |
| 2入力多値領域＋SAT/DC | **1** | 6561/65536 |

多値領域は各対に`{01,10,11}`を許す積。1領域を普通のPI積項に戻せば256項に展開される。
通常のSOPの下界に反するのではなく、平坦なSOPとは別の表現で保持している。
16入力の全65,536割当てを独立列挙し、生成領域の和集合が真理値表と完全一致することを確認した。

逆向きの人工例`G = OR_i (xi AND yi)`でも比較した。
補助変数固定の縮約はTseitinで6,305項、NNF＋PGで29項。
非検出SATを使う方式はどちらも8項。この結果からCNF変換と意味論的DCの効果を分離できる。

## 実装

- `encoding.py`: AND/XORのDAGから正負を共有するNNFを作り、片方向PG節へ変換。
  XORを真理値表へ全面展開せず、二項XORの正負DAGを共有する。
- `nnf_pg`: `selector -> F`と`!selector -> !F`を両方符号化し、二つのSATソルバで共有。
  **片方向PGの出力を単に否定して非検出としてはいけない**。
- `nnf_pg_split`: 正負それぞれの依存閉包だけを検出側／非検出側へ渡す追加切り分け。
  多値状態の変数IDは両側で一致させる。
- `build.py`: 既存のnative列挙器を`build/`へコピーし、固定した補助変数の下で各節の充足を維持する
  PI縮約を追加。節ごとの真リテラル数と出現リストで1回の走査にする。
  `witness`単独と、それを意味論的DCの初期キューブにする`seed`を比較。
  禁止節を縮約判定へ入れないので重複被覆を許す。
- `tabular.py`: 公式TabularAllSATの射影入力と出力を接続。
  元PIをprojection対象に指定し、生成キューブの和集合を既存BDDで数える。
  公式のモデル数表示は照合に使うだけで、FDPの計算元は生成被覆。
  一方向の列挙器なので、NNF＋PGは正側だけを渡す。
- `verify.py`: ランダム小式・定数・正負出力の射影検査、c17aゴールデン、独立元ゲートCNF検証、人工例の全数検査。

PCOUNT、外部モデルカウンタ、回路からの検出BDD構築は使っていない。
列挙器はSAT＋DCで動作し、BDDは生成済み被覆だけに使う。

## 実回路の比較

同じ前処理後の故障論理関数に対し比較した。表の時間は**列挙処理**で、BDD・検証・共通ネット読込を含まない。
ネイティブ方式は3秒の列挙制限、Tabularは起動・入力読込・キューブ書出しを含む3秒の外部制限。
Tabularのキューブ書出しは逐次、既存nativeは列挙後に一括出力する違いもある。
`results/comparison.csv`にはBDDを含む`total_seconds`も保存する。

| 故障 | 現行・多値3入力 | NNF＋PG・多値3入力 | 正負分離PG・多値3入力 | 公式Tabular＋NNF＋PG |
|---|---:|---:|---:|---:|
| s5378 n673gat sa0 | 620領域 / 0.148秒 | 732 / 0.255秒 | 816 / 0.299秒 | 3秒未完了 |
| s5378 n1592gat sa0 | 915 / 0.204秒 | 1,045 / 0.289秒 | 1,015 / 0.346秒 | 37,742 / 2.069秒 |
| s5378 n291gat sa1 | 1,471 / 0.895秒 | 1,645 / 1.483秒 | 1,519 / 1.212秒 | 3秒未完了 |
| s38584 g16349 sa1 | 3秒未完了 | 3秒未完了 | 3秒未完了 | 3秒未完了 |
| b19 P2_P1_P1_U3002 sa0 | 3秒未完了 | 3秒未完了 | 3秒未完了 | 3秒未完了 |

s38584の行は**構造から固定選択した組**の比較。
前回完成した「試験被覆からの組学習＋広い領域からBDD投入」の結果を取り消すものではない。
未完了条件のキューブ数が少ないことは高速化の証拠として扱わない。

通常PIキューブ同士では、n291gatでNNF＋PG＋seedが5,609→4,090項、2.028→1.751秒になった。
ただし上表の既存多値方式より遅く、他の2完了故障でも一貫した改善がないので一律採用しない。
公式Tabularのn1592gatでは、Tseitinは3秒未完了、NNF＋PGは完了となり符号化の効果自体は確認できる。
その場合もFDPまで約2.752秒で、現行多値の約0.258秒を上回れなかった。

## 検証と限界

- 小さな混合AND/XOR式12個×64入力。PGの両極性でSAT/UNSATを直接照合した。
- c17aの36故障×基本8条件=288実行がゴールデン一致し、元回路CNFと被覆が等価。
- 正負分離PGの72実行、公式Tabularの72実行も同様に一致・等価。合計432実行。
- 人工例2種類×8条件=16実行を元の16入力真理値表で全数検査。
- 実回路60条件のうち完了した25条件はすべて元ゲートCNFとの等価性を独立証明。
  Tabularの完了被覆は、BDDの和集合体積と各キューブ体積の和も一致し、非重複性を確認した。
- 本体ソースは変更していない。今回の検証は試作に対する独立CNF検証で、本体のGT_BDD実験を新規に再実行したという意味ではない。

探索的な単回測定で、一部の検証と計測は並行した。微小な速度差の有意性は主張しない。
上表は5個の選んだ難故障であり、全回路・全故障のベンチマークではない。TDFも未検証。

## 次に優先する方向

1. 現在のentailmentベースDCを維持する。高速な局所判定だけへ戻すと回数が増える。
2. 平坦なPI-SOPへの展開を避け、多値入力組・独立因子を保持する。
   上の下界が現れる関数族では、列挙器の高速化よりも表現の変更が必要になる。
3. 入力組学習は未完了故障へ限定し、異なる故障間の同形な部分問題の厳密な再利用と組み合わせる。
   これは前回までの結果と今回の比較からの方針であり、新しい全故障高速化を実証したという意味ではない。

## 再現

既存の`verification/linear_scaling/build.sh`でSAT・被覆BDD用バイナリを用意した状態で、リポジトリ直下から:

```bash
python3 verification/allsat_encoding/build.py
python3 verification/allsat_encoding/verify.py
python3 verification/allsat_encoding/run.py
python3 verification/allsat_encoding/split_pilot.py
python3 verification/allsat_encoding/setup_tabular.py
python3 verification/allsat_encoding/tabular.py
python3 verification/allsat_encoding/report.py
```

`setup_tabular.py`だけ初回ネット接続が必要。公式ソースは
`ad4a071310581990b7834f1f076ccfb54d592bbf`へ固定し、変更せず使用する。
`vendor/`, `build/`, `runs/`は中間物。集約データ・検証結果は`results/`。
[全60条件のCSV](results/comparison.csv)、[公式ソース記録](results/tabular_source.json)、
[人工例の検証](results/families_verification.json)を参照。

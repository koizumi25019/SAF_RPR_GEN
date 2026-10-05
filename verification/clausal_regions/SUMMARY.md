# 共通CNF因子を残すSAT/DC被覆

実験整理日: 2026-09-17。先行する探索実験を保存し、後続のFFR試作と区別する。
本体未統合、SAFステム故障のみ。回路直結BDD・PCOUNTは使用しない。

## 方式

PIキューブの短縮だけでは、例えば `AND_i(x_i OR y_i)` の最小SOPが `2^m` 項必要という
表現上の限界を越えられない。そこで最大6入力の局所述語を用い、次の形で検出関数を保持する。

```
D = H AND (Q1 OR ... OR Qn)
H = C1 AND ... AND Ck
```

各Cは局所述語上の1〜2リテラル節、Qは述語上のキューブ。
`D AND NOT C` のUNSATをSATで証明し、検出入力が必ず満たす共通CNFを先に取り出す。
次に `H AND Qi AND NOT D` のUNSATを確認しながらQiのDC拡大を行う。
検出ソルバ上では `D⇒H` のためQiだけの禁止節を追加してよい。
最後に検出ソルバがUNSATなら完全。`H AND NOT D` 自体がUNSATなら残余は空キューブ1個でよい。

これは既存論文のアルゴリズムをそのまま実装したものではなく、
因数分解と意味論的DCを組み合わせた独立試作。
[SAT 2024 Entailing Generalization Boosts Enumeration](https://drops.dagstuhl.de/storage/00lipics/lipics-vol305-sat2024/LIPIcs.SAT.2024.13/LIPIcs.SAT.2024.13.pdf)
は意味論的なキューブ拡大を扱う。ここではその一般化判定の外側に共通条件Hを残す。
構造上に現れない分解を探索する先行研究として
[SAT-based Boolean bi-decomposition](https://alcom.ee.ntu.edu.tw/assets/publications/dac08-bd.pdf)
も関連するが、その分割探索は今回未実装。

`--implicates all` は全1〜2リテラル節（述語数kに対して2k²候補）をSAT検査する。
`--implicates propagation` はDを仮定した単位伝搬と、各述語の正負仮定下の伝搬で候補を得て、
それを改めてSAT検査する。全ての意味論的含意を見つける保証はないが、正しさは損なわない。

カウンタは生成された `H AND Qi` の和集合だけをBDDにする。
述語の真理値表は最大6変数。回路の全検出関数をBDD入力に渡さない。

## 得られた結果

| s5378故障 | 保存済み多値幅3の領域数 | 伝搬版の領域数 | 共通節数 | 伝搬版時間 |
|---|---:|---:|---:|---:|
| n2428gat/sa0 | 3,011 | 37 | 11 | 0.0568秒 |
| n2432gat/sa0 | 1,910 | 33 | 14 | 0.0521秒 |
| n2426gat/sa0 | 1,889 | 33 | 14 | 0.0542秒 |
| n740gat/sa0 | 2,115 | 1,291 | 113 | 3.401秒 |

上記4件は元ゲートCNFにより被覆の健全性・完全性を証明し、厳密有理数が保存済み値と一致。
単回の探索測定。ベースラインの結果は `baseline/*.combined.json`、新方式は `runs/*_prop_w6.result.json`。
多値版は214座標で、共通CNF版は支持座標圧縮・幅6述語を使い、前処理の測定範囲も異なる。
従ってこの表は新しい領域表現の有効例であって、Hの有無だけを変えた速度比較ではない。

負の結果もある。

- n1592gatの全候補版は373領域だが、伝搬版は1,953領域・1.974秒へ悪化。
  既存の多値版915領域、補集合版76領域に一律に勝つわけではない。
- n673gat、n291gatの全候補版は1,140／4,586領域。以前の入力組手法より大きい。
- s38584 g16349は全候補版で7,997領域・39.056秒となり、既存の1,940領域や
  weighted版1,643領域に及ばない。被覆は原回路CNFと等価だった。
- 全候補走査のO(k²)のSAT費用は大きい。伝搬候補だけでは良い共通節を逃す場合がある。
- `clausal.py` の汎用CNF領域のcore-only拡大も試したが、人工例で被覆選択が悪く、主方式にしない。

後続の[FFR経路分解](../ffr_decomposition/SUMMARY.md)は、共通条件を構造から直接取り出す方法。
n2428gatで6因子・合計64キューブとなり、37個のCNF領域とは単位が違うが、
両者とも積の条件をPI-SOPへ平坦化しないという根本方針を共有する。

## 検証・実装範囲

- c17aの24ステム故障でゴールデンFDP一致、元ゲートCNFに対する独立被覆等価性。
- 定数検出関数0／1の2例、空被覆・空領域・恒偽述語の正負を含むカウンタ4境界例。
- `verify.py` は元ゲートの真理値表からCNFを再構築し、変換座標の線形独立性も確認。
- 伝搬版の上表4故障と、全候補版のn673、n1592、n291、g16349等の証明を保存。
- `formula_union.cc` のmask=0述語を許容する修正と、定数検出関数での空述語集合への対応を実施。
- 時間制限は残余列挙部分に対する制限であり、含意抽出全体の厳密な壁時計上限ではない。

```bash
bash verification/clausal_regions/build.sh
python3 verification/clausal_regions/regression.py
python3 verification/clausal_regions/factored_cnf_sop.py \
  --net input/circuit/s5378_C.v --fault n2428gat --stuck 0 --width 6 \
  --implicates propagation --seconds 3 \
  --prefix verification/clausal_regions/runs/reproduce_n2428
python3 verification/clausal_regions/verify.py \
  --net input/circuit/s5378_C.v \
  --prefix verification/clausal_regions/runs/reproduce_n2428
```

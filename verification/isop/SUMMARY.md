# キューブ被覆の表現限界調査: ISOP オラクルと short-implicant 実験

実施日: 2026-07-20 / ブランチ: `verification`

## 問い

現行フローが大量のキューブを生成する原因を、次の2つに分離する。

1. XID/列挙順が悪く、大きく重複するキューブを選んでいる。
2. 検出関数 `D_f` を平坦な DNF（キューブの和）で表すこと自体が大きい。

前者なら素項生成・被覆選択の改善が効く。後者なら AllSAT エンジンを交換しても定数倍改善に留まり、
FDP の目的には DNF を実体化しない projected model counting / knowledge compilation が必要になる。

## 調査した手法

- Yu et al., *All-SAT Using Minimal Blocking Clauses* (VLSID 2014): SAT解を素項へ拡大し、重複を許す
  minimal blocking clause で非disjointカバーを列挙する。現行 `MAXDC_QX` と原理的に近い。
- Toda et al., *AllSAT for Combinational Circuits* (SAT 2023, HALL): ternary simulation の TALE、
  dual-rail/MaxSAT で短い implicant を探す MARS。既存の実測比較は `verification/pcount/SUMMARY.md`。
- Spallitta et al., *Disjoint Projected Enumeration without Blocking Clauses* (AIJ 2025): chronological
  backtracking と implicant shrinking で blocking clause を避ける。ただし出力は disjoint であり、
  `n673gat` のように disjoint path が巨大な故障の出力サイズ問題は解消しない。
- Minato-Morreale ISOP: BDDから prime かつ irredundant なSOPを構成する。minimum SOPではなく
  変数順依存なので下界ではないが、「SAT列挙と独立に作った大域被覆」として診断に使える。
- Umans, *The Minimum Equivalent DNF Problem and Shortest Implicants* (JCSS 2001): minimum equivalent DNF は
  `Sigma_2^P` 完全であり、任意の回路に対して少数キューブを保証する一般解は期待できない。

## 導入した検証機能

### `GT_ISOP=1`

`src/fdp/gt_verify.c` に CUDD の `Cudd_zddIsop(D_f,D_f)` を使う独立診断を追加した。
検出関数BDDからISOPをZDDとして構築し、項数、ZDDノード数、現行列挙数との比を出す。
返されたBDDが `D_f` と完全一致することも検査する。通常実行は不変。

```bash
cd build
GT_ISOP=1 GT_BDD=1 ./main_release -set ../input/script/c17a.set
```

出力例:

```text
[GT_ISOP] n673gat,sa0 enum=1000 complete=0 isop=60955 ... exact=1
```

### `MAXDC_QX_MULTI=N`

HALL/MARS の short implicant の着想を、外部MaxSATなしで試す軽量実験。QuickXplain のリテラル順を
決定的にシャッフルして `N` 個の集合極小素項を生成し、最短を採用する（最大32）。これは
最小リテラル数素項の厳密解ではない。

```bash
MAXDC=1 MAXDC_QX=1 MAXDC_QX_MULTI=8 GT_ISOP=1 ./main_release -set <set-file>
```

## 結果

| 回路・故障 | 現行/既知の列挙 | `MAXDC_QX` | ISOP項数 | 判断 |
|---|---:|---:|---:|---|
| c17a 全代表故障（合計） | 75 | - | 53 | 小規模でも大域被覆に29.3%の余地 |
| b12 `MEMORY_REG_0__0__SCAN_IN/sa0` | 521,746 | 1（既存結果） | 1 | 爆発は列挙戦略由来、MAXDCで解決済み |
| b12 `COUNT_REG2_1__SCAN_IN/sa0` | 224 | 12 | 11 | QXはISOPにほぼ到達 |
| s5378 `n1080gat/sa0` | CORE 32（既存結果） | 1 | 1 | QXで解決 |
| s5378 `n1148gat/sa0` | CORE 18（既存結果） | 1 | 1 | QXで解決 |
| s5378 `n673gat/sa0` | CORE 74,056 | - | **60,955** | 平坦DNF自体が数万項級 |

`n673gat/sa0` の1000キューブ打ち切り比較:

| 方法 | FDP（部分被覆） | shorter prime | wall |
|---|---:|---:|---:|
| `MAXDC_QX` | 2.9410585462e-02 | - | 4.627s |
| `MAXDC_QX_MULTI=8` | 2.9410585462e-02 | 0 / 1000 | 28.502s |

多始点化は被覆を1ビットも改善せず6.2倍遅いため、採用候補から外す。スイッチは再現・追加検証用に
default-offで残す。

ISOPは minimum SOP ではないので「60,955が数学的最小」とは主張しない。しかし同じ故障に対し、
HALL TALE=70,717、HALL ROC=69,350、現行CORE=74,056、独立ISOP=60,955と異なる4方式が同じ
数万項帯に集まる。したがって、単なる列挙順の不具合ではなく DNF 表現限界だという強い証拠になる。

## 結論と方針

「この手法が全面的に無理」ではない。検出関数が少数素項で表せる故障には `MAXDC_QX` が非常に有効で、
実際に52万項を1項へ落とせる。一方、`n673gat` 型を少数キューブで**完全被覆する**要求は、現在の
SAT→素項→blocking clauseという枠内では難しい。短い素項探索を強化しても、大域被覆の項数は消えない。

FDPの厳密値が目的なら、故障ごとに次のエスカレーションを採るのが現実的:

1. 少数キューブで済む故障: `XID + MAXDC_QX`。
2. limit到達故障: DNFを作らず、projected model counting（現行 `PCOUNT`、DUALIZA系）で厳密値を得る。
3. PCOUNTにも難しい故障: separator幅を減らす変数順（MINCE系）または d-DNNF/decision-DNNF
   コンパイルを実験する。

次の実装候補は `PCOUNT` の変数順最適化。TabularAllSATはsolverメモリには効くがdisjoint出力なので、
`n673gat` の「少数キューブ」には逆方向である。キューブ自体が必須なら Espresso型の
expand/reduce/irredundant と集合被覆を試す余地はあるが、minimum DNF の計算量と候補素項数が障壁になる。

## 文献

- Yu et al., 2014: https://sites.cs.ucsb.edu/~nestan/pdf/VLSID14.pdf
- Toda et al. (HALL), SAT 2023: https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SAT.2023.9
- Spallitta et al., AIJ 2025: https://cca.informatik.uni-freiburg.de/papers/SpallittaSebastianiBiere-AIJ25.pdf
- Minato, 1993: https://cir.nii.ac.jp/crid/1573668923830855040
- CUDD `Cudd_zddIsop`: https://add-lib.scce.info/assets/doxygen-cudd-documentation/cuddZddIsop_8c.html
- Sasao and Butler, 2001（ISOPはminimumとは限らない）: https://kyutech.repo.nii.ac.jp/records/368
- Umans, 2001: https://www.sciencedirect.com/science/article/pii/S0022000001917751
- Möhle and Biere (DUALIZA), ICTAI 2018: https://fmv.jku.at/papers/MoehleBiere-ICTAI18.pdf
- Lagniez et al., KR 2024（decision-DNNFからdisjoint partial model列挙）:
  https://proceedings.kr.org/2024/48/kr2024-0048-lagniez-et-al.pdf

## 回帰確認

- `cmake --build build`: debug/releaseとも成功。
- c17aゴールデン: `net_name,f_type,fdp` が全行一致。
- c17a `GT_ISOP=1 GT_BDD=1`: 22代表故障、ISOP等価性失敗0、`ALL VERIFIED`。
- s27 TDF `GT_ISOP=1 GT_BDD=1`: 48代表故障、ISOP等価性失敗0、`ALL VERIFIED`。
- b12 count `MAXDC_QX_MULTI=8 GT_ISOP=1 GT_BDD=1`: 完全列挙、`ALL VERIFIED`。

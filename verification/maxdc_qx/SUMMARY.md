# 爆発故障の根本原因の特定と、検出オラクル素項展開による解消

実施日: 2026-06-29 / ブランチ: verification
関連: [[maxdc-core-reeval-post-fix]] [[cube-enumeration-diminishing-returns]] [[cube-trend-verifier]]
土台: `verification/gt_bdd/SUMMARY.md`（3バグ修正後の健全な baseline）

## 課題

「ドントケア判定(XID)が原因で 1 故障あたりのテストキューブ生成回数が膨大化する」。
完全列挙が終わらない故障が存在する（b12 の最重故障は **521,746 本**）。

## 根本原因（新規・GT_COVER で証明）

`src/fdp/gt_verify.c` に診断 `GT_COVER=1` を追加。回路から直接構築した検出関数 D_f について
**BDD ノード数・1-パス数(=BDD が誘導する disjoint カバーのサイズ)** を、実際に生成された
キューブ数と並べて出す。

b12 の爆発故障で実測（`GT_BDD=1 GT_COVER=1`）:

```
[GT_COVER] MEMORY_REG_0__0__SCAN_IN,sa0 cubes=10479 det_paths=2 det_nodes=2 fdp=0.500000
[GT_COVER] COUNT_REG2_1__SCAN_IN,sa0    cubes=224   det_paths=189 det_nodes=29 fdp=0.469467
```

`MEMORY_REG_0__0__SCAN_IN/sa0` は完全列挙で **521,746 本**生成するが、真の D_f は
**det_nodes=2 / fdp=0.5 ＝ 実質「単一リテラル(あるPI=値)」**。正解はたった **1 キューブ**。

→ **爆発の主因は「DC判定が甘くキューブが小さい」ではない。** XID はパターン毎の局所ドントケア
しか見ないため、**グローバルには 1 本で済む空間を、ほぼ素項だが互いに重なり合う
near-duplicate キューブで何十万本にも刻む**（＝ cube_trend が観測した「Xマスクだけずれる
near-duplicate 量産」の正体）。キューブは既に**ほぼ素項サイズ**（b12 平均ケア 12.4 本）で、
DC判定の質ではなく**カバーの冗長性**が問題。

## 解決策: 検出オラクルへの素項展開

各 XID キューブを「非検出オラクル CNF（正常 ∧ 故障コーン ∧ サイト縮退 ∧ 全PO一致=z0）が
UNSAT を保つ範囲で」素項へ縮約する（`src/fdp/experiment.c`）。グローバルに冗長なケアビットが
落ち、1 本が広い空間を覆う＝禁止節が広域を潰すので**冗長カバーが崩壊**する。

3 つの縮約法を実装・比較した（いずれも `MAXDC=1` 必須、既定無効で本番動作は不変）:

| env | 方式 | revert | 特徴 |
|---|---|---|---|
| `MAXDC_CORE` | UNSATコア一括(1 solve/cube) | あり(~34%) | 既存。最速だがコアが不十分だと縮約破棄 |
| `MAXDC_QX` | **QuickXplain(Junker 2004)** | **0** | 新規。真の極小素項。O(\|prime\|·log n) solve、常に健全 |
| `MAXDC_CORE MAXDC_HYB` | **core→revert時のみQX救済** | **0** | 新規。易しい多数は1 solve、coreが外した分だけQX |

### 爆発故障単体 `MEMORY_REG_0__0__SCAN_IN/sa0`

| 方式 | cubes | wall |
|---|---:|---:|
| baseline(XID) | 521,746 | 完走せず |
| MAXDC_CORE | 2 | 0.00s |
| MAXDC_HYB | 2 | 0.00s |
| MAXDC_QX | **1**（真の最小） | 0.00s |

**52万本→1〜2本**。これが探していた有効策。

### b12 全体（cap=100000、全 2872 代表故障）

| 方式 | wall | total_cubes | max_cube | reverts |
|---|---:|---:|---:|---:|
| MAXDC_CORE | **62s** | 125,114 | 1588 | 40,422 |
| MAXDC_HYB | 89s | 110,436 (−12%) | 1501 | 0 |
| MAXDC_QX | 189s | **100,928 (−19%)** | 1474 | 0 |

- **3方式とも全故障 complete・fdp 完全一致**（baseline は完全列挙では終わらない）。
- core が最速。QX が最小本数だが 3x 遅い。HYB は中間（coreの+44%時間でQX削減の60%を回収）。

### s641（無制限・完全、ケア余地が小さい複雑 D_f の代表）

| 方式 | cubes | wall |
|---|---:|---:|
| baseline | 27,819 | 7.0s |
| MAXDC_CORE | 22,784 | 3.3s |
| MAXDC_HYB | 21,066 | 5.7s |
| MAXDC_QX | 20,238 | 13.0s |

複雑 D_f（素項≈XID出力で落とせるビットが少ない）では QX の分割solveが割高。
ここは core/HYB が費用対効果で勝る。

## 健全性検証（GT_BDD=1, exact 照合）

- c17a ゴールデン一致（QX / HYB とも fdp 完全一致）。
- **ALL VERIFIED（UNSOUND=0 / complete-but-NOT-exact=0）**:
  - QX: c17a, s298, s510, s641, s5378_l30(4551故障)
  - HYB: s298, s510, s641, s1494（冗長12故障も D_f=定数0 で厳密）, s5378_l30(4551故障)
- 既定動作は無変更（s27_C ゴールデン PASS, 全 env 無効時）。

## 結論 / 使い分け

- **爆発（完全列挙が終わらない）の解消には素項展開が決定打**。原因はDC判定の質ではなく
  「ほぼ素項の near-duplicate を冗長に量産すること」で、検出オラクル素項展開が広域禁止節で潰す。
- **速度最優先**: `MAXDC_CORE`（既存。最速・爆発解消・+完成率）。
- **本数最小・決定的・revertゼロ**: `MAXDC_QX`（真の極小素項。複雑回路では割高）。
- **バランス**: `MAXDC_CORE MAXDC_HYB`（易しい多数は core 速度、core が破棄した分だけ QX で救済）。

伸びしろ: 複雑 D_f の難故障（s5378 に残る ~1500、b12 には無し）は素項展開でも本数が多い＝
D_f 自体が本質的に複雑。ここは disjoint 列挙 or anytime 上下界が次の研究軸。
</content>

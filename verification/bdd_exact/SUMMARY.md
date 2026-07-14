# BDD直接法フォールバック（BDD_EXACT）: incomplete の根絶

実施日: 2026-07-09 / ブランチ: verification
関連: [[cube-explosion-research-state]] [[gt-bdd-exact-verifier]]
前段: `verification/dual/SUMMARY.md`（双対列挙。complete 救済は部分的だった）

## 動機

limit 到達（incomplete）が手法の弱点。理想は incomplete をなくすこと。
制約: CaDiCaL 改造なし・外部 model counting ツールなし。

先行検討（dual/SUMMARY.md の候補リスト）のうち、保留していた
**「難故障のみ検出関数 D_f を BDD で直接構築する」**（PLATO 系の直接法）を採用。
GT_BDD 検証ツールが同じ計算を既に持っており、s5378 全4551故障や s15850 の
det_paths=71.7T 級の故障でも D_f を構築できる実績があったため。

## 仕組み（env `BDD_EXACT=1`、既定無効）

- 列挙が limit 内で終わる故障は従来どおり（キューブ由来の fdp）。
- **打ち切りになった故障だけ**、`GT_ExactCountStr`（`gt_verify.c` の D_f 構築を
  `gt_build_det` として共通化）で D_f = OR_PO(good XOR faulty) を直接構築し、
  ミンターム数から厳密 fdp を計算して **complete=1** で報告する。
- 正常回路の BDD は初回のみ構築して常駐共有。故障ごとには TFO コーンだけ再構築。
- キューブ集合は部分被覆のまま残る（支配流用・DropDeteFault にそのまま有効）。
- 出力の意味論: **complete=1 ⇔ fdp が厳密**（「列挙が完了」ではなくなる）。

## 実験結果（-limit 30、全代表故障、main_release, WSL2）

| 回路 | baseline wall / incomplete | BDD_EXACT wall / incomplete |
|---|---|---|
| s5378 (4551故障) | 56.6s / 2,980 | **70.2s / 0**（+24%） |
| b12 (2872故障) | — | **15.2s / 0**（37MB） |
| s13207 | — | **129.5s / 0**（86MB） |
| s15850 | 282.5s / 4,645 | **2400s でも未完**（下記参照） |
| s35932 / s38417 / s38584 | **baseline 自体が 900s 超**（s38417 で確認） | 900s 超（帰属は列挙側） |

- s15850 の最難故障 g9317/sa0（det_paths=71.7T、従来 limit=50000+QX でも真値の90.4%）は
  **単体 1.84s（正常回路BDD構築込み）で厳密 fdp=4.8713e-04, complete=1**。
  よって1故障あたりの D_f 構築自体は軽い。それでも全体が 2400s で終わらないのは、
  capped 4,645 故障のコーン再構築が続く間に CUDD の動的リオーダリング（autodyn sift）が
  繰り返し走ってスラッシングするため、という仮説。対策として `GT_NODYN=1`
  （正常回路構築後に一度 sift してから凍結）を実装済み・**効果は未計測**
  （セッション終了時点で実行環境の都合により保留）。
- s38417 級は baseline の列挙自体が実用時間で終わらないため、incomplete 以前の問題
  （本手法の責任範囲外）。

## 正しさの検証

- `GT_BDD=1` 併用で s5378_l30 全4551故障 **ALL VERIFIED**（UNSOUND=0）。
- baseline の complete 故障と fdp 完全一致・capped 故障で下界違反なし（s5378, 10538行）。
- **独立クロスチェック**: s5378 を limit100 で回し完全列挙できた故障の「列挙由来の厳密 fdp」と
  BDD_EXACT@limit30 の値が **2615/2615 完全一致**（別経路の計算同士）。
- デフォルト（env なし）は無変更: c17a ゴールデン一致・s27_C 一致・s1494 冗長12。

## 結論: 不採用（ユーザー判断・2026-07-09）

**D_f をネットリストから完全 BDD 構成してミンターム数を数えるのは従来手法
（PLATO 系の直接 BDD 法）そのもの**であり、本研究が SAT 列挙＋キューブで
置き換えようとしている当のベースラインに逃げることになるため、本番手法としては不採用。
コードは GT_BDD 検証ツールの共通化（`gt_build_det` / `GT_ExactCountStr`）として残し、
**検証・参照値生成用ツール**の位置づけとする。

技術的知見（今後の参照用）:
- 中規模（s5378/b12/s13207）なら incomplete 0 化は可能で、コストも capped 故障数比例（s5378 +24%）。
- s15850 は 2400s でも未完。1故障単体は 1.84s なので、故障を重ねるごとの
  autodyn sift スラッシングが疑い（`GT_NODYN=1` 凍結スイッチは実装済み・未計測）。
- s38417 級は baseline の列挙自体が 900s 超（incomplete 以前の問題）。
- incomplete の根絶は **SAT 列挙パラダイム内**で解く必要がある → 次の候補は
  Shannon 分割による完全化（`verification/split/SUMMARY.md` 予定）。

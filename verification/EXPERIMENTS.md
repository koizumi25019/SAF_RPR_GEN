# 実験ブランチ一覧（検証系）

## ブランチ構成

```
baseline ─────────────────   ← 一本線（安定・本番手法）
verification ──┬─ exp/full-miter        ← 実験は verification から枝分かれ
               ├─ exp/incremental-sat
               └─ exp/<次の実験>
```

- **baseline** … 安定トランク。実行時間改善などで「ずっと適用する」と決めた手法だけを `cherry-pick`
  で昇格させる。
- **verification** … baseline + 検証ツール（`GT_BDD` 厳密検証・`CUBE_TREND`）+ 軽量な env 実験
  （`MAXDC`/`DIVPO`/`XSTAT` など default-off で共存させたいもの）。
- **exp/\*** … 個別の実験ブランチ。`verification` から枝を切る。ソルバ構築やメインループを作り替える
  ような**侵襲的な実験**や、未完成・互いに排他なものはここで育てる。
  - 良ければ → baseline へ `cherry-pick` で昇格。
  - 諦めたら → ブランチを残して参照用にするか、`git branch -D` で削除。

新しい実験を始めるとき：
```bash
git switch -c exp/<名前> verification
```

## 現在の実験ブランチ

### verification/isop（現行コードで実装済み・env GT_ISOP / MAXDC_QX_MULTI）

キューブ爆発が列挙戦略由来か、平坦DNF表現そのものの大きさかを切り分ける。
`GT_ISOP=1` は独立な Minato-Morreale ISOP の項数を測る恒久診断。`MAXDC_QX_MULTI=N` は
複数の集合極小素項から短いものを選ぶ実験だが、s5378最難故障では改善0・6.2倍遅く棄却。
結果と文献は `verification/isop/SUMMARY.md`。

### exp/full-miter （現行コードで実装済み・env FULL_MITER）
故障ごとに TPG モデル（正常回路＋故障コーン＋検出節）を作り直す代わりに、**全回路ミターを一度だけ
構築し、assumption で対象 net の故障注入を ON/OFF** して使い回す方式。CNF が固定なので CaDiCaL が
学習節を全故障で再利用できる。`FULL_MITER=1` で本体が `RunFullMiter` に分岐（通常パスは無改変）。
- 実装: `src/fdp/cnf/faulty_miter.{c,h}` + `fault_detection_prob.c` の `RunFullMiter`。
- 正しさ: c17a/s27_C ゴールデン一致、s1494 冗長=12、GT_BDD で c17a/s27/s208/s298/s510/s641
  すべて UNSOUND=0 ALL VERIFIED（fdp は通常モードと一致）。
- **性能（要改善）**: 現状はむしろ遅い（s641 で 3.7s→9.7s）。原因は毎 solve で n_net 個の act を
  再 assume すること、ブロッキング節が単一ソルバに蓄積すること。次の最適化：
  (1) act を log 段デコーダ/故障セレクタ化して選択を O(log n_net) assume に、
  (2) 定期的なソルバ再構築で retire 済みブロッキング節を排出。
- 旧版（古い土台 `cb76242` の原案, 2026-05）は `exp/full-miter-old` に退避（`git show 49aa759`）。

### exp/incremental-sat （参照用・古い土台）
リスタート型のテスト生成（毎回 solver を作り直し禁止節を積む）の代わりに、**単一の solver を保持し
activation literal（活性化リテラル）で制約を出し入れ**してインクリメンタルに解く方式。採用せず記録のみ。
バグ修正（EA/XID）より前の古い土台 `0cd6209` 上にあり、そのままマージはできない（再挑戦時は現行
コードへ作り直す）。詳細は `git show 4121b22`。

> 旧 `develop_xid`（初期 XID 開発, 2026-04）は現行のインライン XID に成熟版として統合済み・固有価値
> 無しのため 2026-06-25 に完全削除した。

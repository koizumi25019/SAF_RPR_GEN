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

## 現在の実験ブランチ（参照用・古い土台）

下記2つはバグ修正（EA/XID）より前の古い土台にある過去の試み。**そのままマージはできない**ので、
再挑戦するときはアイデアを現行コードへ作り直す（中身は設計参照として使う）。

### exp/incremental-sat
リスタート型のテスト生成（毎回 solver を作り直し禁止節を積む）の代わりに、**単一の solver を保持し
activation literal（活性化リテラル）で制約を出し入れ**してインクリメンタルに解く方式。採用せず記録のみ。
分岐元 `0cd6209`。詳細は `git show 4121b22`。

### exp/full-miter
故障ごとに TPG モデル（正常回路＋故障コーン＋検出節）を作り直す代わりに、**全回路ミターを一度だけ
構築し、assumption で対象 net の故障注入を ON/OFF** して使い回す方式。支配流用（種キューブ/禁止節）
との噛み合わせは要再設計。分岐元 `cb76242`。詳細は `git show 49aa759`。

> 旧 `develop_xid`（初期 XID 開発, 2026-04）は現行のインライン XID に成熟版として統合済み・固有価値
> 無しのため 2026-06-25 に完全削除した。

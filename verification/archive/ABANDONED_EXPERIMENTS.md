# 放棄実験アーカイブ（検証系）

実行時間改善などのために試したが採用に至らなかった実験。**コードは注釈付きタグで永久保存**して
あり（`git branch` 一覧には出ないが完全復元できる）、また試したくなったらここから復元する。

各実験は当時のベースライン（EA/XID のバグ修正より前）の上に作られている点に注意。現行 `baseline`/
`verification` とは土台が違うので、再挑戦時はアイデアを現行コードへ作り直すのが基本（そのまま
マージはできない）。

## 一覧

| タグ | 内容 | 分岐元 | 復元 |
|---|---|---|---|
| `archive/incremental-sat` | 活性化リテラルによるインクリメンタルSAT | `0cd6209` | `git switch -c retry/incremental-sat archive/incremental-sat` |
| `archive/full-miter-incremental` | 全回路ミター + net毎 故障注入スイッチ（assumptionベース） | `cb76242` | `git switch -c retry/full-miter archive/full-miter-incremental` |
| `archive/develop_xid` | 旧XID開発（xid系プログラム/s208テストセット/netlist構造変更） | `18e66a8` | `git switch -c retry/develop_xid archive/develop_xid` |

## 詳細

### archive/incremental-sat
リスタート型のテスト生成（毎回 solver を作り直し禁止節を積む）の代わりに、**単一の solver を保持し
activation literal（活性化リテラル）で制約を出し入れ**してインクリメンタルに解く方式。
commit メッセージ上は "kept as record"（＝採用せず記録のみ）。詳細は `git show 4121b22`。

### archive/full-miter-incremental
故障ごとに TPG モデル（正常回路＋故障コーン＋検出節）を作り直す代わりに、**全回路ミターを一度だけ
構築し、assumption で対象 net の故障注入を ON/OFF** して使い回す方式。詳細は `git show 49aa759`。

### archive/develop_xid
インライン XID（`InlineXID`）に置き換わる前の、初期の XID 実装・テストセット作成・netlist 構造
変更を含む旧開発ブランチ。現行とは XID の実装思想が異なるので、過去実装の参照用。詳細は
`git log archive/develop_xid`。

---
（注）各タグ末尾には当時の "limit check >= 修正" など重複コミットも含まれるが、これは現 `baseline`
に既に入っている `f29a751` と同等。実験本体は上記 commit を参照。

# テストと検証

コマンドはリポジトリ直下から実行する。

## 回帰テスト

`ctest --test-dir build --output-on-failure` で対象故障の選択順、BDD→GMP の数え上げ・確率変換と CSV 出力のテストを実行する。
Python3がある環境では `tdf_model` も実行し、TDFの独立全入力シミュレーション・設定切替・直列並列一致を確認する。
`batch_runner` は全設定の入力パス・出力先モデルを確認し、隔離した作業ツリーでSAF/TDFの実行、
既定の回路セット、出力ディレクトリの作成、失敗後の続行と終了コードを検証する。既存の実験結果は上書きしない。
対象選択は従来の全走査と照合し、同レベルの順序、処理済みのスキップ、空集合、再初期化を確認する。
空集合、重複、独立全列挙、32ビット桁境界、大きな数え上げ、等価故障と完了フラグを確認する。
パイプライン全体の正しさは `tests/expected/` のゴールデンファイルと CSV 出力を比較して検証する
（`tests/expected/README.md` 参照）。**挙動が変わりうる変更をしたら、c17a を実行して `fdp` 列が
`tests/expected/c17a_result.csv` と一致することを確認する**：

```bash
./scripts/run.sh saf c17a
diff <(cut -d, -f1,2,5 tests/expected/c17a_result.csv | sort) \
     <(cut -d, -f1,2,5 output/saf/full/fdp/c17a.csv | sort)
```

比較するのは `net_name,f_type,fdp`（1,2,5列）**のみ**。`cube_cnt` 列はソルバの解順序やドントケア判定で
変動するため無視する。ゴールデンファイルは固定条件（`-fault` なし=全故障、`-limit` なし=無制限）で
生成され、全行 `complete=1` になる。`output/` は実行で上書きされるが、`tests/expected/` は上書きされない。


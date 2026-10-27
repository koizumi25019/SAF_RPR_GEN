# 正常回路限定化前のSAF実験

- コード: `5da133b1728c27154e41ef4108bf194d1a3f8c80`（ソース無改変）
- 正常CNF: 全正常回路を投入（範囲限定なし）
- 回路: s5378_C, s9234_C, s13207_C, s15850_C, s35932_C, s38417_C, s38584_C
- 全代表故障を自動生成、キューブ上限30、流用off（`MDC_NODOM=1`）
- EA・D-chainは有効（`MDC_NOEA`・`MDC_NOPROP`は未設定）
- 並列化なし。7回路を順番に各1回実行
- ビルド: CMake Releaseターゲット（`-O3 -DNDEBUG -fcommon -D_DEFAULT_SOURCE`）
- コンパイラ: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- CPU: 13th Gen Intel(R) Core(TM) i7-13700K
- ライブラリ: この作業環境の `external/cadical`・`external/cudd` のビルド済み静的ライブラリ

`fdp/` にCSV、`log/` に旧版のCPU時間と外部計測の実経過時間、`settings/` に実行設定を保存する。
`status.csv` は終了した回路と終了コード（0が成功）、`batch.log` は実行の開始・終了を記録する。
各1回の測定であり、中央値ではない。別のマシンで計測した限定化後の時間とは直接比較できない。

一時ソース・実行ファイル・標準出力は `old_code/` にある。全回路終了後にこのディレクトリを削除しても、ここにある結果は残る。

進捗確認（リポジトリ直下から）:

```bash
cat output/saf/before_scope/status.csv
tail output/saf/before_scope/batch.log
```

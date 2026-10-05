# SAF limit30 の XID / 論文 CORE 比較

対象: s5378_C、s9234_C。s13207_C はユーザー指定で今回の実行対象から除外。順序回路からの組合せ化版で縮退故障を調べる。

## 実行

```bash
cd /workspace/SAF_RPR_GEN_core
bash run_paper_core_limit30.sh
# 反復回数を指定する場合
bash run_paper_core_limit30.sh --repeats 3
# 1回路だけを指定する場合
bash run_paper_core_limit30.sh s5378_C
```

C の `build/main_release` を `.set` で動かし、シェルと AWK で集計する。Python は不要。
既定では4ジョブを直列に各1回実行する。複数回の場合は方式の実行順を反復ごとに交互にする。
Release バイナリを先にビルドする必要がある。

## 設定

`input/script/<回路>_{xid,core}_l30.set` の4ファイル。s13207_C の2設定は明示指定用に残している。

```text
-saf
-dc_method core
-dom_reuse off
-core_verify off
-limit 30
```

XID 側は `-dc_method xid`。全代表故障を自動生成し、故障ごとに最大30キューブ。
流用なし、追加検証なし。同じ正側 CaDiCaL を使う。
CORE は core 再確認と削除による極小化を通常処理として実行する。
シェルは、継承された研究・検証用の環境変数を外して性能測定条件を揃える。
`.set` のモデル・方式・limit の指定はそのまま使い、出力先だけ日時別のコピーに差し替える。

## 出力と比較の読み方

出力は `output/paper_core/limit30/comparison/<実行日時>/`。
日時は Asia/Tokyo のタイムゾーンを明示する。

- `measurements.csv`: 回路、方式、反復、CPU/実時間、ドントケア/生成SAT/BDD時間、
  代表故障数、完了/未完了故障数、総キューブ数。
- `comparison.csv`: 両方式完了、COREのみ完了、XIDのみ完了、両方式未完了、
  少なくとも一方式が未完了の故障の FDP 大小比較、両方式完了の FDP 不一致数。
- `runN/<回路>_fault_comparison.csv`: 代表故障ごとの完了フラグと両方式の FDP。
- `runN/{xid,core}/{set,fdp,log}/`: 実際に使った設定、故障別結果、ログ、標準出力/エラー。
- `core_statistics.csv`: core 抽出直後と削除による極小化後のケアビット数・X率、
  削除試行数、生成キューブ当たりの平均ケアビット数。`core_verify off` の solve 数との一致も確認。
- `binary.sha256`: 測定バイナリの識別。

`complete=1` は厳密 FDP、`complete=0` は生成済みキューブの被覆から得た下界。
打ち切り故障の FDP は方式間で一致しなくてよい。キューブ数・時間だけでなく完了故障数と
部分被覆も比較する。両方式で完了した故障の FDP が違う場合や代表故障キーの欠落は、
比較スクリプトが終了コード1で報告する。これは独立 BDD 検証の代わりではない。
等価故障のエコー行（complete空欄）は代表故障数やキューブ数の集計に含めない。
時間はアプリが記録した全体 CPU 時間を主指標とし、読込・CNF構築・oracle構築・
列挙・ドントケア判定・BDD・出力を含む。既定の1回測定は中央値・統計的優劣を示すものではない。

## 初回実行

2026-10-05、`20261005T233837+0900`。
生データ: `output/paper_core/limit30/comparison/20261005T233837+0900/`。
s13207_C の未実行2ジョブを取り消し、4ジョブを完了・集計した。
s5378_C: XID/CORE CPU 145.639/401.314秒、完了 1,567/2,654（全4,551）。
s9234_C: XID/CORE CPU 407.991/1,588.469秒、完了 3,053/3,721（全6,927）。
両方式完了の FDP 不一致・代表故障欠落は両回路とも0。
小規模と中規模を合わせた表・X率の分析は `COMPARISON.md` を参照。

# core 抽出のみ / XID / core＋極小化の比較

2026-10-07（Asia/Tokyo）。管理された仮想環境での実測。
`verification` (a8718df) から分岐した `feature/paper-core` (9a55da7) を元に、
専用ブランチ `exp/core-only-comparison` と worktree `/workspace/SAF_RPR_GEN_core_only` を作成。
今回は小規模4回路。縮退故障 SAF、低電力off、全代表故障、無制限完全列挙、流用off。
中規模 s5378_C/s9234_C の core 抽出のみは本実験に含まない。

## 実装と条件

一般化の実装は C。`.set` → シェル → C ATPG の順で動かす。
新オプション `-core_minimize on|off`（削除極小化）、`-core_recheck on|off`（core追加再確認）。
両方とも既定onなので既存の設定は従来通り。

| 方式 | dc_method | core_minimize | core_recheck | timed core_verify | キューブ当たり否定側solve |
|---|---|---|---|---|---|
| xid | xid | on（不使用） | on（不使用） | off | 0 |
| core_only | core | off | off | off | 1 |
| core_min | core | on | on | off | 2＋core直後のケア数k |

core_only は完全入力モデルの非検出UNSATを1回求め、`ccadical_failed`で抽出したcoreをそのままキューブにする。
core を取り出すAPIは追加のsolveではない。生成SATと合わせて生成キューブの一般化まで2回。
別途、各故障の列挙終了を判定する生成SATなどがあるため、実行全体のSAT総数が単純に2×キューブ数ではない。
core_only では従来ログの `prime` フィールドも最終care数を表し、極小性を意味しない。

同一Releaseバイナリ、CPU0固定、全ジョブ直列。方式ごとにウォームアップ1回を除外し、7回測定。
3方式の順序を反復ごとに循環。CPU指標は C 計測ラッパーによるwait4の子user+system、起動から終了まで。
計測ラッパーは親側、ATPG自身のCPUだけを集計。性能測定中のstdoutは/dev/null、CSV/log/stderrは保存。
検証・研究getenvスイッチは実行前に除去し、proxy/credential環境は保持する。
GT/core追加検証は別実行で、性能時間に含めない。Python/openpyxlは終了後の集計・Excel作成のみ。

## キューブ数とCPU中央値

| 回路 | XID CPU秒 | coreのみ CPU秒 | 極小化あり CPU秒 | XIDキューブ | coreのみキューブ | 極小化ありキューブ |
|---|---:|---:|---:|---:|---:|---:|
| c17a | 0.008518 | 0.009574 | 0.010115 | 65 | 64 | 53 |
| s27_C | 0.009612 | 0.011833 | 0.013210 | 104 | 116 | 104 |
| s208_C | 0.335719 | 0.705419 | 0.327971 | 10,163 | 9,270 | 1,272 |
| s298_C | 0.227986 | 0.299673 | 0.380505 | 1,578 | 1,692 | 1,115 |

全方式で全代表故障 complete=1、全故障FDP一致。各方式内の全7回でキューブ数・core統計が同一。
coreのみのキューブはXID比で c17aは約1.5%減、s208_Cは約8.8%減だが、s27_Cは約11.5%増、s298_Cは約7.2%増。
極小化なしのほうが少ないキューブになる保証はない。
CPU中央値は今回4回路ともcoreのみがXIDより長く、s208_Cは約2.10倍。
c17a/s27_Cは数ミリ秒単位で範囲が重なるため、小さな速度差の一般化はしない。

s208_Cの coreのみ→極小化ありではキューブ9,270→1,272。
ドントケアCPU中央値は0.116→0.170秒に増えるが、生成SATは0.449→0.047秒、BDDは0.036→0.009秒。
従って削除照会を省略しても全体は速くならず、coreのみCPU0.705419秒に対して極小化あり0.327971秒となった。
これらの区間合計はoracle構築・読込・出力等を含まないため全体CPUとは一致しない。
core_min/XIDの s208_C CPU範囲も重なるので、小さな中央値差だけで高速化を断定しない。

## X率と否定側solve

| 回路 | coreのみ列挙のX率 | 極小化あり列挙のcore直後X率 | 極小化後X率 | coreのみsolve | 極小化ありsolve |
|---|---:|---:|---:|---:|---:|
| c17a | 44.06% | 42.26% | 48.30% | 64 | 259 |
| s27_C | 53.94% | 54.26% | 56.73% | 116 | 541 |
| s208_C | 51.16% | 60.76% | 67.57% | 9,270 | 12,027 |
| s298_C | 71.85% | 72.29% | 75.75% | 1,692 | 7,482 |

ここでcoreのみは独立した列挙実験であり、以前の極小化あり実行の中間値とは異なる。
極小化の有無によって禁止節、次のSATモデル、core、キューブ分布が変わる。
X率は全生成キューブの入力ビット位置数に対する集計値で、故障ごとの単純平均ではない。
全core_only実行で solves=cubes、deletion_queries=0、core_care=final_care を確認。
全core_min実行で solves=2*cubes+core_care、deletion_queries=core_care を確認。

## 正しさ確認

- 3方式×4回路の独立BDDは全てALL VERIFIED（性能測定とは別の実行）。
- coreのみの小規模4回路、577代表故障、11,142キューブについて独立2値シミュレーション。
  X全展開の延べ56,871,634パターンが全て故障を検出し、全検出集合・FDPも完全一致。
  キューブ採取ラッパーを用いたシミュレーション実行のCSVは性能実験のcore_only CSVとバイト単位で一致。
- 256個の3入力真理値表×4スイッチ条件、計4,096検出ミンタームで健全性を確認。
  極小化onの条件では極小性も確認。空入力、空core、非検出入力の拒否も確認。
- 既定XIDの9回路の独立BDD、c17a Debug/Releaseゴールデン一致。
- .set/CLI設定、従来環境変数との互換・優先、不正値拒否を確認。

`results/core_only_regression.json` / `results/core_only_simulation.json` に記録。

## 再現と出力

```bash
cd /workspace/SAF_RPR_GEN_core_only
# 初回のみ。既存のCMakeビルドを使う場合も同じソースからDebug/Releaseをビルドする。
bash verification/paper_core/build_compare.sh
bash run_core_only_comparison.sh
# 回路と回数を指定する場合
bash run_core_only_comparison.sh --repeats 7 s208_C s298_C
```

入力設定は `input/script/{c17a,s27_C,s208_C,s298_C}_{xid,core_only,core_min}_compare.set`。
一つずつ既存シェルで実行する場合は、例えば `bash run_paper_core_experiments.sh s208_C_core_only_compare`。
既存シェルは測定の繰返し・3方式の結果比較を行わない。比較測定には上記の専用シェルを使う。

生データ: `/workspace/SAF_RPR_GEN_core_only/output/core_only_comparison/20261007T153920+0900/`。
各実行の使用設定、CSV、log、stderr、wait4時間、ソース親コミット・パッチ、バイナリ/ライブラリSHA256を保存。
ウォームアップ・別検証を除いた84回の時間はmeasurements.csv、12条件の要約はsummary.csv/json。
Gitに保存するスナップショットは `results/core_only_comparison.json`。
`core_only_comparison.xlsx` は数値だけの8シート。方式別比較、実行時間、キューブ数、判定回数・X率、
時間内訳、測定明細、別実行BDD検証、条件。グラフは含めない。
以前の `paper_core_comparison.xlsx` / `tdf_power_on_results.xlsx` は変更していない。

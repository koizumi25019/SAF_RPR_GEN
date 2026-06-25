# 期待値ファイル

プログラム変更時の正解データを置く場所です。
ここのファイルは実行で上書きされません（`output/` とは別管理）。

## 使い方

1. プログラムを実行して `output/<条件>/fdp/*.csv` を生成する（c17a なら `output/full/fdp/c17a.csv`）
2. 期待値と **fdp 列**を照合する（cube_cnt は無視する）：

   ```bash
   # 例: c17a — net_name,f_type,fdp の3列だけ比較
   diff <(cut -d, -f1,2,5 expected/c17a_result.csv   | sort) \
        <(cut -d, -f1,2,5 output/full/fdp/c17a.csv   | sort)
   ```

   差分が出なければ回帰なし。差分が出たら、その変更が意図したものか確認する。
3. 意図した仕様変更で fdp が変わった場合は、期待値ファイルを更新する。

### 比較で見る列・見ない列

| 列 | 比較 | 理由 |
|----|------|------|
| `net_name`, `f_type` | ◯ | 故障の識別子 |
| `fdp` | ◎（これが正解） | 完全列挙時の真の検出確率。ソルバーやドントケア判定に依らず一意に定まる |
| `cube_cnt` | ✕ | テストキューブの分割数。ソルバーの解順序やドントケア判定で変動しうる |
| `complete` | △ | 無制限生成なら全行 1 のはず。`0` が出たら打ち切り＝生成条件ミス |

cube_cnt はソルバーやドントケア判定で変わりうるが、**完全列挙（complete=1）なら
テストキューブの和集合 ＝ fdp は一意に定まる**ので、回帰判定は fdp 列で行う。

## 命名規則

出力は **条件ごとのディレクトリ**に分ける（`output/<条件>/fdp/<回路>.csv`、
条件 = `-limit` 値→`limit30` 等 / `-limit` 無し→`full`）。期待値ファイルは固定条件
（`-fault` 無し＝全故障・`-limit` 無し＝無制限）で生成するので、対応する出力は常に `full/` 配下。

| 出力 | 期待値 |
|------|--------|
| `output/full/fdp/c17a.csv` | `expected/c17a_result.csv` |

## 生成条件

期待値は以下の固定条件で生成する（条件を揃えないと比較の意味が無くなる）。

- **対象故障 = 全故障**
  `.set` に `-fault` を**書かない**。ネットリストから全ネットの sa0/sa1
  （等価故障を整理した代表故障）が自動生成される。 → [read.c:49-80](../src/fdp/read.c#L49-L80)

- **テストキューブ生成 = 上限なし（無制限）**
  `.set` に `-limit` を**書かない**（または `-limit 0`）。UNSAT まで完全列挙し、
  全故障が `complete=1` になる。
  → [fault_detection_prob.c:362-370](../src/fdp/fault_detection_prob.c#L362-L370) で
  `limit <= 0` を「無制限」として扱う。
  正の値を指定するとその回数で打ち切られ `complete=0` の行が出るので、期待値生成では指定しない。

`.set` の最小例（c17a / [input/script/c17a.set](../input/script/c17a.set)）:

```
-net   ../input/circuit/c17a.v
-fdp   ../output/full/fdp/c17a.csv
-log   ../output/full/log/c17a.txt
# -fault は書かない → 全故障
# -limit は書かない → 上限なし（無制限）→ 出力は full/ 配下
```

# XIDとSB-XIDの実行時切替

`exp/sb-xid` の同じ `build/main_release` / `build/main_debug` で、
`-dc_method xid|sbxid` を指定してドントケア判定を切り替えられる。
省略時は従来どおり `sbxid`。CLIと `.set` の両方で使え、後の指定を優先する。

| 指定 | 実行する処理 |
|---|---|
| `-dc_method xid` | 旧来の回路構造ベース `InlineXID()` |
| `-dc_method sbxid` | claspによるSB-XID v1、CEGARなし |

ATPG、正常CNF範囲、EA、禁止節、BDD、故障workerは共通である。
旧XID本体の判定ロジックは変更せず、baselineの正常信号値復元処理を
`src/fdp/xid/xid_model_values.c` として戻した。PIのSAT値から全正常信号値を復元する。
正常SAT値の任意照合 `FDP_NORMAL_SCOPE_VALIDATE=1` は両方式で使用できる。

SB-XIDの1秒探索制限、`-sbxid_pi_objective`、`FDP_SBXID_VERIFY` はSB-XIDモードに適用する。
XIDモードはclaspを呼ばない。この比較用実行体のビルドには、引き続きclaspライブラリが必要。
TDFは両モードで回路読込前に拒否する。

## ビルド

claspを導入済みなら、リポジトリルートで以下を実行する。

```bash
cmake -S . -B build -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$PWD/build/clasp-install"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

## 個別に切り替える

同じ入力・15 worker・30キューブ上限・流用offの例。
リポジトリルートで実行し、CSVとログの保存先を方式別にする。

```bash
mkdir -p output/dc_compare/manual/fdp output/dc_compare/manual/log

build/main_release -saf -net input/circuit/saf/s5378_C.v \
  -dc_method xid -jobs 15 -dom_reuse off -limit 30 \
  -fdp output/dc_compare/manual/fdp/xid.csv \
  -log output/dc_compare/manual/log/xid.txt

build/main_release -saf -net input/circuit/saf/s5378_C.v \
  -dc_method sbxid -jobs 15 -dom_reuse off -limit 30 \
  -fdp output/dc_compare/manual/fdp/sbxid.csv \
  -log output/dc_compare/manual/log/sbxid.txt
```

既存の `.set` の後ろに指定する方法も使える。
たとえば `-set config.set -dc_method xid` は、設定ファイル内の方式をCLIで上書きする。
`.set` のみを変更する場合は、次のいずれかを1行で記載する。

```text
-dc_method xid
```

```text
-dc_method sbxid
```

既存の `run_sbxid_s5378.sh` は、方式を省略した従来の設定でSB-XIDを実行する。

## 両方式をまとめて比較する

以下はXID、SB-XIDを順に実行する。同時実行してCPUを取り合うことを避け、
両方式で回路・worker数・列挙上限をそろえ、流用をoffにする。

```bash
# 引数：回路名、worker数、1故障のキューブ上限
bash run_compare_dc.sh s5378_C 15 30
```

引数省略時も `s5378_C 15 30` になる。`-fault` を指定しないため全代表SAFを生成する。
結果は次のディレクトリへ保存する。同じ引数で再実行すると同じ出力を更新する。

```text
output/dc_compare/s5378_C/limit30/jobs15/
├── fdp/xid.csv
├── fdp/sbxid.csv
├── log/xid.txt
├── log/sbxid.txt
└── timing.csv
```

`timing.csv` の列は次のとおり。

| 列 | 内容 |
|---|---|
| `method` | `xid` / `sbxid` |
| `wall_seconds` | 本体の経過時間。ログの `Time` |
| `cpu_seconds` | 親と全workerの合計CPU時間 |
| `dont_care_cpu_seconds` | 選択したドントケア判定のCPU時間 |
| `complete_faults` | 完全列挙できた代表故障数 |
| `limited_faults` | 上限に達して未完了の代表故障数 |

各ログの `Don't-care Method` に使用方式を記録する。
`CPU Time (Don't care)` は、XIDモードでは旧XIDの処理時間、SB-XIDモードではSB-XIDの処理時間になる。
claspの版・探索上限・最適/暫定/時間切れ等はSB-XIDモードのログに出す。
`Total Cubes / Reused Cubes` でキューブ数も比較できる。

上限0での小回路の例：

```bash
bash run_compare_dc.sh c17a 2 0
```

0はキューブ数の上限をなくす指定で、SB-XIDのclasp探索は各呼出し1秒のままである。
得られるキューブが変わるため、後続のseed列も変わる。
上限付きではFDP下界や完了故障数が異なる場合があるので、時間と合わせて確認する。
この比較は方式全体の実行時間を評価するもので、固定seedごとの求解時間比較とは区別する。

## 動作確認

10件のCTestが成功。新規の `dc_method_switch` は、c17a・s27_Cについて
Debug/Release、XID/SB-XID、直列流用off・2 worker流用off・直列流用onの24条件で完全FDPを比較する。
c17aの既知期待値、既定SB-XID、CLIによる `.set` 上書き、不正値・値なしの拒否も確認する。
TDFの拒否は両方式、CLI/`.set`、Debug/Releaseで確認する。

クラウドでは、別にビルドしたbaseline `23abc61` とXIDモードの結果、
変更前 `d6c4dd0` の実行体とSB-XIDモードの結果を比較し、
故障ごとのFDP・キューブ数・完了フラグが一致した。
比較スクリプトも `c17a 2 0` で実行し、両方式のCSVと時間集計が生成されることを確認した。
検証記録は [dc_method_checks.json](dc_method_checks.json) に保存した。

全s5378の比較実験は、この変更の動作確認では実行していない。

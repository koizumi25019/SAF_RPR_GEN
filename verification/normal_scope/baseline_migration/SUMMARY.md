# baseline移植・正しさ検証・単回速度比較

2026-10-07。対象はbaselineのSAF/XID、移植前コミット `5da133b`、移植後 `b3958a2`。
ユーザーの指定に合わせ、baselineでは `ccadical_add()` の直接投入を維持する。
検証版のCNF_ADDマクロやDIMACS teeを持ち込まない。

## 実装

- 正常CNFの範囲限定はbaselineで既定on。`FDP_NORMAL_SCOPE=0` で従来動作。
- `CreateTPGmodel()` 後に `NormalScopeBuild()` を1回実行する。
- 故障点・TFOの全信号、EAで単位節を投入した `EA_UP` 信号を出発点として全TFIを残す。
- 故障回路のサイド入力はTFOのファンインに含まれる。D-chain・検出・励起の正常信号は
  TFOまたは故障点に含まれる。EAによる別方向の含意はEA_UP信号として追加する。
- Fault/DC/EAの3ソースは元baselineと同一で、直接 `ccadical_add()` を呼ぶ。
- 必要PIのSAT値と範囲外PIの0補完から全正常値を復元し、範囲外PIは最終的にX。
- 全PI・入力順・分母・CSV形式は維持する。実験ログにon/offを記録する。

## 検証方法

`build.py` は実baselineから通常Debug/Releaseを作る。
さらに隔離コピーにだけキューブ保存フックを入れたcapture版を作る。
本番baselineへ検証コードを入れない。

別の検証用バイナリはverification版の独立GT検証器を使用するが、SAT・XID・範囲限定を
実行しない。baselineの実生成キューブを読み、PI順序を照合し、元ネットリストの
全正常回路と物理故障注入から検出関数を構築して健全性と完了被覆の等価性を検査する。
検証版のATPG出力と数値だけを比較する方式ではない。

- SAF無制限9回路で移植前baselineの全FDP・completeと一致、独立GT全通過。
- c17aゴールデン、Debug/Release、範囲限定off一致。
- c17a/s27_C/s208_C/s298_Cは元Verilogから独立全入力シミュレーションし、全X展開の健全性と
  完了被覆の等価性も確認。
- c17a/s208_Cは流用onでも元baselineとFDP一致、GT・全入力シミュレーション通過。
- s208_CのEA無効・D-chain無効の切り分けモードもGT通過。
- s5378_C全4,551代表故障・limit30・流用offの実生成キューブを全件GT照合。
  UNSOUND=0、complete-but-NOT-exact=0、完了1,572故障。

## 計測

通常Release（保存フック・GT・内部値照合なし）で、移植前と移植後を各1回測る。
s5378_C全4,551代表故障、limit30、`MDC_NODOM=1` で流用off。
同じ外部ライブラリ・GCC最適化フラグを用い、ビルド・検証を並行しない。
経過時間はPythonの単調時計で測る。元baselineのログはCPU時間であり、区別する。

| 条件 | 経過時間 | 完了故障数 |
|---|---:|---:|
| 適用前baseline（全正常CNF） | 111.432秒 | 1,567 |
| 適用後baseline（範囲限定） | 46.074秒 | 1,572 |

**2.42倍**。s5378_C・全4,551代表SAF・XID・30キューブ上限・流用off・Release。
各条件1回、中央値ではない。CPU時間クォータ2コア相当の同じクラウド環境で計測。
両方式で完了した故障のFDP不一致は0。通常Releaseの出力は独立検証済みcapture版と一致。
完了→未完了は4件、未完了→完了は9件で、完了集合は単調には増えない。

詳細とバイナリSHA256は `results.json`。

## 再現

```bash
python3 verification/normal_scope/baseline_migration/build.py
python3 verification/normal_scope/baseline_migration/check.py
```

別配置なら `BASELINE_ROOT=/path/to/baseline` と必要に応じ
`FDP_EXTERNAL_DIR=/path/to/external` を指定する。
検証・計測結果は `results.json`、各実行の設定・CSV・キューブ・ログはGit対象外の `runs/`。
通常のbaseline実行にはこの検証ディレクトリは不要。

新しいGCCで `strdup()` を宣言できるよう、baselineの `read.c` のPOSIX機能レベルを
200809Lに修正した。移植前のバイナリは元ソースを変えず `-D_DEFAULT_SOURCE` 付きでビルドする。

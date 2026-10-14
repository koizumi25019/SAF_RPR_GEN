# LOC方式の遷移故障

baseline `4aa06c7` へ verification `bf8faa8c` のTDFモデルを移植した。
baselineのSAT→XID→BDD、直接のCaDiCaL呼出し、常時EA、故障単位並列、確率計算とCSV出力の分離を維持する。

## モデルの指定と実行

CLIと `.set` の両方で `-tdf` / `-saf` を指定できる。省略時はSAF。
引数と `.set` は読み込み順に反映し、後に現れた指定を使う。

```text
-tdf
-net ../input/circuit/s27.v
-fdp ../output/full/fdp/s27_tdf_auto.csv
-log ../output/full/log/s27_tdf_auto.txt
# -fault省略: 全代表STR/STFを生成
# -limit省略または0以下: UNSATまで列挙
```

```bash
mkdir -p output/full/{fdp,log}
cd build
./main_release -set ../input/script/s27_tdf_auto.set
# CLIで.setの指定を上書きする場合
./main_release -set ../input/script/s27_tdf_auto.set -tdf -jobs 4
```

提供設定は `{s27,s208,s5378}_tdf{,_auto}.set`。
`_auto` は故障リストなし、接尾辞なしは同梱故障リストを読む。
s27/s208は無制限、s5378は `-limit 30`。
s5378の出力には `output/limit30/{fdp,log}` を作成する。
SAFとTDFのCSV・ログは異なる名前にしてある。切替だけでは出力パスは変わらない。

任意の故障リストは、各行を `信号名 STR` または `信号名 STF` とする。
空白・タブを区切りに使える。モデルと異なる故障タイプはエラー。
回路名は展開前の信号名を使い、内部の1時刻目コピー名を指定しない。
外部ファイルは既存パーサに合わせLF改行を使う。

## LOC・励起・観測・FDP

- PIは2時刻で同じ値。1時刻目のFF Qはスキャン設定可能な自由入力。
- 2時刻目のFF Qは、1時刻目のFF Dの正常値を引き継ぐ。
- STRは `L1=0, L2正常=1, L2故障=0`、STFはその逆。
- 故障は2時刻目に注入し、2時刻目のFF D（PPO）の差だけを観測する。POは観測しない。
- 正常回路は対象PPOのTFIに、1時刻目の励起のTFIを加える。
  故障回路・D-chainには観測に必要な範囲だけを使う。
- XIDは検出と励起を両方正当化し、キューブの全X割当てで両条件を維持する。
- 自由入力は元PIと初期FF状態。BDDでキューブ和集合を数え、`2^(PI数+FF数)`で割る。
- 完了時は厳密FDP、limit到達時は被覆下界で `complete=0`。キューブ間の重複は二重に数えない。

PI上のTDFは時刻間で値が変わらないため0。PPOに伝搬しない故障も0。
等価故障はBUF/INVだけで、INVではSTR/STFを反転する。FFを置換したBUFは跨がない。
SAFの支配関係は使用せず、TDFでは流用offにする。
`-jobs`の既存の規則は維持し、jobs>1と明示的な `-dom_reuse on` はエラー。
素のDFF入り回路に対応し、DFFなし、RDFF、DFFS、RDFFSはエラーにする。

## 検証の再現

```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
# baselineだけを取得したcloneでは、隔離検証器のソースを先に取得
git fetch origin verification
python3 tests/fault_pool_build.py --checker
python3 tests/tdf_check.py --binary build/main_release --debug build/main_debug \
    --capture build/fault_pool_checks/main_capture
python3 tests/tdf_medium_check.py
```

`tests/tdf_check.py` は元Verilogを独立に読み、全入力について1時刻目・FF引継ぎ・2時刻目故障注入・
PPO観測を2値シミュレーションする。本体CNF/SAT/BDDや時間展開ネットリストを参照値に使わない。
s27は128入力、s208は524,288入力。小回路はPI直結FF、同じ信号を受ける複数FF、BUF/INV、
XOR/XNOR、POのみへ伝搬する故障を含む。全CSV行（等価故障を含む）のFDPを照合する。
capture版では生成した全キューブの全X展開の健全性と、検出集合の完全被覆を照合する。
直列/4並列/Debug/Release、故障リスト明示/自動、モデル指定・不正入力、既定SAFのc17ゴールデンも確認する。

`tests/tdf_medium_check.py` はs5378の既存150故障サンプル・limit30について、直列/4並列の全CSV一致、
verificationとの両方完了したFDP一致、実キューブの独立BDD照合を行う。
検証器とcaptureフックは隔離ビルドだけに入り、出荷バイナリには入らない。

結果は `build/tdf_checks/results.json` と `build/tdf_medium_checks/results.json`、
保存した結果は `docs/transition_fault_results.json` を参照。
SAF回帰は移植前の `4aa06c7` を別バイナリにして、既存の `tests/fault_pool_check.py` で比較した。
中規模の上限付き検証は全故障の完全列挙を保証するものではない。

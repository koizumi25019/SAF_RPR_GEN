SAF FDP Gen
故障検出確率算出プログラム
縮退故障（SAF）・LOC方式の遷移遅延故障（TDF）に対応。

■ビルド
./setup.sh
mkdir -p build && cd build && cmake .. && make

■コマンド
【入力】
-set <file>	：スクリプトファイル（.set）を読み込む
-saf / -tdf	：故障モデル（.set内にも指定可能。省略時は-saf）

■サンプルコマンド
-------------------------------------------------
スクリプトファイル		：c17a.set
-------------------------------------------------
cd build && ./main_release -set ../input/script/c17a.set

TDFの5回路（s9234, s35932, s38417, b14, b17）を順番に実行（リポジトリ直下から）：
./run_tdf.sh
回路を指定する場合：./run_tdf.sh s9234 b14
TDF設定は <回路名>_tdf.set、故障リスト明示版は <回路名>_tdf_list.set。

■スクリプトコマンド（.set 内。build/ からの相対パスで記述する）
回路の配置：input/circuit/saf/ は組合せ回路（SAF用）、input/circuit/tdf/ はDFF入り順序回路（TDF用）。
.set の -net は ../input/circuit/saf/<回路名>.v または ../input/circuit/tdf/<回路名>.v を指定する。
故障モデルは -saf / -tdf で指定する（ディレクトリ名からは自動判定しない）。
-net	：【入力】回路ネットリストファイル（.v）
-fault	：【入力】故障リストファイル（省略時は選択モデルの全代表故障を自動生成）
-saf / -tdf：故障モデル。TDFの故障リストはSTR/STF、省略時は全代表故障を自動生成
-fdp	：【出力】故障検出確率 CSV
-log	：【出力】ログファイル
-limit	：【入力】故障ごとのテストキューブ生成上限（省略または <=0 で無制限＝UNSATまで完全列挙）

■注意事項
出力は実行条件（-limit の値、省略時は full）ごとにディレクトリを分けて
output/<条件>/{fdp,log}/<回路名>.{csv,txt} に生成される。

詳細は CLAUDE.md を参照。
遷移故障の設定例・観測点・検証手順は docs/transition_fault.md を参照。

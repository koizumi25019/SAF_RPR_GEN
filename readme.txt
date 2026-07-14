SAF FDP Gen
故障検出確率算出プログラム

■ビルド
./setup.sh
mkdir -p build && cd build && cmake .. && make

■コマンド
【入力】
-set <file>	：スクリプトファイル（.set）を読み込む

■サンプルコマンド
-------------------------------------------------
スクリプトファイル		：c17a.set
-------------------------------------------------
cd build && ./main_release -set ../input/script/c17a.set

■スクリプトコマンド（.set 内。build/ からの相対パスで記述する）
-net	：【入力】回路ネットリストファイル（.v）
-fault	：【入力】故障リストファイル（省略時は全代表故障 sa0/sa1 を自動生成）
-fdp	：【出力】故障検出確率 CSV
-log	：【出力】ログファイル
-cube_analysis	：【出力】キューブ解析ファイル
-limit	：【入力】故障ごとのテストキューブ生成上限（省略または <=0 で無制限＝UNSATまで完全列挙）

■注意事項
出力は実行条件（-limit の値、省略時は full）ごとにディレクトリを分けて
output/<条件>/{fdp,log,cube_analysis}/<回路名>.{csv,txt} に生成される。

詳細は CLAUDE.md を参照。

SAF FDP Gen
故障検出確率算出プログラム
現在のSB-XID版は縮退故障（SAF）専用。TDFは一時無効で、-tdf指定時はエラー終了する。

■ビルド
./setup.sh
claspのライブラリ導入手順：src/fdp/sbxid/README.md を参照。
cmake -S . -B build/sbxid -DCMAKE_PREFIX_PATH="$PWD/build/clasp-install" -DBUILD_TESTING=ON
cmake --build build/sbxid --parallel 2
ctest --test-dir build/sbxid --output-on-failure

■コマンド
【入力】
-set <file>	：スクリプトファイル（.set）を読み込む
-saf		：縮退故障（省略時のモデル）。-tdfは現在無効。

■サンプルコマンド
-------------------------------------------------
スクリプトファイル		：c17a.set
-------------------------------------------------
cd build && ./sbxid/main_release -set ../input/script/c17a.set

TDFのソース・設定・run_tdf.shは保持しているが、現在のSB-XID版では実行できない。

■スクリプトコマンド（.set 内。build/ からの相対パスで記述する）
回路の配置：input/circuit/saf/ は組合せ回路（SAF用）、input/circuit/tdf/ はDFF入り順序回路（TDF用）。
.set の -net は ../input/circuit/saf/<回路名>.v または ../input/circuit/tdf/<回路名>.v を指定する。
故障モデルは -saf / -tdf で指定する（ディレクトリ名からは自動判定しない）。
-net	：【入力】回路ネットリストファイル（.v）
-fault	：【入力】故障リストファイル（省略時は選択モデルの全代表故障を自動生成）
-saf	：縮退故障。-tdfは現在無効
-fdp	：【出力】故障検出確率 CSV
-log	：【出力】ログファイル
-limit	：【入力】故障ごとのテストキューブ生成上限（省略または <=0 で無制限＝UNSATまで完全列挙）

■注意事項
出力は実行条件（-limit の値、省略時は full）ごとにディレクトリを分けて
output/<条件>/{fdp,log}/<回路名>.{csv,txt} に生成される。

詳細は CLAUDE.md を参照。
遷移故障の設定例・観測点・検証手順は docs/transition_fault.md を参照。

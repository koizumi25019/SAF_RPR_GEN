# 現行FDPへのSB-XID移植

今回のビルドはSAF専用。`-tdf` はCLI・`.set` のどちらでも回路読込前にエラー終了する。TDFへのSB-XID移植は行わない。

同梱v1のCEGARなし版を使う。参照元は
[saxid.c（52bb7ac）](https://github.com/koizumi25019/SAF_RPR_GEN/blob/52bb7ac268d2425ab2a8089c7b929fa87d262322/SB-XID/src/saxid.c)。
本体のネットリスト・CaDiCaL・列挙・BDD・故障workerを使い、旧XIDはビルド対象から外した。旧XIDのファイルは参照用に残している。

クラウドではclasp 3.3.9の導入、本体のDebug/Releaseビルド、本物のclaspを使った小回路の完全FDP比較まで完了した。結果は [実ライブラリでの確認記録](../../../docs/sb_xid_real_clasp_checks.json) を参照。

## 呼出経路

```text
本体: CaDiCaLのSATモデル → 全PIの2値seedを作る
SBコア: 正常/故障シミュレーション → PATH/JUSTIFY → K=1のOPB
clasp接続(C++): メモリOPBを公式パーサへ渡し、最適化する
SBコア: 実行可能モデルのケアPIに元seedの値を残し、他をXにする
本体: 禁止節とCubeSetへ登録 → 次のSAT求解 → 最後にBDDでFDPを計算
```

- 各workerが自分のSB作業状態を持つ。claspは各呼出しで作成・破棄し、探索は1スレッド。
- 求解の経過時間上限は1秒。OPB生成・読込・前処理は探索時間に含めない。停止は協調的なため、1秒で中断要求した後に終了処理を待つ。
- 最適性未証明でも、取得済みの完全な実行可能モデルがあれば採用する。
- モデルなしUNKNOWNは元の2値seedへ戻す。UNSAT、元seedの非検出、解析/確保等の失敗はエラー。
- 求解後の3値検証は通常無効。`FDP_SBXID_VERIFY=1` で有効にでき、不一致時はseedへ戻して計数する。
- `FDP_NORMAL_SCOPE_VALIDATE=1` では、元seedの正常シミュレーション値を、保持した正常CNFのSAT値と照合する。
- 出力は本体のPI順の `0/1/X`。CSVの列は維持し、方式・clasp版・最適/暫定・時間切れ・UNKNOWN復帰・デバッグ検証件数をログに出す。
- 故障のlevel順整列は行わず、故障ハッシュ表の走査順を使う。EA用のlevelは維持する。

## 故障側の制約をTFOへ限定する

故障点から出力側へ到達する範囲（TFO）にある内部信号だけに、故障側専用のケア変数と正当化制約を作る。
TFO外は `varS[i] = varV[i]` とし、サイド入力の正当化は正常側の変数・制約を参照する。
元seedで正常値と故障値が一致するかでは判定しない。TFO内での偶然の値の一致は共有の根拠にしない。
故障点自体の故障側は注入値として扱い、元ゲートの入力由来の正当化は従来どおり生成しない。

TFOはworkerごとに専用ビット集合で保持し、故障点が同じならseed・縮退値が変わっても再利用する。
本体の作業フラグは変更しない。`sbxid_release()` でキャッシュも解放する。

目的関数の正常・故障両側の内部ケア費用は維持する。共有変数へは両項を計上するため、K=1では合計係数が `-2` になる。
PIの主目的重み `(K+1)*n_net+1`、PI変数、正常/故障シミュレーション、PATH/JUSTIFYの抽出範囲、PO候補、1秒制限は維持する。
シミュレーション範囲の限定化と、値に沿った追加の正当化範囲削減は行わない。

`sbxid_fault_scope_test` は小回路の故障・seedとPBOモデルを全列挙し、全実行可能モデルのキューブが全X補完で故障検出すること、
最小ケアPI数、サイド入力の参照、両側分の目的関数重み、故障点切替・キャッシュ解放・本体状態の分離を確認する。
実claspでの変更前比較と問題規模は [確認記録](../../../docs/sb_xid_fault_scope_checks.json) を参照。

範囲外PIは初回0で補完する。ただし、キューブの禁止節へ入ったPIは、以後その故障のSATモデルに従う。UNKNOWNで全PI固定のseedへ戻る場合も、同じseedを再生成せず列挙を進めるために必要である。正常CNFのゲート範囲自体は拡大しない。

## PI目的関数を正常CNFの範囲へ限定する

`-sbxid_pi_objective scope`（既定）では、CaDiCaLが保持する正常回路CNFの範囲にあるPIだけを主目的へ計上する。
`-sbxid_pi_objective all` は従来の全PI目的関数へ戻す。CLIと `.set` の両方で使え、後の指定を優先する。
実行ログの `SB-XID PI Objective` に選んだ方式を記録する。

範囲は故障点の構造TFOにあるPOを選び、その全TFIをたどったもの。
本体は `WriteTPGModel()` 後の `NormalScopeRequiredNet(pi[p]->n)` をPI順の専用配列へ保存し、worker内でその故障のSB-XID呼出しへ渡す。
`NormalScopeRequiredPi()` は禁止節で参照するようになった範囲外PIも含むため、目的関数の判定には使わない。
PI範囲をseedごとに狭める変更は加えない。

今回の比較では、PI変数の割当て、全制約式、PIの重み `(K+1)*n_net+1`、正常・故障の内部ケア費用を両方式でそろえる。
変更する目的関数の項は、正常CNFの範囲外にあるPIの正の項だけである。
範囲外PIはSBのPATH/JUSTIFYにも制約式にも不要なので、全PI版の最適解では独立にケア0へできる。
その項を除いても範囲内PIの最小ケア数と内部費用の最適値は変わらない。
ただし、探索順・同点解・時間切れ時の暫定解は変わり得る。

目的関数から外したPIのケア変数は、モデル上で1になっていても、返すキューブでは必ずXへ正規化する。
解なしUNKNOWN、任意の事後検証に失敗した場合は、従来どおり全PIの元seedへ戻す。
本体から渡したマスクがSBのPATH/JUSTIFYに必要なPIを欠く場合は、求解前にエラーとする。
全PIの一覧・順序、FDPの分母 `2^n_pi` は維持する。

速度・品質の測定結果は [PI目的関数の比較](../../../docs/sb_xid_pi_objective_comparison.md) を参照。

例（リポジトリルートから）：

```bash
mkdir -p output/sbxid/pi-objective-all output/sbxid/pi-objective-scope
build/main_release -net input/circuit/saf/s5378_C.v -limit 30 -jobs 2 -dom_reuse off \
  -sbxid_pi_objective all \
  -fdp output/sbxid/pi-objective-all/s5378_C.csv -log output/sbxid/pi-objective-all/s5378_C.log
build/main_release -net input/circuit/saf/s5378_C.v -limit 30 -jobs 2 -dom_reuse off \
  -sbxid_pi_objective scope \
  -fdp output/sbxid/pi-objective-scope/s5378_C.csv -log output/sbxid/pi-objective-scope/s5378_C.log
```

同じ参照故障・seedを用いた比較は、専用の任意ビルド対象で再現できる。
`SB-XID/tp/1f1p/` のs5378参照データを使う。回路読込・範囲抽出・CSV書込の時間は個々のSB呼出し時間に含めず、
SB呼出し時間にはOPB生成、clasp読込・前処理・探索、任意の3値事後検証を含める。

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="$PWD/build/clasp-install"
cmake --build build --target sbxid_pi_objective_benchmark --parallel 2
python3 tests/sbxid_pi_objective_compare.py \
  --binary build/sbxid_pi_objective_benchmark \
  --output output/sbxid/pi-objective-comparison --samples 4511 --trials 3

# 本体の全代表故障・30キューブ上限を比較。両方式で3値事後検証も有効にする。
python3 tests/sbxid_pi_objective_application_compare.py \
  --binary build/main_release --jobs 2 \
  --output output/sbxid/pi-objective-application
```

## 参照コードからの変更

`sbxid.c` は元の26関数を抜き出したもの。3値ゲート評価、PATH/JUSTIFY抽出、ゲート正当化式を維持し、故障数K=1で使う。
変数割当と制約生成は、上記のTFO外の重複除去を加えた。目的関数・ケアPI抽出には、上記の範囲外PIの除外を加えた。

| 対象 | 変更 |
|---|---|
| ネットリスト | 本体のNLISTを使用。元のNetlist.c/hは組み込まない |
| `value` / `signal` / `flag` | `sbxid_normal_value` / `sbxid_fault_value` / `sbxid_flag` に変更 |
| 関数・作業領域 | `sbxid_` / `SBXID_` 名とファイル内staticへ変更 |
| `FAULT_CTX.signal` | ノード値と区別し `fault_values` 配列へ変更 |
| 入出力 | Windowsのmain・ファイル読込・system呼出し・結果解析を除去。OPBをopen_memstreamへ出力 |
| OPBヘッダ | 書き戻し前の絶対末尾位置を保存し、書き戻し後にそこへseekする。SEEK_ENDだけでは本文が切れるため使わない |
| 後方含意の行バッファ | 固定2048バイトを入力本数から確保する長さへ変更。出力式は同じ |
| エラー | 確保失敗・キューoverflow・未対応ゲートをエラーにする。overflow時にイベントを捨てて継続しない |
| ケース開始 | 専用値をX、専用flagを0へ初期化する。元seedのPO検出も確認する |
| 求解後検証 | 通常省略し、設定で有効化する |

ゲートの正当化式、目的関数の重みと内部項を維持し、TFO外の故障側の変数・制約の重複を除いた。
PI目的関数の範囲は上記の切替を持つ。CEGARの追加は行っていない。対象は既存の組合せ回路で、dangling観測条件の変更・生の順序回路対応は含めない。

## clasp導入前に実行できるテスト

CaDiCaL/CUDDは既存のものを使う。以下はclaspを取得・実行しない。

```bash
cmake -S . -B build/sbxid-core -DFDP_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build/sbxid-core --parallel 2
ctest --test-dir build/sbxid-core --output-on-failure
```

`sbxid_core_test` の求解器はテスト専用の代替実装である。生成した制約を満たす既知モデルを返し、候補抽出、UNKNOWN/UNSAT/エラー、任意の事後検証、状態分離を確認する。`normal_scope_pattern_test` は禁止節へ入った範囲外PIの補完と次故障へのリセットを確認する。これらはclasp接続や本物のSB-XID実験の合格を意味しない。

## clasp導入と本体ビルドを別環境で再現する手順

Linux/WSL2のリポジトリルートで実行する。既に同じディレクトリにclaspがある場合はcloneを繰り返さない。

```bash
sudo apt update
sudo apt install -y build-essential cmake git

git clone --depth 1 --branch v3.3.9 --recurse-submodules --shallow-submodules \
  https://github.com/potassco/clasp.git external/clasp

cmake -S external/clasp -B build/clasp \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
  -DCLASP_BUILD_APP=ON -DCLASP_BUILD_EXAMPLES=ON -DCLASP_BUILD_TESTS=OFF \
  -DCLASP_BUILD_WITH_THREADS=ON -DCLASP_INSTALL_LIB=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/clasp-install"
cmake --build build/clasp --parallel 2
cmake --install build/clasp
build/clasp/bin/clasp --version
build/clasp/bin/clasp_examples

cmake -S . -B build -DBUILD_TESTING=ON \
  -DCMAKE_PREFIX_PATH="$PWD/build/clasp-install"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

非同期の待機/中断APIのため、`CLASP_BUILD_WITH_THREADS=ON` が必要。これは各worker内で探索スレッドを複数にする指定ではない。CMake 4.xで古い依存のポリシーエラーが出る場合、claspの構成へ `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` を追加する。

基準はclasp v3.3.9（`4c708a73fdcb78049e9c3bad863a3f6eab437a07`）とそのlibpotassco（`fa39da44cf9643e1b56389d3dfed6af773305af3`）。本体はCで、`clasp_bridge.cpp` だけC++としてコンパイルする。CMakeが公式 `libclasp` ターゲットをリンクし、未導入時は構成段階でエラーにする。代替求解器で本体を動かす機能は設けない。

導入後は、`clasp_bridge_test` で既知最適解、UNSAT、不正OPB、モデルなしの時間切れ、時間切れ時の暫定モデル保持、時間切れ後の次求解を確認する。その後、`c17a` / `s27_C` の無制限列挙で故障ごとの完全FDPを旧版と比較する。キューブ数やCSV全体のバイト一致は要求しない。完全FDPが一致してから速度を評価する。

## このクラウド環境での実行

導入先は `build/clasp-install`、本体は `build/main_debug` と `build/main_release`。Windowsのexeや外部のclaspプロセスを起動せず、リンク済みのライブラリを使う。

```bash
mkdir -p build/sbxid-results
build/main_release \
  -net "$PWD/input/circuit/saf/c17a.v" \
  -fdp "$PWD/build/sbxid-results/c17a.csv" \
  -log "$PWD/build/sbxid-results/c17a.log" \
  -limit 0 -jobs 2 -dom_reuse off
```

この環境ではCMake/CTestは `/workspace/.onboarding-tools/bin/cmake` と `/workspace/.onboarding-tools/bin/ctest` を使った。再ビルドする場合は、上記の一般的な手順のcmake/ctestをこの絶対パスで実行できる。手元のLinux/WSL2への導入は別途、同じ再現手順を使う。

## 今回の確認状況

- clasp未導入の構成で4つのCTestが通過。
- 本体の全21 C翻訳単位の構文確認が通過。
- 作業用ディレクトリで本体を「常にUNKNOWNを返すテスト専用代替実装」へリンクし、c17a（36行）、s27_C（52行）のjobs=1/2で、故障ごとのFDP・完了状態が改変前と一致。これはUNKNOWN復帰と本体接続の確認であり、claspによる一般化実験ではない。
- 続いてclasp 3.3.9とlibpotassco 1.1.0をcloneし、スレッド対応の静的ライブラリをプロジェクト内へ導入。C++接続、本体Debug/Releaseのビルド、公式サンプル、5つのCTestが通過。
- 本物のclaspでc17a（36行）とs27_C（52行）の完全FDPが改変前と一致。各回路でDebug/Release、jobs=1/2、支配流用on/off、任意の事後検証を確認した。事後検証を有効にした157求解で失敗0件。
- 独立したPBOで約1.000秒での打ち切りを確認。解がない場合はUNKNOWN、暫定モデルがある場合はFEASIBLEとなり、保存したモデルが全制約を満たすことも確認した。
- 今回の受入れ比較は上記2回路。記録した経過時間は動作確認の観測値であり、速度比較の結論には使っていない。詳細は [実ライブラリでの確認記録](../../../docs/sb_xid_real_clasp_checks.json) に保存した。

最新版baseline `65ad73b` の観測出力TFI・常時EA・回路配置へ統合後も、6件のCTestと上記2回路・10条件の完全FDP比較が通過。`tdf_model` の代わりに `tdf_disabled` がTDFの実行拒否を確認する。TDFのソース・設定は将来の移植用に残している。

# 正常CNFの範囲限定（baseline）

baselineの通常実験では常時有効。既存の `.set` をそのまま使える。
故障回路・D-chain・検出・必須割当ては、従来どおり `ccadical_add()` で直接投入する。
CNF投入用のマクロやラッパー、検証器を本体に追加していない。

## 残す範囲（対象出力TFI）

1. `SearchTFO()`で故障点からのTFOを求める。
2. 故障節やEAより前に`NormalScopeBuild(target)`を呼び、TFOに含まれる観測点を選ぶ。
   SAFは宣言されたPO、LOC/TDFは2時刻目のPPO（FF D入力）を使う。
3. 観測点の全TFIをたどり、サイド入力の生成回路もPIまで含める。
   TDFでは、さらに1時刻目の励起ネットの全TFIも正常CNFへ加える。
4. 故障回路・D-chainは、観測点のTFIに含まれるTFO部分について生成する。
   励起用に追加したTFIは故障伝搬の範囲に混ぜない。
5. EAを常に実行し、範囲内の信号についてだけ単位節を投入する。EAを理由に範囲は広げない。

EAの含意処理自体は従来どおり全回路で行う。範囲限定化の考え方は、
Eggersglüß et al., **A New SAT-based ATPG for Generating Highly Compacted Test Sets**, DDECS 2012,
Section II-B / Fig. 2を参照する。
[著者公開PDF](https://agra.informatik.uni-bremen.de/doc/konf/12DDECS-MultipleTarget.pdf#page=3)、
[DOI: 10.1109/DDECS.2012.6219063](https://doi.org/10.1109/DDECS.2012.6219063)。
EA単位節の投入先制限は本ツールの設計であり、論文のEAアルゴリズムを実装したという意味ではない。

XIDも同じ対象PO/PPOだけを検出先として扱い、内部で途切れる枝を出力と誤認しない。
到達可能な観測点がない故障は空節でUNSATにし、FDPを0とする。
閉包内の正常ゲートのCNFだけを元の変数番号・順序で投入する。
省略部分は任意の全PI割当てから元の論理どおり値を補えるため、検出可能な全PI集合は変わらない。
正常CNFの配列自体は従来どおり回路全体について一度だけ作成する。

XIDへ渡す全正常信号値は、必要なPIのSAT値から論理シミュレーションで復元する。
範囲外PIはソルバの未登場変数を読むことなく0で補完し、最終キューブでは明示的にXにする。
全PIの一覧・順序・`n_pi`・FDP分母`2^n_pi`は変更しない。

## 実行と値照合

正常CNFを限定しないモードは持たない。旧`FDP_NORMAL_SCOPE`環境変数は参照しない。
EAも常時ONで、`-ea`などの切替は持たず、旧`MDC_NOEA`環境変数も参照しない。
既存の`.set`をそのまま使う。`.set`内の相対パスはbuildからの実行を前提とする。

```bash
cd build
./main_release -set ../input/script/saf/c17a.set
# 必要範囲のSAT内部値と、PIから復元した値を照合する
FDP_NORMAL_SCOPE_VALIDATE=1 ./main_release -set ../input/script/saf/c17a.set
```

実行ログには`Normal CNF Scope : on (affected-output TFI)`と`Essential Assignment : on`を記録する。
故障並列化は`.set`の`-jobs N`で指定する。設定・時間の読み方は
[fault_parallel.md](fault_parallel.md)を参照。

## 旧方式の移植時検証・計測（2026-10-07）

2026-10-07、適用前baseline `5da133b` と比較。
検証用の別ビルドでbaselineが実際に生成したキューブを保存し、元回路から独立に
検出関数を構築するGT検証器へ渡す。本体にGTやキューブ保存処理は含めない。
小回路では元Verilogから全入力を独立シミュレーションする検証も行う。

結果と再現スクリプトは `verification-normal-cnf-scope` ブランチの
`verification/normal_scope/baseline_migration/` に保存する。
同じライブラリ・Release設定の適用前/適用後を各1回実行し、外部の単調時計で経過時間を測る。
当時のbaselineログはCPU時間のみ。現在は故障並列化に伴い、経過時間も`Time`として記録する。

c17aの既知FDPとDebug/Releaseが一致。9小回路の無制限FDPは適用前baselineと一致し、
baselineの実生成キューブの独立GT検証を全て通過。最初の4回路は全入力シミュレーションも通過。
流用on、EA無効、D-chain無効の確認も通過。s5378_Cは全4,551代表故障について
非健全キューブ0件、完了被覆の不一致0件。

| 条件 | 経過時間 | 完了故障数 |
|---|---:|---:|
| 適用前baseline（全正常CNF） | 111.432秒 | 1,567 |
| 適用後baseline（範囲限定） | 46.074秒 | 1,572 |

**2.42倍**。s5378_C・全4,551代表SAF・XID・30キューブ上限・流用off・Release。
各条件1回、中央値ではない。CPU時間クォータ2コア相当の同じクラウド環境で計測。
両方式で完了した故障のFDP不一致は0。通常Releaseの出力は独立検証済みcapture版と一致。
完了→未完了は4件、未完了→完了は9件で、完了集合は単調には増えない。


範囲限定でSATの解順序が変わり、上限付きの部分FDPや完了故障集合は変化し得る。
未完了故障のFDPは下界。範囲限定だけで全故障の完全列挙を保証するものではない。
この正常CNF限定の検証対象はbaselineのSAF/XID。現在は故障並列化もbaselineへ移植済み。

GCC 14でも従来の `strdup()` を正しく宣言できるよう、`read.c` のPOSIX機能レベルを
`200809L` に修正した。適用前バイナリは元ソースのまま `-D_DEFAULT_SOURCE` を指定してビルドした。

## 対象出力TFIへの移植検証（2026-10-13）

変更前の最新baseline `52bb7ac`を基点に、verification `bf8faa8`の範囲限定化を移植した。
baselineでは直接の`ccadical_add()`を維持し、EA設定の解析や非検出オラクルを追加していない。
検証器・キューブ保存は隔離したビルドだけに入れる。

再現:

```bash
python3 tests/fault_pool_build.py --checker
python3 tests/fault_pool_check.py --reference build/fault_pool_checks/main_reference \
  --capture build/fault_pool_checks/main_capture --checker build/fault_pool_checks/check_covers
```

独立検証器のビルドにはverificationの`bf8faa8`のGitオブジェクトが必要。
未取得なら先に`git fetch origin verification`を実行する。
CMakeの通常ビルドと`ctest --test-dir build --output-on-failure`も使用できる。

9小回路の完全FDPを変更前と比較し、直列・並列のCSV一致、c17ゴールデン、Debug/Release、
c17分岐故障の保持範囲、内部で途切れる枝、常時ONのEA、設定・中断・I/O失敗を検証する。
s5378_Cは上限30で全4,551代表故障を検証し、実生成キューブを元回路の独立GTへ渡す。
上限付きの比較では、両方式で完了した故障のFDPを照合する。
結果は`docs/output_tfi_scope_results.json`に保存する。

今回の検証はすべて通過した。9小回路の完全FDPは変更前と一致し、直列・4並列のCSVも一致。
c17のi→k分岐は正常ゲート定義9個、i→l分岐は8個となった。内部で途切れる枝は非観測として扱う。
EA旧無効化フラグの無視、設定、中断、ワーカー回収、I/O失敗、既存の2ユニットテストも通過。
c17a・s208_C・s5378_Cの実生成キューブを独立GTで検証し、いずれも非健全被覆0件・
完了被覆の不一致0件。保存版と通常版のCSVはバイト単位で一致した。
s5378_Cは全4,551代表故障、上限30、完了1,572件。変更前との共通完了故障のFDP不一致も0件。

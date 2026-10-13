# 対象出力TFIによる正常CNF限定化とEA比較

verificationの576ee07を基準に実装した。正常CNF範囲をEAより先に確定し、EAによる範囲拡大を止める。

## 範囲の定義と参考文献

故障点fのTFOに含まれる観測可能な外部出力をO_fとし、そのTFIの和集合R_fを正常回路の範囲にする。
TFIには出力自身と全サイド入力の生成回路を含め、元のPIまでたどる。通常のSAFでは宣言されたPO、
LOC/TDFでは2時刻目のキャプチャPPOを観測点とする。内部で途切れる枝は観測点に含めない。

正常CNF限定化の考え方は、以下のSection II-B / Fig. 2を参照した。

Stephan Eggersglüß et al., **A New SAT-based ATPG for Generating Highly Compacted Test Sets**, DDECS 2012, pp. 230–235.
DOI: [10.1109/DDECS.2012.6219063](https://doi.org/10.1109/DDECS.2012.6219063)
[著者公開PDF](https://agra.informatik.uni-bremen.de/doc/konf/12DDECS-MultipleTarget.pdf#page=3)

論文は故障の励起・伝搬に構造的に関係する正常回路部分の利用を説明している。本実装ではその範囲を
「対象出力のTFI」として明示的に計算する。EA単位節の投入先制限は本ツールの設計であり、
論文のEAアルゴリズムを実装したという意味ではない。

## 実装

1. `NormalScopeBegin()`で故障ごとのマスクを初期化する。
2. `SearchTFO()`の後、`NormalScopeSelectOutputs()`で対象出力を選び、全TFIを閉じる。
3. 故障回路とD-chainは、この範囲に含まれるTFO信号について生成する。
4. EAの推論は従来どおり全回路で続け、R_f内の信号についてだけ単位節を追加する。
5. TDFの1時刻目励起・低電力の制約がある場合は、それらに必要な正常信号のTFIも残す。
6. 選んだ正常ゲート定義をソルバへ投入する。

限定化は常時有効で、旧`FDP_NORMAL_SCOPE`は参照しない。非検出オラクルへの全正常定義の投入は維持する。
全PI・PI順・FDPの分母も維持する。XIDの正常値はPIから復元し、無関係なPIはXになる。
EA OFFでも、独立に生成する故障励起・出力差・D-chainの制約は残る。

TFOがすべて観測出力に到達する組合せ回路では、TFI(TFO)とTFI(O_f)は同じ。
主な変更はEA割当て先を理由に正常CNFを広げないことと、対象出力から範囲を明示的に定義すること。

## 設定

既存setファイルに以下を追加する。

```text
-ea on
-jobs 2
-dom_reuse off
-limit 30
```

EAなしは`-ea off`。CLIの`-set ... -ea off`でも上書きできる。省略時はEA ON、
従来の`MDC_NOEA`が設定されている場合だけOFFになる。明示的なset/CLI指定が環境変数より優先する。
ログに`Normal CNF Scope: on (affected-output TFI)`と`Essential Assignment: on|off`を記録する。

## ビルド・再現

```bash
python3 verification/output_tfi_scope/build.py
python3 verification/output_tfi_scope/check.py --phase smoke
python3 verification/output_tfi_scope/check.py --phase large
python3 verification/output_tfi_scope/check.py --phase large --circuits s9234_C
python3 verification/output_tfi_scope/check.py --phase benchmark --jobs 2 --circuits s5378_C s9234_C
```

ビルドは既存のCaDiCaL/CUDDライブラリを使う。別worktreeの依存を使う場合は
`FDP_EXTERNAL_DIR=/そのworktree/external`をビルド時に指定する。
基準バイナリは576ee07のソースから隔離してビルドする。通常の出力を上書きせず、実行データは`runs/`に保存する。

## 検証と測定の区別

smokeは小回路9件の完全FDP・c17ゴールデン・EA ON/OFFの範囲一致・分岐故障の保持範囲・
CORE/TDF/低電力の互換・内部で途切れる枝・Debug/Release・設定優先順を検証する。
独立GTはソルバ/CNF/XIDを使わず元回路から検出関数を構築する。
largeはs5378_Cの全4,551代表故障について、EA ON/OFF両方の生成被覆を独立GTで照合する。
`--circuits s9234_C`を指定すると、同様にs9234_Cの全6,927代表故障を照合する。

時間測定はGT・値照合などの検証フックを外し、同じReleaseバイナリ・同じ回路・同じ故障集合・
同じ並列数・上限30・流用offでEAだけを変える。各条件1回、順番に実行する。
実経過時間と全ワーカーを含むCPU時間の両方を記録する。

上限付きの部分FDPはSATの解順序によって変わり得る。両方で列挙完了した故障の厳密FDPを照合し、
未完了の部分FDPまで同一とは主張しない。全故障の完全列挙時間の比較でもない。

結果は`smoke.json`・`large.json`・`large_s9234_C.json`・`benchmark.json`に保存する。

## 単回測定結果（2026-10-13）

Release・SAF/XID・2並列・limit30・流用off・クラウドCPUクォータ2コア相当。
GTとSAT値照合を含めず、同じバイナリでEA ONを実行した後にOFFを実行した。
表の実時間はプログラムの`Time`。外部の単調時計で測った起動〜終了時間もJSONに記録する。

| 回路 | EA ON実時間 | EA OFF実時間 | ONによる実時間短縮 | EA ON合計CPU | EA OFF合計CPU |
|---|---:|---:|---:|---:|---:|
| s5378_C | 19.352秒 | 20.583秒 | 6.0% | 38.542秒 | 40.989秒 |
| s9234_C | 57.331秒 | 64.237秒 | 10.8% | 114.301秒 | 128.045秒 |

短縮率の分母はEA OFFの時間。単回測定なので中央値や安定した優劣を主張しない。
この2回路では、正常CNFを広げずにEAを残す方針が有力。

| 回路 | ON/OFF共通の正常ゲート定義投入延べ数 | 完了故障ON | 完了故障OFF | 両方完了の故障 |
|---|---:|---:|---:|---:|
| s5378_C | 2,461,346 | 1,572 | 1,571 | 1,571 |
| s9234_C | 8,207,563 | 3,046 | 3,052 | 3,044 |

両条件で完了した故障のFDP不一致は0件。正常ゲート定義の延べ数もON/OFFで一致。
EAを変えるとキューブ選択順が変わるため、上限内で完了する故障数も変わり得る。
EA ONがすべての故障で上限付きFDPや完了率を改善するという意味ではない。

## 最終検証結果

小回路9件の完全FDPは、EA ON/OFFと変更前576ee07で一致し、c17ゴールデンも一致。
CORE・TDF・低電力、Debug/Release、設定優先順、非観測の内部枝の検証も通過した。
c17のi→k分岐故障は正常ゲート定義9個、i→l分岐故障は8個となり、EA ON/OFFで同じ範囲。

s5378_Cは全4,551代表故障、s9234_Cは全6,927代表故障について、EA ON/OFF両方を独立GTで確認した。
いずれも非健全キューブ0件・完了被覆の不一致0件でALL VERIFIED。
時間測定時の4つのCSVは、対応する独立GT実行のCSVとバイト単位で一致した。
検証処理を含む実時間を、上記の性能比較には使用していない。

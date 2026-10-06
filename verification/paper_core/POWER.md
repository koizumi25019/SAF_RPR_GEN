# 遷移故障 CORE と低消費電力制約

2026-10-06。C の本体に実装。従来の LOC（PI は2時刻で共有、DFF は1時刻目の D を引き継ぐ）と、
2時刻目 PPO のみを観測する遷移故障モデルを維持する。

## .set の指定と実行

```text
-tdf
-dc_method core
-dom_reuse off
-core_verify off
-low_power on
-wsa_threshold 20
```

制約なしは `-low_power off`。省略時も off。
on の場合は TDF / CORE / 0〜100 の整数閾値を必須とし、欠落・不正値・SAF・XID の指定はエラーにする。
閾値は秒数やケアビット数ではなく、元回路の信号線数に対する百分率。
`core_verify off` でも通常の core 再確認・極小化は実行する。

既存のシェル→C→.set の流れで使える。

```bash
cd /workspace/SAF_RPR_GEN_core
# s208: 低電力なし／20%制約あり
bash run_paper_core_experiments.sh s208_tdf_core s208_tdf_core_lp20
# s27 の設定も用意済み
bash run_paper_core_experiments.sh s27_tdf_core s27_tdf_core_lp20
```

設定例は `input/script/{s27,s208}_tdf_core{,_lp20}.set`。
出力は `output/paper_core/tdf/{power_off,lp20}/{fdp,log}/` に分離する。
閾値を変える場合は `.set` の `-wsa_threshold` と出力先も変更する。
ログに on/off、百分率、元信号線数、整数上限を記録する。
s27 は20%だと検出可能なパターンが0になった（独立全列挙でも一致）。50%では検出可能。

## 制約の意味

添付 `mttg_pbo_generate.c` の信号遷移数制約と同じ種類の制約を CNF にする。
正常回路の2時刻の値を比較し、

\[
 W(x)=\sum_{s\in S}\bigl(v_s^{(1)}(x)\oplus v_s^{(2)}(x)\bigr),\qquad
 P(x)\iff W(x)\le\left\lfloor |S|\,\theta/100\right\rfloor.
\]

`S` は **2時刻展開前のパーサ上の全信号線**。PI、DFF Q、ゲート出力、FOUT 分岐・PO分岐を含む。
各元信号を1回数え、2時刻のコピーを二重に数えない。共有 PI の寄与は0。
閾値の母数にもこの元信号線数を使い、展開後の `n_net` は使わない。
添付コードは `open_pass==NO` の信号を選別するが、現行ネットリストにはその属性がないため全信号が対象。
ファンアウト／容量による追加重み・グリッチ・故障回路の消費電力を表す指標ではない。

XOR、平衡木の二進加算器、上限比較器を完全な等価 CNF で定義する。
電力判定リテラルは `p ↔ P(x)`（符号付きの場合もある）。一方向のカウンタ制約ではなく、
`p` と `¬p` のどちらも厳密に使える。定義 CNF は回路当たり一度構築して各ソルバに再利用する。
GC・電力補助変数・故障回路・比較器の採番を分離し、衝突を避ける。

## CORE の肯定側・否定側

`z` は正常／故障 PPO の不一致を OR でまとめた検出フラグ。
`e1` は1時刻目の初期値、`e2` は2時刻目の遷移先を表す励起条件。
STR: `v1=0 ∧ v2=1`、STF: `v1=1 ∧ v2=0`。

- 生成側: `C_f ∧ z ∧ e1 ∧ e2 ∧ p`。
- CORE側: `C_f ∧ cube ∧ (¬z ∨ ¬e1 ∨ ¬e2 ∨ ¬p)`。
- 制約なし: `p` を省略して同じ手順。

`C_f` は正常回路、故障回路、比較器、電力カウンタの定義。
否定側に生成側の検出／励起／電力の固定単位節、D-chain、EA、禁止節を入れない。
「¬z、¬e1、¬e2、¬p をそれぞれ必須にする」形ではなく、**条件全体の否定を1つの OR 節**にする。
反例問い合わせが UNSAT なら、X の全割当てで検出・励起・電力上限が同時に成立する。
core 抽出後の削除による極小化も、同じ反例ソルバを使う。
2時刻目の励起は検出条件からも含意されるが、TDF の否定節に明示した。

FDP は `Pr[検出 ∧ 励起 ∧ P]`。従来通り、全自由入力（PI＋初期DFF状態）の
`2^n` を分母にする。`Pr[検出 ∧ 励起 | P]` への正規化は行わない。
limit に達した場合は、この事象の被覆下界。
独立 BDD 検証と `BDD_EXACT` も電力制約込みに対応する。
AIG export は電力制約を含められないため、low_power on との併用はエラー。

## 検証

`test_power_encoding.c` で0〜8変数、閾値の境界も含め、5,119完全割当てについて
定義だけなら SAT、正しい判定極性なら SAT、反対極性なら UNSAT を確認した。

`verify_tdf_power.py` は元 Verilog を独立に読み、正常回路の1時刻目→DFF引継ぎ→2時刻目と、
2時刻目への故障注入を2値ビット並列で評価する。各入力割当ての遷移数は NumPy で直接数える。
参照判定には本体 CNF／SAT／BDD を使わない。

| 回路 | 閾値 | 検証内容 |
|---|---|---|
| s27 | off / 0 / 20 / 50 / 100% | 各128入力、全52故障CSV行 |
| s208 | off / 20 / 50 / 100% | 各524,288入力、全416故障CSV行 |
| 追加小回路 | off / 34% | 各32入力、全12故障CSV行 |

11ケース、1,644代表故障処理、1,225キューブ。
core直後の延べ4,279,955展開、極小化後の延べ8,146,302展開は全て検出・励起・電力条件を満たした。
全検出集合の被覆・FDP・等価故障エコー行の FDP も一致。0%と100%の境界も確認した。

追加小回路の同じ遷移故障では、制約なしの `1X0XX` が検出する8入力のうち電力違反がある。
34%（6信号中最大2遷移）では `10000` / `11010` の2入力だけを許可し、
電力上限のために必要なケアビットを保持することを確認した。

この小回路で、PI が直接 DFF に入る場合の既存の等価解析不具合も検出した。
共有 PI に `peer_1t` があるため、入力側の peer の有無だけでは DFF 境界を識別できなかった。
Q の1時刻目コピーが自由状態入力であることでも境界を確認し、等価解析・CSVエコーの両方を修正した。

Debug/Release の s27(50%)・s208(20%)、limit1＋BDD_EXACT の FDP 一致も確認済み。
既定 SAF の c17 ゴールデンと通常9回路の GT_BDD 回帰も `test_power_settings.py` で全て一致した。

```bash
python3 verification/paper_core/test_power_settings.py
python3 verification/paper_core/verify_tdf_power.py --output output/tdf_power_new_check
```

全展開検証は GCC/同梱 CaDiCaL/CUDD と Python/NumPy を使う。
集計結果は `results/tdf_power_simulation.json`、今回の生データは `output/tdf_power_checks_all_faults/`。
実装検証では中規模の全展開シミュレーションは実行していない。
後続の低電力ON性能測定（s27/s208/s5378/s9234）は `POWER_BENCHMARK.md` を参照。
20%制約の中規模2回路は1,800秒の実時間上限で未完走。

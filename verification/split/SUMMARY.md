# Shannon 分割による完全化（SPLIT）と、incomplete 根絶の到達限界

実施日: 2026-07-09 / ブランチ: verification
関連: [[cube-explosion-research-state]] [[bdd-exact-fallback]]
前段: `verification/dual/SUMMARY.md`（双対列挙）、`verification/bdd_exact/SUMMARY.md`（従来手法のため不採用）

## 手法（`src/fdp/experiment.c` 案4、env `SPLIT=1`、既定無効）

打ち切りになった故障の未被覆空間 rem = path ∧ ¬(U∪V) を PI で二分（Shannon 分割）しながら、
各部分空間で検出側 U と非検出側 V のキューブを `SPLIT_BUDGET`（既定8）本ずつ SAT 列挙する。
両側 UNSAT でその部分空間は閉包。分割変数は rem の BDD サポートから選ぶ。
深さ ≤ n_pi で必ず停止し、終了時 U = D_f（キューブは本物のテスト、fdp は従来経路で厳密）。
無改造 CaDiCaL の assumption のみで実現（#SAT の CDP / DPLL-trace 圧縮の分割統治に相当、
ただし component caching なし）。`SPLIT_MAXNODES`（既定100万）超過で従来の capped に戻る安全弁つき。

## 結果

- **c17a, -limit 2（強制打ち切り12故障）**: 全12故障を SPLIT が完全化。nodes=14, 追加 u_cubes=21。
  **GT ALL VERIFIED（exact 含む）・fdp ゴールデン一致・incomplete 0**。機構は正しく動く。
- **s5378, -limit 30, SPLIT単体（MAXNODES 既定100万）**: 最初の難故障で発散し **1800s 無出力**。
- **s5378, -limit 30, SPLIT_MAXNODES=2000 + MAXDC_CORE**: 1500s で 1754/10538 行しか進まず
  （うち incomplete 238 ≒ baseline 同区間の約半分）。**incomplete を半減させるのに ~150x の時間**。

## 結論: 不採用（コスト対効果が成立しない）

det_paths が兆〜京単位の本質的複雑 D_f では、component caching の無い分割は理論どおり
指数的に発散する。node 上限で抑えると「非常に高価な部分改善」にしかならない。

## この時点で確定した全体像（incomplete 根絶の到達限界）

制約（CaDiCaL 改造なし・外部モデルカウンタなし・D_f の構造的BDD直接構成＝従来手法は不可）の下で:

1. **キューブ集合そのものが出力である限り、素項被覆が指数サイズの故障は原理的に完全化できない**
   （出力オブジェクトが指数長。s5378 n673gat/sa0 は det_paths=62G、BDDノードはわずか788
   ＝「コンパクトな非SOP表現なら小さいがSOPでは指数」の典型）。
2. 完全化できる系: (a) 構造からのBDD直接構成（従来手法・不可）、(b) 探索トレースからの
   知識コンパイル（= モデルカウンタの自作に相当）、(c) ソルバ改造列挙。いずれも制約に抵触。
3. よって許された設計空間での現実解は:
   - **incomplete の最小化**: `MAXDC_CORE`（冗長カバー爆発の解消）+ `DUAL`（¬D_f が小さい故障の救済、
     s5378_l30 で +146 complete）
   - **残った incomplete への証明付き区間**: `DUAL` の anytime 上下界 [P(U), 1−P(V)]
   - 参照値・検証用に `BDD_EXACT`（従来手法、ツール扱い）

実装は全て env オプトインで残置（SPLIT 含む）。デフォルト動作は不変
（c17a ゴールデン・s27_C・s1494 冗長12 全 PASS を本日再確認）。

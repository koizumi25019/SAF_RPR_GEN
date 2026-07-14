# 双対列挙（DUAL）: 非検出空間の並行列挙による早期完了と anytime 上下界

実施日: 2026-07-09 / ブランチ: verification
関連: [[cube-explosion-root-cause-prime-expansion]] [[maxdc-core-reeval-post-fix]]
土台: `verification/maxdc_qx/SUMMARY.md`（素項展開 MAXDC までの評価）

## 動機と候補選定

キューブ生成回数の爆発（＝実行時間膨大）に対し、model counting ツール**以外**の解決策を文献調査した。
候補（いずれも SAT ベース）:

1. **双対列挙**（本実装）— 検出空間 D_f と非検出空間 ¬D_f を並行列挙し、早く終わる側で完了を確定
   （Möhle & Biere の dual reasoning / dualizing projected model counting の列挙版アレンジ）。
2. 構成的素項列挙（Previti+ IJCAI'15 の二重ソルバ prime compilation の PI 射影版）— MAXDC が既に
   モデル起点の素項列挙になっており増分が薄いと判断。
3. ブロッキング節なし列挙（chronological backtracking / tabularAllSAT, Spallitta+ 2024）—
   CaDiCaL 内部への改造が必要で重い。将来候補。
4. 位相・決定順ヒューリスティック — DIVPO/DIVPHASE で試行済み・総本数を減らせず却下済み。
5. 難故障のみ D_f を BDD 直接構築へフォールバック — GT_BDD の本番転用。SATキューブ列挙という
   手法自体を放棄するため保留。

## 仕組み（`src/fdp/experiment.c` 案3、env `DUAL=1`、既定無効）

- U = 検出側キューブ和集合（従来の列挙）、V = 非検出側キューブ和集合。
  V は非検出 CNF（z=0）の解を、検出 CNF（z=1）をオラクルに UNSATコア→QuickXplain 救済で
  ¬D_f の素項へ拡大したもの。U ⊆ D_f と V ⊆ ¬D_f は互いに素。
- 完了条件（det 側 UNSAT に追加）:
  - **閉包**: U∪V が恒真 → U = D_f 確定。
  - **V完了**: 非検出側 UNSAT（V = ¬D_f 確定）→ 残り D_f\U = ¬(U∪V) を BDD の disjoint パスで
    取り出し U へ補充して即完了。**ただしパス数 > 4·|U|+64 なら見送り det 列挙続行**
    （無制限だと b12 で 1,637万パスが出て cube_cnt と支配流用の種が爆発した）。
- **遅延起動** `DUAL_START`（既定16）: det 側がこの本数未満で終わる易しい故障では V 側を
  一切動かさない（ソルバ構築も遅延）。
- 終了時 U = D_f が保証されるため、fdp・complete・GT_BDD・支配流用の不変条件は全て保たれる。
- 打ち切り（limit）時は副産物として fdp の**上下界** [P(U), 1−P(V)] を stderr に出す
  （従来は下界 P(U) のみで、誤差の証明が無かった）。

## 実験結果（main_release, WSL2）

### b12_C, -limit 100000, 全2872代表故障, `-fault` なし

| 手法 (env) | wall | total_cubes | incomplete |
|---|---:|---:|---:|
| baseline | 309.2s | 2,451,598 | 4 |
| `DUAL=1` | 232.8s | 1,704,353 | **0** |
| `MAXDC=1 MAXDC_CORE=1` | **55.8s** | **125,114** | 0 |
| CORE + `DUAL=1` | 96.1s | 147,583 | 0 |

DUAL 単体は baseline に対し 1.3x 速・全 complete 化（vcomplete=545 / closed=15）。
ただし**素項展開 MAXDC_CORE が依然圧勝**で、CORE への DUAL 上乗せは逆効果（V側 solve 8.2万回の overhead）。

### s5378_C, -limit 30, 全4551故障（GT_BDD=1 で厳密検証）

| 手法 | wall | incomplete |
|---|---:|---:|
| baseline | 56.6s | 2,980 |
| `DUAL=1` | 100.0s | **2,834**（+146 complete 化） |

**ALL VERIFIED（UNSOUND=0 / complete-but-NOT-exact=0）**。capped 2,834 故障には
`[DUAL] capped ... bounds=[lo,hi]` の上下界が出る。これは MAXDC にも XID にも無い固有能力
（det 側列挙が limit 内で終わらない故障でも fdp を確定・または区間で証明できる）。

### s641_C, 無制限（複雑 D_f・低fdp 側が重い代表）

| 手法 | wall | total_cubes |
|---|---:|---:|
| baseline | 11.1s | 81,040 |
| `DUAL=1` | 20.4s | 102,291 |
| `DUAL=1 DUAL_START=64` | 17.1s | 91,925 |

fdp は全一致。完全列挙が元々終わる回路では純オーバーヘッド（〜1.8x）。DUAL_START で緩和可。

### 回帰（デフォルト＝env なしは無変更）

- c17a ゴールデン一致（DUAL on/off とも）＋ GT ALL VERIFIED。
- s27_C（limit100）期待値一致（missing/mismatch 空）。
- s1494 全故障で冗長故障 12（代表・complete=1 で数える）。

## 追補（2026-07-09）: V ドレイン（DUAL_VLIMIT / DUAL_REMCAP）

det 側が limit で打ち切られた後、**V 側だけを回し続ける「ドレイン」**（`EXP_DualDrain`）を追加した。
V キューブは成果物でなく閉包判定用の内部データなので、`DUAL_VLIMIT` で det の -limit と独立に
増やせる（既定 = -limit）。`DUAL_REMCAP` は remainder パス補充の上限（既定 4·|U|+64）。

s5378_l30（全4551故障、baseline: 57s / incomplete 2,980）の費用対効果曲線:

| 設定 | incomplete | 時間 | 備考 |
|---|---:|---:|---|
| DUAL のみ（V≈14本/故障） | 2,834 | 100s | +146 救済 |
| + drain VLIMIT=300 REMCAP=5000 | **1,713** | ~20分 | +1,267 救済・remainder 449k 本 |
| + drain VLIMIT=5000 REMCAP=20000 | （先頭1653行で 17.5%） | 1500s で 16% しか進まず | 限界収穫逓減 |

**V 予算を増やすほど救済は増えるが、費用は急勾配**（¬D_f の被覆も指数の故障が残るため）。
incomplete ゼロには到達しない（原理的限界は `verification/split/SUMMARY.md` を参照）。

## 結論 / 導入判断

- **本番デフォルトには入れない**（env オプトインのまま）。爆発対策の主役は引き続き MAXDC_CORE。
- **使う場面**:
  - limit 付き実行で complete を増やしたい／打ち切り故障に証明付き上下界が欲しいとき（`DUAL=1`）。
  - MAXDC を使わない構成での爆発回避（baseline 比で速く全 complete）。
- **課題（次の改善候補）**: V 側 1:1 交互のコスト（V にも XID 相当の安価な拡大を入れる、
  閉包判定の間引き）、remainder の ISOP 化（disjoint パスよりコンパクトな被覆）、
  `DUAL_START` の limit 連動の適応化。

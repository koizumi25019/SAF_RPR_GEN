#pragma once
//-------------------------------------------------------------------------------------------------------------
//	experiment.h : 研究用フック（環境変数で制御、デフォルト無効）
//	  案1 MAXDC : 非検出オラクルCNFで各キューブを素項へ拡大（+計測）
//	  案2 MAXHAM: 最大ハミング距離制約による解の多様化
//	  案3 DUAL  : 非検出空間 ¬D_f の並行列挙による早期完了（双対列挙）
//	いずれも未設定なら本番動作を一切変えない。実験結果は verification/SUMMARY.md。
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <cudd.h>
#include "ccadical.h"
#include "./target_fault.h"
#include "./cube_set.h"

/* 案1: env MAXDC が設定されていれば非検出オラクルを構築して返す（無効時 NULL）。
   WriteTPGModel 直後（TFOフラグ/varsfc が当該故障用に設定済み）に呼ぶこと。 */
CCaDiCaL* EXP_MaybeBuildOracle(TARGET* target);

/* 案1: キューブを素項へ拡大（in place）。oracle が NULL なら何もしない。 */
void EXP_Expand(CCaDiCaL* oracle, char* cube);

/* 案1: 故障完了時の集計とオラクル解放。*oracle が NULL なら何もしない。 */
void EXP_OracleDone(CCaDiCaL** oracle, bool limit_hit);

/* 案2: 多様性付き solve。env MAXHAM 未設定または prev==NULL なら素の solve。 */
int EXP_Solve(CCaDiCaL* solver, const char* prev_cube);

/* 案2: 故障ごとの aux 変数採番器を初期化（CNF構築後に呼ぶ） */
void EXP_ResetPerFault(void);

/* 多様化(env DIVPO): 直前の solve で検出先に指定した PO の net id（なければ -1）。
   InlineXID の正当化先を SAT の検出先と揃えるために使う。 */
int EXP_PreferredPONet(void);

/* 案3: env DUAL=1 で双対列挙を初期化（無効時は何もしない）。
   WriteTPGModel 直後（TFOフラグ/varsfc が当該故障用に設定済み）に呼ぶこと。
   ソルバ構築と U の BDD 化は V 側の初回起動（DUAL_START 本目）まで遅延する。 */
void EXP_DualInit(DdManager* gbm, TARGET* target);

/* 案3: 検出キューブ latest を U へ追加し、非検出側を1本進める。
   U∪V の閉包 or ¬D_f 列挙完了（残り D_f\U を cubes へ補充）で完了なら true。
   このとき cubes の和集合は D_f に一致する（complete=1 で従来どおり報告可）。 */
bool EXP_DualStep(CubeSet* cubes, const char* latest);

/* 案3: det 側が limit で打ち切られた後、V 側だけを DUAL_VLIMIT まで回して
   完了を狙う（ドレイン）。完了なら true（cubes の和集合 = D_f）。 */
bool EXP_DualDrain(CubeSet* cubes);

/* 案3: 故障完了時の集計と資源解放（無効時は何もしない）。 */
void EXP_DualDone(FNODE* f, bool limit_hit);

/* 案4: env SPLIT=1 で、limit 到達した故障を Shannon 分割で完全列挙まで持っていく。
   成功したら true（cubes の和集合が D_f に一致し complete=1 で報告できる）。
   env 未設定・SPLIT_MAXNODES 超過時は false（従来どおり capped 扱い）。 */
bool EXP_SplitFinish(DdManager* gbm, CCaDiCaL* det, CCaDiCaL* u_oracle,
                     CubeSet* cubes, TARGET* target);

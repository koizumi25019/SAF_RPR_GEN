#pragma once
//-------------------------------------------------------------------------------------------------------------
//	experiment.h : 研究用フック（環境変数で制御、デフォルト無効）
//	  案1 MAXDC : 非検出オラクルCNFで各キューブを素項へ拡大（+計測）
//	  案2 MAXHAM: 最大ハミング距離制約による解の多様化
//	いずれも未設定なら本番動作を一切変えない。実験結果は verification/SUMMARY.md。
//-------------------------------------------------------------------------------------------------------------
#include "ccadical.h"
#include "./target_fault.h"

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

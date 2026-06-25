#pragma once
//-------------------------------------------------------------------------------------------------------------
//	faulty_miter : 限定化しない「全回路ミター」モデル（実験 env FULL_MITER）
//
//	正常回路(varsgc)と同規模の故障回路(faulty)を全ネットぶん固定で作る。各ネットに
//	故障注入スイッチ（mux）を恒久的に持たせ、故障は assumption だけで選択する：
//	    faulty[i] = mt_act[i] ? mt_sval[i] : driver[i]
//	    driver[i] = 入力(IN/DFF)なら varsgc[i]、ゲートなら gate(faulty[inputs])
//	検出条件：観測点(n_out==0)のうち少なくとも1つで varsgc ≠ faulty（恒久節）。
//
//	故障 k の sa-v をテスト：assume(mt_act[k]=1, mt_sval[k]=v, 他は mt_act[j]=0)。
//	CNF は一度だけ構築するので、SAT ソルバの学習節を全故障で再利用できる。
//-------------------------------------------------------------------------------------------------------------
#include "ccadical.h"

extern int* mt_act;    /**< per-net 活性化変数（assume 1 でそのネットに故障注入） */
extern int* mt_sval;   /**< per-net 縮退値変数（assume 0/1 = sa0/sa1） */

/** 全ミター変数（faulty値・act・sval・drv）に SAT 変数番号を割り当てる（正常回路の後） */
void AllocMiterVars(void);

/** 全回路ミター（故障回路＋注入スイッチ＋検出条件）の節を一度だけソルバに投入する */
void BuildFaultyMiter(CCaDiCaL* solver);

/** ミター変数バッファを解放する */
void FreeMiterVars(void);

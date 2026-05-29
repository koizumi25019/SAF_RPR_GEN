#pragma once
//-------------------------------------------------------------------------------------------------------------
//	act_clause : インクリメンタルSAT用の活性化変数（activation literal）ガード
//	  g_act_lit != 0 の間、cadd(...,0) で閉じる各節に ¬g_act_lit を付け、(¬g_act_lit ∨ 元の節) にする。
//	  各 solve の前に ccadical_assume(solver, g_act_lit) で有効化し、
//	  その故障を終えたら単位節 (¬g_act_lit) を恒久追加して無効化（retire）する。
//	  g_act_lit == 0 のときは素の節（恒久節）として追加される。
//-------------------------------------------------------------------------------------------------------------
#include "ccadical.h"

extern int g_act_lit;   /**< 現在処理中の故障の活性化変数（0 = 無し＝恒久節） */

/** 活性化変数でガードしながら 1 リテラルを節に追加する（lit==0 で節を閉じる） */
static inline void cadd(CCaDiCaL* solver, int lit)
{
	if (lit == 0 && g_act_lit != 0) ccadical_add(solver, -g_act_lit);
	ccadical_add(solver, lit);
}

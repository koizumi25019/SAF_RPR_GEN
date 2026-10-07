#pragma once
//-------------------------------------------------------------------------------------------------------------
//	cnf_dump.h : 検出CNFを DIMACS に書き出す tee（env DUMP_CNF で使用、既定無効）。
//	  WriteTPGModel が solver へ投入する節を、そのまま DIMACS にも複製する。
//	  目的: 厳密モデルカウンタ(d4/sharpSAT-TD/Ganak)で Vi=#SAT を直接数え、
//	  キューブ列挙と比較するため。範囲限定時は自由な内部変数が残るため、全PIへの射影カウントを使う。
//	  全PIへの射影指定 c p show を出す。範囲限定時には c ind を出さない。
//-------------------------------------------------------------------------------------------------------------
#include "ccadical.h"

/* Optional dependency observer, independent of DIMACS dumping. Single-model API. */
void cnf_set_literal_observer(void (*observer)(int));

void cnf_tee_begin(const char* path);   /* この呼出し以降の節を path に記録開始 */
void cnf_tee_lit(int lit);              /* リテラル（0で節終端）を tee に流す */
void cnf_tee_end(int nvars);            /* ヘッダ(p cnf)＋射影行を付けて確定・close */
/* Scoped formulas must use c p show without claiming PI independent support. */
void cnf_tee_end_projected(int nvars);
int  cnf_tee_active(void);

/* 節構築箇所はこのマクロ経由で solver に投入する。tee が有効なときだけ DIMACS にも複製。
   _ll で1回だけ評価するので ++cnf.total.vars のような引数でも二重評価しない。 */
#define CNF_ADD(s, l) do { int _ll = (l); ccadical_add((s), _ll); cnf_tee_lit(_ll); } while (0)

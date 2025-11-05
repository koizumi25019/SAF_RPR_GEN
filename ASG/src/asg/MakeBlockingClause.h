#pragma once
// MakeBlockingClause.h
#ifndef MAKE_BLOCKING_CLAUSE_H
#define MAKE_BLOCKING_CLAUSE_H
//#define XID_TP_DIR "./tools/xid/"

#include <stdio.h>
#include <stdbool.h>
#include "./target.h"

// pattern_file: xid_tp.txt の FILE*  
// fout_pbo: 追記先の PBO ファイル  
// n_vars: 外部入力＋擬似外部入力の総数  
bool make_blocking_clause(TARGET* target, FILE* cube_file);

#endif // MAKE_BLOCKING_CLAUSE_H

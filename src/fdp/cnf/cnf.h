#pragma once

#include <stdio.h>

#include "../../netlist/netlist.h"
#include "../read.h"
#include "../create_TPG_model.h"

//-------------------------------------------------------------------------------------------------------------
//	構造体
//-------------------------------------------------------------------------------------------------------------
/** 問題サイズ */
typedef struct CNFproblemSize
{
	int					  vars;			      /**< 変数数 */
	int					  clauses;				  /**< 節数 */
}
CNFSIZE;

/** cnf */
typedef struct CNFproblem
{
	CNFSIZE				  constant;			  /**< 固定サイズ */
	CNFSIZE				  total;			  /**< 合計サイズ */
}
CNF;

//-------------------------------------------------------------------------------------------------------------
//	グローバル変数
//-------------------------------------------------------------------------------------------------------------
CNF							cnf;				/**< cnf */
unsigned int				fc_po;				/* 出力の故障回路変数 */
int							constraint;			/* 制約 */
int							CONS_SIZE;

//-------------------------------------------------------------------------------------------------------------
//	プロトタイプ宣言
//-------------------------------------------------------------------------------------------------------------
/** 正常回路の制約を作成する */
bool CreateConsGC(
	void
);

/** 正常回路用の変数を割り当てる */
void AssigneVarsGC(
	void
);

/** 正常回路の制約を作成する -AND */
void CreateConsGC_AND(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -NAND */
void CreateConsGC_NAND(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -OR */
void CreateConsGC_OR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -NOR */
void CreateConsGC_NOR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -BUF */
void CreateConsGC_BUF(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -INV */
void CreateConsGC_INV(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -XOR */
void CreateConsGC_XOR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 正常回路の制約を作成する -XNOR */
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する */
bool CreateConsFC(
	CCaDiCaL* solver,       // ★追加
	TARGET* target			  /**< 対象故障 */
);

/** 伝搬先(TFO)を探索する */
void SearchTFO(
	FNODE* target			  /**< 対象故障 */
);

/** 故障伝搬 (Dチェーン) 制約を作成する */
void CreateConsProp(
	CCaDiCaL* solver,
	FNODE* target
);

/** 故障回路の制約を作成する -AND */
void CreateConsFC_AND(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -NAND */
void CreateConsFC_NAND(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -OR */
void CreateConsFC_OR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -NOR */
void CreateConsFC_NOR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -BUF */
void CreateConsFC_BUF(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -INV */
void CreateConsFC_INV(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -XOR */
void CreateConsFC_XOR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< ネットリストへのポインタ */
);

/** 故障回路の制約を作成する -XNOR */
void CreateConsFC_XNOR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< ネットリストへのポインタ */
);

/** 検出回路の制約を作成する */
void CreateConsDC(
	CCaDiCaL* solver,       // ★追加
	FNODE* target		  /**< 対象故障 */
);

/** 伝搬先外部出力を接続する制約を作成する */
void CreateConsDC_XOR(
	CCaDiCaL* solver     // ★追加
);

/** XOR出力を接続する制約を作成する */
void CreateConsDC_OR(
	CCaDiCaL* solver     // ★追加
);

/** 故障励起の制約を作成する */
void CreateConsDC_FE(
	CCaDiCaL* solver,       // ★追加
	FNODE* fnodeptr			  /**< 故障ノードへのポインタ */
);

#pragma once
//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include "../netlist/netlist.h"


//-------------------------------------------------------------------------------------------------------------
//	定義
//-------------------------------------------------------------------------------------------------------------
#define	RESET			  0					  /**< フラグ種別 = リセット */
#define UNASSIGN          0					  /**< フラグ種別 = 未割当  */

#define INIT_OKAY		  true		          /**< 戻り値 = true */
#define INIT_ERROR		  false		          /**< 戻り値 = false */

/** フラグをリセットする */
#define RESET_FLAG		do																				      \
{																											  \
	for(int i= 0; i < n_net; i++)																			  \
	{																									  	  \
		nl[i].flag = RESET;																					  \
	}																										  \
}																											  \
while (false);

/** 故障回路用変数をリセットする */
#define RESET_VARSFC	do																					  \
{																											  \
	for(int i = 0;i < n_net; i++)																			  \
	{																										  \
		nl[i].varsfc = nl[i].varsgc;																		  \
	}																										  \
}																											  \
while(false);

/** 問題サイズをリセットする */
#define RESET_CNF		do																				      \
{																											  \
	cnf.total.vars	= cnf.constant.vars;																	  \
	cnf.total.clauses	= cnf.constant.clauses;																	  \
}																											  \
while (false);

//-------------------------------------------------------------------------------------------------------------
//	プロトタイプ宣言
//-------------------------------------------------------------------------------------------------------------
/** グローバル変数を初期化する */
bool InitGlobalVars(
	void
);

/** ネットリストを初期化する */
void InitGlobalVarsNLIST(
	void
);

/** スタックを初期化する */
void InitGlobalVarsSTACK(
	void
);

/** CNFを初期化する */
void InitGlobalVarsCNF(
	void
);

/** 読み込みデータを初期化する */
void InitGlobalVarsREADDATA(
	void
);

/** キューを初期化する */
void InitGlobalVarsQUE(
	void
);

//CLASP変数の初期化
void InitGlobalVarsCLASP(
	void
);










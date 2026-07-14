//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include "./create_TPG_model.h"
#include "./target_fault.h"
#include "./cnf/cnf.h"
#include "ccadical.h"

void LoadModelToSolver(CCaDiCaL *solver, TARGET* target) {
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN || nl[i].type == DFF) continue;
        if (nl[i].consgc == NULL) continue;
        for (int j = 0; j < nl[i].consgc_len; j++) {
            ccadical_add(solver, nl[i].consgc[j]);
        }
    }
}

//*************************************************************************************************************
//	@name		WriteTPGModel
//	@function	テストパタン生成モデルを構築する
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool WriteTPGModel(
	CCaDiCaL *solver,
	TARGET* target
)
{
	/** TPGモデルを作成する */
	if (CreateTPGmodel(solver, target) != true) return false;

    // 作成された文字列データをソルバに直接投入
    LoadModelToSolver(solver, target);

	return true;
}

//*************************************************************************************************************
//	@name		F@CreateTPGmodel
//	@function	F	テストパタン生成モデルを作成する
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool CreateTPGmodel(
CCaDiCaL* solver,
	TARGET* target
)
{
	/** 故障回路の制約を作成する */
	if (CreateConsFC(solver, target) != true) return false;

	return true;
}
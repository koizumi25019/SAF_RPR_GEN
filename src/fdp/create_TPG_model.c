//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include "./create_TPG_model.h"
#include "./cnf/cnf.h"
#include "ccadical.h"
#include "normal_scope.h"

static void LoadNormalDefinition(CCaDiCaL *solver, int scoped) {
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN || nl[i].type == DFF) continue;
        if (nl[i].consgc == NULL) continue;
        if (scoped && !NormalScopeRequiredNet(i)) continue;
        for (int j = 0; j < nl[i].consgc_len; j++) {
            ccadical_add(solver, nl[i].consgc[j]);
        }
    }
}

/* Preserve the public full-definition loader for any future oracle callers. */
void LoadModelToSolver(CCaDiCaL *solver) {
    LoadNormalDefinition(solver, 0);
}

//*************************************************************************************************************
//	@name		WriteTPGModel
//	@function	テストパタン生成モデルを構築する
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool WriteTPGModel(
	CCaDiCaL *solver,
	FNODE* target
)
{
	/** TPGモデルを作成する */
	if (CreateTPGmodel(solver, target) != true) return false;

    // 作成された文字列データをソルバに直接投入
    NormalScopeBuild();
    if (NormalScopeEnabled()) LoadNormalDefinition(solver, 1);
    else LoadModelToSolver(solver);

	return true;
}

//*************************************************************************************************************
//	@name		F@CreateTPGmodel
//	@function	F	テストパタン生成モデルを作成する
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool CreateTPGmodel(
CCaDiCaL* solver,
	FNODE* target
)
{
	/** 故障回路の制約を作成する */
	if (CreateConsFC(solver, target) != true) return false;

	return true;
}

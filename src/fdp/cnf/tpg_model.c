//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include "fdp/cnf/tpg_model.h"
#include "fdp/cnf/cnf.h"
#include "ccadical.h"
#include "fdp/cnf/normal_scope.h"

/* 正常ゲートの定義（外部入力は consgc を持たない）のうち、対象出力TFI内のものを投入する。 */
static void LoadNormalDefinition(CCaDiCaL *solver) {
    for (int i = 0; i < n_net; i++) {
        if (nl[i].consgc == NULL) continue;
        if (!NormalScopeRequiredNet(i)) continue;
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
	FNODE* target
)
{
	/** 故障回路・Dチェーン・検出回路・必須割当ての制約を作成する（正常CNFの範囲もここで決まる） */
	if (CreateConsFC(solver, target) != true) return false;

    // 対象出力TFI内の正常ゲート定義をソルバへ直接投入
    LoadNormalDefinition(solver);

	return true;
}

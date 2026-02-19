//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./createSGmodel.h"
#include "./target.h"
#include "./opb/opb.h"
#include "ccadical.h"

void AddClauseString(CCaDiCaL *solver, char* clause_str) {
    if (!clause_str) return;
    
    // 文字列をコピーしないとstrtokが元のデータを壊す可能性があるため、
    // 必要ならstrdupして作業用バッファを作るのが安全ですが、
    // ここでは使い捨ての文字列と仮定して直接扱います。
    // もし consgc を後でまた使うなら、コピーしてください。
    char* work_str = strdup(clause_str);
    if (!work_str) return;

    char *token = strtok(work_str, " \t\n");
    while (token != NULL) {
        int lit = atoi(token);
        ccadical_add(solver, lit);
        token = strtok(NULL, " \t\n");
    }
    free(work_str);
}

void LoadModelToSolver(CCaDiCaL *solver, TARGET* target) {
    
    // 1. 正常回路の制約
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type != IN && nl[i].type != DFF) {
            // nl[i].consgc には "1 -2 3 0\n" のような文字列が入っているはず
            if (nl[i].consgc != NULL) {
                AddClauseString(solver, nl[i].consgc);
            }
        }
    }

    // 2. 故障回路の制約
    for (int i = 0; i < target->num; i++) {
        for (int j = 0; j < n_net; j++) {
            if (nl[j].consfc != NULL && nl[j].consfc[i] != NULL) {
                AddClauseString(solver, nl[j].consfc[i]);
            }
        }
    }
}

//*************************************************************************************************************
//	@name		�F�@WriteTPGModel
//	@function	�F	Write the test pattern generation model
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool WriteTPGModel(
	CCaDiCaL *solver,
	TARGET* target
)
{

	/** create the tpg model */
	if (CreateTPGmodel(target) != TPG_MODEL_OKAY) return W_TPG_MODEL_ERROR;

    // 作成された文字列データをソルバに直接投入
    LoadModelToSolver(solver, target);

	return W_TPG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@CreateTPGmodel
//	@function	�F	create the test pattern generation model
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool CreateTPGmodel(
	TARGET* target			  /**< target fault */
)
{
	/** create the constraint for faulty-circuit */
	if (CreateConsFC(target) != TPG_MODEL_OKAY) return TPG_MODEL_OKAY;

	return TPG_MODEL_OKAY;
}










//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./createSGmodel.h"
#include "./target_fault.h"
#include "./cnf/cnf.h"
#include "ccadical.h"

void AddClauseString(CCaDiCaL *solver, char* clause_str) {
    if (!clause_str) return;
    
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
    
    //正常回路の制約
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type != IN && nl[i].type != DFF) {
            // nl[i].consgc には "1 -2 3 0\n" のような文字列が入っているはず
            if (nl[i].consgc != NULL) {
                AddClauseString(solver, nl[i].consgc);
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
	if (CreateTPGmodel(solver, target) != true) return false;

    // 作成された文字列データをソルバに直接投入
    LoadModelToSolver(solver, target);

	return true;
}

//*************************************************************************************************************
//	@name		F@CreateTPGmodel
//	@function	F	create the test pattern generation model
//	@return		F	(bool) okay, error
//*************************************************************************************************************
bool CreateTPGmodel(
CCaDiCaL* solver,
	TARGET* target			  /**< target fault */
)
{
	/** create the constraint for faulty-circuit */
	if (CreateConsFC(solver, target) != true) return false;

	return true;
}
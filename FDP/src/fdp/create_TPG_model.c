//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./create_TPG_model.h"
#include "./target_fault.h"
#include "./cnf/cnf.h"
#include "ccadical.h"

void LoadModelToSolver(CCaDiCaL *solver, TARGET* target) {
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type != IN && nl[i].type != DFF) {
            if (nl[i].consgc != NULL) {
                for (int j = 0; j < nl[i].consgc_len; j++) {
                    ccadical_add(solver, nl[i].consgc[j]);
                }
            }
        }
    }
}

//*************************************************************************************************************
//	@name		WriteTPGModel
//	@function	Write the test pattern generation model
//	@return		(bool) okay, error
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
	TARGET* target
)
{
	/** create the constraint for faulty-circuit */
	if (CreateConsFC(solver, target) != true) return false;

	return true;
}
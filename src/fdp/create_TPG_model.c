//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./create_TPG_model.h"
#include "./target_fault.h"
#include "./cnf/cnf.h"
#include "ccadical.h"
#include "./cnf_dump.h"
#include "./power_constraint.h"
#include "./normal_scope.h"

static void LoadNormalDefinition(CCaDiCaL *solver, int scoped) {
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN || nl[i].type == DFF) continue;
        if (nl[i].consgc == NULL) continue;
        if (scoped && !NormalScopeRequiredNet(i)) continue;
        for (int j = 0; j < nl[i].consgc_len; j++) {
            CNF_ADD(solver, nl[i].consgc[j]);
        }
    }
}

/* Non-detection oracles use this public API and keep complete definitions. */
void LoadModelToSolver(CCaDiCaL *solver, TARGET* target) {
    LoadNormalDefinition(solver, 0);
    PowerLoadDefinition(solver);
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
	NormalScopeBegin();
	/** create the tpg model */
	if (CreateTPGmodel(solver, target) != true) return false;

    /* Include every power-definition dependency BEFORE closing the scope. */
    if (NormalScopeEnabled()) {
        PowerLoadDefinition(solver);
        NormalScopeEnd();
    }

    // 作成された文字列データをソルバに直接投入
    if (NormalScopeEnabled()) LoadNormalDefinition(solver, 1);
    else LoadModelToSolver(solver, target);
    int power = PowerLiteral();
    if (power) { CNF_ADD(solver, power); CNF_ADD(solver, 0); }

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

#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include "ccadical.h"
#include "./target_fault.h"

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** load the (global) good-circuit clauses into a solver */
void LoadModelToSolver(CCaDiCaL *solver, TARGET* target);

//** write the tset pattern generaiton model */
bool WriteTPGModel(
	CCaDiCaL *solver,
	TARGET* target			  /**< target fault */
);

/** create the tset pattern generaiton model */
bool CreateTPGmodel(
	CCaDiCaL* solver,
	TARGET* target			  /**< target fault */
);









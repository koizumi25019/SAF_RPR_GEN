#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include "ccadical.h"
#include "./target.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define W_TPG_MODEL_OKAY         true
#define W_TPG_MODEL_ERROR        false

#define TPG_MODEL_OKAY        true
#define TPG_MODEL_ERROR       false

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
//** write the tset pattern generaiton model */
bool WriteTPGModel(
	CCaDiCaL *solver,
	TARGET* target			  /**< target fault */
);

/** create the tset pattern generaiton model */
bool CreateTPGmodel(
	TARGET* target			  /**< target fault */
);









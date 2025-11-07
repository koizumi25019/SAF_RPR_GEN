#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>

#include "./target.h"
#include "../standard.h"


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
	TARGET* target			  /**< target fault */
);

/** create the tset pattern generaiton model */
bool CreateTPGmodel(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
);









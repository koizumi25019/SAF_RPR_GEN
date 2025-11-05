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
#define SG_MODEL_OKAY         true
#define SG_MODEL_ERROR        false

#define TPG_MODEL_OKAY        true
#define TPG_MODEL_ERROR       false

#define LFSR_MODEL_OKAY        true
#define LFSR_MODEL_ERROR       false

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** create the seed generation model */
bool CreateSGmodel(
	TARGET* target			  /**< target fault */
);

/** create the tset pattern generaiton model */
bool CreateTPGmodel(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
);









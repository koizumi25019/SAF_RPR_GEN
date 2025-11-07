#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "../standard.h"
#include "./read.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define TARGET_OKAY	     true				  /**< return code = okay */
#define TARGET_ERROR	 false				  /**< return code = error */
#define F1				16
#define F2				256
#define F3				4096
#define F4				65536
#define F5				1048576
#define F6				16777216
#define F7				268435456
#define F8				4294967296

//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** target fault structure */
typedef struct TargetFault
{
	int					  num;			      /**< number of target faults */
	struct FaultNode** list;				  /**< list */
}
TARGET;

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** set the target-fault list */
bool SetTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target,			  /**< target-fault list */
	int loop				  /**< number of loop */
);

/* set the direct target */
bool DirectInputTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
);
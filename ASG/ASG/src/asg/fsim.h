/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/fsim.h																	     */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <assert.h>

#include "./read.h"
#include "./target.h"
#include "../lib/lib.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	FSIM_OKAY		  true
#define	FSIM_ERROR		  false
#define FSIM_DIR          "./tools/fsim"
#define FSIM_DET_FILE     "./tools/fsim/det.txt"

/** start the fault simulation */
#define _CALL_FAULT_SIMULATION_SAF_(net, pin)	do															  \
{																											  \
	if(_chdir(FSIM_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',FSIM_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	char* cmd = (char*)NULL;																				  \
	cmd       = (char*)allocMemory(300, sizeof(char));														  \
	sprintf_s(cmd, 300, "XID2 -c ../../%s -tx test.txt -pin ../../%s -fm SAF -fsim YES -det ./det.txt"		  \
                                                                                                 , net, pin); \
	system(cmd);																							  \
	free(cmd);																								  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
																	  \
}																											  \
while(false);


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** fault simulation */
bool FSIM(
	TARGET* target		  /**< target fault */
);

/** drop the detected fault */
bool DropDeteFault(
	TARGET * target		  /**< target fault */
);

/** check for target fault */
bool CheckTarget(
	TARGET* target		  /**< target fault */
);










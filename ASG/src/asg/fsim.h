#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>

#include "./read.h"
#include "./target.h"
#include "../lib/lib.h"

//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	FSIM_OKAY		  true
#define	FSIM_ERROR		  false
#define FSIM_DIR          "./tools/fsim"
#define BDD_DIR          "./tools/bdd"
#define FSIM_DET_FILE     "./tools/fsim/det.txt"

/** start the fault simulation */
#define CALL_XID_SAF(net, pin)	do															                  \
{																											  \
	if(_chdir(FSIM_DIR) != 0)																				  \
	{																										  \
		printf("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		printf("directory %c%s%c does not exist. \n",'"',FSIM_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	char* cmd = (char*)NULL;																				  \
	cmd       = (char*)allocMemory(300, sizeof(char));														  \
	sprintf_s(cmd, 300, "XID2 -c ../../%s -flist xid_fault.txt -tx test.txt -pin ../../%s -fm SAF -xid YES -m2008 YES -otx xid_tp.txt " \
                                                                                                 , net, pin); \
	system(cmd);																							  \
	free(cmd);																								  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		printf("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		printf("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
																	                                          \
}																											  \
while(false);

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------

/** drop the detected fault */
bool DropDeteFault(
	TARGET * target		  /**< target fault */
);










/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	urashima																				 */
/*	file		:	./src/asg/opb/scip/scip.h																 */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../../standard.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define N_SOL             2
#define SOL_SEED          0
#define SOL_TP            1

#define SCIP_DIR          "./tools/scip"
#define SCIP_PRO_FILE     "./tools/scip/problem.txt"
#define SCIP_LOG_FILE     "./tools/scip/log.sol"
#define SCIP_OKAY		  true
#define SCIP_ERROR		  false
#define SCIP_STATUS_ERROR -1
#define SCIP_OBJVAL_ERROR -1
#define	SCIP_STATUS_UNKNOWN			1
#define SCIP_STATUS_USERINTERRUPT	2
#define SCIP_STATUS_NODELIMIT		3
#define SCIP_STATUS_TIMELIMIT		4
#define SCIP_STATUS_MEMLIMIT		5
#define SCIP_STATUS_GAPLIMIT		6
#define SCIP_STATUS_OPTIMAL			7	
#define SCIP_STATUS_INFEASIBLE		8
#define SCIP_STATUS_UNBOUNDED		9
#define SCIP_STATUS_TOTALNODELIMIT	10	
#define SCIP_STATUS_STALLNODELIMIT	11
#define SCIP_STATUS_SOLLIMIT		12
#define SCIP_STATUS_BESTSOLLIMIT	13
#define SCIP_STATUS_RESTARTLIMIT	14
#define SCIP_STATUS_INFORUNBD		15

/** call scip (limit times =60[s]) */
#define	_CALL_SCIP_LIMIT_TIME_60_	do																  		  \
{																											  \
	PrintMessage("	solving ...\n");																		  \
	if(_chdir(SCIP_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',SCIP_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	system("scip -f problem.opb -s ./setting/parameters_limittime60.set -l log.sol -q");					  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
}																											  \
while (false);

/** call scip (limit solutions =1) */
#define	_CALL_SCIP_LIMIT_SOL_1_	   	do																	  	  \
{																											  \
	if(_chdir(SCIP_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',SCIP_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	assert(remove("./log.sol") == 0);																		  \
	PrintMessage("	resolving ...\n");																		  \
	system("scip -f problem.opb -s ./setting/parameters_solutions1.set -l log.sol -q");						  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
}																											  \
while (false);


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** problem size */
typedef struct SCIP_LOG_
{
	int					  status;			  /**< status */
	int					  objval;			  /**< objective value */
	char** sol;				  /**< solution */
}
SCIPLOG;


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
SCIPLOG					   scip;			   /**< scip log */


//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** scip */
bool SCIP(
	void
);

/** get the status */
void SCIPgetStatus(
	FILE* fileptr			  /**< pointer to file */
);

/** get the objective value */
void SCIPgetObjVal(
	FILE* fileptr			  /**< pointer to file */
);

/** get the solution */
void SCIPgetSol(
	FILE* fileptr			  /**< pointer to file */
);










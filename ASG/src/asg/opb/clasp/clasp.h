/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/opb/clasp/clasp.h																 */
/*	deginer		:	T.Sone																			  		 */
/*	date		:	2023.06.21																  				 */
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

#define CLASP_DIR          "./tools/clasp"
#define CLASP_PBO_FILE     "./tools/clasp/pbo.txt"
#define CLASP_LOG_FILE     "./tools/clasp/clasp_result.txt"
#define CLASP_OKAY		  true
#define CLASP_ERROR		  false
#define CLASP_STATUS_ERROR -1
#define CLASP_OBJVAL_ERROR -1
#define CLASP_UNSAT -2
#define	CLASP_STATUS_UNKNOWN		-8
#define CLASP_STATUS_USERINTERRUPT	2
#define CLASP_STATUS_NODELIMIT		3
#define CLASP_STATUS_TIMELIMIT		4
#define CLASP_STATUS_MEMLIMIT		5
#define CLASP_STATUS_GAPLIMIT		6
#define CLASP_STATUS_OPTIMAL		7	
#define CLASP_STATUS_INFEASIBLE		8
#define CLASP_STATUS_UNBOUNDED		9
#define CLASP_STATUS_TOTALNODELIMIT	10	
#define CLASP_STATUS_STALLNODELIMIT	11
#define CLASP_STATUS_SOLLIMIT		12
#define CLASP_STATUS_BESTSOLLIMIT	13
#define CLASP_STATUS_RESTARTLIMIT	14
#define CLASP_STATUS_INFORUNBD		15

//#define SCIP_LOG

/** call clasp (limit times =10[s]) */
#define	_CALL_CLASP_LIMIT_TIME_10_ do																  		  \
{																											  \
	if(_chdir(CLASP_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',CLASP_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	system("clasp.exe pbo.txt --mode=1 >clasp_result.txt --parallel-mode 24");											  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
}																											  \
while (false);																								  \

/** call clasp (limit times =60[s]) */
#define	_CALL_CLASP_LIMIT_TIME_60_	do																  		  \
{																											  \
	PrintMessage("	solving ...\n");																		  \
	if(_chdir(CLASP_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',CLASP_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	system("clasp.exe pbo.txt --time-limit 60 >clasp_result.txt");											  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
}																											  \
while (false);

/** call clasp (limit times =120[s]) */
#define	_CALL_CLASP_LIMIT_TIME_120_	do																  		  \
{																											  \
	PrintCompMessage("	resolving ...\n");														  \
	if(_chdir(CLASP_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',CLASP_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	system("clasp.exe pbo.txt --time-limit 120 >clasp_result.txt");											  \
	if(_chdir("../../") != 0)																			      \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',"../../",'"');							      \
		exit(EXIT_FAILURE);																				      \
	}																										  \
}																											  \
while (false);

/** call clasp (solution =1) */
#define	_CALL_CLASP_SOL_1_	do																  				  \
{																											  \
	PrintCompMessage("	resolving solution 1...\n");														  \
	if(_chdir(CLASP_DIR) != 0)																				  \
	{																										  \
		PrintMessage("\n	SYSTEM ERROR: changing directory is failed. ");									  \
		PrintMessage("directory %c%s%c does not exist. \n",'"',CLASP_DIR,'"');								  \
		exit(EXIT_FAILURE);																				      \
	}																										  \
	system("clasp.exe pbo.txt --models=1 >clasp_result.txt");					  \
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
typedef struct CLASP_LOG_
{
	int					  status;			  /**< status */
	int					  objval;			  /**< objective value */
	char** sol;				  /**< solution */
}
CLASPLOG;


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
CLASPLOG					   clasp;			   /**< clasp log */



//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** clasp */
bool CLASP(
	void
);
/** get the objective value */
void CLASPgetObjVal(
	FILE* fileptr			  /**< pointer to file */
);

/** get the solution */
void CLASPgetSol(
	FILE* fileptr			  /**< pointer to file */
);
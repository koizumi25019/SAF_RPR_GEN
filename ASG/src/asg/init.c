/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/init/init.c																     */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.10.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "./init.h"
#include "./read.h"
#include "./opb/scip/scip.h"
#include "./opb/clasp/clasp.h"
#include "../standard.h"
#include "../lib/lib.h"
#include "../asg/opb/opb.h"
#include "../netlist/netlist.h"
#include "../debug/debug.h"
#include "../asg/asg.h"


//*************************************************************************************************************
//	@name		：　InitGlobalVars
//	@function	：	initialize the global variable
//	@return		：	(bool) okay, error 
//*************************************************************************************************************
bool InitGlobalVars(
	void
)
{
	InitGlobalVarsNLIST();

	InitGlobalVarsSTACK();

	InitGlobalVarsQUE();

	InitGlobalVarsOPB();

	InitGlobalVarsREADDATA();

	InitGlobalVarsCLASP();



#ifdef __DEBUG_INIT__
	_CALL_DEBUG_INIT_
#endif


		PrintMessage("	Initialize the global variables completed ... \n");

	return INIT_OKAY;
}

//*************************************************************************************************************
//	@name		：　InitGlobalVarsNLIST
//	@function	：	initialize the netlist
//	@return		：	(void) 
//*************************************************************************************************************
void InitGlobalVarsNLIST(
	void
)
{
	for (int i = 0; i < n_net; i++)
	{
		nl[i].varsgc = UNASSIGN;
		nl[i].varsfc = UNASSIGN;
		nl[i].flag = RESET;
		nl[i].consgc = (char*)NULL;
		nl[i].consfc = (char**)NULL;
	}

	char* buffer = (char*)NULL;
	char* token = (char*)NULL;
	char* context = (char*)NULL;
	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));
	strcpy_s(buffer,MAXSIZE_BUFFER, opt.file.input.net);
	token = strtok_s(buffer, "/", &context);
	token = strtok_s(NULL, "/", &context);
	token = strtok_s(NULL, "/", &context);
	token = strtok_s(NULL, ".", &context);
	net_name = _strdup(token);

	free(buffer);
	return;
}

//*************************************************************************************************************
//	@name		：　InitGlobalVarsSTACK
//	@function	：	initialize the stack
//	@return		：	(void) 
//*************************************************************************************************************
void InitGlobalVarsSTACK(
	void
)
{
	stack.ptr = 0;
	stack.maxnum = n_net * MAXSIZE_STACK;
	stack.stk = (NLIST**)allocMemory(stack.maxnum, sizeof(NLIST*));

	return;
}

//*************************************************************************************************************
//	@name		：　InitGlobalVarsQUE
//	@function	：	initialize the queue
//	@return		：	(void) 
//*************************************************************************************************************
void InitGlobalVarsQUE(
	void
)
{
	que.maxnum = n_net * MAXSIZE_QUEUE;
	que.front = 0;
	que.num = 0;
	que.rear = 0;
	que.que = (NLIST**)allocMemory(que.maxnum, sizeof(NLIST*));

	return;
}

//*************************************************************************************************************
//	@name		：　InitGlobalVarsOPB
//	@function	：	initialize the opb
//	@return		：	(void) 
//*************************************************************************************************************
void InitGlobalVarsOPB(
	void
)
{
	opb.constant.cons = 0;
	opb.constant.pros = 0;
	opb.constant.spros = 0;
	opb.constant.vars = 0;
	opb.total.cons = 0;
	opb.total.pros = 0;
	opb.total.spros = 0;
	opb.total.vars = 0;

	return;
}

//*************************************************************************************************************
//	@name		：　InitGlobalVarsREADDATA
//	@function	：	initialize the reading data
//	@return		：	(void) 
//*************************************************************************************************************
void InitGlobalVarsREADDATA(
	void
)
{
	for (int i = 0; i < MAXSIZE_HASH; i++)
	{
		readdata.fault.list[i] = (FNODE*)NULL;
	}
	readdata.fault.numdete = 0;
	readdata.fault.numinit = 0;
	readdata.fault.numrema = 0;
	readdata.fault.numred = 0;
	num_clique_flag = false;
	remain_log = (int*)allocMemory(100000, sizeof(int));
	for (int i = 0;i < 10000;i++)
	{
		remain_log[i] = -1;
	}
	detect_log = (int*)allocMemory(100000, sizeof(int));
	for (int i = 0;i < 10000;i++)
	{
		detect_log[i] = -1;
	}
	return;
}

//*************************************************************************************************************
//	@name		：　InitGlobalVarsSCIP
//	@function	：	initialize the scip
//	@return		：	(void) 
//*************************************************************************************************************
#if 0
void InitGlobalVarsSCIP(
	void
)
{
	scip.sol = (char**)NULL;
	scip.sol = (char**)allocMemory(N_SOL, sizeof(char*));

	/** for test pattern */
	scip.sol[SOL_TP] = (char*)allocMemory(n_dff + n_pi + 1, sizeof(char));
	scip.sol[SOL_TP][n_dff + n_pi] = '\0';

	scip.objval = SCIP_OBJVAL_ERROR;
	scip.status = SCIP_STATUS_ERROR;

	return;
}
#endif

//*************************************************************************************************************
//	@name		：　InitGlobalVarsCLASP
//	@function	：	initialize the scip
//	@return		：	(void) 
//*************************************************************************************************************
#if 1
void InitGlobalVarsCLASP(
	void
)
{
	clasp.sol = (char**)NULL;
	clasp.sol = (char**)allocMemory(N_SOL, sizeof(char*));

	/** for test pattern */
	clasp.sol[SOL_TP] = (char*)allocMemory(n_dff + n_pi + 1, sizeof(char));
	clasp.sol[SOL_TP][n_dff + n_pi] = '\0';

	clasp.objval = CLASP_OBJVAL_ERROR;
	clasp.status = CLASP_STATUS_ERROR;

	return;
}
#endif

//*************************************************************************************************************
//	@name		：　InitGlobalVarsTEST
//	@function	：	initialize the test
//	@return		：	(void) 
//*************************************************************************************************************
#if 0
void InitGlobalVarsTEST(
	void
)
{
	test.step1 = 0;
	test.step2 = 0;

	return;
}
#endif

//*************************************************************************************************************
//	@name		：　InitGlobalVarsLOG
//	@function	：	initialize the log
//	@return		：	(void) 
//*************************************************************************************************************
#if 0
void InitGlobalVarsLOG(
	void
)
{
	atpglog.opb.consave = 0.0;
	atpglog.opb.consmax = 0;
	atpglog.opb.consmin = 0;
	atpglog.opb.varsave = 0.0;
	atpglog.opb.varsmax = 0;
	atpglog.opb.varsmin = 0;

	atpglog.scip.status = 0;
	atpglog.scip.sollimit = 0;
	atpglog.scip.optimal = 0;
	atpglog.scip.timelimit = 0;
	atpglog.scip.infeasible = 0;

	atpglog.target.avenum = 0.0;
	atpglog.target.avenumuf = 0.0;
	atpglog.target.maxnum = 0;
	atpglog.target.maxnumuf = 0;
	atpglog.target.minnum = 0;
	atpglog.target.minnumuf = 0;

	return;
}
#endif










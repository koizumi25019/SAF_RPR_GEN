//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "./init.h"
#include "./read.h"
#include "./opb/clasp/clasp.h"
#include "../lib/lib.h"
#include "../asg/opb/opb.h"
#include "../netlist/netlist.h"
#include "../asg/asg.h"


//*************************************************************************************************************
//	@name		�F�@InitGlobalVars
//	@function	�F	initialize the global variable
//	@return		�F	(bool) okay, error 
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

	return INIT_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@InitGlobalVarsNLIST
//	@function	�F	initialize the netlist
//	@return		�F	(void) 
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
//	@name		�F�@InitGlobalVarsSTACK
//	@function	�F	initialize the stack
//	@return		�F	(void) 
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
//	@name		�F�@InitGlobalVarsQUE
//	@function	�F	initialize the queue
//	@return		�F	(void) 
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
//	@name		�F�@InitGlobalVarsOPB
//	@function	�F	initialize the opb
//	@return		�F	(void) 
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
//	@name		�F�@InitGlobalVarsREADDATA
//	@function	�F	initialize the reading data
//	@return		�F	(void) 
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

	return;
}

//*************************************************************************************************************
//	@name		�F�@InitGlobalVarsCLASP
//	@function	�F	initialize the scip
//	@return		�F	(void) 
//*************************************************************************************************************
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









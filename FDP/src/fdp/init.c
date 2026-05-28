//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "./init.h"
#include "./read.h"
#include "../lib/lib.h"
#include "../fdp/cnf/cnf.h"
#include "../netlist/netlist.h"
#include "../fdp/fault_detection_prob.h"
#include "../opt/opt.h"


//*************************************************************************************************************
//	@name		F@InitGlobalVars
//	@function	F	initialize the global variable
//	@return		(bool) okay, error 
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

	return INIT_OKAY;
}

//*************************************************************************************************************
//	@name		ComputeLevels
//	@function	compute the logic level of each net
//	@return		(void)
//*************************************************************************************************************
static void ComputeLevels(void)
{
	for (int i = 0; i < n_net; i++)
		nl[i].level = 0;

	for (int iter = 0; iter < n_net; iter++)
	{
		for (int i = 0; i < n_net; i++)
		{
			if (nl[i].type == IN || nl[i].type == DFF) continue;
			for (int j = 0; j < nl[i].n_in; j++)
			{
				int candidate = nl[i].in[j]->level + 1;
				if (candidate > nl[i].level)
					nl[i].level = candidate;
			}
		}
	}
}

//*************************************************************************************************************
//	@name		InitGlobalVarsNLIST
//	@function	initialize the netlist
//	@return		(void)
//*************************************************************************************************************
void InitGlobalVarsNLIST(
	void
)
{
	for (int i = 0; i < n_net; i++)
	{
		nl[i].varsgc      = UNASSIGN;
		nl[i].varsfc      = UNASSIGN;
		nl[i].flag        = RESET;
		nl[i].logic_value = -1;
		nl[i].ea_flag     = 0;
		nl[i].unique_flag = 0;
		nl[i].consgc      = (int*)NULL;
		nl[i].consgc_len  = 0;
		nl[i].consfc      = (char**)NULL;
	}

	ComputeLevels();

	char* buffer = (char*)NULL;
	char* token = (char*)NULL;
	char* context = (char*)NULL;
	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));
	strncpy(buffer, opt.file.input.net, MAXSIZE_BUFFER);
	token = strtok_r(buffer, "/", &context);
	token = strtok_r(NULL, "/", &context);
	token = strtok_r(NULL, "/", &context);
	token = strtok_r(NULL, ".", &context);
	net_name = strdup(token);

	free(buffer);
	return;
}

//*************************************************************************************************************
//	@name		InitGlobalVarsSTACK
//	@function	initialize the stack
//	@return		(void) 
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
//	@name		InitGlobalVarsQUE
//	@function	initialize the queue
//	@return		(void) 
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
//	@name		InitGlobalVarsOPB
//	@function	initialize the opb
//	@return		(void) 
//*************************************************************************************************************
void InitGlobalVarsOPB(
	void
)
{
	opb.constant.cons = 0;
	opb.constant.vars = 0;
	opb.total.cons = 0;
	opb.total.vars = 0;

	return;
}

//*************************************************************************************************************
//	@name		InitGlobalVarsREADDATA
//	@function	initialize the reading data
//	@return		(void) 
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







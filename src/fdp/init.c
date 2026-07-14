//-------------------------------------------------------------------------------------------------------------
//	インクルード
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
//	@return	F	(bool) 正常, 異常 
//*************************************************************************************************************
bool InitGlobalVars(
	void
)
{
	InitGlobalVarsNLIST();

	InitGlobalVarsSTACK();

	InitGlobalVarsQUE();

	InitGlobalVarsCNF();

	InitGlobalVarsREADDATA();

	return INIT_OKAY;
}

//*************************************************************************************************************
//	@name		ComputeLevels
//	@function	compute the logic level of each net
//	@return		(void)
//*************************************************************************************************************
//	level(IN/DFF)=0, level(gate)=1+max(input levels)。各 net を一度だけ計算し
//	（メモ化）、各辺を一度だけ辿るので O(V+E)。旧版は同じ緩和を n_net 回まわす O(n_net^2)。
static int ComputeLevelOf(NLIST* net)
{
	if (net->level >= 0) return net->level;            // 計算済み（メモ化）
	if (net->type == IN || net->type == DFF)           // レベル0の起点
		return net->level = 0;

	int lev = 0;
	for (int j = 0; j < net->n_in; j++)
	{
		int c = ComputeLevelOf(net->in[j]) + 1;
		if (c > lev) lev = c;
	}
	return net->level = lev;
}

static void ComputeLevels(void)
{
	for (int i = 0; i < n_net; i++)
		nl[i].level = -1;                              // -1 = 未計算

	for (int i = 0; i < n_net; i++)
		ComputeLevelOf(&nl[i]);
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
//	@name		InitGlobalVarsCNF
//	@function	initialize the cnf
//	@return		(void) 
//*************************************************************************************************************
void InitGlobalVarsCNF(
	void
)
{
	cnf.constant.clauses = 0;
	cnf.constant.vars = 0;
	cnf.total.clauses = 0;
	cnf.total.vars = 0;

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







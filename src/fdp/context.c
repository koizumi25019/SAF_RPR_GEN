//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fdp/context.h"
#include "fault/fault_list.h"
#include "common/memory.h"
#include "netlist/traversal.h"
#include "fdp/cnf/cnf.h"
#include "netlist/netlist.h"
#include "fdp/analysis.h"
#include "cli/options.h"


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

static char* ExtractCircuitName(const char* path)
{
	const char* last_slash = strrchr(path, '/');
	const char* filename = last_slash ? last_slash + 1 : path;
	const char* last_dot = strrchr(filename, '.');
	size_t name_length = strlen(filename);
	if (last_dot != NULL && last_dot != filename) {
		name_length = (size_t)(last_dot - filename);
	}

	char* name = allocMemory(name_length + 1, sizeof(char));
	memcpy(name, filename, name_length);
	name[name_length] = '\0';
	return name;
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

	net_name = ExtractCircuitName(opt.file.input.net);
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







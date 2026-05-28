#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "../netlist/netlist.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	RESET			  0					  /**< flag type = reset */
#define UNASSIGN          0					  /**< flag type = unassign  */

#define INIT_OKAY		  true		          /**< return code = true */
#define INIT_ERROR		  false		          /**< return code = false */

/** reset the flag */
#define RESET_FLAG		do																				      \
{																											  \
	for(int i= 0; i < n_net; i++)																			  \
	{																									  	  \
		nl[i].flag = RESET;																					  \
	}																										  \
}																											  \
while (false);

/** reset the variables for faulty-circuit */
#define RESET_VARSFC	do																					  \
{																											  \
	for(int i = 0;i < n_net; i++)																			  \
	{																										  \
		nl[i].varsfc = nl[i].varsgc;																		  \
	}																										  \
}																											  \
while(false);

/** reset the problem size */
#define RESET_OPB		do																				      \
{																											  \
	opb.total.vars	= opb.constant.vars;																	  \
	opb.total.cons	= opb.constant.cons;																	  \
}																											  \
while (false);

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** initialize the global variable */
bool InitGlobalVars(
	void
);

/** initialize the netlist */
void InitGlobalVarsNLIST(
	void
);

/** initialize the stack */
void InitGlobalVarsSTACK(
	void
);

/** initialize the opb */
void InitGlobalVarsOPB(
	void
);

/** initialize the reading data */
void InitGlobalVarsREADDATA(
	void
);

/** initialize the queue */
void InitGlobalVarsQUE(
	void
);

//CLASP変数の初期化
void InitGlobalVarsCLASP(
	void
);










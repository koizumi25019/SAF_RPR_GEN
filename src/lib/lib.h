#pragma once 
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>

#include "../netlist/netlist.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	MAXSIZE_STACK     100				  /**< maximum size of stack */
#define	MAXSIZE_QUEUE     100				  /**< maximum size of queue */
#define	QUE_MODE_ONE      1 				  /**< mode= "only one netlist" */
#define	QUE_MODE_ALLIN    2 				  /**< mode= "all input netlist" */
#define	QUE_MODE_ALLOUT   3 				  /**< mode= "all output netlist" */
#define POSSIBLE		  1					  /**< flag type = possible  */
#define UNPOSSIBLE		  0					  /**< flag type = unpossible  */


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** stack */
typedef struct Stack
{
	int					  maxnum;			  /**< maximum number of elements */
	int					  ptr;				  /**< number of elements */
	NLIST** stk;				  /**< stack */
}
STACK;

/** queue */
typedef struct Queue
{
	int					  maxnum;			  /**< maximum number of elements */
	int					  num;				  /**< number of elements */
	int					  front;			  /**< front index */
	int					  rear;				  /**< rear index */
	NLIST** que;				  /**< que */
}
QUE;

//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
STACK					   stack;			  /**< stack */
QUE						   que;				  /**< que */

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** open the file */
void fileOpen(
	FILE** fileptr,			  /**< pointer to file */
	const char* filename,			  /**< filename */
	const char* mode				  /**< mode */
);

/** allocate the memory */
void* allocMemory(
	size_t			      count,			  /**< number of counts of alloc memory */
	size_t			      size				  /**< typesizeof */
);

/** calculate the hash value */
int calcHash(
	char* buffer			  /** buffer */
);

/** push the netlist in stack */
void stackPUSH(
	NLIST* netptr			  /** pointer to netlist */
);

/** pop the netlist from stack */
NLIST* stackPOP(
	void
);

/** enqueue the netlist to queue */
void queENQ(
	NLIST* netptr,			  /** pointer to netlist */
	int				      mode				  /** mode */
);

/** dequeue the netlist from queue */
NLIST* queDEQ(
	void
);

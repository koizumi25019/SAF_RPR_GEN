#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>

#include "./target_fault.h"
#include "./read.h"
#include "../netlist/netlist.h"
#include "../lib/lib.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define READ_OKAY		  true		          /**< return code   = ture */
#define READ_ERROR		  false				  /**< return code   = false */

#define MAXSIZE_BUFFER	  500000	          /**< maximum size of buffer */
#define MAXSIZE_BITINT	  32		          /**< maximum size of bitint */
#define MAXSIZE_CHAR	  5000

#define FOUND			  true		          /**< return code   = true */
#define NOT_FOUND		  false				  /**< return code   = false */

/** compare string and "End-Of-File" */
#define COMP_EOF(string)		string != (char*)NULL

/** compare string and "New-Line" */
#define COMP_NEWLINE(string)	strcmp(string, "\n")!=0	

#define	MAXSIZE_HASH	  100				  /**< size of fault list */
#define TFR				  2			          /**< transition fault model   = rise */
#define	TFF				  4			          /**< transition fault model   = fall */
#define SF0				  8			          /**< stack-at fault model   = 0 */
#define	SF1				  16			      /**< stack-at fault model   = 1 */

#define	FP				  1					  /**< flag type   = fault point */
#define	TFO				  2			          /**< flag type   = transitive fout */
#define	TPO				  4					  /**< flag type   = transitive priamary-output */
#define	TFO_TPO			  (TFO|TPO)			  /**< flag type   = TFO & TPO */
#define	FP_TPO			  (FP|TPO)			  /**< flag type   = FP & TPO */

#define	DETECTED		  7					  /**< fault detected */
#define	UNDETECTED		  17				  /**< fault undetected */
#define	REDEUNDANT		  27				  /**< fault redeundant */

/** set the type of fault  */
#define	fnodeptr_type___setFaultType(type)	do																  \
{																											  \
	context = strtok_r(NULL, " \n\0", &context);															  \
	if(context == NULL)																						  \
	{																										  \
		printf("\n	FILE ERROR: fault file reading failed. ");									  \
		printf("type of fault error.\n\n");														  \
		return (FNODE*)NULL;																			      \
	}																										  \
	if (!strcmp(context, "sa0"))																			  \
	{																										  \
		type = SF0;																							  \
	}																										  \
	else if (!strcmp(context, "sa1"))																		  \
	{																										  \
		type = SF1;																							  \
	}																										  \
	else																									  \
	{																										  \
		printf("\n	FILE ERROR: fault file reading failed. ");									  \
		printf("%c%s%c unexpected type of fault.\n\n",'"', context, '"');						  \
		return (FNODE*)NULL;																			      \
	}																										  \
}																											  \
while (false);

/** set the pointer to netlist  */
#define	fnodeptr_netptr___setNetPtr(netptr, buffer)	do														  \
{																											  \
	netptr = (NLIST*)NULL;																					  \
	for (int i = 0; i < n_net; i++)																			  \
	{																										  \
		if (!strcmp(nl[i].name, buffer))																	  \
		{																									  \
			netptr = &nl[i];																				  \
		}																									  \
	}																										  \
	if(netptr==(NLIST*)NULL)																				  \
	{																										  \
		printf("\n	FILE ERROR: fault file reading failed. ");									  \
		printf("%c%s%c is thought.\n\n",	'"', buffer, '"');										  \
		return (FNODE*)NULL;																			      \
	}																										  \
}																											  \
while (false);

//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------

/** fault node structre */
typedef struct FaultNode
{
	char* string;					 /**< string */
	char* name;						 /**< name */
	int					  type;		/**< fault type */
	int					  detect;			  /**< detected??? */
	NLIST* netptr;					 /**< pointer to netlist */
	struct FaultNode* nextptr;				  /**< pointer to next node */
	//int id;							/**< id */
}
FNODE;

/** fault list structres */
typedef struct FaultList
{
	int					  numrema;			  /**< number of remaining faults */
	int					  numinit;			  /**< number of initial faults */
	int					  numdete;			  /**< number of detected faults */
	int					  numred;			  /**< number of redundant faults */
	struct FaultNode* list[MAXSIZE_HASH]; /**< fault list */
}
FLIST;


/** read data structres */
typedef struct ReadData
{
	struct FaultList	  fault;			  /**< fault list data */
}
READDATA;


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
READDATA			      readdata;			  /**< reading data */

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------

/** read the fault */
bool ReadFault(
	void
);

/** create the fault lists */
bool CreateFaultList(
	char* buffer			  /**< buffer */
);

/** search for fault node */
bool searchFnode(
	char* buffer,			  /**< buffer (key) */
	FNODE* fnode				  /**< pointer to fault node */
);

/** create the fault node */
FNODE* CreateFaultNode(
	char* buffer			  /**< buffer */
);

//等価故障解析
void AnalyzeEquivalenceFaults(
	void
);

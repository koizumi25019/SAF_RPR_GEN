/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/target.h																	     */
/*	deginer		:	R.miura			covered T.sone													  		 */
/*	date		:	2022.10.01		(2023.10.10)											  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "../standard.h"
#include "./read.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define TARGET_OKAY	     true				  /**< return code = okay */
#define TARGET_ERROR	 false				  /**< return code = error */
#define F1				16
#define F2				256
#define F3				4096
#define F4				65536
#define F5				1048576
#define F6				16777216
#define F7				268435456
#define F8				4294967296

//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** target fault structure */
typedef struct TargetFault
{
	int					  num;			      /**< number of target faults */
	struct FaultNode** list;				  /**< list */
}
TARGET;

/* sort structure*/
typedef struct Sorted {
	unsigned int num;						/**< number of sort strcuture */
	struct Edge* sort;					/**< sort list */
}SORTED;

/* edge structure*/
typedef struct Edge {
	unsigned int id;						/**< id */
	unsigned int n_edge;					/**< number of edge */
}EDGE;
//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** set the target-fault list */
bool SetTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target,			  /**< target-fault list */
	SORTED* sorted,			  /**< sorted list */
	int loop				  /**< number of loop */
);

/** create necessary net */
bool CreateNecessaryNet(
	TARGET* target			  /**< target-fault list */
);

/* create compatible edge*/
bool CreateCompatibleEdge(
	TARGET* target			  /**< target-fault list */
);

/* set the direct target */
bool DirectInputTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
);

/* sort edge Descend*/
void SortEdgeDescend(
	TARGET* target,			  /**< target-fault list */
	SORTED* sorted 			  /**< sorted list */
);

/* sort edge Ascend*/
void SortEdgeAscend(
	TARGET* remain,			  /**< target-fault list */
	SORTED* sorted 			  /**< sorted list */
);


int FuncSortAscend(
	const void* n1, const void* n2
);

int FuncSortDescend(
	const void* n1, const void* n2
);

/** create the compatible infomation */
void CreateCompatibleInfo(
	TARGET* target			  /**< target-fault list */
);

/** initial compatible infomation */
void COMPinti(
	TARGET* target			  /**< target-fault list */
);

/** set the real-target-fault list */
bool RealTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target,			  /**< target-fault list */
	SORTED* sorted, 			  /**< sorted list */
	int loop				  /**< loop variable */
);

/* counter bits */
int CountBits(
	unsigned int n			  /**< target-number */			
);

/* output graph log file */
void OutGraphLogfile(
	TARGET* remain			  /**< target-fault list */
);

void shuffle(
	int* array, int n
);

bool RandomTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
);
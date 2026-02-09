#include <stdio.h>

#include "../../netlist/netlist.h"
#include "../read.h"
#include "../target.h"
#include "../createSGmodel.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	MAXSIZE_CONS      500000			  /**< maximum size of constraint */

//#define	FORMAT_OPB
#define	FORMAT_CNF

/**  */
#define ALLOC_MEM_MINIMIZE	do																				  \
{																											  \
	*minimize= (char*)allocMemory(MAXSIZE_CONS, sizeof(char));											      \
}																											  \
while(false);

/**  */
#define ALLOC_MEM_CONSFC(numalloc)	do																		  \
{																											  \
	for (int i = 0; i < n_net; i++)																			  \
	{																										  \
		nl[i].consfc = (char**)allocMemory(numalloc, sizeof(char*));										  \
 																											  \
		for (int j = 0; j < numalloc; j++)																	  \
		{																									  \
			nl[i].consfc[j] = (char*)NULL;																	  \
		}																									  \
	}																										  \
}																											  \
while(false);


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** problem size */
typedef struct OPBproblemSize
{
	int					  vars;			      /**< number of variables */
	int					  cons;				  /**< number of constraints */
	int					  pros;				  /**< number of products */
	int					  spros;			  /**< number of sizeproducts */
}
OPBSIZE;

/** opb */
typedef struct OPBproblem
{
	OPBSIZE				  constant;			  /**< constant size */
	OPBSIZE				  total;			  /**< total size */
}
OPB;

//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
OPB							opb;				/**< opb */
unsigned int				fc_po;				/* function output */
int							constraint;			/* constraint */
int							CONS_SIZE;

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** create the good-circuit constraint */
bool CreateConsGC(
	void
);

/** assigne the variable for good-circuit */
void AssigneVarsGC(
	void
);

/** create the good-circuit constraint -AND */
void CreateConsGC_AND(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -NAND */
void CreateConsGC_NAND(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -OR */
void CreateConsGC_OR(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -NOR */
void CreateConsGC_NOR(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -BUF */
void CreateConsGC_BUF(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -INV */
void CreateConsGC_INV(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -XOR */
void CreateConsGC_XOR(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the good-circuit constraint -XNOR */
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the faulty-circuit constraint */
bool CreateConsFC(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
);

/** search for transitive-fout */
void SearchTFO(
	FNODE* target			  /**< target fault */
);

/** create the faulty-circuit constraint -AND */
void CreateConsFC_AND(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -NAND */
void CreateConsFC_NAND(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -OR */
void CreateConsFC_OR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -NOR */
void CreateConsFC_NOR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -BUF */
void CreateConsFC_BUF(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -INV */
void CreateConsFC_INV(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -XOR */
void CreateConsFC_XOR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the faulty-circuit constraint -XNOR */
void CreateConsFC_XNOR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
);

/** create the detection-circuit constraint */
void CreateConsDC(
	FNODE* target,			  /**< target fault */
	int 				  numfault			  /**< target index */
);

/** create the constraint for connect transitive-primary output */
void CreateConsDC_XOR(
	char** cons				  /**< constraint */
);

/** create the constraint for connect the xor outputs */
void CreateConsDC_OR(
	char** cons				  /**< constraint */
);

/** create the constraint for fault excitation */
void CreateConsDC_FE(
	char** cons,				  /**< constraint */
	FNODE* fnodeptr			  /**< pointer to fault node */
);

/** calculate the problem size */
void OPBcalcSize(
	OPBSIZE* sizeptr,			  /**< pointer to OPB size */
	int					  vars,				  /**< number of variables */
	int					  cons,				  /**< number of constraints */
	int					  pros,				  /**< number of products */
	int					  spros				  /**< number of sizseproducts */
);

/** make the problem file */
void makeProbFile(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
);

/** make file -write the minimize */
void makeProbFileMini(
	FILE* fileptr,			  /**< pointer to file */
	char** minimize			  /**< minimize */
);

/** make file -write the gc-constraint */
void makeProbFileConsGC(
	FILE* fileptr			  /**< pointer to file */
);

/** make file -write the fc-constraint */
void makeProbFileConsFC(
	FILE* fileptr,			  /**< pointer to file */
	TARGET* target			  /**< target fault */
);

/** make the pbo file */
void makePBOFile(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
);

/** make file -write the minimize */
void makeProbFileMini_clasp(
	FILE* fileptr,			  /**< pointer to file */
	char** minimize			  /**< minimize */
);

/** make file -write the gc-constraint */
void makeProbFileConsGC_clasp(
	FILE* fileptr			  /**< pointer to file */
);

/** make file -write the fc-constraint */
void makeProbFileConsFC_clasp(
	FILE* fileptr,			  /**< pointer to file */
	TARGET* target			  /**< target fault */
);









#pragma once

#include <stdio.h>

#include "../../netlist/netlist.h"
#include "../read.h"
#include "../create_TPG_model.h"

//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** problem size */
typedef struct OPBproblemSize
{
	int					  vars;			      /**< number of variables */
	int					  cons;				  /**< number of constraints */
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
	CCaDiCaL* solver,       // ★追加
	TARGET* target			  /**< target fault */
);

/** search for transitive-fout */
void SearchTFO(
	FNODE* target			  /**< target fault */
);

/** create fault propagation (D-chain) constraints */
void CreateConsProp(
	CCaDiCaL* solver,
	FNODE* target
);

/** create the faulty-circuit constraint -AND */
void CreateConsFC_AND(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -NAND */
void CreateConsFC_NAND(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -OR */
void CreateConsFC_OR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -NOR */
void CreateConsFC_NOR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -BUF */
void CreateConsFC_BUF(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -INV */
void CreateConsFC_INV(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -XOR */
void CreateConsFC_XOR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr			  /**< pointer to netlist */
);

/** create the faulty-circuit constraint -XNOR */
void CreateConsFC_XNOR(
	CCaDiCaL* solver,       // ★追加
	NLIST* netptr		  /**< pointer to netlist */
);

/** create the detection-circuit constraint */
void CreateConsDC(
	CCaDiCaL* solver,       // ★追加
	FNODE* target		  /**< target fault */
);

/** create the constraint for connect transitive-primary output */
void CreateConsDC_XOR(
	CCaDiCaL* solver     // ★追加
);

/** create the constraint for connect the xor outputs */
void CreateConsDC_OR(
	CCaDiCaL* solver     // ★追加
);

/** create the constraint for fault excitation */
void CreateConsDC_FE(
	CCaDiCaL* solver,       // ★追加
	FNODE* fnodeptr			  /**< pointer to fault node */
);


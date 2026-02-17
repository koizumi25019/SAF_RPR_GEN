//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "./opb.h"
#include "../createSGmodel.h"
#include "../init.h"
#include "../../netlist/netlist.h"
#include "../../lib/lib.h"


//*************************************************************************************************************
//	@name		�F�@CreateConsGC
//	@function	�F	create the good-circuit constraint
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool CreateConsGC(
	void
)
{
	/** assigne the variable for good-circuit */
	AssigneVarsGC();

	/** create the good-circuit constraint */
	for (int i = 0; i < n_net; i++)
	{
		switch (nl[i].type)
		{
		case AND:	CreateConsGC_AND(&nl[i]);	break;

		case NAND:	CreateConsGC_NAND(&nl[i]);	break;

		case OR:	CreateConsGC_OR(&nl[i]);	break;

		case NOR:	CreateConsGC_NOR(&nl[i]);	break;

		case INV:	CreateConsGC_INV(&nl[i]);	break;

		case BUF:
		case FOUT:	CreateConsGC_BUF(&nl[i]);	break;

		case EXOR:	CreateConsGC_XOR(&nl[i]);	break;

		case EXNOR:	CreateConsGC_XNOR(&nl[i]);	break;

		case DFF:
		case IN:								break;

		default:
			printf("\n	SYSTEM ERROR: test pattern model generation failed. ");
			printf("some gates are not supported. \n\n");

			return TPG_MODEL_ERROR;
		}
	}
		return TPG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@AssigneVarsGC
//	@function	�F	assigne the variable for good-circuit
//	@return		�F	(void)
//*************************************************************************************************************
void AssigneVarsGC(
	void
)
{

	/** assigne the variable for primary inputs */
	for (int i = 0; i < n_pi; i++) {
		pi[i]->varsgc = ++opb.constant.vars;
		//PrintDebugMessage("x%d��%s varsgc\n", pi[i]->varsgc, pi[i]->name);
	}
	/** assigne the variable for others */
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].varsgc == UNASSIGN)
			nl[i].varsgc = ++opb.constant.vars;
			//PrintDebugMessage("x%d��%s varsgc\n", nl[i].varsgc, nl[i].name);
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_AND
//	@function	�F	create the good-circuit constraint -AND
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_AND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char *cons = (char*)malloc(MAXSIZE_CONS);
    char *p = cons; int rest = MAXSIZE_CONS;

    // (¬x1 ∨ ¬x2 ∨ ... ∨ z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "-%u ", netptr->in[i]->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }
    p += snprintf(p, rest, "%u 0\n", netptr->varsgc + 1);
    rest = MAXSIZE_CONS - (p - cons);

    // (xi ∨ ¬z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "%u -%u 0\n", netptr->in[i]->varsgc + 1, netptr->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }

    OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0); // 節数を加算
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_NAND
//	@function	�F	create the good-circuit constraint -NAND
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_NAND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
char *cons = (char*)malloc(MAXSIZE_CONS);
    char *p = cons; int rest = MAXSIZE_CONS;

    // (¬x1 ∨ ¬x2 ∨ ... ∨ ¬z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "-%u ", netptr->in[i]->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }
    p += snprintf(p, rest, "-%u 0\n", netptr->varsgc + 1);
    rest = MAXSIZE_CONS - (p - cons);

    // (xi ∨ z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "%u %u 0\n", netptr->in[i]->varsgc + 1, netptr->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }

    OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_OR
//	@function	�F	create the good-circuit constraint -OR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_OR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char *cons = (char*)malloc(MAXSIZE_CONS);
    char *p = cons; int rest = MAXSIZE_CONS;

    // (x1 ∨ x2 ∨ ... ∨ ¬z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "%u ", netptr->in[i]->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }
    p += snprintf(p, rest, "-%u 0\n", netptr->varsgc + 1);
    rest = MAXSIZE_CONS - (p - cons);

    // (¬xi ∨ z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "-%u %u 0\n", netptr->in[i]->varsgc + 1, netptr->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }

    OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_NOR
//	@function	�F	create the good-circuit constraint -NOR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_NOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char *cons = (char*)malloc(MAXSIZE_CONS);
    char *p = cons; int rest = MAXSIZE_CONS;

    // (x1 ∨ x2 ∨ ... ∨ z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "%u ", netptr->in[i]->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }
    p += snprintf(p, rest, "%u 0\n", netptr->varsgc + 1);
    rest = MAXSIZE_CONS - (p - cons);

    // (¬xi ∨ ¬z)
    for (int i = 0; i < netptr->n_in; i++) {
        p += snprintf(p, rest, "-%u -%u 0\n", netptr->in[i]->varsgc + 1, netptr->varsgc + 1);
        rest = MAXSIZE_CONS - (p - cons);
    }

    OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_BUF
//	@function	�F	create the good-circuit constraint -BUF
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_BUF(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char *cons = (char*)malloc(MAXSIZE_CONS);
    snprintf(cons, MAXSIZE_CONS, "-%u %u 0\n%u -%u 0\n", 
             netptr->in[0]->varsgc + 1, netptr->varsgc + 1, 
             netptr->in[0]->varsgc + 1, netptr->varsgc + 1);
    OPBcalcSize(&opb.constant, 0, 2, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_INV
//	@function	�F	create the good-circuit constraint -INV
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_INV(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char *cons = (char*)malloc(MAXSIZE_CONS);
    snprintf(cons, MAXSIZE_CONS, "%u %u 0\n-%u -%u 0\n", 
             netptr->in[0]->varsgc + 1, netptr->varsgc + 1, 
             netptr->in[0]->varsgc + 1, netptr->varsgc + 1);
    OPBcalcSize(&opb.constant, 0, 2, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_XOR
//	@function	�F	create the good-circuit constraint -XOR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_XOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char *cons = (char*)malloc(MAXSIZE_CONS);
    unsigned int a = netptr->in[0]->varsgc + 1;
    unsigned int b = netptr->in[1]->varsgc + 1;
    unsigned int z = netptr->varsgc + 1;
    snprintf(cons, MAXSIZE_CONS, "-%u -%u -%u 0\n%u %u -%u 0\n%u -%u %u 0\n-%u %u %u 0\n",
             a, b, z, a, b, z, a, b, z, a, b, z);
    OPBcalcSize(&opb.constant, 0, 4, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_XNOR
//	@function	�F	create the good-circuit constraint -XNOR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
char *cons = (char*)malloc(MAXSIZE_CONS);
    unsigned int a = netptr->in[0]->varsgc + 1;
    unsigned int b = netptr->in[1]->varsgc + 1;
    unsigned int z = netptr->varsgc + 1;
    snprintf(cons, MAXSIZE_CONS, "%u %u %u 0\n-%u -%u %u 0\n-%u %u -%u 0\n%u -%u -%u 0\n",
             a, b, z, a, b, z, a, b, z, a, b, z);
    OPBcalcSize(&opb.constant, 0, 4, 0, 0);
    netptr->consgc = strdup(cons); free(cons);
}










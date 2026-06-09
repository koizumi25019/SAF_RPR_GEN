//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>

#include "./cnf.h"
#include "../create_TPG_model.h"
#include "../init.h"
#include "../../netlist/netlist.h"
#include "../../lib/lib.h"

//*************************************************************************************************************
//	@name		F@CreateConsGC
//	@function	F	create the good-circuit constraint
//	@return		F	(bool) okay, error
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

			return false;
		}
	}
		return true;
}

//*************************************************************************************************************
//	@name		F@AssigneVarsGC
//	@function	F	assigne the variable for good-circuit
//	@return		F	(void)
//*************************************************************************************************************
void AssigneVarsGC(
	void
)
{
	/** assigne the variable for primary inputs */
	for (int i = 0; i < n_pi; i++) {
		pi[i]->varsgc = ++cnf.constant.vars;
	}
	/** assigne the variable for others */
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].varsgc == UNASSIGN)
			nl[i].varsgc = ++cnf.constant.vars;
	}

	return;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_AND
//	@function	F	create the good-circuit constraint -AND
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_AND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int n = netptr->n_in;
	// clause1: n+1 lits + 0;  n clauses of 2 lits + 0
	int sz = (n + 2) + n * 3;
	int* arr = (int*)malloc(sz * sizeof(int));
	int idx = 0;

	// (-x1 ∨ -x2 ∨ ... ∨ z)
	for (int i = 0; i < n; i++)
		arr[idx++] = -(int)netptr->in[i]->varsgc;
	arr[idx++] = (int)netptr->varsgc;
	arr[idx++] = 0;

	// (xi ∨ -z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = (int)netptr->in[i]->varsgc;
		arr[idx++] = -(int)netptr->varsgc;
		arr[idx++] = 0;
	}

	netptr->consgc     = arr;
	netptr->consgc_len = sz;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_NAND
//	@function	F	create the good-circuit constraint -NAND
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_NAND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int n = netptr->n_in;
	int sz = (n + 2) + n * 3;
	int* arr = (int*)malloc(sz * sizeof(int));
	int idx = 0;

	// (-x1 ∨ ... ∨ -xn ∨ -z)
	for (int i = 0; i < n; i++)
		arr[idx++] = -(int)netptr->in[i]->varsgc;
	arr[idx++] = -(int)netptr->varsgc;
	arr[idx++] = 0;

	// (xi ∨ z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = (int)netptr->in[i]->varsgc;
		arr[idx++] = (int)netptr->varsgc;
		arr[idx++] = 0;
	}

	netptr->consgc     = arr;
	netptr->consgc_len = sz;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_OR
//	@function	F	create the good-circuit constraint -OR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_OR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int n = netptr->n_in;
	int sz = (n + 2) + n * 3;
	int* arr = (int*)malloc(sz * sizeof(int));
	int idx = 0;

	// (x1 ∨ ... ∨ xn ∨ -z)
	for (int i = 0; i < n; i++)
		arr[idx++] = (int)netptr->in[i]->varsgc;
	arr[idx++] = -(int)netptr->varsgc;
	arr[idx++] = 0;

	// (-xi ∨ z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = -(int)netptr->in[i]->varsgc;
		arr[idx++] = (int)netptr->varsgc;
		arr[idx++] = 0;
	}

	netptr->consgc     = arr;
	netptr->consgc_len = sz;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_NOR
//	@function	F	create the good-circuit constraint -NOR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_NOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int n = netptr->n_in;
	int sz = (n + 2) + n * 3;
	int* arr = (int*)malloc(sz * sizeof(int));
	int idx = 0;

	// (x1 ∨ ... ∨ xn ∨ z)
	for (int i = 0; i < n; i++)
		arr[idx++] = (int)netptr->in[i]->varsgc;
	arr[idx++] = (int)netptr->varsgc;
	arr[idx++] = 0;

	// (-xi ∨ -z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = -(int)netptr->in[i]->varsgc;
		arr[idx++] = -(int)netptr->varsgc;
		arr[idx++] = 0;
	}

	netptr->consgc     = arr;
	netptr->consgc_len = sz;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_BUF
//	@function	F	create the good-circuit constraint -BUF
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_BUF(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int a = (int)netptr->in[0]->varsgc;
	int z = (int)netptr->varsgc;
	// (-a ∨ z), (a ∨ -z) → 6 ints
	int tmp[] = { -a, z, 0, a, -z, 0 };
	netptr->consgc = (int*)malloc(6 * sizeof(int));
	memcpy(netptr->consgc, tmp, 6 * sizeof(int));
	netptr->consgc_len = 6;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_INV
//	@function	F	create the good-circuit constraint -INV
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_INV(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int a = (int)netptr->in[0]->varsgc;
	int z = (int)netptr->varsgc;
	// (a ∨ z), (-a ∨ -z) → 6 ints
	int tmp[] = { a, z, 0, -a, -z, 0 };
	netptr->consgc = (int*)malloc(6 * sizeof(int));
	memcpy(netptr->consgc, tmp, 6 * sizeof(int));
	netptr->consgc_len = 6;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_XOR
//	@function	F	create the good-circuit constraint -XOR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_XOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int a = (int)netptr->in[0]->varsgc;
	int b = (int)netptr->in[1]->varsgc;
	int z = (int)netptr->varsgc;
	// (-a∨-b∨-z), (a∨b∨-z), (a∨-b∨z), (-a∨b∨z) → 16 ints
	int tmp[] = {
		-a, -b, -z, 0,
		 a,  b, -z, 0,
		 a, -b,  z, 0,
		-a,  b,  z, 0
	};
	netptr->consgc = (int*)malloc(16 * sizeof(int));
	memcpy(netptr->consgc, tmp, 16 * sizeof(int));
	netptr->consgc_len = 16;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_XNOR
//	@function	F	create the good-circuit constraint -XNOR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	int a = (int)netptr->in[0]->varsgc;
	int b = (int)netptr->in[1]->varsgc;
	int z = (int)netptr->varsgc;
	// (a∨b∨z), (-a∨-b∨z), (-a∨b∨-z), (a∨-b∨-z) → 16 ints
	int tmp[] = {
		 a,  b,  z, 0,
		-a, -b,  z, 0,
		-a,  b, -z, 0,
		 a, -b, -z, 0
	};
	netptr->consgc = (int*)malloc(16 * sizeof(int));
	memcpy(netptr->consgc, tmp, 16 * sizeof(int));
	netptr->consgc_len = 16;
}

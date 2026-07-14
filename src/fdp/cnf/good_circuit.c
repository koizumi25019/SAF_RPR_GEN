//-------------------------------------------------------------------------------------------------------------
//	インクルード
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
//	@function	F	正常回路の制約を作成する
//	@return	F	(bool) 正常, 異常
//*************************************************************************************************************
bool CreateConsGC(
	void
)
{
	/** 正常回路用の変数を割り当てる */
	AssigneVarsGC();

	/** 正常回路の制約を作成する */
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
//	@function	F	正常回路用の変数を割り当てる
//	@return		F	(void)
//*************************************************************************************************************
void AssigneVarsGC(
	void
)
{
	/** 外部入力用の変数を割り当てる */
	for (int i = 0; i < n_pi; i++) {
		pi[i]->varsgc = ++cnf.constant.vars;
	}
	/** その他の変数を割り当てる */
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].varsgc == UNASSIGN)
			nl[i].varsgc = ++cnf.constant.vars;
	}

	return;
}

//*************************************************************************************************************
//	@name		F@CreateConsGC_AND
//	@function	F	正常回路の制約を作成する -AND
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_AND(
	NLIST* netptr			  /**< ネットリストへのポインタ */
)
{
	int n = netptr->n_in;
	// 節1: n+1リテラル + 0；n個の節（各2リテラル + 0）
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
//	@function	F	正常回路の制約を作成する -NAND
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_NAND(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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
//	@function	F	正常回路の制約を作成する -OR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_OR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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
//	@function	F	正常回路の制約を作成する -NOR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_NOR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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
//	@function	F	正常回路の制約を作成する -BUF
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_BUF(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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
//	@function	F	正常回路の制約を作成する -INV
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_INV(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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
//	@function	F	正常回路の制約を作成する -XOR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_XOR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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
//	@function	F	正常回路の制約を作成する -XNOR
//	@return		F	(void)
//*************************************************************************************************************
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< ネットリストへのポインタ */
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

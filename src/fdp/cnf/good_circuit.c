//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>

#include "fdp/cnf/cnf.h"
#include "fdp/cnf/tpg_model.h"
#include "fdp/context.h"
#include "netlist/netlist.h"


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

		/* BUF/FOUT/INVは AssigneVarsGC で入力のリテラルを共有するため節を作らない */
		case INV:
		case BUF:
		case FOUT:								break;

		case EXOR:	CreateConsGC_XOR(&nl[i]);	break;

		case EXNOR:	CreateConsGC_XNOR(&nl[i]);	break;

		case IN:								break;

		default:
			printf("\n	SYSTEM ERROR: test pattern model generation failed. ");
			printf("some gates are not supported. \n\n");

			return false;
		}
	}
		return true;
}

/* 正常回路で入力と同値（BUF/FOUT）または反転（INV）になるゲート */
static bool IsGoodAliasGate(const NLIST* netptr)
{
	return netptr->type == BUF || netptr->type == FOUT || netptr->type == INV;
}

/* netptr から BUF/FOUT/INV を入力側へたどり、最初の実ゲート・外部入力（根）の変数を共有する。
 * 経路上の各ネットには、自身から根までのINVの個数の偶奇で符号を付けたリテラルを割り当てる。
 * 正常値は値が一意に決まるので、別変数＋等価節で表すのと解の集合は変わらない。 */
static void AssignAliasLiteral(NLIST* netptr)
{
	NLIST* root = netptr;
	int inversions = 0;
	while (root->varsgc == UNASSIGN && IsGoodAliasGate(root))
	{
		if (root->type == INV) inversions++;
		root = root->in[0];
	}
	if (root->varsgc == UNASSIGN)
		root->varsgc = ++cnf.constant.vars;

	for (NLIST* p = netptr; p != root; p = p->in[0])
	{
		p->varsgc = (inversions % 2) ? -root->varsgc : root->varsgc;
		if (p->type == INV) inversions--;
	}
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
	/** 外部入力用の変数を割り当てる（ブロッキング節・PI値の読み出しは正のリテラルを前提とする） */
	for (int i = 0; i < n_pi; i++) {
		pi[i]->varsgc = ++cnf.constant.vars;
	}
	/** その他の変数を割り当てる。BUF/FOUT/INVは入力のリテラルを共有する */
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].varsgc == UNASSIGN)
			AssignAliasLiteral(&nl[i]);
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
		arr[idx++] = -netptr->in[i]->varsgc;
	arr[idx++] = netptr->varsgc;
	arr[idx++] = 0;

	// (xi ∨ -z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = netptr->in[i]->varsgc;
		arr[idx++] = -netptr->varsgc;
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
		arr[idx++] = -netptr->in[i]->varsgc;
	arr[idx++] = -netptr->varsgc;
	arr[idx++] = 0;

	// (xi ∨ z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = netptr->in[i]->varsgc;
		arr[idx++] = netptr->varsgc;
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
		arr[idx++] = netptr->in[i]->varsgc;
	arr[idx++] = -netptr->varsgc;
	arr[idx++] = 0;

	// (-xi ∨ z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = -netptr->in[i]->varsgc;
		arr[idx++] = netptr->varsgc;
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
		arr[idx++] = netptr->in[i]->varsgc;
	arr[idx++] = netptr->varsgc;
	arr[idx++] = 0;

	// (-xi ∨ -z) for each i
	for (int i = 0; i < n; i++) {
		arr[idx++] = -netptr->in[i]->varsgc;
		arr[idx++] = -netptr->varsgc;
		arr[idx++] = 0;
	}

	netptr->consgc     = arr;
	netptr->consgc_len = sz;
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
	int a = netptr->in[0]->varsgc;
	int b = netptr->in[1]->varsgc;
	int z = netptr->varsgc;
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
	int a = netptr->in[0]->varsgc;
	int b = netptr->in[1]->varsgc;
	int z = netptr->varsgc;
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

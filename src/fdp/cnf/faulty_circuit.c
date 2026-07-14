//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "ccadical.h"
#include "../create_TPG_model.h"
#include "./cnf.h"
#include "../fault_detection_prob.h"
#include "../init.h"
#include "../essential_assignment.h"
#include "../../lib/lib.h"
#include "../../netlist/netlist.h"

//*************************************************************************************************************
//	@name		：	CreateConsFC
//	@function	：	故障回路の制約を作成する
//	@return		：	(bool) 正常, 異常 
//*************************************************************************************************************
bool CreateConsFC(
	CCaDiCaL* solver,
	TARGET* target			  /**< 対象故障 */
)
{
	RESET_CNF;

	FNODE* fault = target->list[0];
	if (fault->detect != UNDETECTED) return true;

	/** 伝搬先(TFO)を探索する */
	SearchTFO(fault);

	/** 故障回路の制約を作成する */
	for (int j = 0; j < n_net; j++)
	{
		if (((nl[j].flag & TFO) == TFO) && ((nl[j].flag & FP) != FP))
		{
			switch (nl[j].type)
			{
			case AND:	CreateConsFC_AND(solver, &nl[j]);		break;
			case NAND:	CreateConsFC_NAND(solver, &nl[j]);		break;
			case OR:	CreateConsFC_OR(solver, &nl[j]);		break;
			case NOR:	CreateConsFC_NOR(solver, &nl[j]);		break;
			case INV:	CreateConsFC_INV(solver, &nl[j]);		break;
			case BUF:
			case FOUT:	CreateConsFC_BUF(solver, &nl[j]);		break;
			case EXOR:	CreateConsFC_XOR(solver, &nl[j]);		break;
			case EXNOR:	CreateConsFC_XNOR(solver, &nl[j]);		break;

			case IN:
			case DFF:										break;

			default:
				printf("\n	SYSTEM ERROR: test pattern model generation failed. ");
				printf("some gates are not supported. \n\n");
				return false;
			}
		}
	}

	/** 故障伝搬(Dチェーン)制約を作成する */
	if (!getenv("MDC_NOPROP")) CreateConsProp(solver, fault);

	/** 検出回路の制約を作成する */
	CreateConsDC(solver, fault);

	/** 必須割当ての単位節を追加する */
	if (!getenv("MDC_NOEA")) EssentialAssignment(solver, fault);

	return true;
}

//*************************************************************************************************************
//	@name		：	SearchTFO
//	@function	：	search for transitive-fout
//	@return		：	(void)
//*************************************************************************************************************
void SearchTFO(
	FNODE* target			  /**< 対象故障 */
)
{
	NLIST* netptr = (NLIST*)NULL;

	numtrannet = 0;
	numtranpo = 0;

	RESET_FLAG;
	RESET_VARSFC;

	target->netptr->flag |= FP;
	stackPUSH(target->netptr);

	while (stack.ptr != 0)
	{
		netptr = stackPOP();

		if ((netptr->flag & TFO) != TFO)
		{
			netptr->flag |= TFO;
			netptr->varsfc  = ++cnf.total.vars;
			netptr->varprop = ++cnf.total.vars;

			numtrannet++;

			if (netptr->n_out != 0)
			{
				for (int i = 0; i < netptr->n_out; i++)
				{
					stackPUSH(netptr->out[i]);
				}
			}
			else
			{
				numtranpo++;
				netptr->flag |= TPO;
			}
		}
	}

	return;
}

//*************************************************************************************************************
//	@name		：	CreateConsProp
//	@function	：	create fault propagation (D-chain) constraints
//	@return		：	(void)
//*************************************************************************************************************
void CreateConsProp(
	CCaDiCaL* solver,
	FNODE* target
)
{
	// 故障サイトでACT=1を強制（伝搬変数を有効化）
	ccadical_add(solver, (int)target->netptr->varprop);
	ccadical_add(solver, 0);

	for (int i = 0; i < n_net; i++)
	{
		if ((nl[i].flag & TFO) != TFO) continue;

		// 制約A: ¬ACT(X) ∨ ACT(Y1) ∨ ACT(Y2) ∨ ...
		// ACT(X)=1 なら少なくとも1つの出力にもACT=1が伝わる
		if (nl[i].n_out > 0)
		{
			ccadical_add(solver, -(int)nl[i].varprop);
			for (int j = 0; j < nl[i].n_out; j++)
			{
				ccadical_add(solver, (int)nl[i].out[j]->varprop);
			}
			ccadical_add(solver, 0);
		}

		// 制約B: ¬ACT(X) ∨ gc(X) ∨ fc(X)
		// ACT(X)=1 なら少なくとも一方が1
		ccadical_add(solver, -(int)nl[i].varprop);
		ccadical_add(solver, (int)nl[i].varsgc);
		ccadical_add(solver, (int)nl[i].varsfc);
		ccadical_add(solver, 0);

		// 制約C: ¬ACT(X) ∨ ¬gc(X) ∨ ¬fc(X)
		// ACT(X)=1 なら少なくとも一方が0（B+CでD値: gc≠fc を表現）
		ccadical_add(solver, -(int)nl[i].varprop);
		ccadical_add(solver, -(int)nl[i].varsgc);
		ccadical_add(solver, -(int)nl[i].varsfc);
		ccadical_add(solver, 0);
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_AND
//	@function	：	故障回路の制約を作成する -AND
//*************************************************************************************************************
void CreateConsFC_AND(CCaDiCaL* solver, NLIST* netptr)
{
	// (¬in1 ∨ ¬in2 ∨ ... ∨ z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, -netptr->in[i]->varsfc);
	}
	ccadical_add(solver, netptr->varsfc);
	ccadical_add(solver, 0);

	// (in_i ∨ ¬z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, netptr->in[i]->varsfc);
		ccadical_add(solver, -netptr->varsfc);
		ccadical_add(solver, 0);
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_NAND
//	@function	：	故障回路の制約を作成する -NAND
//*************************************************************************************************************
void CreateConsFC_NAND(CCaDiCaL* solver, NLIST* netptr)
{
	// (¬in1 ∨ ¬in2 ∨ ... ∨ ¬z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, -netptr->in[i]->varsfc);
	}
	ccadical_add(solver, -netptr->varsfc);
	ccadical_add(solver, 0);

	// (in_i ∨ z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, netptr->in[i]->varsfc);
		ccadical_add(solver, netptr->varsfc);
		ccadical_add(solver, 0);
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_OR
//	@function	：	create the faulty-circuit constriant -OR
//*************************************************************************************************************
void CreateConsFC_OR(CCaDiCaL* solver, NLIST* netptr)
{
	// (x1 ∨ x2 ∨ ... ∨ ¬z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, netptr->in[i]->varsfc);
	}
	ccadical_add(solver, -netptr->varsfc);
	ccadical_add(solver, 0);

	// (¬xi ∨ z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, -netptr->in[i]->varsfc);
		ccadical_add(solver, netptr->varsfc);
		ccadical_add(solver, 0);
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_NOR
//	@function	：	故障回路の制約を作成する -NOR
//*************************************************************************************************************
void CreateConsFC_NOR(CCaDiCaL* solver, NLIST* netptr)
{
	// (x1 ∨ x2 ∨ ... ∨ z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, netptr->in[i]->varsfc);
	}
	ccadical_add(solver, netptr->varsfc);
	ccadical_add(solver, 0);

	// (¬xi ∨ ¬z)
	for (int i = 0; i < netptr->n_in; i++) {
		ccadical_add(solver, -netptr->in[i]->varsfc);
		ccadical_add(solver, -netptr->varsfc);
		ccadical_add(solver, 0);
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_BUF
//	@function	：	故障回路の制約を作成する -BUF
//*************************************************************************************************************
void CreateConsFC_BUF(CCaDiCaL* solver, NLIST* netptr)
{
	int in = netptr->in[0]->varsfc;
	int z  = netptr->varsfc;

	ccadical_add(solver, -in); ccadical_add(solver, z);  ccadical_add(solver, 0);
	ccadical_add(solver, in);  ccadical_add(solver, -z); ccadical_add(solver, 0);
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_INV
//	@function	：	故障回路の制約を作成する -INV
//*************************************************************************************************************
void CreateConsFC_INV(CCaDiCaL* solver, NLIST* netptr)
{
	int in = netptr->in[0]->varsfc;
	int z  = netptr->varsfc;

	ccadical_add(solver, in);  ccadical_add(solver, z);  ccadical_add(solver, 0);
	ccadical_add(solver, -in); ccadical_add(solver, -z); ccadical_add(solver, 0);
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_XOR
//	@function	：	故障回路の制約を作成する -XOR
//*************************************************************************************************************
void CreateConsFC_XOR(CCaDiCaL* solver, NLIST* netptr)
{
	int a = netptr->in[0]->varsfc;
	int b = netptr->in[1]->varsfc;
	int z = netptr->varsfc;

	ccadical_add(solver, -a); ccadical_add(solver, -b); ccadical_add(solver, -z); ccadical_add(solver, 0);
	ccadical_add(solver, a);  ccadical_add(solver, b);  ccadical_add(solver, -z); ccadical_add(solver, 0);
	ccadical_add(solver, a);  ccadical_add(solver, -b); ccadical_add(solver, z);  ccadical_add(solver, 0);
	ccadical_add(solver, -a); ccadical_add(solver, b);  ccadical_add(solver, z);  ccadical_add(solver, 0);
}

//*************************************************************************************************************
//	@name		：	CreateConsFC_XNOR
//	@function	：	故障回路の制約を作成する -XNOR
//*************************************************************************************************************
void CreateConsFC_XNOR(CCaDiCaL* solver, NLIST* netptr)
{
	int a = netptr->in[0]->varsfc;
	int b = netptr->in[1]->varsfc;
	int z = netptr->varsfc;

	ccadical_add(solver, a);  ccadical_add(solver, b);  ccadical_add(solver, z);  ccadical_add(solver, 0);
	ccadical_add(solver, -a); ccadical_add(solver, -b); ccadical_add(solver, z);  ccadical_add(solver, 0);
	ccadical_add(solver, -a); ccadical_add(solver, b);  ccadical_add(solver, -z); ccadical_add(solver, 0);
	ccadical_add(solver, a);  ccadical_add(solver, -b); ccadical_add(solver, -z); ccadical_add(solver, 0);
}
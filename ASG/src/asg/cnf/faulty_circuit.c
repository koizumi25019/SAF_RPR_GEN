//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "ccadical.h" 
#include "../createSGmodel.h"
#include "./cnf.h"
#include "../fault_detection_prob.h"
#include "../init.h"
#include "../../lib/lib.h"
#include "../../netlist/netlist.h"

//*************************************************************************************************************
//	@name		：	CreateConsFC
//	@function	：	create the faulty-circuit constraint
//	@return		：	(bool) okay, error 
//*************************************************************************************************************
bool CreateConsFC(
	CCaDiCaL* solver,       // ★引数に追加
	TARGET* target			  /**< target fault */
)
{
	RESET_OPB;

	for (int i = 0; i < 1; i++)
	{
		printf("targetlist[%d]:%s\n", i, target->list[i]->name);
		if (target->list[i]->detect == UNDETECTED)
		{
			/** search for transitive-fout */
			SearchTFO(target->list[i]);

			/** create the faulty-circuit constraint */
			for (int j = 0; j < n_net; j++)
			{
				if (((nl[j].flag & TFO) == TFO) && ((nl[j].flag & FP) != FP))
				{
					switch (nl[j].type)
					{
					// ★ solver を渡し、不要になった i (numfault) を削除
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

			/** create the detection-circuit constraint */
			// ★ CreateConsDC にも solver を渡すように変更が必要になります
			CreateConsDC(solver, target->list[i]);
		}
	}

	return true;
}

//*************************************************************************************************************
//	@name		：	SearchTFO
//	@function	：	search for transitive-fout
//	@return		：	(void)
//*************************************************************************************************************
void SearchTFO(
	FNODE* target			  /**< target fault */
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
			netptr->varsfc = ++opb.total.vars; // ※これもint型になっている前提

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
//	@name		：	CreateConsFC_AND
//	@function	：	create the faulty-circuit constraint -AND
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
//	@function	：	create the faulty-circuit constraint -NAND
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
//	@function	：	create the faulty-circuit constraint -NOR
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
//	@function	：	create the faulty-circuit constraint -BUF
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
//	@function	：	create the faulty-circuit constraint -INV
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
//	@function	：	create the faulty-circuit constraint -XOR
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
//	@function	：	create the faulty-circuit constraint -XNOR
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
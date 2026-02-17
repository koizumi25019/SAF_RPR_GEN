//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./opb.h"
#include "../asg.h"
#include "../read.h"
#include "../../lib/lib.h"


//*************************************************************************************************************
//	@name		�F�@CreateConsDC
//	@function	�F	create the detection-circuit constraint	//�̏ጟ�o��H����
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC(
	FNODE* target,			  /**< target fault */
	int 				  numfault			  /**< target index */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	cons[0] = '\0';

	/** create the constraint for connect the pseudo-primary output */ //�^���O���o��
	CreateConsDC_XOR(&cons);


	/** create the constraint for connect the xor output */ //�O���o��
	CreateConsDC_OR(&cons);


	/** create the fault excitation constraint */ //�̏��N����
	CreateConsDC_FE(&cons, target);


	target->netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsDC_XOR
//	@function	�F	create the constraint for connect transitive-primary output
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC_XOR(
	char** cons				  /**< constraint */
)
{

	for (int i = 0; i < n_net; i++)
	{
		if ((nl[i].flag & TPO) == TPO)
		{

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
			/**                                 ~x   +  ~y   +  ~z	 >=1 */
// 変数番号を進める (XOR出力用)
			opb.total.vars++;

			size_t len = strlen(*cons);
			size_t rem = MAXSIZE_CONS - len;

			snprintf(*cons + len, rem,
				"-%d -%d -%d 0\n"
				"-%d %d %d 0\n"
				"%d -%d %d 0\n"
				"%d %d -%d 0\n",
				nl[i].varsgc, nl[i].varsfc, opb.total.vars,
				nl[i].varsgc, nl[i].varsfc, opb.total.vars,
				nl[i].varsgc, nl[i].varsfc, opb.total.vars,
				nl[i].varsgc, nl[i].varsfc, opb.total.vars
			);
			//PrintDebugMessage("x%d��DCXOR%d\n", opb.total.vars, i);
			OPBcalcSize(&opb.total, 0, 4, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif
		}
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsDC_OR
//	@function	�F	create the constraint for connect the xor outputs
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC_OR(
	char** cons				  /**< constraint */
)
{
	if (numtranpo > 1)
	{
		/**********************************************************************
		 * DIMACS CNF Format
		 * OR Logic:
		 * Inputs(x) -> Output(z) : -x z 0
		 * Output(z) -> Inputs(x) : x1 x2 ... -z 0
		 *********************************************************************/

		for (int var = ++opb.total.vars - numtranpo; var < opb.total.vars; var++)
		{
			//~x + z >=1  ->  -x z 0
			size_t len = strlen(*cons);
			snprintf(*cons + len, MAXSIZE_CONS - len, "-%d %d 0\n",
				var,
				opb.total.vars);
		}

		for (int var = opb.total.vars - numtranpo; var < opb.total.vars; var++)
		{
			//x +  ->  x 
			size_t len = strlen(*cons);
			snprintf(*cons + len, MAXSIZE_CONS - len, "%d ",
				var);
		}

		//~z >=1  ->  -z 0
		size_t len = strlen(*cons);
		snprintf(*cons + len, MAXSIZE_CONS - len, "-%d 0\n",
			opb.total.vars);

		// 制約数(節数)を numtranpo + 1 個追加
		OPBcalcSize(&opb.total, 0, numtranpo + 1, 0, 0);
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsDC_FE
//	@function	�F	create the constraint for fault excitation
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC_FE(
	char** cons,				  /**< constraint */
	FNODE* fnodeptr			  /**< pointer to fault node */
)
{
/**********************************************************************
	 * DIMACS CNF Format
	 * Fault Excitation (Hard Constraints):
	 * SF0: x_gc=1, x_fc=0  ->  x_gc 0, -x_fc 0
	 * SF1: x_gc=0, x_fc=1  -> -x_gc 0,  x_fc 0
	 *********************************************************************/
	size_t len = strlen(*cons);

	if (fnodeptr->type == SF0)
	{
		snprintf(*cons + len, MAXSIZE_CONS - len,
			"%d 0\n"
			"-%d 0\n",
			fnodeptr->netptr->varsgc,
			fnodeptr->netptr->varsfc
		);
	}
	else if (fnodeptr->type == SF1)
	{
		snprintf(*cons + len, MAXSIZE_CONS - len,
			"-%d 0\n"
			"%d 0\n",
			fnodeptr->netptr->varsgc,
			fnodeptr->netptr->varsfc
		);
	}
	
	// 制約数(節数)を2つ追加
	OPBcalcSize(&opb.total, 0, 2, 0, 0);
}
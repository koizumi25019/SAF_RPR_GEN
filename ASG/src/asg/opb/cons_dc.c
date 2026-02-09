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
	/**********************************************************************
	/**      ______                     *
	/**	 ��--| GC |x   ____             *
	/**  ��--|____|--�_�_  �_           *
	/**      ______    ) )DC )--�� z    *	~x ~y ~z + x y ~z +
	/**	 ��--| FC |--�^�^  �^           *		    x ~y z + ~x y z =1
	/**  ��--|____|y   �P�P             *
	/**                                 *
	/*********************************************************************/

	for (int i = 0; i < n_net; i++)
	{
		if ((nl[i].flag & TPO) == TPO)
		{
#ifdef FORMAT_OPB
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
			/**                              ~x   ~y   ~z				 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d ~x%d ~x%d",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				++opb.total.vars
			);
			/**                                  x   y   ~z				 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s +1 ~x%d x%d x%d",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				opb.total.vars
			);
			/**                                  x   ~y   z				 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s +1 x%d ~x%d x%d",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				opb.total.vars
			);
			/**                                  ~x   y   z				 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s +1 x%d x%d ~x%d =1;\n",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				opb.total.vars
			);

			OPBcalcSize(&opb.total, 0, 1, 4, 12);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
			/**                                 ~x   +  ~y   +  ~z	 >=1 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d +1 ~x%d >=1;\n",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				++opb.total.vars
			);
			/**                                 ~x   +  y   +  z   >=1	 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d +1 x%d >=1;\n",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				opb.total.vars
			);
			/**                                 x   +  ~y   +  z   >=1	 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d +1 x%d >=1;\n",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				opb.total.vars
			);
			/**                                 x   +  y   +  ~z   >=1	 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 x%d +1 ~x%d >=1;\n",
				*cons,
				nl[i].varsgc,
				nl[i].varsfc,
				opb.total.vars
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
	/**********************************************************************
	/**     ____x1          ___                   *
	/**	 ��-|  |----------�_�_ �_ d1              *
	/**     |GC|x2          ) )DC)--|  ___        *
	/**  ��-|__|---| |----�^�^ �^   |--�_ �_ det  *
	/**            |-)-|    �P�P         )OR)--�� *	   ~d1 ~d2 + det =1
	/**     ____y1|--| |    ___     |--�^ �^      *
	/**	 ��-|  |--|    |--�_�_ �_ d2|  �P�P       *
	/**     |FC|y2          ) )DC)--|             *
	/**  ��-|__|----------�^�^ �^                 *
	/**                     �P�P                  *
	/*********************************************************************/
	if (numtranpo > 1)
	{
#ifdef FORMAT_OPB
		/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
		sprintf_s(*cons, MAXSIZE_CONS, "%s1", *cons);
		/**																 */
		for (int i = ++opb.total.vars - numtranpo; i < opb.total.vars; i++)
		{
			/**                             ~x ~y						 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s ~x%d",
				*cons,
				i
			);
		}
		/**                                   z   =1					 */
		sprintf_s(*cons, MAXSIZE_CONS, "%s +1 x%d =1;\n",
			*cons,
			opb.total.vars
		);

		OPBcalcSize(&opb.total, 0, 1, 1, numtranpo);
		/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
		/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
		for (int i = ++opb.total.vars - numtranpo; i < opb.total.vars; i++)
		{
			/**                                 ~x(y)+  z   >=1				 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d >=1;\n",
				*cons,
				i,
				opb.total.vars
			);
			//PrintDebugMessage("x%d��DCOR\n", opb.total.vars, i);
		}
		for (int i = opb.total.vars - numtranpo; i < opb.total.vars; i++)
		{
			/**                                 x(y)+						 */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +",
				*cons,
				i
			);
		}
		/**                                 ~z   >=1						 */
		sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
			*cons,
			opb.total.vars
		);

		OPBcalcSize(&opb.total, 0, numtranpo + 1, 0, 0);
		/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif
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
	/**     ____gc-1t_____    ____gc-2t_____
	/**  ��-|   ___      |-��-|   ___      |	    ___
	/**     | --|  �_1(0)|    | --|  �_0(1)|------�_�_ �_ det
	/**	 ��-|   |AND )-- |-��-|   |AND )-- |	    ) )DC)--
	/**     | --|__�^ x1 |    | --|__�^ x2 | |----�^�^ �^  1
	/**  ��-|____________|-��-|____________| |	   �P�P
	/**                          |      |	 |  * * * * * * * * * * * * *
	/**                       ___|__fc__|___ |	*  safe
	/**                       |   ___      | |	*  	x1 ~x2 x2' det =1
	/**                       | --|  �_1(0)| |	*   ~x1 x2 ~x2' det =1
	/**                       |   |AND )-- |-|	*
	/**                       | --|__�^ x2'|	*  unsafe
	/**                       |____________|	*	r + x1 ~x2 x2' det =1
	/**	                                        *   r + ~x1 x2 ~x2' det =1
	/**********************************************************************/
	if (fnodeptr->relax == false)
	{
		if (fnodeptr->type == SF0)
		{
#ifdef FORMAT_OPB
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d ~x%d x%d >=1;\n",
				*cons,
				fnodeptr->netptr->varsgc,
				fnodeptr->netptr->varsfc,
				opb.total.vars
			);

			OPBcalcSize(&opb.total, 0, 1, 1, 3);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
				*cons,
				fnodeptr->netptr->varsgc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
				*cons,
				fnodeptr->netptr->varsfc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
				*cons,
				opb.total.vars
			);

			OPBcalcSize(&opb.total, 0, 3, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif
		}

		if (fnodeptr->type == SF1)
		{
#ifdef FORMAT_OPB
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d x%d x%d >=1;\n",
				*cons,
				fnodeptr->netptr->varsgc,
				fnodeptr->netptr->varsfc,
				opb.total.vars
			);

			OPBcalcSize(&opb.total, 0, 1, 1, 3);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
				*cons,
				fnodeptr->netptr->varsgc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
				*cons,
				fnodeptr->netptr->varsfc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
				*cons,
				opb.total.vars
			);

			OPBcalcSize(&opb.total, 0, 3, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif
		}
	}

	else
	{
		if (fnodeptr->type == SF0)
		{
#ifdef FORMAT_OPB
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 x%d ~x%d x%d =1;\n",
				*cons,
				++opb.total.vars,
				fnodeptr->netptr->varsgc,
				fnodeptr->netptr->varsfc,
				opb.total.vars - 1
			);

			OPBcalcSize(&opb.total, 0, 1, 1, 3);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
				*cons,
				++opb.total.vars,
				fnodeptr->netptr->varsgc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d >=1;\n",
				*cons,
				opb.total.vars,
				fnodeptr->netptr->varsfc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
				*cons,
				opb.total.vars,
				opb.total.vars - 1
			);
			//PrintDebugMessage("x%d��DCFESF0\n", opb.total.vars);
			OPBcalcSize(&opb.total, 0, 3, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif
		}

		if (fnodeptr->type == SF1)
		{
#ifdef FORMAT_OPB
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d x%d x%d >=1;\n",
				*cons,
				++opb.total.vars,
				fnodeptr->netptr->varsgc,
				fnodeptr->netptr->varsfc,
				opb.total.vars - 1
			);

			OPBcalcSize(&opb.total, 0, 1, 1, 3);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d >=1;\n",
				*cons,
				++opb.total.vars,
				fnodeptr->netptr->varsgc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
				*cons,
				opb.total.vars,
				fnodeptr->netptr->varsfc
			);
			sprintf_s(*cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
				*cons,
				opb.total.vars,
				opb.total.vars - 1
			);
			//PrintDebugMessage("x%d��DCFESF1\n", opb.total.vars);
			OPBcalcSize(&opb.total, 0, 3, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif
		}
	}

	return;
}










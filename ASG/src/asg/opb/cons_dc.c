/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/opb/cons_dc.c																     */
/*	deginer		:	R.miura	(covered T.Sone)														  		 */
/*	date		:	2022.09.01	(2023.09.28)												  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


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
//	@name		ÅFÅ@CreateConsDC
//	@function	ÅF	create the detection-circuit constraint	//åÃè·åüèoâÒòHêßñÒ
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsDC(
	FNODE* target,			  /**< target fault */
	int 				  numfault			  /**< target index */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));


	/** create the constraint for connect the pseudo-primary output */ //ã^éóäOïîèoóÕ
	CreateConsDC_XOR(&cons);


	/** create the constraint for connect the xor output */ //äOïîèoóÕ
	CreateConsDC_OR(&cons);


	/** create the fault excitation constraint */ //åÃè·ó„ãNêßñÒ
	CreateConsDC_FE(&cons, target);


	target->netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsDC_XOR
//	@function	ÅF	create the constraint for connect transitive-primary output
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsDC_XOR(
	char** cons				  /**< constraint */
)
{
	/**********************************************************************
	/**      ______                     *
	/**	 Å°--| GC |x   ____             *
	/**  Å°--|____|--Å_Å_  Å_           *
	/**      ______    ) )DC )--Å° z    *	~x ~y ~z + x y ~z +
	/**	 Å°--| FC |--Å^Å^  Å^           *		    x ~y z + ~x y z =1
	/**  Å°--|____|y   ÅPÅP             *
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
			//PrintDebugMessage("x%dÅ®DCXOR%d\n", opb.total.vars, i);
			OPBcalcSize(&opb.total, 0, 4, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif
		}
	}

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsDC_OR
//	@function	ÅF	create the constraint for connect the xor outputs
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsDC_OR(
	char** cons				  /**< constraint */
)
{
	/**********************************************************************
	/**     ____x1          ___                   *
	/**	 Å°-|  |----------Å_Å_ Å_ d1              *
	/**     |GC|x2          ) )DC)--|  ___        *
	/**  Å°-|__|---| |----Å^Å^ Å^   |--Å_ Å_ det  *
	/**            |-)-|    ÅPÅP         )OR)--Å° *	   ~d1 ~d2 + det =1
	/**     ____y1|--| |    ___     |--Å^ Å^      *
	/**	 Å°-|  |--|    |--Å_Å_ Å_ d2|  ÅPÅP       *
	/**     |FC|y2          ) )DC)--|             *
	/**  Å°-|__|----------Å^Å^ Å^                 *
	/**                     ÅPÅP                  *
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
			//PrintDebugMessage("x%dÅ®DCOR\n", opb.total.vars, i);
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
//	@name		ÅFÅ@CreateConsDC_FE
//	@function	ÅF	create the constraint for fault excitation
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsDC_FE(
	char** cons,				  /**< constraint */
	FNODE* fnodeptr			  /**< pointer to fault node */
)
{
	/**********************************************************************
	/**     ____gc-1t_____    ____gc-2t_____
	/**  Å°-|   ___      |-Å°-|   ___      |	    ___
	/**     | --|  Å_1(0)|    | --|  Å_0(1)|------Å_Å_ Å_ det
	/**	 Å°-|   |AND )-- |-Å°-|   |AND )-- |	    ) )DC)--
	/**     | --|__Å^ x1 |    | --|__Å^ x2 | |----Å^Å^ Å^  1
	/**  Å°-|____________|-Å°-|____________| |	   ÅPÅP
	/**                          |      |	 |  * * * * * * * * * * * * *
	/**                       ___|__fc__|___ |	*  safe
	/**                       |   ___      | |	*  	x1 ~x2 x2' det =1
	/**                       | --|  Å_1(0)| |	*   ~x1 x2 ~x2' det =1
	/**                       |   |AND )-- |-|	*
	/**                       | --|__Å^ x2'|	*  unsafe
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
			//PrintDebugMessage("x%dÅ®DCFESF0\n", opb.total.vars);
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
			//PrintDebugMessage("x%dÅ®DCFESF1\n", opb.total.vars);
			OPBcalcSize(&opb.total, 0, 3, 0, 0);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
#endif
		}
	}

	return;
}










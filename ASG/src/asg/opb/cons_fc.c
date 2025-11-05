/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/opb/cons_fc.c																	 */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../createSGmodel.h"
#include "./opb.h"
#include "../asg.h"
#include "../init.h"
#include "../../lib/lib.h"
#include "../../standard.h"
#include "../../netlist/netlist.h"


//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC
//	@function	ÅF	create the faulty-circuit constraint
//	@return		ÅF	(bool) okay, error 
//*************************************************************************************************************
bool CreateConsFC(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
)
{
	ALLOC_MEM_MINIMIZE;
	ALLOC_MEM_CONSFC(target->num);

	RESET_OPB;

	for (int i = 0; i < target->num; i++)
	{
		//PrintDebugMessage("targetlist[%d]:%s\n", i, target->list[i]->name);
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
					case AND:	CreateConsFC_AND(&nl[j], i);		break;

					case NAND:	CreateConsFC_NAND(&nl[j], i);		break;

					case OR:	CreateConsFC_OR(&nl[j], i);			break;

					case NOR:	CreateConsFC_NOR(&nl[j], i);		break;

					case INV:	CreateConsFC_INV(&nl[j], i);		break;

					case BUF:
					case FOUT:	CreateConsFC_BUF(&nl[j], i);		break;

					case EXOR:	CreateConsFC_XOR(&nl[j], i);		break;

					case EXNOR:	CreateConsFC_XNOR(&nl[j], i);		break;

					case IN:
					case DFF:										break;

					default:
						PrintErrorMessage("\n	SYSTEM ERROR: test pattern model generation failed. ");
						PrintErrorMessage("some gates are not supported. \n\n");

						return TPG_MODEL_ERROR;
					}
				}
			}

			/** create the detection-circuit constraint */
			CreateConsDC(target->list[i], i);

			/** create the minimize */
			if (target->list[i]->relax == true) {
				CreateMini(minimize, 1, opb.total.vars);
			}
#ifdef __DEBUG_OPB_CONS_FDC__
			_CALL_DEBUG_OPB_CONS_FDC_(*minimize, target, i);
#endif
		}
	}

	return TPG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		ÅFÅ@SearchTFO
//	@function	ÅF	search for transitive-fout
//	@return		ÅF	(void)
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
			netptr->varsfc = ++opb.total.vars;
			//PrintDebugMessage("x%dÅ®%s varsfc\n", netptr->varsfc,netptr->name);

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
//	@name		ÅFÅ@CreateConsFC_AND
//	@function	ÅF	create the faulty-circuit constraint -AND
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_AND(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 Å@  _______				*
	/**	 x Å°----|      Å_			  	*	x y + ~z  =1
	/**		Å@Å@ |  AND   Åj---Å° z		*
	/**	 y Å°----|______Å^				*	(x + ~z) (y + ~z) (~x + ~y + z)
	/**								    *
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	cons[0] = '1';
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                            x  y								 */
		sprintf_s(cons, MAXSIZE_CONS, "%s x%d",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                  ~z   =1						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d =1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+  ~z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d >=1;\n",
			cons,
			netptr->in[i]->varsfc,
			netptr->varsfc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif // FORMAT_CNF

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_NAND
//	@function	ÅF	create the faulty-circuit constraint -NAND
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_NAND(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 Å@  _______				*
	/**	 x Å°----|      Å_			  	*	x y + z  =1
	/**		Å@Å@ |  NAND  ÅZ---Å° z		*
	/**	 y Å°----|______Å^				*	(x + z) (y + z) (~x + ~y + ~z)
	/**								    *
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	cons[0] = '1';
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                            x  y								 */
		sprintf_s(cons, MAXSIZE_CONS, "%s x%d",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                  z   =1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d =1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+  z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
			cons,
			netptr->in[i]->varsfc,
			netptr->varsfc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                ~z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_OR
//	@function	ÅF	create the faulty-circuit constriant -OR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_OR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 Å@  ______					*
	/**	 x Å°----Å_    Å_			  	*	~x ~y + z  =1
	/**		Å@Å@   ) OR  )---Å° z		*
	/**	 y Å°----Å^    Å^				*	(~x + z) (~y + z) (x + y + ~z)
	/**			 ÅPÅPÅP 				*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	cons[0] = '1';
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                            ~x ~y							 */
		sprintf_s(cons, MAXSIZE_CONS, "%s ~x%d",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                  z   =1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d =1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+  z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d >=1;\n",
			cons,
			netptr->in[i]->varsfc,
			netptr->varsfc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                ~z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_NOR
//	@function	ÅF	create the faulty-circuit constraint -NOR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_NOR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 Å@  ______					*
	/**	 x Å°----Å_    Å_			  	*	~x ~y + ~z  =1
	/**		Å@Å@   ) NOR ÅZ---Å° z		*
	/**	 y Å°----Å^    Å^				*	(~x + ~z) (~y + ~z) (x + y + z)
	/**			 ÅPÅPÅP 				*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	cons[0] = '1';
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                            ~x ~y							 */
		sprintf_s(cons, MAXSIZE_CONS, "%s ~x%d",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                  ~z   =1						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d =1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+  ~z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d >=1;\n",
			cons,
			netptr->in[i]->varsfc,
			netptr->varsfc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +",
			cons,
			netptr->in[i]->varsfc
		);
	}
	/**                                z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
		cons,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_BUF
//	@function	ÅF	create the faulty-circuit constraint -BUF
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_BUF(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**			|Å_		Å@Å@Å@Å@		*
	/**			|  Å_		Å@Å@Å@Å@	*		x y + ~x ~y  =1
	/**	 x Å°---|BUF >---Å° y		*
	/**			|  Å^		Å@Å@		*		(x + ~y) (~x + y)
	/**         |Å^					*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                              ~x   ~y   +  x   y   =1		     */
	sprintf_s(cons, MAXSIZE_CONS, "1 ~x%d ~x%d +1 x%d x%d =1;\n",
		netptr->in[0]->varsfc,
		netptr->varsfc,
		netptr->in[0]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 2, 4);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif // FORMAT_OPB

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**				                   x1  +  ~x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->varsfc
	);
	/**				                   ~x1  +  x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 2, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_INV
//	@function	ÅF	create the faulty-circuit constraint -INV
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_INV(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**			|Å_		Å@Å@Å@Å@		*
	/**			|  Å_		Å@Å@Å@Å@	*		~x y + x ~y  =1
	/**	 x Å°---|INV ÅZ---Å° y		*
	/**			|  Å^		Å@Å@		*		(~x + ~y) (x + y)
	/**         |Å^					*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                              ~x   y   +  x   ~y   =1		 	 */
	sprintf_s(cons, MAXSIZE_CONS, "1 ~x%d x%d +1 x%d ~x%d =1;\n",
		netptr->in[0]->varsfc,
		netptr->varsfc,
		netptr->in[0]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 2, 4);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**				                   ~x1  +  ~x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->varsfc
	);
	/**				                   x1  +  x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 2, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_XOR
//	@function	ÅF	create the faulty-circuit constraint -XOR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_XOR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		  Å@   ÅQÅQ					*
	/**	 x	Å°---Å_Å_   Å_				*	~x ~y ~z + x y ~z +
	/**		  Å@Å@ ) )XOR )---Å° z		*
	/**	 y	Å°---Å^Å^	Å^				*		x ~y z + ~x y z =1
	/**	        Å@ ÅPÅP  Å@Å@Å@Å@		*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                              ~x   ~y   ~z						*/
	sprintf_s(cons, MAXSIZE_CONS, "1 ~x%d ~x%d ~x%d",
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                  x   y   ~z						*/
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d x%d ~x%d",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                  x   ~y   z						*/
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d ~x%d x%d",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                  ~x   y   z						*/
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d x%d x%d",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 4, 12);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                                ~x   +  ~y   +  ~z	>=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                x   +  y   +  ~z	  >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                x   +  ~y   +  z	  >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                ~x   +  y   +  z	  >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 4, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsFC_XNOR
//	@function	ÅF	create the faulty-circuit constraint -XNOR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsFC_XNOR(
	NLIST* netptr,			  /**< pointer to netlist */
	int					  numfault			  /**< number of faults */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		  Å@   ÅQÅQ					*
	/**	 x	Å°---Å_Å_   Å_				*	x y z + ~x ~y z +
	/**		  Å@Å@ ) )XORNÅZ---Å° z		*
	/**	 y	Å°---Å^Å^	Å^				*		~x y ~z + x ~y ~z =1
	/**	        Å@ ÅPÅP  Å@Å@Å@Å@		*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                              x   y   z							 */
	sprintf_s(cons, MAXSIZE_CONS, "1 x%d x%d x%d",
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                  ~x   ~y   z					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d ~x%d x%d",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                  ~x   y   ~z									 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d x%d ~x%d",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                  x   ~y   ~z									 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d ~x%d ~x%d",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 1, 4, 12);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                                x   +  y   +  z	 >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                ~x   +  ~y   +  z   >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                ~x   +  y   +  ~z	 >=1		 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);
	/**                                x   +  ~y   +  ~z	   >=1		 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsfc,
		netptr->in[1]->varsfc,
		netptr->varsfc
	);

	OPBcalcSize(&opb.total, 0, 4, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}










/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/opb/cons_gc.c																	 */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "./opb.h"
#include "../createSGmodel.h"
#include "../init.h"
#include "../../standard.h"
#include "../../netlist/netlist.h"
#include "../../lib/lib.h"
#include "../../debug/debug.h"


//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC
//	@function	ÅF	create the good-circuit constraint
//	@return		ÅF	(bool) okay, error
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
			PrintErrorMessage("\n	SYSTEM ERROR: test pattern model generation failed. ");
			PrintErrorMessage("some gates are not supported. \n\n");

			return TPG_MODEL_ERROR;
		}
	}


#ifdef __DEBUG_OPB_CONS_GC__
	_CALL_DEBUG_OPB_CONS_GC_
#endif


		return TPG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		ÅFÅ@AssigneVarsGC
//	@function	ÅF	assigne the variable for good-circuit
//	@return		ÅF	(void)
//*************************************************************************************************************
void AssigneVarsGC(
	void
)
{

	/** assigne the variable for primary inputs */
	for (int i = 0; i < n_pi; i++) {
		pi[i]->varsgc = ++opb.constant.vars;
		//PrintDebugMessage("x%dÅ®%s varsgc\n", pi[i]->varsgc, pi[i]->name);
	}
	/** assigne the variable for others */
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].varsgc == UNASSIGN)
			nl[i].varsgc = ++opb.constant.vars;
			//PrintDebugMessage("x%dÅ®%s varsgc\n", nl[i].varsgc, nl[i].name);
	}

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_AND
//	@function	ÅF	create the good-circuit constraint -AND
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_AND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/*********************************************************************
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
			netptr->in[i]->varsgc
		);
	}
	/**                                  ~z   =1						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d =1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+  ~z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d >=1;\n",
			cons,
			netptr->in[i]->varsgc,
			netptr->varsgc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +",
			cons,
			netptr->in[i]->varsgc
		);
	}
	/**                                z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif // FORMAT_CNF

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_NAND
//	@function	ÅF	create the good-circuit constraint -NAND
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_NAND(
	NLIST* netptr			  /**< pointer to netlist */
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
			netptr->in[i]->varsgc
		);
	}
	/**                                  z   =1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d =1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+  z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
			cons,
			netptr->in[i]->varsgc,
			netptr->varsgc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +",
			cons,
			netptr->in[i]->varsgc
		);
	}
	/**                                ~z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_OR
//	@function	ÅF	create the good-circuit constraint -OR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_OR(
	NLIST* netptr			  /**< pointer to netlist */
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
			netptr->in[i]->varsgc
		);
	}
	/**                                  z   =1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d =1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+  z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d >=1;\n",
			cons,
			netptr->in[i]->varsgc,
			netptr->varsgc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +",
			cons,
			netptr->in[i]->varsgc
		);
	}
	/**                                ~z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d >=1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_NOR
//	@function	ÅF	create the good-circuit constraint -NOR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_NOR(
	NLIST* netptr			  /**< pointer to netlist */
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
			netptr->in[i]->varsgc
		);
	}
	/**                                  ~z   =1						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d =1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 1, netptr->n_in);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                ~x(y)+  ~z   >=1				 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d >=1;\n",
			cons,
			netptr->in[i]->varsgc,
			netptr->varsgc
		);
	}
	for (int i = 0; i < netptr->n_in; i++)
	{
		/**                                x(y)+						 */
		sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +",
			cons,
			netptr->in[i]->varsgc
		);
	}
	/**                                z   >=1							 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d >=1;\n",
		cons,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, netptr->n_in + 1, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_BUF
//	@function	ÅF	create the good-circuit constraint -BUF
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_BUF(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**			|Å_		Å@Å@Å@Å@		*
	/**			|  Å_		Å@Å@Å@Å@	*		x y + ~x ~y  =1
	/**	 x Å°---|BUF >---Å° y			*
	/**			|  Å^		Å@Å@		*		(x + ~y) (~x + y)
	/**         |Å^						*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                              ~x   ~y   +  x   y   =1			 */
	sprintf_s(cons, MAXSIZE_CONS, "1 ~x%d ~x%d +1 x%d x%d =1;\n",
		netptr->in[0]->varsgc,
		netptr->varsgc,
		netptr->in[0]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 2, 4);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**				                   x1  +  ~x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->varsgc
	);
	/**				                   ~x1  +  x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 2, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_INV
//	@function	ÅF	create the good-circuit constraint -INV
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_INV(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**			|Å_		Å@Å@Å@Å@		*
	/**			|  Å_		Å@Å@Å@Å@	*		~x y + x ~y  =1
	/**	 x Å°---|INV ÅZ---Å° y			*
	/**			|  Å^		Å@Å@		*		(~x + ~y) (x + y)
	/**         |Å^						*
	/*********************************************************************/
#ifdef FORMAT_OPB
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                              ~x   y   +  x   ~y   =1			 */
	sprintf_s(cons, MAXSIZE_CONS, "1 ~x%d x%d +1 x%d ~x%d =1;\n",
		netptr->in[0]->varsgc,
		netptr->varsgc,
		netptr->in[0]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 2, 4);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**				                   ~x1  +  ~x2  >=1				     */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->varsgc
	);
	/**				                   x1  +  x2  >=1					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 2, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_XOR
//	@function	ÅF	create the good-circuit constraint -XOR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_XOR(
	NLIST* netptr			  /**< pointer to netlist */
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
	/**                              ~x   ~y   ~z						 */
	sprintf_s(cons, MAXSIZE_CONS, "1 ~x%d ~x%d ~x%d",
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                  x   y   ~z						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d x%d ~x%d",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                  x   ~y   z						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d ~x%d x%d",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                  ~x   y   z						 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d x%d x%d",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 4, 12);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                                ~x   +  ~y   +  ~z   >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                x   +  y   +  ~z	  >=1  			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                x   +  ~y   +  z	  >=1  			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d +1 x%d >=1;\n;",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                ~x   +  y   +  z	  >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 4, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsGC_XNOR
//	@function	ÅF	create the good-circuit constraint -XNOR
//	@return		ÅF	(void)
//*************************************************************************************************************
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< pointer to netlist */
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
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                  ~x   ~y   z					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d ~x%d x%d",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                  ~x   y   ~z					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 ~x%d x%d ~x%d",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                  x   ~y   ~z					 */
	sprintf_s(cons, MAXSIZE_CONS, "%s +1 x%d ~x%d ~x%d",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 1, 4, 12);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

#ifdef FORMAT_CNF
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
	/**                                x   +  y   +  z   >=1;			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                ~x   +  ~y   +  z   >=1 			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 ~x%d +1 x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                ~x   +  y   +  ~z   >=1  		 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 ~x%d +1 x%d +1 ~x%d >=1;\n;",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);
	/**                                x   +  ~y   +  ~z   >=1			 */
	sprintf_s(cons, MAXSIZE_CONS, "%s1 x%d +1 ~x%d +1 ~x%d >=1;\n",
		cons,
		netptr->in[0]->varsgc,
		netptr->in[1]->varsgc,
		netptr->varsgc
	);

	OPBcalcSize(&opb.constant, 0, 4, 0, 0);
	/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#endif

	netptr->consgc = _strdup(cons);

	free(cons);

	return;
}










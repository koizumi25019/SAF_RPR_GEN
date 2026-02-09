//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "./opb.h"
#include "../createSGmodel.h"
#include "../init.h"
#include "../../netlist/netlist.h"
#include "../../lib/lib.h"


//*************************************************************************************************************
//	@name		�F�@CreateConsGC
//	@function	�F	create the good-circuit constraint
//	@return		�F	(bool) okay, error
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

			return TPG_MODEL_ERROR;
		}
	}


#ifdef __DEBUG_OPB_CONS_GC__
	_CALL_DEBUG_OPB_CONS_GC_
#endif


		return TPG_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@AssigneVarsGC
//	@function	�F	assigne the variable for good-circuit
//	@return		�F	(void)
//*************************************************************************************************************
void AssigneVarsGC(
	void
)
{

	/** assigne the variable for primary inputs */
	for (int i = 0; i < n_pi; i++) {
		pi[i]->varsgc = ++opb.constant.vars;
		//PrintDebugMessage("x%d��%s varsgc\n", pi[i]->varsgc, pi[i]->name);
	}
	/** assigne the variable for others */
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].varsgc == UNASSIGN)
			nl[i].varsgc = ++opb.constant.vars;
			//PrintDebugMessage("x%d��%s varsgc\n", nl[i].varsgc, nl[i].name);
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsGC_AND
//	@function	�F	create the good-circuit constraint -AND
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_AND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/*********************************************************************
	/**		 �@  _______				*
	/**	 x ��----|      �_			  	*	x y + ~z  =1
	/**		�@�@ |  AND   �j---�� z		*
	/**	 y ��----|______�^				*	(x + ~z) (y + ~z) (~x + ~y + z)
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
//	@name		�F�@CreateConsGC_NAND
//	@function	�F	create the good-circuit constraint -NAND
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_NAND(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 �@  _______				*
	/**	 x ��----|      �_			  	*	x y + z  =1
	/**		�@�@ |  NAND  �Z---�� z		*
	/**	 y ��----|______�^				*	(x + z) (y + z) (~x + ~y + ~z)
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
//	@name		�F�@CreateConsGC_OR
//	@function	�F	create the good-circuit constraint -OR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_OR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 �@  ______					*
	/**	 x ��----�_    �_			  	*	~x ~y + z  =1
	/**		�@�@   ) OR  )---�� z		*
	/**	 y ��----�^    �^				*	(~x + z) (~y + z) (x + y + ~z)
	/**			 �P�P�P 				*
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
//	@name		�F�@CreateConsGC_NOR
//	@function	�F	create the good-circuit constraint -NOR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_NOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		 �@  ______					*
	/**	 x ��----�_    �_			  	*	~x ~y + ~z  =1
	/**		�@�@   ) NOR �Z---�� z		*
	/**	 y ��----�^    �^				*	(~x + ~z) (~y + ~z) (x + y + z)
	/**			 �P�P�P 				*
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
//	@name		�F�@CreateConsGC_BUF
//	@function	�F	create the good-circuit constraint -BUF
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_BUF(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**			|�_		�@�@�@�@		*
	/**			|  �_		�@�@�@�@	*		x y + ~x ~y  =1
	/**	 x ��---|BUF >---�� y			*
	/**			|  �^		�@�@		*		(x + ~y) (~x + y)
	/**         |�^						*
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
//	@name		�F�@CreateConsGC_INV
//	@function	�F	create the good-circuit constraint -INV
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_INV(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**			|�_		�@�@�@�@		*
	/**			|  �_		�@�@�@�@	*		~x y + x ~y  =1
	/**	 x ��---|INV �Z---�� y			*
	/**			|  �^		�@�@		*		(~x + ~y) (x + y)
	/**         |�^						*
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
//	@name		�F�@CreateConsGC_XOR
//	@function	�F	create the good-circuit constraint -XOR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_XOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		  �@   �Q�Q					*
	/**	 x	��---�_�_   �_				*	~x ~y ~z + x y ~z +
	/**		  �@�@ ) )XOR )---�� z		*
	/**	 y	��---�^�^	�^				*		x ~y z + ~x y z =1
	/**	        �@ �P�P  �@�@�@�@		*
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
//	@name		�F�@CreateConsGC_XNOR
//	@function	�F	create the good-circuit constraint -XNOR
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsGC_XNOR(
	NLIST* netptr			  /**< pointer to netlist */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	/**********************************************************************
	/**		  �@   �Q�Q					*
	/**	 x	��---�_�_   �_				*	x y z + ~x ~y z +
	/**		  �@�@ ) )XORN�Z---�� z		*
	/**	 y	��---�^�^	�^				*		~x y ~z + x ~y ~z =1
	/**	        �@ �P�P  �@�@�@�@		*
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










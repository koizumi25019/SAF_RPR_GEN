/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/opb/cons_lfsr.c																 */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "./opb.h"
#include "./scip/scip.h"
#include "./clasp/clasp.h"
#include "../read.h"
#include "../createSGmodel.h"
#include "../../lib/lib.h"
#include "../../debug/debug.h"


//*************************************************************************************************************
//	@name		ÅFÅ@CreateLFSRmodel
//	@function	ÅF	create the lfsr model
//	@return		ÅF	(bool) okay, error 
//*************************************************************************************************************
bool CreateLFSRmodel(
	void
)
{
	BIT_INT* poly = (BIT_INT*)NULL;
	BIT_INT* lfsr = (BIT_INT*)NULL;
	int				xor_flag = 0;

	/** set the polynomial */
	LFSRsetPolynomial(&poly);

	/** set the lfsr */
	LFSRinti(&lfsr);

	/** alloc the memory for phase-shifter constraint */
	if (n_pi + n_dff + n_dffs > 5000)	CONS_SIZE = MAXSIZE_CONS * 30;
	else if (n_pi + n_dff + n_dffs > 3000)	CONS_SIZE = MAXSIZE_CONS * 4;
	else if (n_pi + n_dff + n_dffs > 1000)	CONS_SIZE = MAXSIZE_CONS * 2;
	else CONS_SIZE = MAXSIZE_CONS;


	for (int i = 0; i < readdata.psnet.num; i++)
		readdata.psnet.nlist[i].cons = (char*)allocMemory(CONS_SIZE, sizeof(char));

	for (int i = 0; i < readdata.psnet.maxslength; i++)
	{
		/** simulate the LFSR */
		if (i != 0)
			SimulateLFSR(lfsr, poly, 1);

		/** create the pahase-shifer constraint */
		if (CreateConsPhaseShifter(lfsr, i) != LFSR_MODEL_OKAY) return LFSR_MODEL_ERROR;
	}

	/** free the memory */
	free(poly->flag);
	free(poly);
	for (int i = 0; i < readdata.lfsrnet.num; i++) { 
		free(lfsr[i].flag); 
	}
	free(lfsr);
#ifdef NDEBUG
	assert(poly == NULL);
	assert(lfsr == NULL);
#endif

	/** alloc the memory for solution for seed */
	clasp.sol[SOL_SEED] = (char*)allocMemory(readdata.lfsrnet.num + 1, sizeof(char));
	clasp.sol[SOL_SEED][readdata.lfsrnet.num + 1] = '\0';

#ifdef __DEBUG_OPB_CONS_PS__
	_CALL_DEBUG_OPB_CONS_PS_
#endif // __DEBUG_OPB_CONS_PS__


		PrintMessage("\n	Create the lfsr model completed ... \n");

	return LFSR_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		ÅFÅ@LFSRsetPolynomial
//	@function	ÅF	set the polynomial
//	@return		ÅF	(void)
//*************************************************************************************************************
void LFSRsetPolynomial(
	BIT_INT** poly				  /**< polynomial */
)
{
	(*poly) = (BIT_INT*)allocMemory(1, sizeof(BIT_INT));

	(*poly)->int_num = N_LFSR;

	(*poly)->flag = (unsigned int*)allocMemory(6, sizeof(unsigned int));

	SET_POLYNOMIAL(readdata.lfsrnet.num, (*poly));

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@LFSRinit
//	@function	ÅF	initial the LFSR
//	@return		ÅF	(void)
//*************************************************************************************************************
void LFSRinti(
	BIT_INT** lfsr				  /**< lfsr */
)
{
	*lfsr = (BIT_INT*)allocMemory(readdata.lfsrnet.num, sizeof(BIT_INT));

	for (int i = 0; i < readdata.lfsrnet.num; i++)
	{
		(*lfsr)[i].int_num = (unsigned int)N_LFSR;

		(*lfsr)[i].flag = (unsigned int*)allocMemory((*lfsr)[i].int_num, sizeof(unsigned int));

		/** set the all-bits to zero */
		bitintSetAll_Zero(&(*lfsr)[i]);

		/** set the i-bits to one */
		bitintSetNbit_One(&(*lfsr)[i], (unsigned)(i));
	}

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@LFSRsimulate
//	@function	ÅF	simulate the LFSR
//	@return		ÅF	(void) 
//*************************************************************************************************************
void SimulateLFSR(
	BIT_INT* lfsr,				  /**< lfsr */
	BIT_INT* poly,			      /**< polynomial */
	int                   mcycle			  /**< number of cycles */
)
{
	BIT_INT* xor;
	xor = (BIT_INT*)allocMemory(1, sizeof(BIT_INT));
	xor ->int_num = N_LFSR;
	xor ->flag = (unsigned int*)allocMemory(xor ->int_num, sizeof(unsigned int));

	/** set the all-bits to zero */
	bitintSetAll_Zero(xor);

	/** simulate the LFSR */
	for (int cycle = 0; cycle < mcycle; cycle++)
	{
		/** calculate the LSB */
		for (int i = 0; i < readdata.lfsrnet.num; i++)
		{
			if (bitintGetNbit(poly, i) == true)
			{
				for (int j = 0; j < N_LFSR; j++)
					xor ->flag[j] ^= lfsr[i].flag[j];
			}
		}

		/** shift */
		for (int i = readdata.lfsrnet.num - 1; i >= 1; i--)
		{
			for (unsigned int j = 0; j < lfsr[i].int_num; j++)
				lfsr[i].flag[j] = lfsr[i - 1].flag[j];
		}

		/** set the LSB to xor-result */
		for (unsigned int j = 0; j < lfsr[0].int_num; j++)
			lfsr[0].flag[j] = xor ->flag[j];

	}

	free(xor ->flag);
	free(xor);

	return;
}

//*************************************************************************************************************
//	@name		ÅFÅ@CreateConsLFSR_XOR
//	@function	ÅF	create the lfsr constraint for xor
//	@return		ÅF	(bool) okay, error 
//*************************************************************************************************************
bool CreateConsPhaseShifter(
	BIT_INT* lfsr,				  /**< lfsr */
	int      		      mcycle			  /**< lfsr */
)
{
	int			nin = 0;
	int* in = (int*)NULL;
	int			one_flag = 0;

	/**  */
	for (int i = 0; i < readdata.psnet.num; i++)
	{
		if (readdata.psnet.nlist[i].out_schain[mcycle]->n_out != 0)
		{

			nin = PSgetInputBit(&readdata.psnet.nlist[i], lfsr, &in);

#ifdef NDEBUG
			assert(nin < 15);
#endif
			/** create the constraint */
			//one_flag = 0;

			/** number of inputs >=1   */
			if (nin != 1)
			{
				/**********************************************************************
				/**		  Å@   ÅQÅQ					*
				/**	 x1	Å°---Å_Å_   Å_				*
				/**	     :     ) )XOR )---Å° z		*
				/**	 xN	Å°---Å^Å^	Å^				*
				/**	        Å@ ÅPÅP  Å@Å@Å@Å@		*
				/*********************************************************************/					
				for (int j = 0; j < (int)pow(2, nin); j++)												
				{
					one_flag = 0;

					/** input +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- */
					for (int k = 0; k < nin; k++)														
					{

						/** n-bit =1  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+ */						
						if ((j & (int)pow(2, k)) != false)
						{
							/**          x                                           */
							if (k == 0)
							{
								sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
									"%s1 x%d ", readdata.psnet.nlist[i].cons, in[k]);
							}
							else
							{
								sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
									"%s+1 x%d ", readdata.psnet.nlist[i].cons, in[k]);
							}
						}
						/** n-bit =0  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+ */						
						else
						{
							/**          ~x                                          */
							if (k == 0)
							{
								sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
									"%s1 ~x%d ", readdata.psnet.nlist[i].cons, in[k]);
							}
							else
							{
								sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
									"%s+1 ~x%d ", readdata.psnet.nlist[i].cons, in[k]);
							}
							one_flag++;
						}
					}
					/** +-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

					/** output +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
					/**     n-bit =1  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+ */
					if (one_flag % 2 == 1)
					{
						sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
							/**   z   >=1									         */
							"%s+1 x%d >=1; \n",
							readdata.psnet.nlist[i].cons,
							readdata.psnet.nlist[i].out_schain[mcycle]->varsgc
						);
					}
					/**     n-bit =0  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+ */
					else /** if (flag_one % 2 == 0) */
					{
						sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
							/**   ~z   >=1									         */
							"%s+1 ~x%d >=1; \n",
							readdata.psnet.nlist[i].cons,
							readdata.psnet.nlist[i].out_schain[mcycle]->varsgc
						);
					}
					/** +-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
				}
				OPBcalcSize(&opb.total, 0, (int)pow(2,nin), 0, 0);
			}

			/** number of inputs =1   */
			else
			{
				/**********************************************************************
				/**		  Å@   ÅQÅQ					*
				/**	 	     Å_Å_   Å_				*
				/**	 x	Å°-----) )XOR )---Å° y		*		(x + ~y) (~x + y)
				/**	         Å^Å^	Å^				*
				/**	        Å@ ÅPÅP  Å@Å@Å@Å@		*
				/*********************************************************************/
				sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
					/**  x   +  ~y   >=1										 */
					"%s1 x%d +1 ~x%d >=1;\n",
					readdata.psnet.nlist[i].cons,
					in[0],
					readdata.psnet.nlist[i].out_schain[mcycle]->varsgc
				);
				sprintf_s(readdata.psnet.nlist[i].cons, CONS_SIZE,
					/**  x   +  ~y   >=1										 */
					"%s1 ~x%d +1 x%d >=1;\n",										
					readdata.psnet.nlist[i].cons,
					in[0],
					readdata.psnet.nlist[i].out_schain[mcycle]->varsgc
				);
				/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
			}
			//printf("\n");
			//if (mcycle > 80)
			//{
			//	printf("	readdata.psnet.nlist[%2d].cons(length)[%2d]:%d\n", i, mcycle, (int)(strlen(readdata.psnet.nlist[i].cons)));
			//}
			free(in);
		}
	}

	//ps debug
	//for (int i = 0; i < readdata.psnet.num; i++) {
	//	printf("readdata.psnet.nlist[%d].cons:\n%s\n", i, readdata.psnet.nlist[i].cons);
	//}





	return LFSR_MODEL_OKAY;
}

//*************************************************************************************************************
//	@name		ÅFÅ@PSgetInputBitint
//	@function	ÅF	get the phase-shifter input-bitint
//	@return		ÅF	(int) number of phase-phase-shifter input-bitint
//*************************************************************************************************************
int PSgetInputBit(
	PS_NLIST* psnet,			  /**< phase-shifter net */
	BIT_INT* lfsr,				  /**< lfsr */
	int** in				  /**< in */
)
{
	BIT_INT* xor = (BIT_INT*)NULL;
	int			nin = 0;
	int			index = 0;

	/**  */
	xor = (BIT_INT*)allocMemory(1, sizeof(BIT_INT));
	xor ->int_num = N_LFSR;
	xor ->flag = (unsigned int*)allocMemory(xor ->int_num, sizeof(unsigned int));
	bitintSetAll_Zero(xor);

	for (int i = 0; i < psnet->nin; i++)
	{
		for (int j = 0; j < N_LFSR; j++)
		{
			xor ->flag[j] ^= lfsr[psnet->in[i] - 1].flag[j];
			//printf("xor->flag[%d][%d]=%d\n",i, j, xor ->flag[j]);
		}
	}
	//printf("---------------------------------------------------\n");

	for (int j = 0; j < readdata.lfsrnet.num; j++)
	{
		if (bitintGetNbit(xor, (unsigned int)j) == true)
		{
			nin++;
		}
	}

	*in = (int*)allocMemory(nin, sizeof(int));
	for (int i = 0; i < readdata.lfsrnet.num; i++)
	{
		if (bitintGetNbit(xor, (unsigned int)i) == true)
		{
			(*in)[index++] = i + 1;
			//printf("in[%d]=%d\n", index - 1, i + 1);
		}
	}

	free(xor ->flag);
	free(xor);

	return nin;
}










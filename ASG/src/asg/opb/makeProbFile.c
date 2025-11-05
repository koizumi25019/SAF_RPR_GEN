/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																					     */
/*	file		:	./src/atpg/opb/makeProbFile.c														     */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "./opb.h"
#include "./scip/scip.h"
#include "../read.h"
#include "../target.h"
#include "../../lib/lib.h"
#include "../../netlist/netlist.h"


//*************************************************************************************************************
//	@name		：　makeProbFile
//	@function	：	make the problem file
//	@return		：	(void)
//*************************************************************************************************************
void makeProbFile(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
)
{
	FILE* fileptr = (FILE*)NULL;


	/** open the "problem.opb" in write-mode */
	fileOpen(&fileptr, "./tools/scip/problem.opb", "w");


	/** write the minimize */
	makeProbFileMini(fileptr, minimize);

	/** write the constrant for good-circuti */
	makeProbFileConsGC(fileptr);


	/** write the constrant for faulty-circuti */
	makeProbFileConsFC(fileptr, target);


	/** close the "problem.opb" in write-mode */
	fclose(fileptr);

	return;
}

//*************************************************************************************************************
//	@name		：　makeProbFileMini
//	@function	：	make file -write the minimize
//	@return		：	(void)
//*************************************************************************************************************
void makeProbFileMini(
	FILE* fileptr,			  /**< pointer to file */
	char** minimize			  /**< minimize */
)
{
#ifdef NDEBUG
	assert(*minimize != NULL);
#endif

	if (strcmp(*minimize, "") != 0)
	{
		fprintf(fileptr, "max : %s;\n", *minimize);
	}
	else
	{
		fprintf(fileptr, "max : 1 x%d;\n", ++opb.total.vars);
	}

	return;
}
//*************************************************************************************************************
//	@name		：　makeProbFileConsGC
//	@function	：	make file -write the gc-constraint
//	@return		：	(void)
//*************************************************************************************************************
void makeProbFileConsGC(
	FILE* fileptr			  /**< pointer to file */
)
{
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].type != IN && nl[i].type != DFF)
		{
			fprintf(fileptr, "%s", nl[i].consgc);
		}
	}

	return;
}

//*************************************************************************************************************
//	@name		：　OPBmakefileConsFC
//	@function	：	make file -write the fc-constraint
//	@return		：	(void)
//*************************************************************************************************************
void makeProbFileConsFC(
	FILE* fileptr,			  /**< pointer to file */
	TARGET* target			  /**< target fault */
)
{
	for (int i = 0; i < target->num; i++)
	{
		for (int j = 0; j < n_net; j++)
		{
			if (nl[j].consfc[i] != NULL)
			{
				fprintf(fileptr, "%s", nl[j].consfc[i]);
			}
		}
	}

	return;
}










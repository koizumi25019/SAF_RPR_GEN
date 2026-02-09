//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "./opb.h"
#include "../read.h"
#include "../target.h"
#include "../../lib/lib.h"
#include "../../netlist/netlist.h"


//*************************************************************************************************************
//	@name		�F�@makeProbFile
//	@function	�F	make the problem file
//	@return		�F	(void)
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
//	@name		�F�@makeProbFileMini
//	@function	�F	make file -write the minimize
//	@return		�F	(void)
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
//	@name		�F�@makeProbFileConsGC
//	@function	�F	make file -write the gc-constraint
//	@return		�F	(void)
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
//	@name		�F�@OPBmakefileConsFC
//	@function	�F	make file -write the fc-constraint
//	@return		�F	(void)
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










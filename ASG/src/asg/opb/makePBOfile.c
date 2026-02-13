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
//	@name		�F�@makePBOFile
//	@function	�F	make the pbo file
//	@return		�F	(void)
//*************************************************************************************************************
void makePBOFile(
	TARGET* target			  /**< target fault */
)
{
	FILE* fileptr = (FILE*)NULL;


	/** open the "problem.opb" */
	fileOpen(&fileptr, "./pbo.txt", "w");

	/** write the constrant for good-circut */
	makeProbFileConsGC_clasp(fileptr);


	/** write the constrant for faulty-circut */
	makeProbFileConsFC_clasp(fileptr, target);


	/** close the "problem.opb" in write-mode */
	fclose(fileptr);

	return;
}

//*************************************************************************************************************
//	@name		�F�@makeProbFileConsGC_clasp
//	@function	�F	make file -write the gc-constraint
//	@return		�F	(void)
//*************************************************************************************************************
void makeProbFileConsGC_clasp(
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
//	@name		�F�@OPBmakefileConsFC_clasp
//	@function	�F	make file -write the fc-constraint
//	@return		�F	(void)
//*************************************************************************************************************
void makeProbFileConsFC_clasp(
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
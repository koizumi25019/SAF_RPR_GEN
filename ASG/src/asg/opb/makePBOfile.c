/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																			     */
/*	file		:	./src/atpg/opb/makePBOfile.c														     */
/*	deginer		:	T.Sone																			  		 */
/*	date		:	2023.06.21																  				 */
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
#include "./clasp/clasp.h"
#include "../read.h"
#include "../target.h"
#include "../../lib/lib.h"
#include "../../netlist/netlist.h"


//*************************************************************************************************************
//	@name		：　makePBOFile
//	@function	：	make the pbo file
//	@return		：	(void)
//*************************************************************************************************************
void makePBOFile(
	char** minimize,			  /**< minimize */
	TARGET* target			  /**< target fault */
)
{
	FILE* fileptr = (FILE*)NULL;


	/** open the "problem.opb" */
	fileOpen(&fileptr, "./tools/clasp/pbo.txt", "w");


	/** write the minimize */
	makeProbFileMini_clasp(fileptr, minimize);

	/** write the constrant for good-circut */
	makeProbFileConsGC_clasp(fileptr);


	/** write the constrant for faulty-circut */
	makeProbFileConsFC_clasp(fileptr, target);


	/** close the "problem.opb" in write-mode */
	fclose(fileptr);

	return;
}

//*************************************************************************************************************
//	@name		：　makeProbFileMini_clasp
//	@function	：	make file -write the minimize
//	@return		：	(void)
//*************************************************************************************************************
void makeProbFileMini_clasp(
	FILE* fileptr,			  /**< pointer to file */
	char** minimize			  /**< minimize */
)
{
	if (strcmp(*minimize, "") != 0)
	{
		fprintf(fileptr, "* #variable= %d ", ++opb.total.vars);
		fprintf(fileptr, "#constraint= %d\n", opb.total.cons);
		fprintf(fileptr, "*\n");
		fprintf(fileptr, "*  this is a dummy instance\n");
		fprintf(fileptr, "*\n");

		//---------------------------------------------
		//	PBOファイル書き込み（最適化関数）
		//---------------------------------------------

		fprintf(fileptr, "min: %s;\n", *minimize);
	}
	else
	{
		sprintf_s(*minimize, MAXSIZE_CONS, "1 x%d", ++opb.total.vars);
		fprintf(fileptr, "* #variable= %d ", ++opb.total.vars);
		fprintf(fileptr, "#constraint= %d\n", opb.total.cons);
		fprintf(fileptr, "*\n");
		fprintf(fileptr, "*  this is a dummy instance\n");
		fprintf(fileptr, "*\n");

		//---------------------------------------------
		//	PBOファイル書き込み（最適化関数）
		//---------------------------------------------

		fprintf(fileptr, "min: %s;\n", *minimize);
	}

	
	return;
}

//*************************************************************************************************************
//	@name		：　makeProbFileConsGC_clasp
//	@function	：	make file -write the gc-constraint
//	@return		：	(void)
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
//	@name		：　OPBmakefileConsFC_clasp
//	@function	：	make file -write the fc-constraint
//	@return		：	(void)
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
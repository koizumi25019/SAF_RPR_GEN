/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/asg.c																	         */
/*	deginer		:	R.miura			covered T.sone													  		 */
/*	date		:	2022.10.01		(2023.10.10)											  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>

#include "./createSGmodel.h"
#include "./asg.h"
#include "./init.h"
#include "./read.h"
#include "./fsim.h"
#include "./opb/opb.h"
#include "./opb/scip/scip.h"
#include "./opb/clasp/clasp.h"
#include "../standard.h"


//*************************************************************************************************************
//	@name		：　ASG
//	@function	：	automatic seed generation
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ASG(
	void
)
{
	TARGET  remain;
	TARGET	target;
	SORTED  sorted;
	FILE* fpseed = (FILE*)NULL;
	FILE* fptp = (FILE*)NULL;
	int loop = 0;
	int temp_numrema;

	/**  */
	fileOpen(&fpseed, opt.file.output.seed, "w");

	OUTPUT_TEST_PATTERN__ON fileOpen(&fptp, opt.file.output.test, "w");

	/** initialize the globl-variable */
	if (InitGlobalVars() != INIT_OKAY) return ASG_ERROR;


	/** read the file */
	if (ReadFile() != READ_OKAY) return ASG_ERROR;


	/** create the constraint for good-circuit */
	if (CreateConsGC() != TPG_MODEL_OKAY) return ASG_ERROR;


	/** create the lfsr model */
	if (CreateLFSRmodel() != LFSR_MODEL_OKAY) return ASG_ERROR;


	remain_log[0] = readdata.fault.numrema;
	while (readdata.fault.numrema != 0)
	{
		temp_numrema = readdata.fault.numrema;

		/** set the target fault set */
		SetTarget(&remain,&target,&sorted,loop++);


		/** read the file */
		if (CreateSGmodel(&target) != SG_MODEL_OKAY) return ASG_ERROR;


		/** start solver */
		//if (SCIP() != SCIP_OKAY) return ASG_ERROR;
		if (CLASP() != CLASP_OKAY) {
			PrintErrorMessage("\n	SYSTEM ERROR: solver error. ");
			PrintErrorMessage("Clasp status is unknown for the Automatic Seed Generator...\n");
			colorDef
			break;
		}

		/** output the solution */
		OutSolution(fpseed, fptp);


		/** fault simulation */
		if (FSIM(&target) != FSIM_OKAY) return ASG_ERROR;


		/** free the memory */
		FreeMemory(&remain,&target);

		remain_log[loop] = readdata.fault.numrema + readdata.fault.numred;
		detect_log[loop - 1] = temp_numrema - readdata.fault.numrema;

	}

	if (readdata.fault.numred != 0)
	{
		FreeRedundant();
	}

	num_seed = loop;

	fclose(fpseed);
	OUTPUT_TEST_PATTERN__ON fclose(fptp);

	return ASG_OKAY;
}

//*************************************************************************************************************
//	@name		：　OutSolution
//	@function	：	output the solution
//	@return		：	(bool) okay, error
//*************************************************************************************************************
void OutSolution(
	FILE* fpseed,			  /**< pointer to seed file */
	FILE* fptp			      /**< pointer to test pattern file */
)
{

	/** output the seed */
	fprintf(fpseed, "%s\n", clasp.sol[SOL_SEED]);

	/** output the test pattern */
	OUTPUT_TEST_PATTERN__ON
	{
		fprintf(fptp, "%s\n", clasp.sol[SOL_TP]);
	}
	
	/** for fault simulation */
	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, "./tools/fsim/test.txt", "w");
	fprintf(fileptr, "%s\n", clasp.sol[SOL_TP]);
	fclose(fileptr);
	

	return;
}

//*************************************************************************************************************
//	@name		：　FreeMemory
//	@function	：	free the memory
//	@return		：	(void)
//*************************************************************************************************************
void FreeMemory(
	TARGET* remain,			  /**< remain fault */
	TARGET* target			  /**< target fault */
)
{
	/** free the target fault lists */
	free(remain->list);
	free(target->list);
#ifdef NDEBUG
	assert(target->list == (TARGET**)NULL);
#endif

	/** free the faulty-circuit constraints  */
	for (int i = 0; i < n_net; i++)
	{
		for (int j = 0; j < target->num; j++)
		{
			if (nl[i].consfc[j] != NULL)
			{
				free(nl[i].consfc[j]);
#ifdef NDEBUG
				assert(nl[i].consfc[j] == (char*)NULL);
#endif

			}
		}

		free(nl[i].consfc);
#ifdef NDEBUG
		assert(nl[i].consfc == (char**)NULL);
#endif
	}

	return;
}

//*************************************************************************************************************
//	@name		：　FreeRedundant
//	@function	：	free the Redundant fault list
//	@return		：	(void)
//*************************************************************************************************************
void FreeRedundant(
	void
)
{
	int		numfault = 0;
	FNODE* tmp = (FNODE*)NULL;
	FILE* fpuntest = (FILE*)NULL;

	OUTPUT_UNTEST_PATTERN__ON	fileOpen(&fpuntest, opt.file.output.untestable, "w");

	/** free the Redundant fault list */
	PrintErrorMessage("\n	SYSTEM ERROR: fault simulation error. ");
	PrintErrorMessage("Redeundant fault for the Automatic Seed Generator...\n");

	for (int i = 0; i < MAXSIZE_HASH; i++)
	{
		tmp = readdata.fault.list[i];

		while (tmp != NULL)
		{
			if (tmp->detect == REDEUNDANT)
			{
				/** printf redundant-faults name */
				PrintErrorMessage("\t[ %d ] %s", numfault++, tmp->string);
				OUTPUT_UNTEST_PATTERN__ON
				{
					fprintf(fpuntest,"%s",tmp->string);
				}
			}
			if (numfault == readdata.fault.numred) break;
			tmp = tmp->nextptr;
		}

		if (numfault == readdata.fault.numred) break;
	}

	colorDef
	return;
}









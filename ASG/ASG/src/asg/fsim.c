/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/fsim.c																		 */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.09.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

#include "./asg.h"
#include "./read.h"
#include "./fsim.h"
#include "./target.h"
#include "./opb/clasp/clasp.h"
#include "../opt/opt.h"
#include "../lib/lib.h"
#include "../standard.h"


//*************************************************************************************************************
//	@name		：　FSIM
//	@function	：	fault simulation
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool FSIM(
	TARGET* target		  /**< target fault */
)
{
	if (clasp.status != CLASP_UNSAT)
	{
		/** call the fault-simulation */
		_CALL_FAULT_SIMULATION_SAF_(opt.file.input.net, opt.file.output.pin);


		/** drop the detected fault */
		if (DropDeteFault(target) != FSIM_OKAY) return FSIM_ERROR;


		/** check for target fault */
		//if (CheckTarget(target) != FSIM_OKAY) return FSIM_ERROR;
	}
	else
	{
		target->list[0]->detect = REDEUNDANT;
		readdata.fault.numrema--;
		readdata.fault.numred++;
		bitintSetNbit_Zero(detflag, target->list[0]->id);
	}

	PrintMessage("	Fault simulation completed ... \n");

	return	FSIM_OKAY;
}

//*************************************************************************************************************
//	@name		：　DropDeteFault
//	@function	：	drop the detected fault
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool DropDeteFault(
	TARGET* target
)
{
	/* variable declaration */
	FILE* fileptr = (FILE*)NULL;
	char* buffer = (char*)NULL;
	FNODE* tmp = (FNODE*)NULL;
	int num_detect = 0;

	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** open the "detected fault file" in read-mode */
	fileOpen(&fileptr, FSIM_DET_FILE, "r");

	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		/* calulate hash */
		int hash = calcHash(buffer);
		tmp = readdata.fault.list[calcHash(buffer)];

		/* search fault and update detection infomation */
		while (tmp != (FNODE*)NULL)
		{
			if (!strcmp(tmp->string, buffer))
			{
				if (tmp->detect == UNDETECTED)
				{
					tmp->detect = DETECTED;
					num_detect++;
					readdata.fault.numdete++;
					readdata.fault.numrema--;
					bitintSetNbit_Zero(detflag, tmp->id);
				}
				else if (tmp->detect == REDEUNDANT)
				{
					tmp->detect = DETECTED;
					num_detect++;
					readdata.fault.numdete++;
					readdata.fault.numrema--;
					readdata.fault.numred--;
				}
				break;
			}
			tmp = tmp->nextptr;
		}
	}

	if (num_detect == 0)
	{
		if (CheckTarget(target) != FSIM_OKAY) return FSIM_ERROR;
	}

	free(buffer);

	/** close the "detected fault file" in read-mode */
	fclose(fileptr);

	assert(!remove(FSIM_DET_FILE));

	return FSIM_OKAY;
}

//*************************************************************************************************************
//	@name		：　CheckTarget
//	@function	：	check for target fault
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CheckTarget(
	TARGET* target			  /**< target fault */
)
{
	int num_red = 0;
	for (int i = 0; i < target->num; i++)
	{
		if (target->list[i]->detect == REDEUNDANT)
		{
			num_red++;
		}
	}
	if (num_red == target->num)
	{
		PrintErrorMessage("\n	SYSTEM ERROR: fault simulation error. ");
		PrintErrorMessage("Redeundant fault for the Automatic Seed Generator...\n");
		for (int i = 0;i < target->num;i++)
		{
			PrintErrorMessage("\t[ %d ] %s", i, target->list[i]->string);
		}
		PrintErrorMessage("\n\tnum of REDEUNDANT fault -> %d\n",target->num);
		colorDef
		return	FSIM_ERROR;
	}
	return	FSIM_OKAY;
}










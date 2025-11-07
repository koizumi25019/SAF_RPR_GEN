//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "./target.h"
#include "./read.h"
#include "../lib/lib.h"
#include "../standard.h"


//*************************************************************************************************************
//	@name		：　SetTarget
//	@function	：	set the target-fault list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool SetTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target,			  /**< target-fault list */
	int loop				  /**< number of loop */
)
{
	int		numfault = 0;
	//int		relax_flag = 0;
	FNODE* tmp = (FNODE*)NULL;

	/** set the number of target-faults */
	remain->num = readdata.fault.numrema;
	remain->list = (FNODE**)allocMemory(remain->num, sizeof(FNODE*));

	/** set the target-fault */
	for (int i = 0; i < MAXSIZE_HASH; i++)
	{
		tmp = readdata.fault.list[i];

		while (tmp != NULL)
		{
			if (tmp->detect == UNDETECTED)
			{
				/** add the fault in target-fault list */
				remain->list[numfault] = tmp;
				remain->list[numfault]->relax = true;
				if (loop == 0)
				{
					tmp->id = numfault;
				}
				numfault++;

				/** if target number of fault is reached,.break */
				if (numfault == remain->num) break;
			}

			tmp = tmp->nextptr;
		}

		if (numfault == remain->num) break;
	}

	if (DirectInputTarget(remain, target) != TARGET_OKAY) return TARGET_ERROR;

	return TARGET_OKAY;
}

//*************************************************************************************************************
//	@name		：　DirectInputTarget
//	@function	：	set the direct target
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool DirectInputTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
)
{
	target->num = 1;

	target->list = (FNODE**)allocMemory(target->num, sizeof(FNODE*));

	for (int i = 0;i < target->num;i++)
	{
		target->list[i] = remain->list[i];
	}
	target->list[0]->relax = false;
	return TARGET_OKAY;
}
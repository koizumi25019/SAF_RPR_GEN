//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "./fault_detection_prob.h"
#include "./read.h"
#include "./target_fault.h"
#include "./cnf/cnf.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		F@DropDeteFault
//	@function	F	drop the detected fault
//	@return		F	(bool) okay, error
//*************************************************************************************************************
bool DropDeteFault(
	TARGET* target
)
{
	/* variable declaration */
	FNODE* tmp = (FNODE*)NULL;
	char fault_buffer[200];


	if (target->list[0]->type == SF0) {
		sprintf(fault_buffer, "%s\t%s\n", target->list[0]->name,"sa0");
	}
	else {
		sprintf(fault_buffer, "%s\t%s\n", target->list[0]->name,"sa1");
	}

		/* calulate hash */
		int hash = calcHash(fault_buffer);
		tmp = readdata.fault.list[hash];

		/* search fault and update detection infomation */
		while (tmp != (FNODE*)NULL)
		{
			if (!strcmp(tmp->string, fault_buffer))
			{
				if (tmp->detect == UNDETECTED)
				{
					tmp->detect = DETECTED;
					readdata.fault.numdete++;
					readdata.fault.numrema--;
				}
				else if (tmp->detect == REDEUNDANT)
				{
					tmp->detect = DETECTED;
					readdata.fault.numdete++;
					readdata.fault.numrema--;
					readdata.fault.numred--;
				}
				break;
			}
			tmp = tmp->nextptr;
		}

	return true;
}
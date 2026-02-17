//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "./asg.h"
#include "./read.h"
#include "./target.h"
#include "../opt/opt.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		�F�@DropDeteFault
//	@function	�F	drop the detected fault
//	@return		�F	(bool) okay, error
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
	char fault_buffer[200];

	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** open the "detected fault file" in read-mode */
	fileOpen(&fileptr, "./xid_fault.txt", "r");

	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
	if (target->list[0]->type == SF0) {
		sprintf(fault_buffer, "%s\t%s\n", target->list[0]->name, "sa0");
	}
	else {
		sprintf(fault_buffer, "%s\t%s\n", target->list[0]->name, "sa1");
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
					num_detect++;
					readdata.fault.numdete++;
					readdata.fault.numrema--;
				}
				else if (tmp->detect == REDEUNDANT)
				{
					tmp->detect = DETECTED;
					num_detect++;
					readdata.fault.numdete++;
					readdata.fault.numrema--;
					readdata.fault.numred--;
				}
				else
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

	/** close the "detected fault file" in read-mode */
	fclose(fileptr);
	free(buffer);

	return true;
}
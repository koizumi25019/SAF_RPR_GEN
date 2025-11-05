//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "./read.h"
#include "../standard.h"
#include "../lib/lib.h"
#include "../netlist/netlist.h"


//*************************************************************************************************************
//	@name		：　ReadFile
//	@function	：	read the file
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ReadFile(
	void
)
{
	/** read the fault */
	if (ReadFault() != READ_OKAY) return READ_ERROR;

	PrintMessage("	Reading the files completed ... \n");

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：　ReadFault
//	@function	：	read the fault
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ReadFault(
	void
)
{
	READER_FAULT_ON
	{
		FILE * fileptr = (FILE*)NULL;
		char* buffer = (char*)NULL;

		/** open the "fault file" in read-mode */
		fileOpen(&fileptr, opt.file.input.fault, "r");

		buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

		/** create the fault list */
		while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
		{
			if (COMP_NEWLINE(buffer))
			{
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}
			PrintMessage("\r	Reading fault infomation progress  >> %d", readdata.fault.numinit);
		}
		PrintMessage("\n");
		free(buffer);

		/** close the "fault file" in read-mode */
		fclose(fileptr);

		/** create the detection infomation list */
		detflag = (BIT_INT*)allocMemory(1, sizeof(BIT_INT));
		detflag->int_num = readdata.fault.numinit / MAXSIZE_BITINT + 1;
		detflag->flag = (unsigned int*)allocMemory(detflag->int_num, sizeof(unsigned int));
		bitintSetAll_One(detflag);
	}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：　CreateFaultList
//	@function	：	create the fault list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CreateFaultList(
	char* buffer			  /**< buffer */
)
{
	int			hash = 0;
	FNODE* fnodeptr = (FNODE*)NULL;

	/** calcurate the hash */
	hash = calcHash(buffer);

	/** create the fault node */
	if (searchFnode(buffer, readdata.fault.list[hash]) == NOT_FOUND)
	{
		if ((fnodeptr = CreateFaultNode(buffer)) != NULL)
		{
			fnodeptr->nextptr = readdata.fault.list[hash];
			readdata.fault.list[hash] = fnodeptr;

			readdata.fault.numinit++;
			readdata.fault.numrema++;
		}
		else
		{
			return READ_ERROR;
		}
	}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：　searchFnode
//	@function	：	search for fault node
//	@return		：	(bool) found, not found
//*************************************************************************************************************
bool searchFnode(
	char* buffer,			  /**< buffer (key) */
	FNODE* tmp				  /**< pointer to hash-fault list */
)
{
	while (tmp != NULL)
	{
		if (!strcmp(tmp->string, buffer))
		{
			return	FOUND;
		}
		tmp = tmp->nextptr;
	}

	return NOT_FOUND;
}

//*************************************************************************************************************
//	@name		：　CreateFaultNode
//	@function	：	create the fault node
//	@return		：	(FNODE*) pointer to fault node
//*************************************************************************************************************
FNODE* CreateFaultNode(
	char* buffer			  /**< buffer */
)
{
	char* context = (char*)NULL;
	FNODE* fnodeptr = (FNODE*)NULL;

	fnodeptr = (FNODE*)allocMemory(1, sizeof(FNODE));

	/** set the string */
	fnodeptr->string = _strdup(buffer);

	/** set the name */
	fnodeptr->name = _strdup(strtok_s(buffer, " \t\n", &context));

	/** set the type */
	fnodeptr_type___setFaultType(fnodeptr->type);

	/** set the detect */
	fnodeptr->detect = UNDETECTED;

	/** set the pointer to netlist */
	fnodeptr_netptr___setNetPtr(fnodeptr->netptr, fnodeptr->name);

	/** set the relaxation variables */
	fnodeptr->relax = false;

	/** set the pointer to next node */
	fnodeptr->nextptr = (FNODE*)NULL;

	/** set the id */
	fnodeptr->id = -1;

	return fnodeptr;
}

//*************************************************************************************************************
//	@name		：　COMPinit
//	@function	：	initialize the compatible sets infomation
//	@return		：	(void)
//*************************************************************************************************************
void COMPinti(
	TARGET* remain			  /**< target-fault list */
)
{
	/* variable initializion */
	for (int i = 0; i < readdata.fault.numrema; i++)
	{
		remain->list[i]->necenet = (BIT_INT_XP*)allocMemory(1, sizeof(BIT_INT_XP));
		remain->list[i]->necenet->int_num = (n_net - 1) / MAXSIZE_BITINT + 1;
		remain->list[i]->necenet->x_buf = (unsigned int*)allocMemory(
			remain->list[i]->necenet->int_num, sizeof(unsigned int));
		remain->list[i]->necenet->p_buf = (unsigned int*)allocMemory(
			remain->list[i]->necenet->int_num, sizeof(unsigned int));
		All_Bit_X_XP(remain->list[i]->necenet);
		remain->list[i]->nece = (char*)allocMemory(MAXSIZE_BUFFER,sizeof(char));
		remain->list[i]->edge = (BIT_INT*)allocMemory(1, sizeof(BIT_INT));
		remain->list[i]->edge->int_num = (remain->num - 1) / MAXSIZE_BITINT + 1;
		remain->list[i]->edge->flag= (unsigned int*)allocMemory(
			remain->list[i]->edge->int_num, sizeof(unsigned int));
		bitintSetAll_Zero(remain->list[i]->edge);
		for (int j = 0;j < remain->num;j++) 
		{
			bitintSetNbit_One(remain->list[i]->edge, j);
		}
		remain->list[i]->n_edge = remain->num - 1;
	}

	return;
}
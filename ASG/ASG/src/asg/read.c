/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/read.c																		 */
/*	deginer		:	R.miura			covered T.sone													  		 */
/*	date		:	2022.10.01		(2023.10.10)											  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


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
#include "../debug/debug.h"


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

	/** read the xor-tap */
	if (CreateLFSRList() != READ_OKAY) return READ_ERROR;

	/** read the xor-tap */
	if (ReadXORtap() != READ_OKAY) return READ_ERROR;

	/** read the scan-chain */
	if (ReadScanChain() != READ_OKAY) return READ_ERROR;

	/** read the necessary */
	if (ReadNecessary() != READ_OKAY) return READ_ERROR;

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

#ifdef __DEBUG_READ_FAULT__
		_CALL_DEBUG_READ_FAULT_;
#endif // __DEBUG_READ_FAULT__
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
//	@name		：　ReadXORtap
//	@function	：	read the xor-tap
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ReadXORtap(
	void
)
{
	READER_XOR_TAP_ON
	{
		FILE * fileptr = (FILE*)NULL;
		char* buffer = (char*)NULL;
		char* context = (char*)NULL;
		int     index = 0;

		/** open the "xor-tap file" in read-mode */
		fileOpen(&fileptr, opt.file.input.xortap, "r");

		buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

		/** get the number of bits */
		readdata.psnet.num = getNumPhaseShifter(fileptr, buffer);

		/** alloc the memory for phase-shifter netlist */
		readdata.psnet.nlist = (PS_NLIST*)allocMemory(readdata.psnet.num, sizeof(PS_NLIST));

		/** read the xor-tap list */
		for (int i = 0; COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)); i++)
		{
			if (COMP_NEWLINE(buffer))
			{
				/** get the index of phase-shifter */
				index = getPhaseShifterIndex(strtok_s(buffer, ":", &context));

				/** create the xor-tap list */
				if (createXORtapList(index, strtok_s(NULL, "\n\0", &context)) != READ_OKAY) return READ_ERROR;
			}
		}

		free(buffer);

		/** close the "xor-tap file" in read-mode */
		fclose(fileptr);


#ifdef __DEBUG_READ_XOR_TAP__
		_CALL_DEBUG_READ_XOR_TAP_
#endif  // __DEBUG_READ_XOR_TAP__
	}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：　getNumPhaseShifter
//	@function	：	get the number of phase-shifters
//	@return		：	(int) number of phase-shifters
//*************************************************************************************************************
int getNumPhaseShifter(
	FILE* fileptr,		      /**< pointer to file */
	char* buffer		      /**< buffer */
)
{
	int     numps = 0;

	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (COMP_NEWLINE(buffer))
		{
			numps++;
		}
	}
	rewind(fileptr);

	return numps;
}

//*************************************************************************************************************
//	@name		：  getPhaseShifterIndex
//	@function	：	get the index of pahase-shifter
//	@return		：	(int) index of pahase-shifter
//*************************************************************************************************************
int getPhaseShifterIndex(
	char* buffer		      /**< buffer */
)
{
	int		index = -1;

	for (int i = 0; i < strlen(buffer); i++)
	{
		if (isSingleByte(buffer[i]) == true)
		{
			if (isdigit(buffer[i]) != false)
			{
				index = atoi(&buffer[i]);
				break;
			}
		}
	}

#ifdef NDEBUG
	assert(index != -1);
#endif

	return index;
}

//*************************************************************************************************************
//	@name		：  createXORtapList
//	@function	：	create the xor-tap list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool createXORtapList(
	int 				  index,		      /**< index */
	char* buffer		      /**< buffer */
)
{
	char* context = (char*)NULL;
	int		k = 0;

	/** initialize */
	readdata.psnet.nlist[index].id = index + 1;								/** id */
	readdata.psnet.nlist[index].vars = index + 1;								/** variable */
	readdata.psnet.nlist[index].nin = 0;										/** number of inputs */
	readdata.psnet.nlist[index].nout_slength = 0;										/** number of outputs */
	readdata.psnet.nlist[index].name = (char*)allocMemory(30, sizeof(char));		/** name */
	sprintf_s(readdata.psnet.nlist[index].name, 30, "ps_out_%d", readdata.psnet.nlist[index].id);

	/** get the number of xor-taps */
	for (int i = 0; i < strlen(buffer); i++)
	{
		if (buffer[i] == '_')
		{
			readdata.psnet.nlist[index].nin++;

			/** error */
			//if (readdata.psnet.num < readdata.psnet.nlist[index].nin)
			//{
			//	PrintErrorMessage("\n	FILE ERROR: xor-tap file reading failed. ");
			//	PrintErrorMessage("the number of xor-taps is over. \n");

			//	return READ_ERROR;
			//}

		}
	}
	readdata.psnet.nlist[index].in = (int*)allocMemory(readdata.psnet.nlist[index].nin, sizeof(int));

	/** store the xor-tap number */
	readdata.psnet.nlist[index].in[k++] = atoi(strtok_s(buffer, "_\n\0", &context));
	while ((buffer = strtok_s(NULL, "_\n\0", &context)) != NULL)
	{
		readdata.psnet.nlist[index].in[k++] = atoi(buffer);
	}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：　ReadScanChain
//	@function	：	read the scan-chain
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ReadScanChain(
	void
)
{
	READER_SCAN_CHAIN_ON
	{
		FILE * fileptr = (FILE*)NULL;
		char* buffer = (char*)NULL;
		char* context = (char*)NULL;
		char* token = (char*)NULL;
		int     index = 0;
		int     j = 0;

		/** open the "scan-chain file" in read-mode */
		fileOpen(&fileptr, opt.file.input.scanchain, "r");

		buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

		/** get the maximum scan-length */
		readdata.psnet.maxslength = getMaxScanLength(fileptr, buffer);

		/** alloc the memory for output of phase-shifter */
		for (int i = 0; i < readdata.psnet.num; i++)
		{
			if (readdata.psnet.nlist[i].nout_slength >= 1)
				readdata.psnet.nlist[i].out_schain = (NLIST**)allocMemory(readdata.psnet.nlist[i].nout_slength, sizeof(NLIST*));
		}

		/** read the scan-chain */
		for (int i = 0; i < readdata.psnet.maxslength; i++)
		{
			index = 0;
			j = 0;

			fgets(buffer, MAXSIZE_BUFFER, fileptr);

			if (COMP_NEWLINE(buffer))
			{
				while (j < strlen(buffer))
				{
					if (buffer[j] != ' ' && buffer[j] != ',' && buffer[j] != '\n' && buffer[j] != '\0')
					{
						token = strtok_s(&buffer[j], ", \n\0", &context);

						/** store the pointer to netlist */
						for (int k = 0; k < n_net; k++)
						{
							if (!strcmp(token, nl[k].name))
							{
								readdata.psnet.nlist[index++].out_schain[i] = &nl[k];
								break;
							}
						}

						/** error */
						if (readdata.psnet.nlist[index - 1].out_schain[i] == NULL)
						{
							PrintErrorMessage("\n	FILE ERROR: scan-chain file reading failed. ");
							PrintErrorMessage("%s was passed through. \n", token);
							colorDef
							return READ_ERROR;
						}

						if ((token = strtok_s(NULL, "\n\0", &context)) == NULL) break;
						strcpy_s(buffer, MAXSIZE_BUFFER, token);
						j = 0;
					}
					else
					{
						j++;
					}
				}
			}
			else
			{
				i--;
			}
		}

		free(buffer);

		/** close the "scan-chain file" in read-mode */
		fclose(fileptr);


#ifdef __DEBUG_READ_SCAN_CHAIN__
		_CALL_DEBUG_READ_SCAN_CHAIN_
#endif  // __DEBUG_READ_SCAN_CHAIN__
	}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：　getMaxScanLength
//	@function	：	get the maximum scan-length
//	@return		：	(int) maximum scan-length
//*************************************************************************************************************
int getMaxScanLength(
	FILE* fileptr,		      /**< pointer to file */
	char* buffer		      /**< buffer */
)
{
	int     index = 0;
	int     maxscanlength = 0;

	/** get the maximum scan-length */
	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (COMP_NEWLINE(buffer)) maxscanlength++;
	}
	rewind(fileptr);

	/** get the scan-length of each scan-chain */
	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (COMP_NEWLINE(buffer))
		{
			for (int i = 0; i < strlen(buffer); i++)
			{
				if (buffer[i] == ',') readdata.psnet.nlist[index++].nout_slength++;
			}
			readdata.psnet.nlist[index].nout_slength++;
		}
		index = 0;
	}

	rewind(fileptr);

	return maxscanlength;
}


/* add create source code since 2023/10/09 */
//*************************************************************************************************************
//	@name		：  CreateLFSRList
//	@function	：	Create the LFSR list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CreateLFSRList(
	void
)
{
	/* variable declaration */
	char* context = (char*)NULL;
	int		k = 0;
	int		i = 0;

	/* variable initializion */
	readdata.lfsrnet.num = LFSRBIT;
	readdata.lfsrnet.nlist = (LFSR_NLIST*)allocMemory(readdata.lfsrnet.num, sizeof(LFSR_NLIST));
	for (i = 0;i < readdata.lfsrnet.num;i++) 
	{
		readdata.lfsrnet.nlist[i].id = i + 1;
		readdata.lfsrnet.nlist[i].vars = i + 1;
		readdata.lfsrnet.nlist[i].name = (char*)allocMemory(200, sizeof(char));		/** name */
		sprintf_s(readdata.lfsrnet.nlist[i].name, 200, "lsfr_net_%d", readdata.lfsrnet.nlist[i].id);
	}
	/** error */
	if (readdata.lfsrnet.nlist[i-1].vars!=readdata.lfsrnet.num)
	{
		PrintErrorMessage("\n	FILE ERROR: LFSR reading failed. ");
		PrintErrorMessage("the number of lfsr vars failed. \n");

		return READ_ERROR;
	}
	
	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		：  CreateCompatibleInfo
//	@function	：	Create Compatible Infomation
//	@return		：	(bool) okay, error
//*************************************************************************************************************
void CreateCompatibleInfo(
	TARGET* remain			  /**< target-fault list */
)
{
	/* variable declaration */
	int j = 0;
	int type = 0;


	/** add necessary to structure  */

	for (int i = 0; i < remain->num; i++)
	{
		for (j = 0;j < readdata.necenet.num;j++) 
		{
			if (!strcmp(remain->list[i]->string, readdata.necenet.necelist[j].string))
			{
				sprintf_s(remain->list[i]->nece, MAXSIZE_BUFFER, "%s", readdata.necenet.necelist[j].nece);
				free(readdata.necenet.necelist[j].nece);
				break;
			}
		}
	}
	return;
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

//*************************************************************************************************************
//	@name		：  ReadNeessary
//	@function	：	Reading File Necessary
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ReadNecessary(
	void
)
{
	READER_NECESSARY_ON
	{
		/* variable declaration */
		FILE * fileptr = (FILE*)NULL;
		char* buffer = (char*)NULL;
		char* context = (char*)NULL;
		char* string = (char*)NULL;
		char* token = (char*)NULL;
		char* token2 = (char*)NULL;
		int cnt = 0;
		int f_length = 0;
		int hash = 0;
		int type = 0;
		int i = 0;

		/* partitioning allocation */
		buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));
		string = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

		readdata.necenet.necelist = (NECE_NLIST*)allocMemory(readdata.fault.numinit, sizeof(NECE_NLIST));
		
		/** open the "fault file" in read-mode */
		fileOpen(&fileptr, opt.file.input.necessary, "r");

		/** create the fault list */
		while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
		{
			if (COMP_NEWLINE(buffer))
			{
				token = strtok_s(buffer, ", \n\0", &context);
				sprintf_s(string, MAXSIZE_BUFFER, "%s",token);
				token2 = strtok_s(NULL, ", \n\0", &context);
				if (!strcmp(token2, "SF0"))
				{
					type = SF0;
					sprintf_s(string, MAXSIZE_BUFFER, "%s\tsa0\n", string);
				}
				else if (!strcmp(token2, "SF1"))
				{
					type = SF1;
					sprintf_s(string, MAXSIZE_BUFFER, "%s\tsa1\n", string);
				}
				/** calcurate the hash */
				hash = calcHash(string);
				/** create Necessary node */
				if (searchNnode(string,readdata.fault.list[hash],cnt) != NOT_FOUND) 
				{
					token2 = strtok_s(NULL, "\n\0", &context);
					readdata.necenet.necelist[cnt].nece = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));
					sprintf_s(readdata.necenet.necelist[cnt].nece, MAXSIZE_BUFFER, "%s", token2);
					cnt++;
				}
			}
		}
		if (cnt != readdata.fault.numinit) 
		{
			PrintErrorMessage("\n	SYSTEM ERROR: Reading necessary file error. ");
			PrintErrorMessage("	Not enough necessary information. ");
			PrintErrorMessage("\n	num of necessary info = %d", cnt);
			PrintErrorMessage("		num of target fault   = %d\n", readdata.fault.numinit);

			return READ_ERROR;
		}
		else{ readdata.necenet.num = cnt; }
		fclose(fileptr);
	}

	return READ_OKAY;

}

//*************************************************************************************************************
//	@name		：　searchNnode
//	@function	：	search for necessary node
//	@return		：	(bool) found, not found
//*************************************************************************************************************
bool searchNnode(
	char* buffer,			  /**< buffer (key) */
	FNODE* tmp,				  /**< pointer to hash-fault list */
	int no
)
{
	while (tmp != NULL)
	{
		if (!strcmp(tmp->string, buffer))
		{
			readdata.necenet.necelist[no].string = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));
			sprintf_s(readdata.necenet.necelist[no].string, 200, "%s", tmp->string);
			readdata.necenet.necelist[no].id = no;
			readdata.necenet.necelist[no].type = tmp->type;
			readdata.necenet.necelist[no].name = (char*)allocMemory(200, sizeof(char));
			sprintf_s(readdata.necenet.necelist[no].name, 200, "%s", tmp->name);
			return	FOUND;
		}
		tmp = tmp->nextptr;
	}

	return NOT_FOUND;
}
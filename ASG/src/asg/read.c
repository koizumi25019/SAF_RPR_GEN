//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "./read.h"
#include "../lib/lib.h"
#include "../netlist/netlist.h"

//bool TestRelationCounts();

//*************************************************************************************************************
//	@name		�F�@ReadFile
//	@function	�F	read the file
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool ReadFile(
	void
)
{
	/** read the fault */
	if (ReadFault() != READ_OKAY) return READ_ERROR;

	//if (TestRelationCounts() != READ_OKAY)
	//{
	//	fprintf(stderr, "ERROR: Failed to update test relation counts.\n");
	//	return READ_ERROR;
	//}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@ReadFault
//	@function	�F	read the fault
//	@return		�F	(bool) okay, error
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
			printf("\r	Reading fault infomation progress  >> %d", readdata.fault.numinit);
		}
		printf("\n");
		free(buffer);

		/** close the "fault file" in read-mode */
		fclose(fileptr);
	}

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@CreateFaultList
//	@function	�F	create the fault list
//	@return		�F	(bool) okay, error
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
//	@name		�F�@searchFnode
//	@function	�F	search for fault node
//	@return		�F	(bool) found, not found
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
//	@name		�F�@searchFnodePtr
//	@function	�F	find the fault node pointer by string
//	@return		�F	(FNODE*) pointer to found node, or NULL
//*************************************************************************************************************
FNODE* searchFnodePtr(
	char* buffer,
	FNODE* head	
)
{
	FNODE* fnodeptr = head;
	while (fnodeptr != NULL)
	{
		// CreateFaultNode �ŃZ�b�g���� string (���S�� "name type" ������) �Ɣ�r
		if (strcmp(fnodeptr->string, buffer) == 0)
		{
			return fnodeptr;
		}
		fnodeptr = fnodeptr->nextptr;
	}
	return (FNODE*)NULL;
}

//*************************************************************************************************************
//	@name		�F�@CreateFaultNode
//	@function	�F	create the fault node
//	@return		�F	(FNODE*) pointer to fault node
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

	//test relation num initialize
	fnodeptr->test_relation_num = 0;

	/** set the pointer to next node */
	fnodeptr->nextptr = (FNODE*)NULL;

	/** set the id */
	fnodeptr->id = -1;

	return fnodeptr;
}


//*************************************************************************************************************
//	@name		�FTestRelationCounts
//	@function	�F	read test relation file and update existing fault list
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool TestRelationCounts(
)
{
	FILE* fp = NULL;
	char  line_buffer[256];
	char  fault_name[128];
	char  fault_type[32];
	int   relation_count = 0;
	char  hash_buffer[256];
	int   hash = 0;
	FNODE* fnodeptr = (FNODE*)NULL;


	//�e�X�g�֌WPI�t�@�C���I�[�v��
	fileOpen(&fp, opt.file.input.relation, "r");

	while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL)
	{

		if (sscanf(line_buffer, "%s %s %d", fault_name, fault_type, &relation_count) == 3)
		{
			// �����p�̕����񐶐�
			snprintf(hash_buffer, sizeof(hash_buffer), "%s\t%s\n", fault_name, fault_type);

			//CreateFaultList �Ɠ����n�b�V���֐����Ăяo��
			hash = calcHash(hash_buffer);

			//�̏჊�X�g�T��
			fnodeptr = searchFnodePtr(hash_buffer, readdata.fault.list[hash]);
			if (fnodeptr == NULL) {
				printf("fault not found\n");
				exit(1);
			}

			// �e�X�g�֌W�����X�V
			fnodeptr->test_relation_num = relation_count;

		}
	}

	fclose(fp);
	return READ_OKAY;
}
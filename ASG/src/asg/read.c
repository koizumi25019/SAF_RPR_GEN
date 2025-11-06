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

bool TestRelationCounts(char* filename);

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

	char* tmp = "s5378_test_relation.txt";
	if (TestRelationCounts(tmp) != READ_OKAY)
	{
		fprintf(stderr, "ERROR: Failed to update test relation counts.\n");
		return READ_ERROR;
	}

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
//	@name		：　searchFnodePtr
//	@function	：	find the fault node pointer by string
//	@return		：	(FNODE*) pointer to found node, or NULL
//*************************************************************************************************************
FNODE* searchFnodePtr(
	char* buffer,
	FNODE* head	
)
{
	FNODE* fnodeptr = head;
	while (fnodeptr != NULL)
	{
		// CreateFaultNode でセットした string (完全な "name type" 文字列) と比較
		if (strcmp(fnodeptr->string, buffer) == 0)
		{
			return fnodeptr;
		}
		fnodeptr = fnodeptr->nextptr;
	}
	return (FNODE*)NULL;
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

	//test relation num initialize
	fnodeptr->test_relation_num = 0;

	/** set the pointer to next node */
	fnodeptr->nextptr = (FNODE*)NULL;

	/** set the id */
	fnodeptr->id = -1;

	return fnodeptr;
}


//*************************************************************************************************************
//	@name		：TestRelationCounts
//	@function	：	read test relation file and update existing fault list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool TestRelationCounts(
	char* filename
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

	if ((fp = fopen(filename, "r")) == NULL) {
		perror("ERROR: Cannot open test relation file");
		return READ_ERROR;
	}

	while (fgets(line_buffer, sizeof(line_buffer), fp) != NULL)
	{

		if (sscanf(line_buffer, "%s %s %d", fault_name, fault_type, &relation_count) == 3)
		{
			// 検索用の文字列を作成
			snprintf(hash_buffer, sizeof(hash_buffer), "%s\t%s\n", fault_name, fault_type);

			// 既存の CreateFaultList と同じハッシュ関数を呼び出す
			hash = calcHash(hash_buffer);

			// 既存のノードを検索 (新しく追加した searchFnodePtr を使用)
			fnodeptr = searchFnodePtr(hash_buffer, readdata.fault.list[hash]);
			if (fnodeptr == NULL) {
				printf("error\n");
				exit(1);
			}

			// テスト関係数を更新
			fnodeptr->test_relation_num = relation_count;

		}
	}

	fclose(fp);
	return READ_OKAY;
}
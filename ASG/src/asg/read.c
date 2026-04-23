//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "./read.h"
#include "../lib/lib.h"
#include "../netlist/netlist.h"
#include "../opt/opt.h"

//*************************************************************************************************************
//	@name		F@ReadFault
//	@function	F	read the fault
//	@return		F	(bool) okay, error
//*************************************************************************************************************
bool ReadFault(
	void
)
{
	if (opt.file.input.fault != FILE_NOSET)
	
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
	else 
	{
		//-------------------------------------------------------------------
		// 故障ファイルが指定されていない場合：全故障生成＆代表故障の抽出
		//-------------------------------------------------------------------
		int i;
		char buffer[MAXSIZE_BUFFER];

		readdata.fault.numinit = 0;
		readdata.fault.numrema = 0;

		// 等価故障のフラグ整理を実行
		AnalyzeEquivalenceFaults();

		// ネットリストを再度走査し、YESのフラグが残っているものだけ FNODE 化する
		for (i = 0; i < n_net; i++)
		{
			if (nl[i].test_sf0 == YES)
			{
				snprintf(buffer, sizeof(buffer), "%s\tsa0\n", nl[i].name);
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}

			if (nl[i].test_sf1 == YES)
			{
				snprintf(buffer, sizeof(buffer), "%s\tsa1\n", nl[i].name);
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}
		}

		printf("\r	Representative fault generation completed. Total faults: %d\n", readdata.fault.numinit);
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

	/** set the pointer to next node */
	fnodeptr->nextptr = (FNODE*)NULL;

	/** set the id */
	fnodeptr->id = -1;

	return fnodeptr;
}

///*************************************************************************************************************
//	@name		AnalyzeEquivalenceFaults
//	@function	ネットリスト全体を走査し、等価故障のテストフラグをNOにする
//*************************************************************************************************************
void AnalyzeEquivalenceFaults()
{
	int i, j;

	// 1. すべてのネットの故障をテスト対象(YES)として初期化
	for (i = 0; i < n_net; i++)
	{
		nl[i].test_sf0 = YES;
		nl[i].test_sf1 = YES;
	}

	// 2. ゲートのタイプに応じて等価故障を対象外(NO)にしていく
	/*for (i = 0; i < n_net; i++)
	{
		switch (nl[i].type)
		{
			case BUF:
			case INV:
				nl[i].in[0]->test_sf0 = NO;
				nl[i].in[0]->test_sf1 = NO;
				break;

			case AND:
			case NAND:
				// 入力信号線の0縮退故障は等価
				for (j = 0; j < nl[i].n_in; j++) {
					nl[i].in[j]->test_sf0 = NO;
				}
				break;

			case OR:
			case NOR:
				// 入力信号線の1縮退故障は等価
				for (j = 0; j < nl[i].n_in; j++) {
					nl[i].in[j]->test_sf1 = NO;
				}
				break;

			default:
				break;
		}
	}*/
}
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

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
			if (nl[i].test_sa0 == YES)
			{
				snprintf(buffer, sizeof(buffer), "%s\tsa0\n", nl[i].name);
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}

			if (nl[i].test_sa1 == YES)
			{
				snprintf(buffer, sizeof(buffer), "%s\tsa1\n", nl[i].name);
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}
		}

		printf("\r	Representative fault generation completed. Total faults: %d\n", readdata.fault.numinit);
	}

	struct timespec _adfs, _adfe;
	clock_gettime(CLOCK_MONOTONIC, &_adfs);
	AnalyzeDominanceFaults();
	clock_gettime(CLOCK_MONOTONIC, &_adfe);
	printf("AnalyzeDominanceFaults: %.3f sec\n",
		(_adfe.tv_sec - _adfs.tv_sec) + (_adfe.tv_nsec - _adfs.tv_nsec) / 1e9);

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		CreateFaultList
//	@function	create the fault list
//	@return		(bool) okay, error
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
//	@name		searchFnode
//	@function	search for fault node
//	@return		(bool) found, not found
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
//	@name		searchFnodePtr
//	@function	find the fault node pointer by string
//	@return		(FNODE*) pointer to found node, or NULL
//*************************************************************************************************************
FNODE* searchFnodePtr(
	char* buffer,
	FNODE* head	
)
{
	FNODE* fnodeptr = head;
	while (fnodeptr != NULL)
	{
		// CreateFaultNode でセットされた string (完全な "name type" 文字列) と比較
		if (strcmp(fnodeptr->string, buffer) == 0)
		{
			return fnodeptr;
		}
		fnodeptr = fnodeptr->nextptr;
	}
	return (FNODE*)NULL;
}

//*************************************************************************************************************
//	@name		ParseFaultType
//	@function	parse fault type token ("sa0"/"sa1") from strtok_r context and set type
//	@return		(bool) okay, error
//*************************************************************************************************************
static bool ParseFaultType(
	char** saveptr,			/**< strtok_r saveptr */
	int* type_out			/**< output: SF0 or SF1 */
)
{
	char* token = strtok_r(NULL, " \n\0", saveptr);
	if (token == NULL)
	{
		printf("\n\tFILE ERROR: fault file reading failed. ");
		printf("type of fault error.\n\n");
		return false;
	}
	if (!strcmp(token, "sa0"))
	{
		*type_out = SF0;
	}
	else if (!strcmp(token, "sa1"))
	{
		*type_out = SF1;
	}
	else
	{
		printf("\n\tFILE ERROR: fault file reading failed. ");
		printf("%c%s%c unexpected type of fault.\n\n", '"', token, '"');
		return false;
	}
	return true;
}

//*************************************************************************************************************
//	@name		FindNetByName
//	@function	search netlist for a net whose name matches the given string
//	@return		(NLIST*) pointer to matching net, or NULL if not found
//*************************************************************************************************************
static NLIST* FindNetByName(
	const char* name		/**< net name to search */
)
{
	for (int i = 0; i < n_net; i++)
	{
		if (!strcmp(nl[i].name, name))
		{
			return &nl[i];
		}
	}
	printf("\n\tFILE ERROR: fault file reading failed. ");
	printf("%c%s%c is thought.\n\n", '"', name, '"');
	return (NLIST*)NULL;
}

//*************************************************************************************************************
//	@name		CreateFaultNode
//	@function	create the fault node
//	@return		(FNODE*) pointer to fault node
//*************************************************************************************************************
FNODE* CreateFaultNode(
	char* buffer			  /**< buffer */
)
{
	char* context = (char*)NULL;
	FNODE* fnodeptr = (FNODE*)NULL;

	fnodeptr = (FNODE*)allocMemory(1, sizeof(FNODE));

	/** set the string */
	fnodeptr->string = strdup(buffer);

	/** set the name */
	fnodeptr->name = strdup(strtok_r(buffer, " \t\n", &context));

	/** set the type */
	if (!ParseFaultType(&context, &fnodeptr->type))
	{
		free(fnodeptr->string);
		free(fnodeptr->name);
		free(fnodeptr);
		return (FNODE*)NULL;
	}

	/** set the detect */
	fnodeptr->detect = UNDETECTED;

	/** set the pointer to netlist */
	fnodeptr->netptr = FindNetByName(fnodeptr->name);
	if (fnodeptr->netptr == (NLIST*)NULL)
	{
		free(fnodeptr->string);
		free(fnodeptr->name);
		free(fnodeptr);
		return (FNODE*)NULL;
	}

	/** set the pointer to next node */
	fnodeptr->nextptr = (FNODE*)NULL;

	fnodeptr->dominators    = (FNODE**)NULL;
	fnodeptr->n_dominators  = 0;
	fnodeptr->saved_cubes   = (char**)NULL;
	fnodeptr->n_saved_cubes = 0;

	return fnodeptr;
}

//*************************************************************************************************************
//	@name		FindFnodeByNameType
//	@function	find fault node by net name and fault type
//	@return		(FNODE*) pointer to fault node, or NULL if not found
//*************************************************************************************************************
static FNODE* FindFnodeByNameType(
	const char* name,
	int type
)
{
	char buf[MAXSIZE_BUFFER];
	snprintf(buf, sizeof(buf), "%s\t%s\n", name, (type == SF0) ? "sa0" : "sa1");
	int hash = calcHash(buf);
	return searchFnodePtr(buf, readdata.fault.list[hash]);
}

///*************************************************************************************************************
//	@name		AnalyzeEquivalenceFaults
//	@function	ネットリスト全体を走査し、等価故障のテストフラグをNOにする
//*************************************************************************************************************
void AnalyzeEquivalenceFaults()
{
	int i, j;

	// すべてのネットの故障をテスト対象(YES)として初期化
	for (i = 0; i < n_net; i++)
	{
		nl[i].test_sa0 = YES;
		nl[i].test_sa1 = YES;
	}

	// ゲートのタイプに応じて等価故障を対象外(NO)にしていく
	for (i = 0; i < n_net; i++)
	{
		switch (nl[i].type)
		{
			case BUF:
			case INV:
				nl[i].in[0]->test_sa0 = NO;
				nl[i].in[0]->test_sa1 = NO;
				break;

			case AND:
			case NAND:
				// 入力信号線の0縮退故障は等価
				for (j = 0; j < nl[i].n_in; j++) {
					nl[i].in[j]->test_sa0 = NO;
				}
				break;

			case OR:
			case NOR:
				// 入力信号線の1縮退故障は等価
				for (j = 0; j < nl[i].n_in; j++) {
					nl[i].in[j]->test_sa1 = NO;
				}
				break;

			default:
				break;
		}
	}
}

//*************************************************************************************************************
//	@name		AnalyzeDominanceFaults
//	@function	支配関係を解析し、各故障の dominators リストを構築する
//	@note		支配関係: AND/NOR の出力SA1 は各入力SA1 に支配される（入力SA1のテスト集合⊆出力SA1のテスト集合）
//	            OR/NAND  の出力SA0 は各入力SA0 に支配される
//*************************************************************************************************************
void AnalyzeDominanceFaults(void)
{
	int count = 0;

	for (int i = 0; i < n_net; i++)
	{
		int ftype;
		switch (nl[i].type)
		{
		case AND: case NOR:   ftype = SF1; break;
		case OR:  case NAND:  ftype = SF0; break;
		default: continue;
		}

		// ゲート出力の故障（支配故障）を検索
		FNODE* dominated = FindFnodeByNameType(nl[i].name, ftype);
		if (!dominated) continue;

		// 各入力の同タイプ故障（被支配故障）を dominators に追加
		for (int j = 0; j < nl[i].n_in; j++)
		{
			FNODE* dom = FindFnodeByNameType(nl[i].in[j]->name, ftype);
			if (!dom) continue;

			dominated->dominators = (FNODE**)realloc(dominated->dominators,
				(dominated->n_dominators + 1) * sizeof(FNODE*));
			dominated->dominators[dominated->n_dominators++] = dom;
			count++;
		}
	}

	printf("Dominance fault analysis: %d dominance pairs\n", count);
}
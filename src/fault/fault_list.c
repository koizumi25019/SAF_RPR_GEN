#define _POSIX_C_SOURCE 200809L
//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "fault/fault_list.h"
#include "common/memory.h"
#include "io/file.h"
#include "fault/hash.h"
#include "netlist/netlist.h"
#include "cli/options.h"

//*************************************************************************************************************
//	@name		F@ReadFault
//	@function	F	故障を読み込む
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool ReadFault(
	void
)
{
	if (opt.file.input.fault != FILE_NOSET)

	{
		FILE * fileptr = (FILE*)NULL;
		char* buffer = (char*)NULL;

		/** 「故障ファイル」を読み込みモードで開く */
		fileOpen(&fileptr, opt.file.input.fault, "r");

		buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

		/** 故障リストを作成する */
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

		/** 「故障ファイル」を閉じる */
		fclose(fileptr);
	}
	else if (opt.fault_model == FM_TDF)
	{
		//-------------------------------------------------------------------
		// TDF 故障ファイルなし：全代表故障の自動生成
		// 故障サイトは2時刻展開前の全信号線（= peer_1t を持つ 2時刻目コピーと PI）
		//-------------------------------------------------------------------
		int i;
		char buffer[MAXSIZE_BUFFER];

		readdata.fault.numinit = 0;
		readdata.fault.numrema = 0;

		// 等価故障のフラグ整理を実行
		AnalyzeEquivalenceFaultsTDF();

		for (i = 0; i < n_net; i++)
		{
			if (nl[i].peer_1t == (NLIST*)NULL) continue;   // 1時刻目コピーは故障サイトでない

			if (nl[i].test_sa0 == YES)   // STR
			{
				snprintf(buffer, sizeof(buffer), "%s\tSTR\n", nl[i].name);
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}

			if (nl[i].test_sa1 == YES)   // STF
			{
				snprintf(buffer, sizeof(buffer), "%s\tSTF\n", nl[i].name);
				if (CreateFaultList(buffer) != READ_OKAY) return READ_ERROR;
			}
		}

		printf("\r	Representative fault generation completed (TDF). Total faults: %d\n", readdata.fault.numinit);
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

	return READ_OKAY;
}

//*************************************************************************************************************
//	@name		CreateFaultList
//	@function	故障リストを作成する
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool CreateFaultList(
	char* buffer			  /**< バッファ */
)
{
	int			hash = 0;
	FNODE* fnodeptr = (FNODE*)NULL;

	/* 空白・タブ区切りを正規化し、同じ故障を重複登録しない。 */
	{
		static char name[MAXSIZE_BUFFER];
		char type[8];
		if (sscanf(buffer, "%499990s %7s", name, type) == 2)
			snprintf(buffer, MAXSIZE_BUFFER, "%s\t%s\n", name, type);
	}

	/** ハッシュ値を計算する */
	hash = calcHash(buffer);

	/** 故障ノードを作成する */
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
//	@function	故障ノードを探索する
//	@return		(bool) found, not found
//*************************************************************************************************************
bool searchFnode(
	char* buffer,			  /**< バッファ（キー） */
	FNODE* tmp				  /**< ハッシュ故障リストへのポインタ */
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
//	@name		ParseFaultType
//	@function	故障タイプ（SAF: sa0/sa1、TDF: STR/STF）を解析しtypeに設定する
//	@return		(bool) 正常, 異常
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
	if (opt.fault_model == FM_TDF)
	{
		if      (!strcmp(token, "STR")) *type_out = SF0;   /* slow-to-rise: t1=0, t2でsa0挙動 */
		else if (!strcmp(token, "STF")) *type_out = SF1;   /* slow-to-fall: t1=1, t2でsa1挙動 */
		else
		{
			printf("\n\tFILE ERROR: fault file reading failed. ");
			printf("%c%s%c unexpected type of fault (-tdf では STR/STF を指定).\n\n", '"', token, '"');
			return false;
		}
		return true;
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
//	@name		net-name hash : name -> NLIST*（初回に一度だけ構築し、FindNetByName を平均 O(1) にする）
//*************************************************************************************************************
typedef struct NetHashEntry { NLIST* net; struct NetHashEntry* next; } NetHashEntry;
static NetHashEntry** net_hash      = (NetHashEntry**)NULL;
static int            net_hash_size = 0;

static unsigned long NetNameHash(const char* s)
{
	unsigned long h = 5381;                       // djb2ハッシュ
	for (; *s; ++s) h = ((h << 5) + h) + (unsigned char)*s;
	return h;
}

static void BuildNetHash(void)
{
	net_hash_size = n_net * 2 + 1;                // 充填率 ~0.5
	net_hash = (NetHashEntry**)calloc((size_t)net_hash_size, sizeof(NetHashEntry*));
	for (int i = 0; i < n_net; i++)
	{
		unsigned long h = NetNameHash(nl[i].name) % (unsigned long)net_hash_size;
		NetHashEntry* e = (NetHashEntry*)malloc(sizeof(NetHashEntry));
		e->net      = &nl[i];
		e->next     = net_hash[h];
		net_hash[h] = e;
	}
}

//*************************************************************************************************************
//	@name		FindNetByName
//	@function	指定した名前に一致するネットをネットリストから探す
//	@return		(NLIST*) 一致するネットへのポインタ、無ければ NULL
//*************************************************************************************************************
static NLIST* FindNetByName(
	const char* name		/**< 探索するネット名 */
)
{
	if (net_hash == (NetHashEntry**)NULL) BuildNetHash();

	unsigned long h = NetNameHash(name) % (unsigned long)net_hash_size;
	for (NetHashEntry* e = net_hash[h]; e != NULL; e = e->next)
	{
		if (!strcmp(e->net->name, name)) return e->net;
	}

	printf("\n\tFILE ERROR: fault file reading failed. ");
	printf("%c%s%c is thought.\n\n", '"', name, '"');
	return (NLIST*)NULL;
}

//*************************************************************************************************************
//	@name		CreateFaultNode
//	@function	故障ノードを作成する
//	@return		(FNODE*) 故障ノードへのポインタ
//*************************************************************************************************************
FNODE* CreateFaultNode(
	char* buffer			  /**< バッファ */
)
{
	char* context = (char*)NULL;
	FNODE* fnodeptr = (FNODE*)NULL;

	fnodeptr = (FNODE*)allocMemory(1, sizeof(FNODE));

	/** 文字列を設定する */
	fnodeptr->string = strdup(buffer);

	/** 名前を設定する */
	fnodeptr->name = strdup(strtok_r(buffer, " \t\n", &context));

	/** タイプを設定する */
	if (!ParseFaultType(&context, &fnodeptr->type))
	{
		free(fnodeptr->string);
		free(fnodeptr->name);
		free(fnodeptr);
		return (FNODE*)NULL;
	}

	/** 検出状態を設定する */
	fnodeptr->detect = UNDETECTED;

	/** ネットリストへのポインタを設定する */
	fnodeptr->netptr = FindNetByName(fnodeptr->name);
	if (fnodeptr->netptr == (NLIST*)NULL)
	{
		free(fnodeptr->string);
		free(fnodeptr->name);
		free(fnodeptr);
		return (FNODE*)NULL;
	}

	/** TDF: 励起条件を課す1時刻目コピー（2時刻展開時に peer_1t が張られている） */
	fnodeptr->exc_netptr = (NLIST*)NULL;
	if (opt.fault_model == FM_TDF)
	{
		fnodeptr->exc_netptr = fnodeptr->netptr->peer_1t;
		if (fnodeptr->exc_netptr == (NLIST*)NULL)
		{
			printf("\n\tFILE ERROR: fault file reading failed. ");
			printf("%c%s%c is not a 2nd-frame net (TDF).\n\n", '"', fnodeptr->name, '"');
			free(fnodeptr->string);
			free(fnodeptr->name);
			free(fnodeptr);
			return (FNODE*)NULL;
		}
	}

	/** 次ノードへのポインタを設定する */
	fnodeptr->nextptr = (FNODE*)NULL;

	return fnodeptr;
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

/* TDF equivalence: BUF/INV only, never across a DFF boundary. */
void AnalyzeEquivalenceFaultsTDF(void)
{
	int i;

	// すべてのネットの故障をテスト対象(YES)として初期化
	for (i = 0; i < n_net; i++)
	{
		nl[i].test_sa0 = YES;   // STR
		nl[i].test_sa1 = YES;   // STF
	}

	for (i = 0; i < n_net; i++)
	{
		if (nl[i].type != BUF && nl[i].type != INV) continue;
		if (nl[i].peer_1t == (NLIST*)NULL) continue;          // 1時刻目コピーのゲートは対象外

		/* DFF Q at t2 is a BUF, but its peer at t1 is a free state input.
           A shared primary input also has peer_1t, so testing only the BUF's
           input peer would incorrectly collapse PI faults across the DFF. */
        if (nl[i].peer_1t->type == IN) continue;
		NLIST* in0 = nl[i].in[0];
		if (in0->peer_1t == (NLIST*)NULL) continue;           // DFF置換BUF：時刻境界は跨がない

		// 入力線の遷移故障（STR/STF とも）は出力線の故障で代表される
		in0->test_sa0 = NO;
		in0->test_sa1 = NO;
	}
}

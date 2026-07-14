//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>

#include "./opt.h"
#include "../fdp/read.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		OPT
//	@function	オプションを解析する
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool OPT(
	int					  argc,			      /**< コマンド引数の数 */
	char** argv			      /**< コマンド引数 */
)
{
	/** オプションを初期化する */
	OPTinit();

	/** オプションを設定する */
	if (OPTset(argc, argv) != OPT_OKAY)
	{
		return OPT_ERROR;
	}

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		OPTinit
//	@function	オプションを初期化する
//	@return		(void)
//*************************************************************************************************************
void OPTinit(
	void
)
{
	/** ファイル名を初期化する */
	OPTinitFile();

	return;
}

//*************************************************************************************************************
//	@name		OPTinitFile
//	@function	ファイル名を初期化する
//	@return		(void)
//*************************************************************************************************************
void OPTinitFile(
	void
)
{
	opt.file.input.fault		 = FILE_NOSET;
	opt.file.input.net			 = FILE_NOSET;
	opt.file.output.log			 = FILE_NOSET;

	return;
}

//*************************************************************************************************************
//	@name		F@OPTset
//	@function	F	オプションを設定する
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool OPTset(
	int				      argc,				  /**< コマンド引数の数 */
	char** argv				  /**< コマンド引数 */
)
{
	for (int i = 1; i < argc; i++)
	{
		/** ネットリスト */
		if (strcmp(argv[i], "-net") == 0)
			opt.file.input.net = strdup(argv[++i]);

		/** 故障リスト */
		else if (strcmp(argv[i], "-fault") == 0)
			opt.file.input.fault = strdup(argv[++i]);

		/** ログ */
		else if (strcmp(argv[i], "-log") == 0)
			opt.file.output.log = strdup(argv[++i]);

		/** fdp結果  */
		else if (strcmp(argv[i], "-fdp") == 0)
			opt.file.output.fdp = strdup(argv[++i]);

		/** limit（テスト生成上限） */
		else if (strcmp(argv[i], "-limit") == 0)
			opt.file.input.limit = atoi(argv[++i]);

		/** set ファイルを読み込む */
		else if (strcmp(argv[i], "-set") == 0)
			return OPTread(argv[++i]);

		else
		{
			printf("\n	COMMAND ERROR: option setup is failed. ");
			printf("%c%s%c is not expected.\n\n", '"', argv[i], '"');

			return OPT_ERROR;
		}
	}

	return OPT_OKAY;
}


//*************************************************************************************************************
//	@name		OPTreadValue
//	@function	ディレクティブ行から値（先頭の空白を飛ばした最初のトークン）を取り出して複製する
//	@return		(char*) strdup した値。値が無ければ NULL
//	@note		first_delim は最初の取り出しの区切り。"-net" は空白区切り
//	            (" \n\0")、それ以外は行末まで ("\n\0") という従来挙動をそのまま渡す。
//*************************************************************************************************************
static char* OPTreadValue(
	char** context,			  /**< strtok_r の saveptr */
	const char* first_delim	  /**< 最初の取り出しに使う区切り文字 */
)
{
	char* token = strtok_r(NULL, first_delim, context);
	if (token == NULL) return (char*)NULL;

	for (int i = 0; token[i] != '\0'; i++)
	{
		if (token[i] != ' ' && token[i] != '\t')
			return strdup(strtok_r(&token[i], " \n\0", context));
	}
	return (char*)NULL;
}

//*************************************************************************************************************
//	@name		OPTread
//	@function	設定ファイルを読み込む
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool OPTread(
	char* filename			  /**< ファイル名 */
)
{
	FILE* fileptr = (FILE*)NULL;
	char* buffer = (char*)NULL;
	char* context = (char*)NULL;
	char* token1 = (char*)NULL;
	char* token2 = (char*)NULL;

	/** 「設定ファイル」を読み込みモードで開く */
	fileOpen(&fileptr, filename, "r");

	buffer = allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** オプションを読み込む */
	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (buffer[0] == '-')
		{
			token1 = strtok_r(buffer, " \n\0", &context);

			/** ネットリスト */
			if (strcmp(token1, "-net") == 0)
				opt.file.input.net = OPTreadValue(&context, " \n\0");

			/** 故障リスト */
			else if (strcmp(token1, "-fault") == 0)
				opt.file.input.fault = OPTreadValue(&context, "\n\0");

			/** ログ */
			else if (strcmp(token1, "-log") == 0)
				opt.file.output.log = OPTreadValue(&context, "\n\0");

			/** fdp結果 */
			else if (strcmp(token1, "-fdp") == 0)
				opt.file.output.fdp = OPTreadValue(&context, "\n\0");

			/** limit 設定 */
			else if (strcmp(token1, "-limit") == 0)
			{
				token2 = strtok_r(NULL, " \n\0", &context);

				// 次トークンのNULLチェック
				if (token2 != NULL)
				{
					int val = atoi(token2);

					if (val == 0)
					{
						opt.file.input.limit = 0;
					}
					else
					{
						opt.file.input.limit = val;
					}
				}
			}

			else
			{
				printf("\n	COMMAND ERROR: option setup is failed. ");
				printf("%c%s%c is not expected.\n\n", '"', token1, '"');

				return OPT_ERROR;
			}
		}
	}

	free(buffer);

	fclose(fileptr);

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		OPTcheck
//	@function	必須オプションをチェックする
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool OPTcheck(
	void
)
{

	if (OPTcheckFile() != OPT_OKAY)	return OPT_ERROR;

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		OPTcheckFile
//	@function	必須ファイルをチェックする
//	@return		(bool) 正常, 異常
//*************************************************************************************************************
bool OPTcheckFile(
	void
)
{
	if (opt.file.input.net == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no netlist file.\n\n");

		return OPT_ERROR;
	}

	if (opt.file.input.fault == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no fault list file.\n\n");

		return OPT_ERROR;
	}

	if (opt.file.output.log == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no log file.\n\n");

			return OPT_ERROR;
	}

	if (opt.file.output.fdp == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no fdp result file.\n\n");

			return OPT_ERROR;
	}

	return	OPT_OKAY;
}
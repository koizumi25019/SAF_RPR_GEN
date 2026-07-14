//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "./lib.h"
#include "../fdp/read.h"


//*************************************************************************************************************
//	@name		fileOpen
//	@function	ファイルを開く
//	@return		(void)
//*************************************************************************************************************
void fileOpen(
    FILE** fileptr,           /**< ファイルポインタ */
    const char* filename,     /**< ファイル名 */
    const char* mode          /**< モード */
)
{
    // 標準関数 fopen を使用し、戻り値をポインタの参照先に代入する
    *fileptr = fopen(filename, mode);

    // fopen は失敗すると NULL を返すので、それでエラー判定を行う
    if (*fileptr == NULL)
    {
        printf("\n  FILE ERROR: file open error. ");
        printf("%c%s%c cannot open.\n", '"', filename, '"');
        exit(EXIT_FAILURE);
    }

    return;
}

//*************************************************************************************************************
//	@name		allocMemory
//	@function	メモリを確保する
//	@return		(void*) 確保したメモリへのポインタ
//*************************************************************************************************************
void* allocMemory(
	size_t			      count,			  /**< 確保する要素数 */
	size_t			      size				  /**< 型のサイズ */
)
{
	void* ptr = (void*)NULL;


	if ((ptr = calloc(count, size)) == (void*)NULL)
	{
		printf("\n	MEMORY ERROR: memory allocation could not be secured.\n");

		exit(EXIT_FAILURE);
	}

	return	ptr;
}

//*************************************************************************************************************
//	@name		calcHash
//	@function	ハッシュ値を計算する
//	@return		(int) ハッシュ値
//*************************************************************************************************************
int calcHash(
	char* buffer			  /** バッファ */
)
{
	int hashval = 0;

	for (size_t i = 0; i < strlen(buffer); i++)
	{
		hashval += buffer[i];
	}

	return hashval % MAXSIZE_HASH;
}

//*************************************************************************************************************
//	@name		stackPUSH
//	@function	ネットリストをスタックにpushする
//	@return		(viod)
//*************************************************************************************************************
void stackPUSH(
	NLIST* netptr			  /** ネットリストへのポインタ */
)
{
	stack.stk[stack.ptr++] = netptr;

	if (stack.ptr >= stack.maxnum)
	{
		printf("\n	SYSTEM ERROR: stack overflow\n");

		exit(EXIT_FAILURE);
	}

	return;
}

//*************************************************************************************************************
//	@name		stackPOP
//	@function	ネットリストをスタックからpopする
//	@return		(NLIST*) ネットリストへのポインタ
//*************************************************************************************************************
NLIST* stackPOP(
	void
)
{
	if (stack.ptr < 0)
	{
		printf("\n	SYSTEM ERROR: stack underflow\n");

		exit(EXIT_FAILURE);
	}

	return	stack.stk[--stack.ptr];
}

//*************************************************************************************************************
//	@name		queENQ
//	@function	ネットリストをキューにenqueueする
//	@return		(void)
//*************************************************************************************************************
void queENQ(
	NLIST* netptr,			  /** ネットリストへのポインタ */
	int				      mode				  /** モード */
)
{
	switch (mode)
	{
	case QUE_MODE_ONE:
		que.que[que.rear++] = netptr;
		que.num++;

		if (que.maxnum == que.rear) que.rear = 0;

		if (que.maxnum < que.num)
		{
			printf("\n	SYSTEM ERROR: queue over flow.\n");

			exit(EXIT_FAILURE);
		}

		break;

	case QUE_MODE_ALLOUT:
		for (int i = 0; i < netptr->n_out; i++)
		{
			que.que[que.rear++] = netptr->out[i];
			que.num++;

			if (que.maxnum == que.rear) que.rear = 0;

			if (que.maxnum < que.num)
			{
				printf("\n	SYSTEM ERROR: queue over flow.\n");

				exit(EXIT_FAILURE);
			}
		}

		break;

	case QUE_MODE_ALLIN:
		for (int i = 0; i < netptr->n_in; i++)
		{
			que.que[que.rear++] = netptr->in[i];
			que.num++;

			if (que.maxnum == que.rear) que.rear = 0;

			if (que.maxnum < que.num)
			{
				printf("\n	SYSTEM ERROR: queue over flow.\n");

				exit(EXIT_FAILURE);
			}
		}

		break;

	default:
		printf("\n	SYSTEM ERROR: stack mode error.\n");

		exit(EXIT_FAILURE);

		break;
	}

	return;
}

//*************************************************************************************************************
//	@name		queDEQ
//	@function	ネットリストをキューからdequeueする
//	@return		(NLIST*) ネットリストへのポインタ
//*************************************************************************************************************
NLIST* queDEQ(
	void
)
{
	NLIST* netptr = (NLIST*)NULL;

	netptr = que.que[que.front++];
	que.num--;

	if (que.maxnum == que.front)
	{
		que.front = 0;
	}

	if (que.num < 0)
	{
		printf("\n	SYSTEM ERROR: queue under flow.\n");

		exit(EXIT_FAILURE);
	}

	return	netptr;
}

//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "./lib.h"
#include "../fdp/read.h"


//*************************************************************************************************************
//	@name		fileOpen
//	@function	open the file
//	@return		(void)
//*************************************************************************************************************
void fileOpen(
    FILE** fileptr,           /**< pointer to file */
    const char* filename,     /**< filename */
    const char* mode          /**< mode */
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
//	@function	allocate the memory
//	@return		(void*) pointer to allocated the memory
//*************************************************************************************************************
void* allocMemory(
	size_t			      count,			  /**< number of counts of alloc memory */
	size_t			      size				  /**< typesizeof */
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
//	@function	calculate the hash value
//	@return		(int) hash value
//*************************************************************************************************************
int calcHash(
	char* buffer			  /** buffer */
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
//	@function	push the netlist in stack
//	@return		(viod)
//*************************************************************************************************************
void stackPUSH(
	NLIST* netptr			  /** pointer to netlist */
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
//	@function	pop the netlist from stack
//	@return		(NLIST*) pointer to netlist
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
//	@function	enqueue the netlist to queue
//	@return		(void)
//*************************************************************************************************************
void queENQ(
	NLIST* netptr,			  /** pointer to netlist */
	int				      mode				  /** mode */
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
//	@function	dequeue the netlist from queue
//	@return		(NLIST*) pointer to nelist
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

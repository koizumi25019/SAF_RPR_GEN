#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist/traversal.h"

STACK stack;
QUE que;
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

NLIST* stackPOP(
	void
)
{
	/* 空のスタックから取り出すと stk[-1] を読むため、取り出す前に確認する。 */
	if (stack.ptr <= 0)
	{
		printf("\n	SYSTEM ERROR: stack underflow\n");

		exit(EXIT_FAILURE);
	}

	return	stack.stk[--stack.ptr];
}

void queENQ(
	NLIST* netptr			  /** ネットリストへのポインタ */
)
{
	if (que.num >= que.maxnum)
	{
		printf("\n	SYSTEM ERROR: queue over flow.\n");

		exit(EXIT_FAILURE);
	}

	que.que[que.rear++] = netptr;
	que.num++;

	if (que.maxnum == que.rear) que.rear = 0;

	return;
}

NLIST* queDEQ(
	void
)
{
	NLIST* netptr = (NLIST*)NULL;

	/* 空のキューから取り出すと未設定の要素を読むため、取り出す前に確認する。 */
	if (que.num <= 0)
	{
		printf("\n	SYSTEM ERROR: queue under flow.\n");

		exit(EXIT_FAILURE);
	}

	netptr = que.que[que.front++];
	que.num--;

	if (que.maxnum == que.front)
	{
		que.front = 0;
	}

	return	netptr;
}

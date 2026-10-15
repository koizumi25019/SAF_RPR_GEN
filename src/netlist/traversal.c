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
	if (stack.ptr < 0)
	{
		printf("\n	SYSTEM ERROR: stack underflow\n");

		exit(EXIT_FAILURE);
	}

	return	stack.stk[--stack.ptr];
}

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

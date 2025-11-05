//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>

#include "./target.h"
#include "./read.h"
#include "../lib/lib.h"
#include "../debug/debug.h"
#include "../standard.h"


//*************************************************************************************************************
//	@name		：　SetTarget
//	@function	：	set the target-fault list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool SetTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target,			  /**< target-fault list */
	SORTED* sorted,			  /**< sorted list */
	int loop				  /**< number of loop */
)
{
	int		numfault = 0;
	//int		relax_flag = 0;
	FNODE* tmp = (FNODE*)NULL;

	/** set the number of target-faults */
	remain->num = readdata.fault.numrema;
	remain->list = (FNODE**)allocMemory(remain->num, sizeof(FNODE*));

	/** set the target-fault */
	for (int i = 0; i < MAXSIZE_HASH; i++)
	{
		tmp = readdata.fault.list[i];

		while (tmp != NULL)
		{
			if (tmp->detect == UNDETECTED)
			{
				/** add the fault in target-fault list */
				remain->list[numfault] = tmp;
				remain->list[numfault]->relax = true;
				if (loop == 0)
				{
					tmp->id = numfault;
				}
				numfault++;

				/** if target number of fault is reached,.break */
				if (numfault == remain->num) break;
			}

			tmp = tmp->nextptr;
		}

		if (numfault == remain->num) break;
	}

	if (DirectInputTarget(remain, target) != TARGET_OKAY) return TARGET_ERROR;

#ifdef __DEBUG_SET_REMAIN__
	_CALL_DEBUG_SET_REMAIN_(remain);
#endif // __DEBUG_SET_REMAIN__

	return TARGET_OKAY;
}


//*************************************************************************************************************
//	@name		：　RealTarget
//	@function	：	set the real target-fault list
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool RealTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target,			  /**< target-fault list */
	SORTED* sorted,			  /**< sorted list */
	int loop				  /**< loop variable */
)
{
	CNODE** clique = (CNODE**)NULL;
	CNODE* cnodeptr = (CNODE*)NULL;
	CNODE* head = (CNODE*)NULL;
	CNODE* p = (CNODE*)NULL;
	int depth=0;				/* depth */
	int select;				/* select no. */
	int j = 0;
	int now_id = 0;		/*  initial remain number */
	int flag = 0, select_flag = 0, end_flag = 0, d_flag = 0;
	int count = 0;
	int* select_id;
	select_id = (int*)calloc(remain->num+1, sizeof(int));
	for (int i = 0; i < remain->num; i++) select_id[i] = -1;



	/* number of taget faults < 300 */
	/* fault selection from compatible fault sets */
	if (remain->num < 300)
	{
		if (num_clique_flag == false) 
		{ 
			num_clique_flag = true;
		}

		if (RandomTarget(remain, target) != TARGET_OKAY) return TARGET_ERROR;
		//if (DirectInputTarget(remain,target) != TARGET_OKAY) return TARGET_ERROR;
		return TARGET_OKAY;
	}
	else
	{
		/* sort edge */
		if (loop == 0) {
			SortEdgeDescend(remain, sorted);
		}
	}

	/* initialized to fault detection information */
	for (int k = 0;k < head->edge->int_num;k++)
	{
		head->edge->flag[k] = detflag->flag[k];
	}

	/* Counting the number of adjacent vertices */
	for (int k = 0;k < head->edge->int_num;k++)
	{
		count += CountBits(head->edge->flag[k]);
	}
	head->num_r = count;


	while ((depth < remain->num) && (head->num_r != 0))
	{
		j = 0;
		select_flag = 0;
		while (j < readdata.fault.numinit) {
			if (select_flag == 1)break;
			select = sorted->sort[j].id;
			if (bitintGetNbit(head->edge, select) == 1) {
				select_id[depth] = select;
				select_flag = 1;
				bitintSetNbit_Zero(head->edge,select);

				/* debug bitint */
				//unsigned int decimalNumber;
				//unsigned int binaryNumber[32];
				//int k = 0;
				//int l = 0;
				//for (int m = 0;m < head->edge->int_num;m++)
				//{
				//	k = 0;
				//	decimalNumber = head->edge->flag[m];
				//	while (decimalNumber > 0)
				//	{
				//		binaryNumber[k] = decimalNumber % 2;
				//		decimalNumber /= 2;
				//		k++;
				//	}
				//	PrintDebugMessage("\n[%d]\thead  ->  edge[%d]     \t:  ", depth, m);
				//	for (l = k - 1;l >= 0;l--)
				//	{
				//		PrintDebugMessage("%d", binaryNumber[l]);
				//	}
				//}

				{
					cnodeptr->nextptr = head;
					head = cnodeptr;

					/* Search for the current ID of the selected fault */
					for (now_id = 0;now_id < remain->num;now_id++)
					{
						if (select == remain->list[now_id]->id)
						{
							break;
						}
					}
					select_id[depth++] = now_id;
					for (int k = 0;k < cnodeptr->edge->int_num;k++)
					{
						head->edge->flag[k] 
						= (head->nextptr->edge->flag[k]) 
						& (remain->list[now_id]->edge->flag[k]);
					}
					count = 0;
					for (int k = 0;k < head->edge->int_num;k++)
					{
						count += CountBits(head->edge->flag[k]);
					}
					head->num_r = count;
				}
				return TARGET_ERROR;
				
				/* debug bitint */
				//unsigned int decimalNumber;
				//unsigned int binaryNumber[32]; 
				//int k = 0;
				//int l = 0;
				//for (int m = 0;m < head->edge->int_num;m++)
				//{
				//	k = 0;
				//	decimalNumber = head->edge->flag[m];
				//	while (decimalNumber > 0)
				//	{
				//		binaryNumber[k] = decimalNumber % 2;
				//		decimalNumber /= 2;
				//		k++;
				//	}
				//	PrintDebugMessage("\n[%d]\thead  ->  edge[%d]     \t:  ", depth, m);
				//	for (l = k - 1;l >= 0;l--)
				//	{
				//		PrintDebugMessage("%d", binaryNumber[l]);
				//	}
				//}
			}
			j++;
		}
	}

	/* initialize target-list*/
	target->num = depth-1;
	target->list = (FNODE**)allocMemory(target->num, sizeof(FNODE*));

	/* set target fault */
	for (int i = 0;i < target->num;i++)
	{
		target->list[i] = remain->list[select_id[i+1]];
	}
	target->list[0]->relax = false;

	/* free structure */
	while (head)
	{
		p = head;
		head=head->nextptr;
		free(p);
	}
	//free(sorted->sort);
#ifdef __DEBUG_SET_TARGET__
	_CALL_DEBUG_SET_TARGET_(target);
#endif // __DEBUG_SET_TARGET__

	return TARGET_OKAY;
}


//*************************************************************************************************************
//	@name		：　DirectInputTarget
//	@function	：	set the direct target
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool DirectInputTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
)
{
	target->num = 1;

	target->list = (FNODE**)allocMemory(target->num, sizeof(FNODE*));

	for (int i = 0;i < target->num;i++)
	{
		target->list[i] = remain->list[i];
	}
	target->list[0]->relax = false;
#ifdef __DEBUG_SET_TARGET__
	_CALL_DEBUG_SET_TARGET_(target);
#endif // __DEBUG_SET_TARGET__
	return TARGET_OKAY;
}

//*************************************************************************************************************
//	@name		：　SortEdgeDescend
//	@function	：	sort edge Descend
//	@return		：	null
//*************************************************************************************************************
void SortEdgeDescend(
	TARGET* remain,			  /**< target-fault list */
	SORTED* sorted 			  /**< sorted list */
) 
{
	sorted->num = remain->num;
	sorted->sort = (EDGE*)allocMemory(sorted->num, sizeof(EDGE));


	for (int i = 0; i < sorted->num; i++) 
	{
		sorted->sort[i].id = i;
		sorted->sort[i].n_edge = remain->list[i]->n_edge;
	}
	qsort(sorted->sort, remain->num, sizeof(EDGE), FuncSortDescend);

}

//*************************************************************************************************************
//	@name		：　SortEdgeAscend
//	@function	：	sort edge Ascend
//	@return		：	null
//*************************************************************************************************************
void SortEdgeAscend(
	TARGET* remain,			  /**< target-fault list */
	SORTED* sorted 			  /**< sorted list */
)
{
	sorted->num = remain->num;
	sorted->sort = (EDGE*)allocMemory(sorted->num, sizeof(EDGE));


	for (int i = 0; i < sorted->num; i++)
	{
		sorted->sort[i].id = i;
		sorted->sort[i].n_edge = remain->list[i]->n_edge;
	}
	qsort(sorted->sort, remain->num, sizeof(EDGE), FuncSortAscend);

}

//*************************************************************************************************************
//	@name		：　sort_ascend
//	@function	：	sort_ascend
//	@return		：	
//*************************************************************************************************************
int FuncSortAscend(
	const void* n1, const void* n2
) 
{
	if (((EDGE*)n1)->n_edge > ((EDGE*)n2)->n_edge) {
		return 1;
	}
	else if (((EDGE*)n1)->n_edge < ((EDGE*)n2)->n_edge) {
		return -1;
	}
	else {
		return 0;
	}
}

//*************************************************************************************************************
//	@name		：　sort_descend
//	@function	：	sort_descend
//	@return		：	
//*************************************************************************************************************
int FuncSortDescend(
	const void* n1, const void* n2
)
{
	if (((EDGE*)n1)->n_edge > ((EDGE*)n2)->n_edge) {
		return -1;
	}
	else if (((EDGE*)n1)->n_edge < ((EDGE*)n2)->n_edge) {
		return 1;
	}
	else {
		return 0;
	}
}

//*************************************************************************************************************
//	@name		：　CountBits
//	@function	：	bits counter
//	@return		：	num bits (int)
//*************************************************************************************************************
int CountBits(
	unsigned int n
) 
{
	n = (n & 0x55555555) + (n >> 1 & 0x55555555);
	n = (n & 0x33333333) + (n >> 2 & 0x33333333);
	n = (n & 0x0f0f0f0f) + (n >> 4 & 0x0f0f0f0f);
	n = (n & 0x00ff00ff) + (n >> 8 & 0x00ff00ff);
	n = (n & 0x0000ffff) + (n >> 16 & 0x0000ffff);
	return n;
}

//*************************************************************************************************************
//	@name		：　shuffle
//	@function	：	shuffle number
//	@return		：	void
//*************************************************************************************************************
void shuffle(
	int* array, int n
)
{
	for (int i = n - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		int temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
}

//*************************************************************************************************************
//	@name		：　RandomTarget
//	@function	：	set the random target
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool RandomTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
)
{
	target->list = (FNODE**)allocMemory(target->num, sizeof(FNODE*));

	/* select random number */
	int* numbers;
	numbers = (int*)allocMemory(remain->num, sizeof(int));
	for (int i = 0;i < remain->num;i++)
	{
		numbers[i] = i;
	}
	srand((unsigned int)time(NULL));
	shuffle(numbers, remain->num);

	/* set random fault set  */
	for (int i = 0;i < target->num;i++)
	{
		target->list[i] = remain->list[numbers[i]];
	}
	target->list[0]->relax = false;

	free(numbers);
	//printf("targetlist[0]:%s", target->list[0]->string);
#ifdef __DEBUG_SET_TARGET__
	_CALL_DEBUG_SET_TARGET_(target);
#endif // __DEBUG_SET_TARGET__
	return TARGET_OKAY;
}
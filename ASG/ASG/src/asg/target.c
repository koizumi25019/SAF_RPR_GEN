/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/asg/target.c																		 */
/*	deginer		:	R.miura			covered T.sone													  		 */
/*	date		:	2022.10.01		(2023.10.10)											  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


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
	
#ifdef __DEBUG_SET_REMAIN__
	_CALL_DEBUG_SET_REMAIN_(remain);
#endif // __DEBUG_SET_REMAIN__

	READER_NECESSARY_ON
	{
		if (loop == 0)
		{
			/** include necessary info for target fault */
			COMPinti(remain);
			CreateCompatibleInfo(remain);
			if (CreateNecessaryNet(remain) != TARGET_OKAY) return TARGET_ERROR;
			if (CreateCompatibleEdge(remain) != TARGET_OKAY) return TARGET_ERROR;
			OutGraphLogfile(remain);

			/* generate truly target fault set */
			if (RealTarget(remain,target,sorted,loop) != TARGET_OKAY) return TARGET_ERROR;
		}
		else
		{
			/* generate truly target fault set */
			if (RealTarget(remain, target, sorted, loop) != TARGET_OKAY) return TARGET_ERROR;
		}
	}

	READER_NECESSARY_OFF
	{
		/* random order target faults */
		if (RandomTarget(remain,target) != TARGET_OKAY) return TARGET_ERROR;

		/* top to bottom target faults */
		//if (DirectInputTarget(remain,target) != TARGET_OKAY) return TARGET_ERROR;
	}
	return TARGET_OKAY;
}

//*************************************************************************************************************
//	@name		：　CreateNecessaryNet
//	@function	：	making graph for compatible
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CreateNecessaryNet(
	TARGET* remain			  /**< target-fault list */
)
{
	char* buffer = (char*)NULL;
	char* context = (char*)NULL;
	char* token = (char*)NULL;
	char* token2 = (char*)NULL;


	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	PrintMessage("\n");

	/* set necessary infomation */
	for (int i = 0;i < remain->num;i++)
	{
		sprintf_s(buffer, MAXSIZE_BUFFER, "%s", remain->list[i]->nece);
		token = strtok_s(buffer, "=, ,\n", &context);
		while (token != NULL) {
			token2 = strtok_s(NULL, "=, ,\n", &context);
			for (int j = 0;j < n_net;j++)
			{
				if (!strcmp(token, nl[j].name))
				{
					if (!strcmp(token2, "0"))
					{
						Set_NINT_Zero_XP(remain->list[i]->necenet, (unsigned int)j);
					}
					else if (!strcmp(token2, "1"))
					{
						Set_NINT_One_XP(remain->list[i]->necenet, (unsigned int)j);
					}
					else
					{
						return TARGET_ERROR;
					}
					break;
				}
			}
			token = strtok_s(NULL, "=, ,\n", &context);
		}
		free(remain->list[i]->nece);
		PrintMessage("\r	Reading necessary infomation progress  >> %.2f%%", ((float)(i + 1) / (float)remain->num) * 100);
	
	}
#ifdef __DEBUG_SET_TARGET_NECESSARY__
	_CALL_DEBUG_SET_TARGET_NECESSARY_(remain);
#endif // __DEBUG_SET_TARGET_NECESSARY__

	return TARGET_OKAY;
}

//*************************************************************************************************************
//	@name		：　CreateCompatibleEdge
//	@function	：	making graph for compatible fault sets
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool CreateCompatibleEdge(
	TARGET* remain			  /**< target-fault list */
)
{
	int comp_flag = 0;
	int value = 0;
	unsigned int buffer;
	clock_t half_start,half_end;

	/* initialize comp variable */
	BIT_INT_XP* comp;

	half_start = clock();
	comp = (BIT_INT_XP*)allocMemory(1, sizeof(BIT_INT_XP));
	comp->int_num = n_net / MAXSIZE_BITINT + 1;
	comp->x_buf = (unsigned int*)allocMemory(
		comp->int_num, sizeof(unsigned int));
	comp->p_buf = (unsigned int*)allocMemory(
		comp->int_num, sizeof(unsigned int));
	All_Bit_X_XP(comp);
	PrintMessage("\n");

	/* create compatible infomation */
	for (int i = 0;i < remain->num;i++)
	{
		for (int j = 0;j < remain->num;j++)
		{
			comp_flag = 0;
			if (i != j) 
			{
				for (int k = 0;k < comp->int_num;k++)
				{
					if (comp_flag == 1)break;
					/* all X */
					All_Bit_X_XP(comp);
					/* search comp */
					comp->x_buf[k] &= remain->list[i]->necenet->x_buf[k];
					comp->x_buf[k] &= remain->list[j]->necenet->x_buf[k];
					comp->p_buf[k] &= remain->list[i]->necenet->p_buf[k];
					comp->p_buf[k] &= remain->list[j]->necenet->p_buf[k];
					/* search crash */
					buffer = comp->x_buf[k];
					buffer |= comp->p_buf[k];
					if (buffer != 0xFFFFFFFF)
					{
						bitintSetNbit_Zero(remain->list[i]->edge, j);
						comp_flag = 1;
						remain->list[i]->n_edge -= 1;
					}
				}
			}
		}
		PrintMessage("\r	            Generating graph progress  >> %.2f%%", ((float)(i + 1) / (float)remain->num) * 100);
	}
#ifdef __DEBUG_SET_TARGET_COMP__
	_CALL_DEBUG_SET_TARGET_COMP_(remain);
#endif // __DEBUG_SET_TARGET_COMP__
	half_end = clock();
	time_graph = half_end - half_start;
	PrintMessage("\n	Making Graph CPU time：	%.3f[s]\n", (float)time_graph / CLOCKS_PER_SEC);

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
			num_clique = loop + 1; 
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


	/* first process */
	/* maximum clique extraction from compatible fault set */
	head = CreateCliqueNode(depth++, -1, readdata.fault.numinit);

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

				/* create next depth node */
				if ((cnodeptr = CreateCliqueNode(depth, select, readdata.fault.numinit)) != NULL)
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
				else return TARGET_ERROR;
				
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
//	@name		：　CreateCliqueNode
//	@function	：	create the clique node
//	@return		：	(CNODE*) pointer to clique node
//*************************************************************************************************************
CNODE* CreateCliqueNode(
	int depth,				/* depth */
	int select,				/* select no. */
	int num					/* remain->num */
)
{
	/* variable declaration */
	char* context = (char*)NULL;
	CNODE* cnodeptr = (CNODE*)NULL;

	/* partitioning allocation */
	cnodeptr = (CNODE*)allocMemory(1, sizeof(CNODE));

	/** set the depth */
	cnodeptr->depth = depth;

	/** set the select no. */
	cnodeptr->select = select;

	/** set the num comp */
	cnodeptr->num_r = 0;

	/** set the edge */
	cnodeptr->edge = (BIT_INT*)allocMemory(1, sizeof(BIT_INT));
	cnodeptr->edge->int_num = num / 32 + 1;
	cnodeptr->edge->flag = (unsigned int*)allocMemory(
		cnodeptr->edge->int_num, sizeof(unsigned int));


	/** set the pointer to next node */
	cnodeptr->nextptr = (CNODE*)NULL;

	return cnodeptr;
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
	if (opt.mode.target == SINGLE) target->num = 1;
	else if (opt.mode.target == MULTIPLE) 	target->num = remain->num;

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
//	@name		：　OutGraphLogfile
//	@function	：	output the log
//	@return		：	null
//*************************************************************************************************************
void OutGraphLogfile(
	TARGET* remain			  /**< target-fault list */
)
{
	/* variable declaration */
	int min = 0;
	int max = 0;
	int sum = 0;

	/* calulate */
	min = remain->list[0]->n_edge;
	for (int i = 0;i < remain->num;i++)
	{
		if (min > remain->list[i]->n_edge)
		{
			min = remain->list[i]->n_edge;
		}
		if (max < remain->list[i]->n_edge)
		{
			max = remain->list[i]->n_edge;
		}
		sum += remain->list[i]->n_edge;
	}
	
	/** output graph log file */
	FILE * fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.graphlog, "w");
	fprintf(fileptr, "//-------------------------------------------------------\n");
	fprintf(fileptr, "//          Compatible Fault Graph infomation\n");
	fprintf(fileptr, "//-------------------------------------------------------\n");
	fprintf(fileptr, "//  Number of vertex                : %d\n", readdata.fault.numinit);
	fprintf(fileptr, "//  Number of edges                 : %d\n", sum / 2);
	fprintf(fileptr, "//  Number of minimum degree        : %d\n", min);
	fprintf(fileptr, "//  Number of maximum degree        : %d\n", max);
	fprintf(fileptr, "//  Number of average degree        : %d\n", sum / remain->num);
	fprintf(fileptr, "//  Percentage of branch density    : %.2f%%\n", ((float)sum / (float)(remain->num * (remain->num - 1)) * 100));
	fprintf(fileptr, "//-------------------------------------------------------\n\n");

	/** output number of adjacent vertices in graph */
	//for (int i = 0;i < remain->num;i++)
	//{
	//	if (remain->list[i]->type == SF1)
	//	{
	//		fprintf(fileptr, "%30s\tsa1    %3d\n", remain->list[i]->name, remain->list[i]->n_edge);
	//	}
	//	else
	//	{
	//		fprintf(fileptr, "%30s\tsa0    %3d\n", remain->list[i]->name, remain->list[i]->n_edge);
	//	}
	//}

	for (int i = 0;i < remain->num;i++)
	{
		for (int j = i + 1;j < remain->num;j++)
		{
			if (bitintGetNbit(remain->list[i]->edge, j) == 1)
			{
				fprintf(fileptr, "%s %d,%s %d\n", remain->list[i]->name, remain->list[i]->type, remain->list[j]->name, remain->list[j]->type);
			}
		}
	}

	fclose(fileptr);
	return;
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
	if (opt.mode.target == SINGLE) target->num = 1;
	else if (opt.mode.target == MULTIPLE)
	{
		if (remain->num > 100)	target->num = 100;
		else target->num = remain->num;
	}
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
	//printf("targetlist[0]:%s", target->list[0]->string);
#ifdef __DEBUG_SET_TARGET__
	_CALL_DEBUG_SET_TARGET_(target);
#endif // __DEBUG_SET_TARGET__
	return TARGET_OKAY;
}
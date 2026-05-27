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
//	@name		�F�@fileOpen
//	@function	�F	open the file
//	@return		�F	(void)
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
//	@name		�F�@allocMemory
//	@function	�F	allocate the memory
//	@return		�F	(void*) pointer to allocated the memory
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
//	@name		�F�@calcHash
//	@function	�F	calculate the hash value
//	@return		�F	(int) hash value
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
//	@name		�F�@stackPUSH
//	@function	�F	push the netlist in stack
//	@return		�F	(viod)
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
//	@name		�F�@stackPOP
//	@function	�F	pop the netlist from stack
//	@return		�F	(NLIST*) pointer to netlist
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
//	@name		�F�@queENQ
//	@function	�F	enqueue the netlist to queue
//	@return		�F	(void)
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
//	@name		�F�@queDEQ
//	@function	�F	dequeue the netlist from queue
//	@return		�F	(NLIST*) pointer to nelist
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

//=========================================================================
//  �֐��� : queue_emp
//  �@  �\ : �X�^�b�N���󂩔���
//  �߂�l : 0�i���j�C1�i�L�j
//  ��  �� : �Ȃ�
//=========================================================================
int queue_emp() {

	if (que.front == que.rear) {
		return 0;
	}

	return 1;

}

//*************************************************************************************************************
//	@name		�F�@bitintSetAll_One
//	@function	�F	set the all-bits to one
//	@return		�F	(void)
//*************************************************************************************************************
void bitintSetAll_One(
	BIT_INT* bitint			  /** bit int */
)
{
	for (unsigned int i = 0; i < bitint->int_num; i++)
	{
		/** set the all-bits to one */
		bitint->flag[i] = 0xFFFFFFFF;
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@bitintSetAll_Zero
//	@function	�F	set the all-bits to zero
//	@return		�F	(void)
//*************************************************************************************************************
void bitintSetAll_Zero(
	BIT_INT* bitint			  /** bit int */
)
{
	for (unsigned int i = 0; i < bitint->int_num; i++)
	{
		/** set the all-bits to zero */
		bitint->flag[i] = 0;
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@bitintSetNbit_One
//	@function	�F	set the n-bits to one
//	@return		�F	(void)
//*************************************************************************************************************
void bitintSetNbit_One(
	BIT_INT* bitint,			  /** bit int */
	unsigned int	      nbit				  /** n-bit */
)
{
	bitint->flag[nbit / (8 * sizeof(unsigned int))] |= MASK_INT[nbit % (8 * sizeof(unsigned int))];

	return;
}

//*************************************************************************************************************
//	@name		�F�@bitintSetNbit_Zero
//	@function	�F	set the n-bits to zero
//	@return		�F	(void)
//*************************************************************************************************************
void bitintSetNbit_Zero(
	BIT_INT* bitint,			  /** bit int */
	unsigned int	      nbit				  /** n-bit */
)
{

	bitint->flag[nbit / (8 * sizeof(unsigned int))] &= ~MASK_INT[nbit % (8 * sizeof(unsigned int))];

	return;
}

//*************************************************************************************************************
//	@name		�F�@bitintGetNbit
//	@function	�F	get the n-bit
//	@return		�F	(void)
//*************************************************************************************************************
unsigned int bitintGetNbit(
	BIT_INT* bitint,			  /** bit int */
	unsigned int	      nbit				  /** n-bit */
)
{

	if ((nbit % (8 * sizeof(unsigned int))) == 0)
	{
		if (nbit == false)
		{
			if ((bitint->flag[0] & MASK_INT[0]) == false)	return false;
			else if ((bitint->flag[0] & MASK_INT[0]) != false)  return true;
			else
			{
				printf("\n	SYSTEM ERROR: get the n-bit error.\n");

				exit(EXIT_FAILURE);
			}
		}
		else
		{
			if ((bitint->flag[nbit / (8 * sizeof(unsigned int))] & MASK_INT[0]) == false)	return false;
			else if ((bitint->flag[nbit / (8 * sizeof(unsigned int))] & MASK_INT[0]) != false)  return true;
			else
			{
				printf("\n	SYSTEM ERROR: get the n-bit error.\n");

				exit(EXIT_FAILURE);
			}
		}
	}
	else
	{
		if ((bitint->flag[(nbit / (8 * sizeof(unsigned int)))] & MASK_INT[nbit % (8 * sizeof(unsigned int))]) == false) return false;
		else if ((bitint->flag[(nbit / (8 * sizeof(unsigned int)))] & MASK_INT[nbit % (8 * sizeof(unsigned int))]) != false) return true;
		else
		{
			printf("\n	SYSTEM ERROR: get the n-bit error.\n");

			exit(EXIT_FAILURE);
		}
	}

}

//*************************************************************************************************************
//	@name		�F�@isSingleByte
//	@function	�F	determines if single-byte characters are used
//	@return		�F	(bool) true, false
//*************************************************************************************************************
bool isSingleByte(
	int					  c					  /** characters */
)
{
	if (c >= 0 && c <= 255)
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------
//  �֐��� : All_INT_One_XP
//  �@  �\ : �S�r�b�g��1�ɃZ�b�g( flag=1 )
//  �߂�l : �Ȃ�
//  ��  �� : �|�C���^�z��(�t���O�i�[��)
//------------------------------------------------------------------------
void All_INT_One_XP(BIT_INT_XP* all_one)
{
	unsigned int i;

	for (i = 0; i < all_one->int_num; i++)
	{
		all_one->x_buf[i] = 0xFFFFFFFF;	// x_buf = 1
		all_one->p_buf[i] = 0;			// p_buf = 0
	}

}


//------------------------------------------------------------------------
//  �֐��� : All_INT_Zero_XP
//  �@  �\ : �S�r�b�g��0�ɃZ�b�g( flag=0 )
//  �߂�l : �Ȃ�
//  ��  �� : �|�C���^�z��(�t���O�i�[��)
//------------------------------------------------------------------------
void All_INT_Zero_XP(BIT_INT_XP* all_zero)
{
	unsigned int i;

	for (i = 0; i < all_zero->int_num; i++) {
		all_zero->x_buf[i] = 0;				// x_buf = 0
		all_zero->p_buf[i] = 0xFFFFFFFF;		// p_buf = 1
	}

}


//------------------------------------------------------------------------
//  �֐��� : All_Bit_X_XP
//  �@  �\ : �S�r�b�g��X(�h���g�P�A)�ɃZ�b�g( X�o�b�t�@=0 ,P�o�b�t�@=0 )
//  �߂�l : �Ȃ�
//  ��  �� : �|�C���^�z��(�e�X�g�p�^�[���i�[��)
//------------------------------------------------------------------------
void All_Bit_X_XP(BIT_INT_XP* all_x)
{
	unsigned int i;

	for (i = 0; i < all_x->int_num; i++) {
		all_x->x_buf[i] = 0xFFFFFFFF;		// x_buf = 1
		all_x->p_buf[i] = 0xFFFFFFFF;		// p_buf = 1
	}

}


//------------------------------------------------------------------------
//  �֐��� : Set_NINT_One_XP
//  �@  �\ : �w��r�b�g�ڂ�1���Z�b�g
//  �߂�l : �Ȃ�
//  ��  �� : �|�C���^�z��, �w��r�b�g��
//------------------------------------------------------------------------
void Set_NINT_One_XP(BIT_INT_XP* one_set, unsigned int pi_num) {

	one_set->x_buf[pi_num / (8 * sizeof(unsigned int))] |= MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// flag = 1
	one_set->p_buf[pi_num / (8 * sizeof(unsigned int))] &= ~MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// flag = 1
}


//------------------------------------------------------------------------
//  �֐��� : Set_NINT_Zero_XP
//  �@  �\ : �w��r�b�g�ڂ�0���Z�b�g
//  �߂�l : �Ȃ�
//  ��  �� : �|�C���^�z��, �w��r�b�g��
//------------------------------------------------------------------------
void Set_NINT_Zero_XP(BIT_INT_XP* zero_set, unsigned int pi_num) {

	zero_set->x_buf[(pi_num / (8 * sizeof(unsigned int)))] &= ~MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// flag = 0
	zero_set->p_buf[(pi_num / (8 * sizeof(unsigned int)))] |= MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// flag = 0
}


//------------------------------------------------------------------------
//  �֐��� : Set_NBit_X_XP
//  �@  �\ : �w��r�b�g�ڂ�X���Z�b�g( X�o�b�t�@=1 ,P�o�b�t�@=1 )
//  �߂�l : �Ȃ�
//  ��  �� : �|�C���^�z��(�e�X�g�p�^�[���i�[��), �w��r�b�g��
//------------------------------------------------------------------------
void Set_NBit_X_XP(BIT_INT_XP* x_set, unsigned int pi_num)
{

	x_set->x_buf[(pi_num / (8 * sizeof(unsigned int)))] |= MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// x_buf = 1
	x_set->p_buf[(pi_num / (8 * sizeof(unsigned int)))] |= MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// p_buf = 1

}


//------------------------------------------------------------------------
//  �֐��� : Get_NBit_INT_XP
//  �@  �\ : �w��r�b�g�̒l�𓾂�( 0 or 1 or X ?)
//  �߂�l : �i�[�l( 0 or 1 or X )
//  ��  �� : �|�C���^�z��, �w��r�b�g��
//------------------------------------------------------------------------
unsigned int  Get_NBit_INT_XP(BIT_INT_XP* get_n, unsigned int pi_num) {

	// �w��r�b�g�̒T��
	//=======================================================================
	if ((pi_num % (8 * sizeof(unsigned int))) == 0) {
		//=======================================================================
		if (pi_num == 0) {
			// �w��r�b�g�̒��g���m�F(0, 1)
			if ((get_n->x_buf[0] & MASK_INT[0]) == 0 && (get_n->p_buf[0] & MASK_INT[0]) != 0)	return 0; // (x_buf=0 , p_buf=1)

			else if ((get_n->x_buf[0] & MASK_INT[0]) != 0 && (get_n->p_buf[0] & MASK_INT[0]) == 0) return 1; // (x_buf=1 , p_buf=0)

			else if ((get_n->x_buf[0] & MASK_INT[0]) != 0 && (get_n->p_buf[0] & MASK_INT[0]) != 0)return 3; // (x_buf=1 , p_buf=1)

			else {
				printf("�i�[ ERROR\n");
				exit(-1);
			}
		}
		//=======================================================================
		else {
			// �w��r�b�g�̒��g���m�F(0, 1)
			if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0) return 0; // (x_buf=0 , p_buf=1)

			else if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0) return 1; // (x_buf=1 , p_buf=0)

			else if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0) return 3; // (x_buf=1 , p_buf=1)

			else {
				printf("�i�[ ERROR\n");
				exit(-1);
			}
		}
		//=======================================================================
	}
	//=======================================================================
	else {
		// �w��r�b�g�̒��g���m�F(0, 1)
		if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0) return 0; // (x_buf=0 , p_buf=1)

		else if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0) return 1; // (x_buf=1 , p_buf=0)

		else if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0) return 3; // (x_buf=1 , p_buf=1)

		else {
			printf("�i�[ ERROR\n");
			exit(-1);
		}
	}
	//=======================================================================

}


//------------------------------------------------------------------------
//  �֐��� : Get_NBit_Xbuf
//  �@  �\ : X�o�b�t�@�݂̂̎w��r�b�g�̒l�𓾂�( 0 or 1 ?)
//  �߂�l : �i�[�l( 0 or 1 )
//  ��  �� : �|�C���^�z��(�e�X�g�p�^�[���i�[��), �w��r�b�g��
//------------------------------------------------------------------------
unsigned int  Get_NBit_Xbuf(BIT_INT_XP* get_n, unsigned int pi_num)
{

	// �w��r�b�g�̒T��
	if ((pi_num % (8 * sizeof(unsigned int))) == 0) {
		//====================================================================
		if (pi_num == 0) {
			// �w��r�b�g�̒��g���m�F(0, 1)
			if ((get_n->x_buf[0] & MASK_INT[0]) == 0) return 0; // (x_buf=0)

			else if ((get_n->x_buf[0] & MASK_INT[0]) != 0) return 1; // (x_buf=1)

			else {
				printf("TP�i�[ ERROR\n");
				exit(-1);
			}
		}
		//====================================================================
		else {
			// �w��r�b�g�̒��g���m�F(0, 1)
			if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0) return 0; // (x_buf=0)

			else if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0) return 1; // (x_buf=1)

			else {
				printf("TP�i�[ ERROR\n");
				exit(-1);
			}
		}
		//====================================================================
	}
	//====================================================================
	else {
		// �w��r�b�g�̒��g���m�F(0, 1)
		if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0) return 0; // (x_buf=0)

		else if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0) return 1; // (x_buf=1)

		else {
			printf("TP�i�[ ERROR\n");
			exit(-1);
		}
	}
}
//------------------------------------------------------------------------
//  �֐��� : Get_NBit_XP
//  �@  �\ : �w��r�b�g�̒l�𓾂�(xbuf,pbuf)
//  �߂�l : �i�[�l( 0 or 1 or 2 or 3 )
//  ��  �� : �|�C���^�z��, �w��r�b�g��
//------------------------------------------------------------------------
unsigned int  Get_NBit_XP(BIT_INT_XP* get_n, unsigned int pi_num) {

	// �w��r�b�g�̒T��
	//=======================================================================
	if ((pi_num % (8 * sizeof(unsigned int))) == 0) {
		//=======================================================================
		if (pi_num == 0) {
			// �w��r�b�g�̒��g���m�F(0, 1)
			if ((get_n->x_buf[0] & MASK_INT[0]) == 0 && (get_n->p_buf[0] & MASK_INT[0]) == 0)	return 0; // (x_buf=0 , p_buf=0)

			else if ((get_n->x_buf[0] & MASK_INT[0]) == 0 && (get_n->p_buf[0] & MASK_INT[0]) != 0)	return 1; // (x_buf=0 , p_buf=1)

			else if ((get_n->x_buf[0] & MASK_INT[0]) != 0 && (get_n->p_buf[0] & MASK_INT[0]) == 0) return 2; // (x_buf=1 , p_buf=0)

			else if ((get_n->x_buf[0] & MASK_INT[0]) != 0 && (get_n->p_buf[0] & MASK_INT[0]) != 0)return 3; // (x_buf=1 , p_buf=1)

			else {
				printf("�i�[ ERROR\n");
				exit(-1);
			}
		}
		//=======================================================================
		else {
			// �w��r�b�g�̒��g���m�F(0, 1)
			if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0) return 0; // (x_buf=0 , p_buf=0)

			else if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0) return 1; // (x_buf=0 , p_buf=1)

			else if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0) return 2; // (x_buf=1 , p_buf=0)

			else if ((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0) return 3; // (x_buf=1 , p_buf=1)

			else {
				printf("�i�[ ERROR\n");
				exit(-1);
			}
		}
		//=======================================================================
	}
	//=======================================================================
	else {
		// �w��r�b�g�̒��g���m�F(0, 1)
		if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0) return 0; // (x_buf=0 , p_buf=0)

		else if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0) return 1; // (x_buf=0 , p_buf=1)

		else if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0) return 2; // (x_buf=1 , p_buf=0)

		else if ((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0) return 3; // (x_buf=1 , p_buf=1)

		else {
			printf("�i�[ ERROR\n");
			exit(-1);
		}
	}

}




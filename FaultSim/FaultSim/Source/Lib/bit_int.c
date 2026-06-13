//------------------------------------------------------------------------
//unsigned intの32ビットフラグ
//File name : bit_tp.c
//Date : 2011/7/12
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------

#include	<stdio.h>
#include	<stdlib.h>
#include	"./bit_int.h"
#include	"./alloc.h"


//------------------------------------------------------------------------
// プロタイプ宣言
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  関数名 : Alloc_Bit_INT
//  機  能 : ポインタ配列作成(32bitフラグ)
//  戻り値 : ポインタ配列
//  引  数 : 外部入力数(横幅)
//------------------------------------------------------------------------
BIT_INT *Alloc_Bit_INT(unsigned int num)
{

	unsigned int i;
	BIT_INT *temp;

	if( (temp = (BIT_INT *)malloc(sizeof(BIT_INT))) == NULL ){
		printf("ERROR : mallocできない\n");
	}

	temp->flag = NULL;


	// intの確保数の計算
	if((num % (8 * sizeof(unsigned int))) == 0){
		temp->int_num = num / (8 * sizeof(unsigned int));
	}
	else{
		temp->int_num = (num / (8 * sizeof(unsigned int))) + 1;
	}

	//メモリ確保
	if( (temp->flag = (unsigned int*)malloc(sizeof(unsigned int) * temp->int_num)) == NULL){
		printf("ERROR : mallocできない\n");
	}

	//全ビット0で初期化
	for(i=0; i<temp->int_num; i++){
		temp->flag[i] = 0;
	}

	return temp;
}


//------------------------------------------------------------------------
//  関数名 : All_INT_One
//  機  能 : 全ビットを1にセット( flag=1 )
//  戻り値 : なし
//  引  数 : ポインタ配列(フラグ格納先)
//------------------------------------------------------------------------
void All_INT_One(BIT_INT *all_one)
{
	unsigned int i;

	for(i=0; i< all_one->int_num; i++){
		all_one->flag[i] = 0xFFFFFFFF;	//オール1
	}

}


//------------------------------------------------------------------------
//  関数名 : All_INT_Zero
//  機  能 : 全ビットを0にセット( flag=0 )
//  戻り値 : なし
//  引  数 : ポインタ配列(フラグ格納先)
//------------------------------------------------------------------------
void All_INT_Zero(BIT_INT *all_zero)
{
	unsigned int i;

	for(i=0; i< all_zero->int_num; i++){
		all_zero->flag[i] = 0;	//オール0
	}

}


//------------------------------------------------------------------------
//  関数名 : Set_NINT_One
//  機  能 : 指定ビット目に1をセット
//  戻り値 : なし
//  引  数 : ポインタ配列, 指定ビット数
//------------------------------------------------------------------------
void Set_NINT_One(BIT_INT *one_set, unsigned int pi_num){

	one_set->flag[pi_num / (8 * sizeof(unsigned int))] |= MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// flag = 1

	/*
	unsigned int array_num;
	unsigned int bit_n;

	array_num = pi_num / (8 * sizeof(unsigned int)) ;
	bit_n = pi_num % (8 * sizeof(unsigned int));

	one_set->flag[array_num] |= MASK_INT[bit_n];	// flag = 1
	*/
}


//------------------------------------------------------------------------
//  関数名 : Set_NINT_Zero
//  機  能 : 指定ビット目に0をセット
//  戻り値 : なし
//  引  数 : ポインタ配列, 指定ビット数
//------------------------------------------------------------------------
void Set_NINT_Zero(BIT_INT *zero_set, unsigned int pi_num){

	zero_set->flag[(pi_num / (8 * sizeof(unsigned int)))] &= ~MASK_INT[pi_num % (8 * sizeof(unsigned int))];	// flag = 0

	/*
	unsigned int array_num;
	unsigned int bit_n;

	
	array_num = (pi_num / (8 * sizeof(unsigned int)));
	bit_n = pi_num % (8 * sizeof(unsigned int));
	
	zero_set->flag[array_num] &= ~MASK_INT[bit_n];	// flag = 0
	*/

}

//------------------------------------------------------------------------
//  関数名 : Get_NBit_INT
//  機  能 : 指定ビットの値を得る( 0 or 1 ?)
//  戻り値 : 格納値( 0 or 1 )
//  引  数 : ポインタ配列, 指定ビット数
//------------------------------------------------------------------------
unsigned int  Get_NBit_INT(BIT_INT *get_n, unsigned int pi_num){

	// 指定ビットの探索
	//=======================================================================
	if((pi_num % (8 * sizeof(unsigned int))) == 0){
		//=======================================================================
		if(pi_num==0){
			// 指定ビットの中身を確認(0, 1)
			if((get_n->flag[0] & MASK_INT[0]) == 0)	return 0; // (flag == 0)

			else if((get_n->flag[0] & MASK_INT[0]) != 0) return 1; // (flag == 1)

			else{
				printf("格納 ERROR\n");
				exit(-1);
			}
		}
		//=======================================================================
		else{
			// 指定ビットの中身を確認(0, 1)
			if((get_n->flag[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) == 0)	return 0; // (flag == 0)

			else if((get_n->flag[pi_num / (8 * sizeof(unsigned int))] & MASK_INT[0]) != 0) return 1; // (flag == 1)

			else{
				printf("格納 ERROR\n");
				exit(-1);
			}
		}
		//=======================================================================
	}
	//=======================================================================
	else{
		// 指定ビットの中身を確認(0, 1)
		if((get_n->flag[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) == 0)	return 0; // (flag == 0)

		else if((get_n->flag[(pi_num / (8 * sizeof(unsigned int)))] & MASK_INT[pi_num % (8 * sizeof(unsigned int))]) != 0) return 1; // (flag == 1)

		else{
			printf("格納 ERROR\n");
			exit(-1);
		}
	}
	//=======================================================================


	/*
	unsigned int array_num;
	unsigned int bit_n;


	// 指定ビットの探索
	if((pi_num % (8 * sizeof(unsigned int))) == 0){
		if(pi_num==0){
			array_num = 0;
			bit_n = 0;
		}else{
		array_num = pi_num / (8 * sizeof(unsigned int)) ;
		bit_n = 0;
		}
	}
	else
	{
		array_num = (pi_num / (8 * sizeof(unsigned int)));
		bit_n = pi_num % (8 * sizeof(unsigned int));
	}


	// 指定ビットの中身を確認(0, 1)
	if((get_n->flag[array_num] & MASK_INT[bit_n]) == 0)
	{
		return 0; // (flag == 0)
	}

	else if((get_n->flag[array_num] & MASK_INT[bit_n]) != 0)
	{
		return 1; // (flag == 1)
	}

	else{
		printf("格納 ERROR\n");
		exit(-1);
	}
	*/

}
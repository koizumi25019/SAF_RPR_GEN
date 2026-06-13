//------------------------------------------------------------------------
//unsigned intでのビット単位テストパターン格納[X-buf と P-buf]
//File name : bit_tp.c
//Date : 2010/5/26
//Designer : Yamazaki brothers
//Ver : 0.01
//memo : 高速化のために可読性を落としますた
//------------------------------------------------------------------------

#include	<stdio.h>
#include	<stdlib.h>
#include	"./bit_tp.h"
#include	"./alloc.h"

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//------------------------------------------------------------------------
// プロタイプ宣言
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  関数名 : Alloc_Bit_TP
//  機  能 : ポインタ配列作成(テストパターン格納用)
//  戻り値 : ポインタ配列
//  引  数 : 外部入力数(横幅)
//------------------------------------------------------------------------
BIT_TP *Alloc_Bit_TP(unsigned int num)
{

	unsigned int i;
	BIT_TP *temp;

	temp = (BIT_TP *)malloc(sizeof(BIT_TP));
	temp->x_buf = NULL;
	temp->p_buf = NULL;


	// intの確保数の計算
	if((num % (8 * sizeof(unsigned int))) == 0)
	{
		temp->int_num = num / (8 * sizeof(unsigned int));
	}
	else
	{
		temp->int_num = (num / (8 * sizeof(unsigned int))) + 1;
	}


	//Xバッファ、Pバッファのメモリ確保
	temp->x_buf = (unsigned int*)malloc(temp->int_num * sizeof(unsigned int));
	temp->p_buf = (unsigned int*)malloc(temp->int_num * sizeof(unsigned int));


	//全ビットXで初期化
	for(i=0; i<temp->int_num; i++){
		temp->x_buf[i] = 0xFFFFFFFF;		// x_buf = 全部1
		temp->p_buf[i] = 0xFFFFFFFF;		// p_buf = 全部1
	}

	return temp;
}


//------------------------------------------------------------------------
//  関数名 : All_Bit_One
//  機  能 : 全ビットを1にセット( Xバッファ=1 ,Pバッファ=0 )
//  戻り値 : なし
//  引  数 : ポインタ配列(テストパターン格納先)
//------------------------------------------------------------------------
void All_Bit_One(BIT_TP *all_one)
{
	unsigned int i;

	for(i=0; i< all_one->int_num; i++)
	{
		all_one->x_buf[i]=0xFFFFFFFF;	// x_buf = 1
		all_one->p_buf[i]=0;			// p_buf = 0
	}

}


//------------------------------------------------------------------------
//  関数名 : All_Bit_Zero
//  機  能 : 全ビットを0にセット( Xバッファ=1 ,Pバッファ=0 )
//  戻り値 : なし
//  引  数 : ポインタ配列(テストパターン格納先)
//------------------------------------------------------------------------
void All_Bit_Zero(BIT_TP *all_zero)
{
	unsigned int i;

	for(i=0; i< all_zero->int_num; i++){
		all_zero->x_buf[i]=0;				// x_buf = 0
		all_zero->p_buf[i]=0xFFFFFFFF;		// p_buf = 1
	}

}


//------------------------------------------------------------------------
//  関数名 : All_Bit_X
//  機  能 : 全ビットをX(ドントケア)にセット( Xバッファ=0 ,Pバッファ=0 )
//  戻り値 : なし
//  引  数 : ポインタ配列(テストパターン格納先)
//------------------------------------------------------------------------
void All_Bit_X(BIT_TP *all_x)
{
	unsigned int i;

	for(i=0; i< all_x->int_num; i++){
		all_x->x_buf[i]=0xFFFFFFFF;		// x_buf = 1
		all_x->p_buf[i]=0xFFFFFFFF;		// p_buf = 1
	}

}


//------------------------------------------------------------------------
//  関数名 : Set_NBit_One
//  機  能 : 指定ビット目に1をセット( Xバッファ=1 ,Pバッファ=0 )
//  戻り値 : なし
//  引  数 : ポインタ配列(テストパターン格納先), 指定ビット数
//------------------------------------------------------------------------
void Set_NBit_One(BIT_TP *one_set, unsigned int pi_num)
{

	one_set->x_buf[pi_num / (8 * sizeof(unsigned int))] |= MASK[pi_num % (8 * sizeof(unsigned int))];	// x_buf = 1
	one_set->p_buf[pi_num / (8 * sizeof(unsigned int))] &= ~MASK[pi_num % (8 * sizeof(unsigned int))];	// p_buf = 0

	/*
	unsigned int array_num;
	unsigned int bit_n;

	array_num = pi_num / (8 * sizeof(unsigned int)) ;
	bit_n = pi_num % (8 * sizeof(unsigned int));

	one_set->x_buf[array_num] |= MASK[bit_n];	// x_buf = 1
	one_set->p_buf[array_num] &= ~MASK[bit_n];	// p_buf = 0
	*/
}


//------------------------------------------------------------------------
//  関数名 : Set_NBit_Zero
//  機  能 : 指定ビット目に0をセット( Xバッファ=0 ,Pバッファ=1 )
//  戻り値 : なし
//  引  数 : ポインタ配列(テストパターン格納先), 指定ビット数
//------------------------------------------------------------------------
void Set_NBit_Zero(BIT_TP *zero_set, unsigned int pi_num)
{
	
	zero_set->x_buf[pi_num / (8 * sizeof(unsigned int))] &= ~MASK[pi_num % (8 * sizeof(unsigned int))];	// x_buf = 0
	zero_set->p_buf[pi_num / (8 * sizeof(unsigned int))] |= MASK[pi_num % (8 * sizeof(unsigned int))];	// p_buf = 1

	/*
	unsigned int array_num;
	unsigned int bit_n;

	
	array_num = (pi_num / (8 * sizeof(unsigned int)));
	bit_n = pi_num % (8 * sizeof(unsigned int));
	
	zero_set->x_buf[array_num] &= ~MASK[bit_n];	// x_buf = 0
	zero_set->p_buf[array_num] |= MASK[bit_n];	// p_buf = 1
	*/
}

//------------------------------------------------------------------------
//  関数名 : Set_NBit_X
//  機  能 : 指定ビット目にXをセット( Xバッファ=1 ,Pバッファ=1 )
//  戻り値 : なし
//  引  数 : ポインタ配列(テストパターン格納先), 指定ビット数
//------------------------------------------------------------------------
void Set_NBit_X(BIT_TP *x_set, unsigned int pi_num)
{
	
	x_set->x_buf[(pi_num / (8 * sizeof(unsigned int)))] |= MASK[pi_num % (8 * sizeof(unsigned int))];	// x_buf = 1
	x_set->p_buf[(pi_num / (8 * sizeof(unsigned int)))] |= MASK[pi_num % (8 * sizeof(unsigned int))];	// p_buf = 1

	/*
	unsigned int array_num;
	unsigned int bit_n;

	
	array_num = (pi_num / (8 * sizeof(unsigned int)));
	bit_n = pi_num % (8 * sizeof(unsigned int));
	
	x_set->x_buf[array_num] |= MASK[bit_n];	// x_buf = 1
	x_set->p_buf[array_num] |= MASK[bit_n];	// p_buf = 1
	*/
}


//------------------------------------------------------------------------
//  関数名 : Get_NBit
//  機  能 : 指定ビットの値を得る( 0 or 1 or X ?)
//  戻り値 : 格納値( 0 or 1 or X )
//  引  数 : ポインタ配列(テストパターン格納先), 指定ビット数
//------------------------------------------------------------------------
unsigned int  Get_NBit(BIT_TP *get_n, unsigned int pi_num)
{
	// 指定ビットの探索
	if((pi_num % (8 * sizeof(unsigned int))) == 0){
		//====================================================================
		if(pi_num==0){
			// 指定ビットの中身を確認(0, 1, X)
			if((get_n->x_buf[0] & MASK[0]) == 0 && (get_n->p_buf[0] & MASK[0]) != 0)return 0; // (x_buf=0 , p_buf=1)

			else if((get_n->x_buf[0] & MASK[0]) != 0 && (get_n->p_buf[0] & MASK[0]) == 0)return 1; // (x_buf=1 , p_buf=0)

			else if((get_n->x_buf[0] & MASK[0]) != 0 && (get_n->p_buf[0] & MASK[0]) != 0)return 3; // (x_buf=1 , p_buf=1)
			
			else{
				printf("TP格納 ERROR\n");
				exit(-1);
			}
		}
		//====================================================================
		else{

			if((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) == 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) != 0) return 0; // (x_buf=0 , p_buf=1)

			else if((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) != 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) == 0) return 1; // (x_buf=1 , p_buf=0)

			else if((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) != 0 && (get_n->p_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) != 0) return 3; // (x_buf=1 , p_buf=1)
			
			else{
				printf("TP格納 ERROR\n");
				exit(-1);
			}
		}
		//====================================================================
	}
	//====================================================================
	else{

		if((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) == 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) != 0) return 0; // (x_buf=0 , p_buf=1)

		else if((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) != 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) == 0) return 1; // (x_buf=1 , p_buf=0)

		else if((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) != 0 && (get_n->p_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) != 0) return 3; // (x_buf=1 , p_buf=1)
			
		else{
			printf("TP格納 ERROR\n");
			exit(-1);
		}
	}
	//====================================================================


	/*
	unsigned int array_num;
	unsigned int bit_n;


	// 指定ビットの探索
	if((pi_num % (8 * sizeof(unsigned int))) == 0){
		if(pi_num==0){
			array_num = 0;
			bit_n = 0;
		}
		else{
			array_num = pi_num / (8 * sizeof(unsigned int)) ;
			bit_n = 0;
		}
	}
	else{
		array_num = (pi_num / (8 * sizeof(unsigned int)));
		bit_n = pi_num % (8 * sizeof(unsigned int));
	}


	// 指定ビットの中身を確認(0, 1, X)
	if((get_n->x_buf[array_num] & MASK[bit_n]) == 0 && (get_n->p_buf[array_num] & MASK[bit_n]) != 0)
	{
		return 0; // (x_buf=0 , p_buf=1)
	}

	else if((get_n->x_buf[array_num] & MASK[bit_n]) != 0 && (get_n->p_buf[array_num] & MASK[bit_n]) == 0)
	{
		return 1; // (x_buf=1 , p_buf=0)
	}

	else if((get_n->x_buf[array_num] & MASK[bit_n]) != 0 && (get_n->p_buf[array_num] & MASK[bit_n]) != 0)
	{
		return 3; // (x_buf=1 , p_buf=1)
	}

	else{
		printf("TP格納 ERROR\n");
		exit(-1);
	}
	*/
}

//------------------------------------------------------------------------
//  関数名 : Get_NBit_Xbuf
//  機  能 : Xバッファのみの指定ビットの値を得る( 0 or 1 ?)
//  戻り値 : 格納値( 0 or 1 )
//  引  数 : ポインタ配列(テストパターン格納先), 指定ビット数
//------------------------------------------------------------------------
unsigned int  Get_NBit_Xbuf(BIT_TP *get_n, unsigned int pi_num)
{

	// 指定ビットの探索
	if((pi_num % (8 * sizeof(unsigned int))) == 0){
		//====================================================================
		if(pi_num==0){
			// 指定ビットの中身を確認(0, 1)
			if((get_n->x_buf[0] & MASK[0]) == 0) return 0; // (x_buf=0)

			else if((get_n->x_buf[0] & MASK[0]) != 0) return 1; // (x_buf=1)

			else{
				printf("TP格納 ERROR\n");
				exit(-1);
			}
		}
		//====================================================================
		else{
			// 指定ビットの中身を確認(0, 1)
			if((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) == 0) return 0; // (x_buf=0)

			else if((get_n->x_buf[pi_num / (8 * sizeof(unsigned int))] & MASK[0]) != 0) return 1; // (x_buf=1)

			else{
				printf("TP格納 ERROR\n");
				exit(-1);
			}
		}
		//====================================================================
	}
	//====================================================================
	else{
		// 指定ビットの中身を確認(0, 1)
		if((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) == 0) return 0; // (x_buf=0)

		else if((get_n->x_buf[(pi_num / (8 * sizeof(unsigned int)))] & MASK[pi_num % (8 * sizeof(unsigned int))]) != 0) return 1; // (x_buf=1)

		else{
			printf("TP格納 ERROR\n");
			exit(-1);
		}
	}
	//====================================================================

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
	if((get_n->x_buf[array_num] & MASK[bit_n]) == 0)
	{
		return 0; // (x_buf=0)
	}

	else if((get_n->x_buf[array_num] & MASK[bit_n]) != 0)
	{
		return 1; // (x_buf=1)
	}
	else{
		printf("TP格納 ERROR\n");
		exit(-1);
	}
	*/
}
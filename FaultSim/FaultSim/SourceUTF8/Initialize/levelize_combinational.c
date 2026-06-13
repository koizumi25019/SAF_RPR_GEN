//------------------------------------------------------------------------
//File name : levelize_combinational.c
//Date : 2011/7/11
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Lib/alloc.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		LEV_DEBUG

#define		LEVELED		11		//レベル計算済み
#define		STACK		22		//レベル未計算(スタックに積んでる)

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : levelize_combinational
//  機  能 : レベライズ[組合せ回路用]
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	levelize_combinational(){

	int		i,j;

	int		n_level_stack;		//level_stack配列の格納数
	int		math_possible;		//レベライズ計算可能フラグ{YES, NO}
	int		max_input_level;	//レベライズ対象の入力信号線の最大レベル
	int		*n_level;			//各レベルの信号線数(n_level[0]: レベル0の信号線数)
	int		count;				//カウンタ

	NLIST **level_stack;
	NLIST *target_line;
	
	//============================================================
	// メモリ確保
	//============================================================
	n_level = INT_alloc(n_net);			//とりあえず全信号線数分確保
	level_stack = PNL_alloc(n_net);		//全信号線数分確保

	
	//============================================================
	// 初期化
	//============================================================
	count			= 0;		//カウンタ初期化
	max_level		= 0;		//最大ゲート段数(外部変数)
	n_level_stack	= 0;		//level_stack配列の格納数
	max_input_level	= 0;		//入力信号線の最大レベル初期化
	math_possible	= YES;		//レベライズ計算可能フラグ{YES, NO}
	target_line		= NULL;		//レベライズ対象信号線初期化

	for(i=0; i<n_net; i++){
		level_stack[i] = NULL;	//レベルスタック初期化
		nl[i].flag = 0;			//フラグ初期化
		n_level[i] = 0;			//各レベルの信号線数を全て0に初期化
	}

	
	//============================================================
	// PIのレベライズ
	//============================================================
	for(i=0; i<n_pi; i++){

		// PIをレベル0に設定
		pi[i]->level = 0;			//PIはゲート段数『0』
		pi[i]->flag  = LEVELED;		//レベル計算済み


		//【未レベライズ信号線探索】PI->out[j]をlevel_stackに積む
		for(j=0; j<pi[i]->n_out; j++){
			if(pi[i]->out[j]->flag != LEVELED){
				level_stack[n_level_stack] = pi[i]->out[j];		//level_stackに未レベライズ信号線を積む
				pi[i]->out[j]->flag = STACK;			//level_stackに積んでるよ
				n_level_stack++;						//level_stack格納数更新
			}
		}

		n_level[pi[i]->level]++;	//レベル【pi[i]->level】の信号線数を更新
	}
	
	//n_level_stack調整
	n_level_stack--;

	
	//============================================================
	// レベル計算【レベル計算済み：LEVELED　レベル未計算：STACK】
	//============================================================
	while(n_level_stack != -1){

		//スタックから信号線を1つ取り出す
		target_line = level_stack[n_level_stack];
		
		//-----------------------------------
		//対象信号線がレベル計算可能か判定
		//-----------------------------------
		for(i=0; i<target_line->n_in; i++){

			//対象信号線の入力にレベル未計算信号線がある
			if(target_line->in[i]->flag != LEVELED){
				math_possible = NO;
				break;
			}

			//レベル計算済み信号線だった
			else{
				if(max_input_level < target_line->in[i]->level){
					max_input_level = target_line->in[i]->level;		//入力信号線の最大レベル更新
				}
			}
		}

		//-----------------------------------
		//計算可能の場合
		//-----------------------------------
		if(math_possible == YES){

			//対象信号線にレベル割当(入力の最大レベル+1)
			target_line->level = max_input_level+1;

			//レベル計算済みフラグ設定
			target_line->flag = LEVELED;

			//レベル【target_line->level】の信号線数を更新
			n_level[target_line->level]++;

			//回路中の最大レベル更新
			if(target_line->level > max_level){
				max_level = target_line->level;
			}

			//対象信号線の出力をスタックに積むか判定
			for(j=0; j<target_line->n_out; j++){
				if(target_line->out[j]->flag!=STACK && target_line->out[j]->flag!=LEVELED){
					level_stack[n_level_stack] = target_line->out[j];
					n_level_stack++;
				}
			}

			n_level_stack--;		//スタック格納数を減らす(更新)

		}
		//-----------------------------------
		//計算不可能の場合
		//-----------------------------------
		else if(math_possible == NO){
			target_line->flag = 0;
			n_level_stack--;
		}
		max_input_level = 0;
		math_possible = YES;

	}

#ifdef LEV_DEBUG
	for(i=0; i<max_level+1; i++){
		printf("レベル:%d  信号線数:%d\n", i, n_level[i]);
	}
#endif

	//============================================================
	// レベライズ構造体初期化
	//============================================================
	//メモリ確保
	nml_lev=(struct Levelize*)calloc(max_level+1,sizeof(struct Levelize));
	lev_temp=(struct Levelize*)calloc(max_level+1,sizeof(struct Levelize));
	

	for(i=0; i<max_level+1; i++){

		//初期化
		nml_lev[i].net		= NULL;
		nml_lev[i].n_net	= 0;
		lev_temp[i].net		= NULL;
		lev_temp[i].n_net	= 0;

		//メモリ確保(対象レベルの信号線数分)
		nml_lev[i].net = (NLIST **)calloc(n_level[i]+1,sizeof(NLIST*));
		lev_temp[i].net = (NLIST **)calloc(n_level[i]+1,sizeof(NLIST*));

		//レベライズ構造体の信号線ポインタ初期化
		for(j=0; j<n_level[i]+1; j++){
			nml_lev[i].net[j]	= NULL;
			lev_temp[i].net[j]	= NULL;
		}

	}
	
	//============================================================
	// レベライズ構造体に格納
	//============================================================
	for(i=0; i<n_net; i++){
		//レベライズ構造体に格納
		nml_lev[nl[i].level].net[nml_lev[nl[i].level].n_net] = &nl[i];
		nml_lev[nl[i].level].n_net++;	//対象レベルに含まれる信号線数更新
	}


#ifdef LEV_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG:レベライズ確認【組合せ回路】\n");
	printf("//------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("レベル%d: %d個\n",i, nml_lev[i].n_net);
		for(j=0; j<nml_lev[i].n_net; j++){
			printf("name: %s\n", nml_lev[i].net[j]->name);
		}
		printf("\n");
	}
#endif

}//END
//------------------------------------------------------------------------
//論理SIM結果とstilファイル出力期待値の比較
//File name : comp_exp_sequential.c
//Date : 2013/10/6
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../StandardHead.h"
#include	"../option.h"
#include	"../Lib/alloc.h"

//------------------------------------------------------------------------
// プロタイプ宣言
//------------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

	#define DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//------------------------------------------------------------------------
//  関数名 : comp_exp_2v_sequential
//  機  能 : 【2値順序回路】論理SIM結果とstilファイル出力期待値の比較
//  戻り値 : なし
//  引  数 : テストパターンファイルポインタ
//------------------------------------------------------------------------
void comp_exp_2v_sequential(FILE *fp){

	int				i,j;
	char			sout_name[100];	//スキャンアウト名
	char			temp[MAXN];
	char			get_temp;
	int				min_level = max_level+1;
	int				min_id;
	
	//===========================================================================
	// ファイルポインタを先頭に戻す
	//===========================================================================
	rewind(fp);

	//===========================================================================
	// スキャンアウト名読み込み
	//===========================================================================
	//ScanStructuresがある行までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanStructures")!=NULL){
			break;
		}
	}

	//ScanInがある行までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanIn")!=NULL){
			break;
		}
	}

	get_temp=fgetc(fp);

	//---------------------------------------------------------------
	//ScanOut名を保存
	//---------------------------------------------------------------
	//文字配列インデックス初期化
	i=0;

	//始まりの"まで移動
	do{
		get_temp=fgetc(fp);
	}while(get_temp!='"');

	//終わりの"までの間をコピー
	do{
		get_temp=fgetc(fp);
		sout_name[i] = get_temp;
		i++;
	}while(get_temp!='"');

	//最後にNULL文字代入
	sout_name[i] = '\0';

#ifdef DEBUG
	printf("\nScanOut名：%s\n", sout_name);

	printf("PPOピン配列\n");
	for(j=n_ppo-1; j>=0; j--){
		printf("%s (%s)\n", ppo[j]->name, ppo[j]->name_ins);
	}

	printf("\nPPIピン配列\n");
	for(j=n_ppo-1; j>=0; j--){
		printf("%s (%s)\n", ppi[j]->name, ppi[j]->name_ins);
	}

	printf("\nPPIピン配列\n");
	for(j=0; j<n_ppi; j++){
		printf("%s \n", ppi_tp[j].pin_name);
	}

	printf("\nPPIピン配列\n");
	for(j=0; j<n_ppi; j++){
		if(s_nl[ppi_tp[j].nl_id].type != DFF){
			printf("%s(%d) \n", ppi_tp[j].pin_name, s_nl[ppi_tp[j].nl_id].type);
		}
	}

#endif	
	//===========================================================================
	// 出力期待値比較
	//===========================================================================
	//Ann {* fast_sequential *}がある場所までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Ann {* fast_sequential *}")!=NULL){
			break;
		}
	}

	//もう1回Ann {* fast_sequential *}がある場所までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Ann {* fast_sequential *}")!=NULL){
			break;
		}
	}
	//--------------------------------------------
	//テストパターン番号初期化
	//--------------------------------------------
	i = 0;
	
	//--------------------------------------------
	//出力期待値比較
	//--------------------------------------------
	do{
		
		//--------------------------------------------
		//patternがある場所までポインタを移動
		//--------------------------------------------
		while(fgets(temp,MAXN,fp) !=NULL){
			if(strstr(temp,"load_unload")!=NULL){
				break;
			}
		}

		//--------------------------------------------
		//PPO番号初期化
		//--------------------------------------------
#ifdef DEBUG
		printf("\n%dパターン目LSIM結果(PPO)\n", i);
		for(j=0; j<n_ppi; j++){
			if(Get_NBit_Xbuf(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) == 1){
				printf("H");
			}
			else if(Get_NBit_Xbuf(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) == 0){
				printf("L");
			}
		}
		printf("\n");
#endif		
		j = 0;

		//--------------------------------------------
		//1行読み込み
		//--------------------------------------------
		get_temp = fgetc(fp);
	
		//--------------------------------------------
		// = まで移動
		//--------------------------------------------
		do{
			get_temp = fgetc(fp);
		}while(get_temp!='=');
		
		//--------------------------------------------
		//;までの間(出力期待値)を論理SIM結果と比較
		//--------------------------------------------
		do{
			//PPOの出力期待値を取得
			get_temp = fgetc(fp);

			//比較
			if(get_temp == 'H'){
				if(Get_NBit_Xbuf(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) != 1){
					printf("\n//--------------------------------------------------\n");
					printf("//ERROR: 出力期待値比較\n");
					printf("//--------------------------------------------------\n");
					printf("%dパターン目のPPO[%d]の出力期待値が異なります！\n", i, j);
					//exit(-1);
					if(min_level > s_nl[ppi_tp[j].nl_id].in[0]->level){
						min_level = s_nl[ppi_tp[j].nl_id].in[0]->level;
						min_id = j;
					}
				}
			}
			else if(get_temp == 'L'){
				if(Get_NBit_Xbuf(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) != 0){
					printf("\n//--------------------------------------------------\n");
					printf("//ERROR: 出力期待値比較\n");
					printf("//--------------------------------------------------\n");
					printf("%dパターン目のPPO[%d]の出力期待値が異なります！\n", i, j);
					//exit(-1);
					if(min_level > s_nl[ppi_tp[j].nl_id].in[0]->level){
						min_level = s_nl[ppi_tp[j].nl_id].in[0]->level;
						min_id = j;
					}
				}
			}
			else{
			}

			//PPO番号更新
			j++;

		}while(get_temp!=';');

		printf("\n%dパターン目のPPO[%d](level%d)の出力期待値が異なります！\n", i, min_id, min_level);
		printf("PPI:%s (%s)\n\n",s_nl[ppi_tp[min_id].nl_id].name, s_nl[ppi_tp[min_id].nl_id].name_ins);
		exit(-1);
		i++;

	}while(i != n_tp);

	

#ifdef DEBUG
	printf("出力期待値比較終了\n");
#endif
	
}


//------------------------------------------------------------------------
//  関数名 : comp_exp_3v_sequential
//  機  能 : 【3値順序回路】論理SIM結果とstilファイル出力期待値の比較
//  戻り値 : なし
//  引  数 : テストパターンファイルポインタ
//------------------------------------------------------------------------
void comp_exp_3v_sequential(FILE *fp){

	int				i,j;
	char			sout_name[100];	//スキャンアウト名
	char			temp[MAXN];
	char			get_temp;
	
	//===========================================================================
	// ファイルポインタを先頭に戻す
	//===========================================================================
	rewind(fp);

	//===========================================================================
	// スキャンアウト名読み込み
	//===========================================================================
	//ScanStructuresがある行までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanStructures")!=NULL){
			break;
		}
	}

	//ScanInがある行までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanIn")!=NULL){
			break;
		}
	}

	get_temp=fgetc(fp);

	//---------------------------------------------------------------
	//ScanOut名を保存
	//---------------------------------------------------------------
	//文字配列インデックス初期化
	i=0;

	//始まりの"まで移動
	do{
		get_temp=fgetc(fp);
	}while(get_temp!='"');

	//終わりの"までの間をコピー
	do{
		get_temp=fgetc(fp);
		sout_name[i] = get_temp;
		i++;
	}while(get_temp!='"');

	//最後にNULL文字代入
	sout_name[i] = '\0';

#ifdef DEBUG
	printf("\nScanOut名：%s\n", sout_name);

	printf("PPOピン配列\n");
	for(j=n_ppo-1; j>=0; j--){
		printf("%s (%s)\n", ppo[j]->name, ppo[j]->name_ins);
	}

	printf("\nPPIピン配列\n");
	for(j=n_ppo-1; j>=0; j--){
		printf("%s (%s)\n", ppi[j]->name, ppi[j]->name_ins);
	}

	printf("\nPPIピン配列\n");
	for(j=0; j<n_ppi; j++){
		printf("%s \n", ppi_tp[j].pin_name);
	}

#endif	
	//===========================================================================
	// 出力期待値比較
	//===========================================================================
	//Ann {* fast_sequential *}がある場所までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Ann {* fast_sequential *}")!=NULL){
			break;
		}
	}

	//もう1回Ann {* fast_sequential *}がある場所までポインタを移動
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Ann {* fast_sequential *}")!=NULL){
			break;
		}
	}
	//--------------------------------------------
	//テストパターン番号初期化
	//--------------------------------------------
	i = 0;
	
	//--------------------------------------------
	//出力期待値比較
	//--------------------------------------------
	do{
		
		//--------------------------------------------
		//patternがある場所までポインタを移動
		//--------------------------------------------
		while(fgets(temp,MAXN,fp) !=NULL){
			if(strstr(temp,"load_unload")!=NULL){
				break;
			}
		}

		//--------------------------------------------
		//PPO番号初期化
		//--------------------------------------------
#ifdef DEBUG
		printf("\n%dパターン目LSIM結果(PPO)\n", i);
		for(j=0; j<n_ppi; j++){
			if(Get_NBit(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) == 1){
				printf("H");
			}
			else if(Get_NBit(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) == 0){
				printf("L");
			}
			else if(Get_NBit(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) == 3){
				printf("N");
			}
		}
		printf("\n");
#endif		
		j = 0;

		//--------------------------------------------
		//1行読み込み
		//--------------------------------------------
		get_temp = fgetc(fp);
	
		//--------------------------------------------
		// = まで移動
		//--------------------------------------------
		do{
			get_temp = fgetc(fp);
		}while(get_temp!='=');
		
		//--------------------------------------------
		//;までの間(出力期待値)を論理SIM結果と比較
		//--------------------------------------------
		do{
			//PPOの出力期待値を取得
			get_temp = fgetc(fp);

			//比較
			if(get_temp == 'H'){
				if(Get_NBit(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) != 1){
					printf("\n//--------------------------------------------------\n");
					printf("//ERROR: 出力期待値比較\n");
					printf("//--------------------------------------------------\n");
					printf("%dパターン目のPPO[%d]の出力期待値が異なります！\n", i, j);
					exit(-1);
				}
			}
			else if(get_temp == 'L'){
				if(Get_NBit(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) != 0){
					printf("\n//--------------------------------------------------\n");
					printf("//ERROR: 出力期待値比較\n");
					printf("//--------------------------------------------------\n");
					printf("%dパターン目のPPO[%d]の出力期待値が異なります！\n", i, j);
					exit(-1);
				}
			}
			else if(get_temp == 'N'){
				if(Get_NBit(s_nl[ppi_tp[j].nl_id].in[0]->nval_t2, i) != 3){
					printf("\n//--------------------------------------------------\n");
					printf("//ERROR: 出力期待値比較\n");
					printf("//--------------------------------------------------\n");
					printf("%dパターン目のPPO[%d]の出力期待値が異なります！\n", i, j);
					exit(-1);
				}
			}
			else{
			}

			//PPO番号更新
			j++;

		}while(get_temp!=';');

		i++;

	}while(i != n_tp);

	

#ifdef DEBUG
	printf("出力期待値比較終了\n");
#endif
	
}
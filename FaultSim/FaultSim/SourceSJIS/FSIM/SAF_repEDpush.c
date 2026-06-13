//------------------------------------------------------------------------
//File name : SAF_repEDpush.c
//Date : 2012/8/21
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Lib/bit_tp.h"
#include	"../Lib/bit_int.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define ED_DEBUG

//---------------------------------------------------------------------
// 静的変数
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_2v_repEDpush
//  機  能 : 【代表信号線】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//----------------------------------------------
void SAF_2v_repEDpush(NLIST *e_net,int ed_flag){
	
	int				i;

#ifdef ED_DEBUG
	int				j;
#endif
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:lev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, lev_temp[i].n_net);
		for(j=0; j<lev_temp[i].n_net; j++){
			printf("%s\n", lev_temp[i].net[j]->name);
		}
		printf("\n");
	}
#endif
	
	//=========================================================================
	// イベントドリブンスタックに対象信号線の【代表信号線】出力を全てプッシュ
	//=========================================================================
	for(i=0; i<e_net->n_next_event; i++){	//出力代表信号線数分ループ
	
		//対象信号線の出力信号線が　計算済みで無い　&&　計算候補フラグも立ってない
		if(e_net->next_event[i]->flag!=ed_flag && e_net->next_event[i]->flag!=(ed_flag-1) ){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->next_event[i]->level].net[lev_temp[e_net->next_event[i]->level].n_net] = e_net->next_event[i];
			
			//出力信号線に計算候補フラグ(ed_flag-1)を代入
			e_net->next_event[i]->flag = ed_flag-1;

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->next_event[i]->level].n_net++;
		}			

	}
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:挿入後lev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, lev_temp[i].n_net);
		for(j=0; j<lev_temp[i].n_net; j++){
			printf("%s flag:%d\n", lev_temp[i].net[j]->name, lev_temp[i].net[j]->flag);
		}
		printf("\n");
	}
	/*
	printf("//-------------------------------\n");
	printf("// DEBUG:flag確認\n");
	printf("//-------------------------------\n");
	for(i=0; i<n_net; i++){
		printf("[%d]:%s flag:%d\n", i, nl[i].name, nl[i].flag);
	}*/
	
	printf("\n");
#endif

}

//----------------------------------------------
//  関数名 : SAF_3v_repEDpush
//  機  能 : 【代表信号線】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//----------------------------------------------
void SAF_3v_repEDpush(NLIST *e_net,int ed_flag){
	
	int				i;

#ifdef ED_DEBUG
	int				j;
#endif
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:lev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, lev_temp[i].n_net);
		for(j=0; j<lev_temp[i].n_net; j++){
			printf("%s\n", lev_temp[i].net[j]->name);
		}
		printf("\n");
	}
#endif
	
	//=========================================================================
	// イベントドリブンスタックに対象信号線の【代表信号線】出力を全てプッシュ
	//=========================================================================
	for(i=0; i<e_net->n_next_event; i++){	//出力代表信号線数分ループ
	
		//対象信号線の出力信号線が　計算済みで無い　&&　計算候補フラグも立ってない
		if(e_net->next_event[i]->flag!=ed_flag && e_net->next_event[i]->flag!=(ed_flag-1) ){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->next_event[i]->level].net[lev_temp[e_net->next_event[i]->level].n_net] = e_net->next_event[i];
			
			//出力信号線に計算候補フラグ(ed_flag-1)を代入
			e_net->next_event[i]->flag = ed_flag-1;

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->next_event[i]->level].n_net++;
		}			

	}
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:挿入後lev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, lev_temp[i].n_net);
		for(j=0; j<lev_temp[i].n_net; j++){
			printf("%s\n", lev_temp[i].net[j]->name);
		}
		printf("\n");
	}
	
	printf("//-------------------------------\n");
	printf("// DEBUG:flag確認\n");
	printf("//-------------------------------\n");
	for(i=0; i<n_net; i++){
		printf("[%d]:%s flag:%d\n", i, nl[i].name, nl[i].flag);
	}
	
	printf("\n");
#endif

}
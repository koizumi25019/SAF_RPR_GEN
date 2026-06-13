//------------------------------------------------------------------------
//File name : EDpush.c
//Date : 2013/9/16
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
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
	//#define FLAG_DEBUG

//---------------------------------------------------------------------
// 静的変数
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_EDpush
//  機  能 : 【縮退故障用】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//----------------------------------------------
void SAF_EDpush(NLIST *e_net, unsigned int ed_flag){
	
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
	// イベントドリブンスタックに対象信号線の出力を全てプッシュ
	//=========================================================================
	for(i=0; i<e_net->n_out; i++){	//出力代表信号線数分ループ
	
		//対象信号線の出力信号線が　計算済みで無い　&&　計算候補フラグも立ってない
		if(e_net->out[i]->flag!=ed_flag && e_net->out[i]->flag!=(ed_flag-1) ){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->out[i]->level].net[lev_temp[e_net->out[i]->level].n_net] = e_net->out[i];
			
			//出力信号線に計算候補フラグ(ed_flag-1)を代入
			e_net->out[i]->flag = ed_flag-1;

#ifdef FLAG_DEBUG
			printf("%s (ed_flag=%d)\n", e_net->out[i]->name, e_net->out[i]->flag);
#endif

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->out[i]->level].n_net++;
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
//  関数名 : TDF_EDpush
//  機  能 : 【遷移故障用】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//----------------------------------------------
void TDF_EDpush(S_NLIST *e_net,unsigned int ed_flag){
	
	int				i;

#ifdef ED_DEBUG
	int				j;
#endif
	
	//=========================================================================
	// 【DEBUG】slev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:slev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, slev_temp[i].n_net);
		for(j=0; j<slev_temp[i].n_net; j++){
			printf("%s\n", slev_temp[i].net[j]->name);
		}
		printf("\n");
	}
#endif
	
	//=========================================================================
	// イベントドリブンスタックに対象信号線の出力をプッシュ(※DFFはプッシュしてはいけない！)
	//=========================================================================
	for(i=0; i<e_net->n_out; i++){	//出力代表信号線数分ループ
	
		//対象信号線の出力信号線が　【DFFで無い】 && 【テスト可能信号線】 && 計算済みで無い　&&　計算候補フラグも立ってない
		if(e_net->out[i]->type!=DFF && e_net->out[i]->testable_net==YES && e_net->out[i]->flag!=ed_flag && e_net->out[i]->flag!=(ed_flag-1) ){

			//slev_tempに出力信号線をプッシュ
			slev_temp[e_net->out[i]->level].net[slev_temp[e_net->out[i]->level].n_net] = e_net->out[i];
			
			//出力信号線に計算候補フラグ(ed_flag-1)を代入
			e_net->out[i]->flag = ed_flag-1;

#ifdef FLAG_DEBUG
			printf("プッシュ：%s (ed_flag=%d)\n", e_net->out[i]->name, e_net->out[i]->flag);
#endif

			//slev_tempの保持信号線数をインクリメント
			slev_temp[e_net->out[i]->level].n_net++;
		}			

	}
	
	//=========================================================================
	// 【DEBUG】slev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:挿入後slev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, slev_temp[i].n_net);
		for(j=0; j<slev_temp[i].n_net; j++){
			printf("%s flag:%d\n", slev_temp[i].net[j]->name, slev_temp[i].net[j]->flag);
		}
		printf("\n");
	}
	/*
	printf("//-------------------------------\n");
	printf("// DEBUG:flag確認\n");
	printf("//-------------------------------\n");
	for(i=0; i<n_snet; i++){
		printf("[%d]:%s flag:%d\n", i, nl[i].name, nl[i].flag);
	}*/
	
	printf("\n");
#endif

}
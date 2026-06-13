//------------------------------------------------------------------------
//File name : LSIM_only_cpi.c
//Date : 2013/11/23
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>

#include	"../Netlist/s_netlist.h"
#include	"../option.h"
#include	"../StandardHead.h"

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	TDF_LSIMpush			(S_NLIST*,unsigned int);


//----------------------------------------------
//関数名：LSIM_only_cpi
//機能　：【遷移故障】制御ポイント挿入した付加回路部のみLSIM(FFCからDFFまでの間)
//引数　：ffc(FFC信号線)
//戻り値：なし
//※FFCとCPの値は再設置済み，2値・3値共通
//----------------------------------------------
void LSIM_only_cpi(S_NLIST *ffc, unsigned int ed_flag){

	int			k;
	unsigned int j;
	int			event_lev;					//レベル
	int			level_flag=0;				//event_lev更新時にbreakしないようにする
	S_NLIST		*temp_net;


	//=================================================
	// 1時刻目論理シミュレーション
	//=================================================
#ifdef DEBUG
	printf("\n//====================================\n");
	printf("// 1時刻目CPI部LSIM\n");
	printf("//====================================\n");
#endif

	//--------------------------------------------
	// FFCの出力信号線をレベルスタックにエンキュー
	//--------------------------------------------
	TDF_LSIMpush(ffc, ed_flag);
	
	event_lev = ffc->out[0]->level;

	
	//--------------------------------------------
	// 1時刻目論理シミュレーション
	//--------------------------------------------
	while(1){

		//------------------------------------------
		// レベライズスタックから信号線を取り出す
		//------------------------------------------
		temp_net = slev_temp[event_lev].net[slev_temp[event_lev].n_net-1];	//ケツから取り出す
		slev_temp[event_lev].n_net--;										//信号線保持数デクリメント
#ifdef DEBUG
		printf("%s (type:%d)\n", temp_net->name, temp_net->type);
#endif

		//------------------------------------------
		// 論理演算
		//------------------------------------------
		if(temp_net->type != DFF){
			
			//論理演算
			(*sfunc_t1_lsim[temp_net->type])(temp_net);
			
			//出力信号線をエンキュー
			TDF_LSIMpush(temp_net, ed_flag);
		}
		
		//-------------------------------------------
		// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
		//-------------------------------------------
		if(slev_temp[event_lev].n_net == 0){

			//レベライズスタック内を探索(PI⇒PO側へ)
			for(k=event_lev+1; k<ffc->link_cpi[0]->in[0]->level+1; k++){	//対象FFの入力(PPO)までループ
				//保持信号線数≠0のレベルスタック発見
				if(slev_temp[k].n_net != 0){
					event_lev = k;		//event_lev更新
					level_flag++;		//break阻止
					break;
				}
			}

			//レベライズスタックの中身が全部空だった
			if(level_flag == 0){
				break;	//イベントドリブン終了
			}

			//次のevent_lev更新時に備えてlevel_flagを初期化
			level_flag = 0;

		}
	}
	

	//=================================================
	// 2時刻目論理シミュレーション
	//=================================================
#ifdef DEBUG
	printf("\n//====================================\n");
	printf("// 2時刻目CPI部LSIM\n");
	printf("//====================================\n");
#endif

	//--------------------------------------------
	// FFCの出力信号線をレベルスタックにエンキュー
	//--------------------------------------------
	TDF_LSIMpush(ffc, ed_flag+2);
	
	event_lev = ffc->out[0]->level;
	
	//--------------------------------------------
	// FFCに2時刻目PPI値を印加 (2時刻目FFCは0になるはず)
	//--------------------------------------------
	for(j=0; j<ffc->nval_t2->int_num; j++){
		ffc->nval_t2->x_buf[j] = ffc->in[0]->nval_t1->x_buf[j];		//オール0
		ffc->nval_t2->p_buf[j] = ffc->in[0]->nval_t1->p_buf[j];		//オール1になるはず
	}

	//--------------------------------------------
	// 2時刻目論理シミュレーション
	//--------------------------------------------
	while(1){

		//------------------------------------------
		// レベライズスタックから信号線を取り出す
		//------------------------------------------
		temp_net = slev_temp[event_lev].net[slev_temp[event_lev].n_net-1];	//ケツから取り出す
		slev_temp[event_lev].n_net--;										//信号線保持数デクリメント
#ifdef DEBUG
		printf("%s (type:%d)\n", temp_net->name, temp_net->type);
#endif

		//------------------------------------------
		// 論理演算
		//------------------------------------------
		if(temp_net->type != DFF){
			
			//論理演算
			(*sfunc_t2_lsim[temp_net->type])(temp_net);
			
			//出力信号線をエンキュー
			TDF_LSIMpush(temp_net, ed_flag+2);
		}

		//-------------------------------------------
		// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
		//-------------------------------------------
		if(slev_temp[event_lev].n_net == 0){

			//レベライズスタック内を探索(PI⇒PO側へ)
			for(k=event_lev+1; k<ffc->link_cpi[0]->in[0]->level+1; k++){	//対象FFの入力(PPO)までループ
				//保持信号線数≠0のレベルスタック発見
				if(slev_temp[k].n_net != 0){
					event_lev = k;		//event_lev更新
					level_flag++;		//break阻止
					break;
				}
			}

			//レベライズスタックの中身が全部空だった
			if(level_flag == 0){
				break;	//イベントドリブン終了
			}

			//次のevent_lev更新時に備えてlevel_flagを初期化
			level_flag = 0;

		}
	}

}

//----------------------------------------------
//  関数名 : TDF_LSIMpush
//  機  能 : 【遷移故障用】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//※s_nl[i].cpi_lsimを使用
//----------------------------------------------
void TDF_LSIMpush(S_NLIST *e_net,unsigned int ed_flag){
	
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
		if(e_net->out[i]->type!=DFF && e_net->out[i]->testable_net==YES && e_net->out[i]->cpi_lsim!=ed_flag && e_net->out[i]->cpi_lsim!=(ed_flag-1) ){

			//slev_tempに出力信号線をプッシュ
			slev_temp[e_net->out[i]->level].net[slev_temp[e_net->out[i]->level].n_net] = e_net->out[i];
			
			//出力信号線に計算候補フラグ(ed_flag-1)を代入
			e_net->out[i]->cpi_lsim = ed_flag-1;

#ifdef FLAG_DEBUG
			printf("プッシュ：%s (ed_flag=%d)\n", e_net->out[i]->name, e_net->out[i]->cpi_lsim);
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
			printf("%s cpi_lsim:%d\n", slev_temp[i].net[j]->name, slev_temp[i].net[j]->cpi_lsim);
		}
		printf("\n");
	}
	/*
	printf("//-------------------------------\n");
	printf("// DEBUG:cpi_lsim確認\n");
	printf("//-------------------------------\n");
	for(i=0; i<n_snet; i++){
		printf("[%d]:%s cpi_lsim:%d\n", i, nl[i].name, nl[i].cpi_lsim);
	}*/
	
	printf("\n");
#endif

}
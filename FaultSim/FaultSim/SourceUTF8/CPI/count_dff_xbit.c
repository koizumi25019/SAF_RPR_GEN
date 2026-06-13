//------------------------------------------------------------------------
//File name : count_dff_xbit.c
//Date : 2013/11/28
//Designer : H.Yamazaki
//Ver : 0.01
//
//memo:
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<time.h>

#include	"../StandardHead.h"
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../option.h"
#include	"../XID/Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

	//#define	DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//--------------------------------------------------------------------
//関数名：count_dff_xbit
//機能　：2時刻目PPI(1時刻目PPO)に含まれるX率を計算
//引数　：なし
//戻り値：なし
//--------------------------------------------------------------------
void count_dff_xbit(){

	int				i,j;
	int				n_x = 0;		//DFFのドントケア数
	int				n_resp_x = 0;	//回路応答信号線(オリジナルPPO)のドントケア数
	int				n_nonffc_x = 0;	//FFC部以外のドントケア数


	//=============================================================
	// ノーマルXIDモード時
	//=============================================================
	if(opt.xid == YES){
		
		//----------------------------------------
		// X数計算
		//----------------------------------------
		for(i=0; i<n_tp; i++){
			for(j=0; j<n_ppi; j++){
				if(Get_NBit(ppi[j]->xid_nval_t2, i) == 3){
					n_x++;
				}
			}
		}

		//----------------------------------------
		// 2時刻目PPIのX率表示
		//----------------------------------------
		printf("2時刻目PPIのX率  :%.2lf%%\n",((double)n_x/(double)(n_tp*n_ppi))*100 );
	}

	
	//=============================================================
	// CPI_XIDモード時
	//=============================================================
	else if(opt.cpi_xid == YES){

		//----------------------------------------
		// X数計算
		//----------------------------------------
		for(i=0; i<n_tp; i++){

			//2時刻目PPIのX数計算
			for(j=0; j<n_ori_dff; j++){
				if(Get_NBit(ori_dff[j]->xid_nval_t2, i) == 3){
					n_x++;
#ifdef DEBUG
					printf("tp[%d]オリジナルFF: %s (%d %d)\n",i,  ori_dff[j]->name, Get_NBit(ori_dff[j]->xid_nval_t1, i), Get_NBit(ori_dff[j]->xid_nval_t2, i));
#endif
				}
			}


			//1時刻目回路応答のX数計算
			for(j=0; j<n_ori_dff; j++){
				if(Get_NBit(ori_dff[j]->in[0]->in[0]->in[0]->xid_nval_t1, i) == 3){
					n_resp_x++;
#ifdef DEBUG
					printf("tp[%d]回路応答: %s (%d %d)\n",i,  ori_dff[j]->in[0]->in[0]->in[0]->name, Get_NBit(ori_dff[j]->in[0]->in[0]->in[0]->xid_nval_t1, i), Get_NBit(ori_dff[j]->in[0]->in[0]->in[0]->xid_nval_t2, i));
#endif
				}
			}


			//FFC部以外のX数計算(オリジナルPI)
			for(j=0; j<n_ori_pi; j++){
				if(Get_NBit(ori_pi[j]->xid_nval_t1, i) == 3){
					n_nonffc_x++;
				}
			}
			//FFC=1のCP部
			for(j=0; j<n_ffc; j++){
				if(Get_NBit(ffc[j]->nval_t1, i) == 1){
					if(Get_NBit(ffc[j]->link_cpi[0]->link_cpi[0]->xid_nval_t1, i) == 3){
						n_nonffc_x++;
					}
				}
			}
			//FFC部以外のX数計算(オリジナルDFF)
			for(j=0; j<n_ori_dff; j++){
				if(Get_NBit(ori_dff[j]->xid_nval_t1, i) == 3){
					n_nonffc_x++;
				}
			}


		}
		
		//----------------------------------------
		// 2時刻目PPIのX率表示
		//----------------------------------------
		printf("2時刻目PPIのX率            :%.2lf%%\n",((double)n_x/(double)(n_tp*n_ori_dff))*100 );
		printf("1時刻目回路応答のX率       :%.2lf%%\n",((double)n_resp_x/(double)(n_tp*n_ori_dff))*100 );
		printf("FFC以外のX率               :%.2lf%%\n",((double)n_nonffc_x/(double)(n_tp*(n_ori_pi+n_cpi+n_ori_dff)))*100 );
	}
}
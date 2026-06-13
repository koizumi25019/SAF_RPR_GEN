//------------------------------------------------------------------------
//File name : change_cpi_test_pattern.c
//Date : 2013/11/28
//Designer : H.Yamazaki
//Ver : 0.01
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

#define	DEBUG

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
//関数名：change_cpi_test_pattern
//機能　：FFC=0の信号線のX抽出後値=0, CP信号線値=Xに変更
//引数　：なし
//戻り値：なし
//--------------------------------------------------------------------
void change_xbit(){

	int				i,j;
	unsigned int	k;


	for(i=0; i<n_tp; i++){
		for(j=0; j<n_ffc; j++){
			//FFC=0に設定されていた信号線を特定
			if(Get_NBit(ffc[j]->nval_t1, i) == 0){

				//CPのX抽出後値を全てXに変更
				for(k=0; k<ffc[j]->xid_nval_t1->int_num; k++){
					//1時刻目値
					ffc[j]->link_cpi[0]->link_cpi[0]->xid_nval_t1->x_buf[k] = 0xFFFFFFFF;		//オール1
					ffc[j]->link_cpi[0]->link_cpi[0]->xid_nval_t1->p_buf[k] = 0xFFFFFFFF;		//オール1
					
					//2時刻目値
					ffc[j]->link_cpi[0]->link_cpi[0]->xid_nval_t2->x_buf[k] = 0xFFFFFFFF;		//オール1
					ffc[j]->link_cpi[0]->link_cpi[0]->xid_nval_t2->p_buf[k] = 0xFFFFFFFF;		//オール1
				}
			}
		}
	}
}
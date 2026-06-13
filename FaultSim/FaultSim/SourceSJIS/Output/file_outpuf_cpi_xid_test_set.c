//------------------------------------------------------------------------
//File name : file_outpuf_cpi_xid_test_set.c
//Date : 2013/12/18
//Designer : H.Yamazaki
//Ver : 0.01
//
//memo:
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
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
//関数名：file_outpuf_cpi_xid_test_set
//機能　：【遷移故障用：txt形式】CPI_X抽出後テスト集合{0,1,X}のファイル出力
//引数　：tx(出力用txtファイルポインタ)
//戻り値：なし
//--------------------------------------------------------------------
void	file_outpuf_cpi_xid_test_set(FILE* fp_ori, FILE* fp_cp){
	
	int			tp_id, pi_id;
	int			nval;
		

	//=================================================
	// オリジナル回路部のテスト集合出力
	//=================================================
	if(opt.ori_tp == YES){
		for(tp_id=0; tp_id<n_tp; tp_id++){
		
			//----------------------------------
			//オリジナルPI出力
			//----------------------------------
			for(pi_id=0; pi_id<n_ori_pi; pi_id++){
				//X抽出後の値を得る
				nval = Get_NBit(ori_pi[pi_id]->xid_nval_t1, tp_id);
				
				//0出力
				if(nval == 0){
					fprintf(fp_ori, "0");
				}

				//1出力
				else if(nval == 1){
					fprintf(fp_ori, "1");
				}

				//X出力
				else if(nval == 3){
					fprintf(fp_ori, "X");
				}
			}
			
			//----------------------------------
			//オリジナルPPI出力
			//----------------------------------
			for(pi_id=0; pi_id<n_ori_dff; pi_id++){
				//X抽出後の値を得る
				nval = Get_NBit(ori_dff[pi_id]->xid_nval_t1, tp_id);

				//0出力
				if(nval == 0){
					fprintf(fp_ori, "0");
				}

				//1出力
				else if(nval == 1){
					fprintf(fp_ori, "1");
				}

				//X出力
				else if(nval == 3){
					fprintf(fp_ori, "X");
				}
			}
			//テストパターンごとに改行
			fprintf(fp_ori, "\n");
		}
	}

	//=================================================
	// PI＋CP部のテスト集合出力
	//=================================================
	if(opt.cp_tp == YES){

		for(tp_id=0; tp_id<n_tp; tp_id++){
		
			//----------------------------------
			//オリジナルPI出力
			//----------------------------------
			for(pi_id=0; pi_id<n_ori_pi; pi_id++){
				//X抽出後の値を得る
				nval = Get_NBit(ori_pi[pi_id]->xid_nval_t1, tp_id);
				
				//0出力
				if(nval == 0){
					fprintf(fp_cp, "0");
				}

				//1出力
				else if(nval == 1){
					fprintf(fp_cp, "1");
				}

				//X出力
				else if(nval == 3){
					fprintf(fp_cp, "X");
				}
			}
			
			//----------------------------------
			//CP部出力(FFC=1の部分のみ)
			//----------------------------------
			for(pi_id=0; pi_id<n_cp; pi_id++){

				//FFC=1か判定
				if(Get_NBit(cp[pi_id]->link_cpi[0]->link_cpi[1]->xid_nval_t1, tp_id) == 1){
					
					//X抽出後の値を得る
					nval = Get_NBit(cp[pi_id]->xid_nval_t1, tp_id);

					//0出力
					if(nval == 0){
						fprintf(fp_cp, "0");
					}

					//1出力
					else if(nval == 1){
						fprintf(fp_cp, "1");
					}

					//X出力
					else if(nval == 3){
						fprintf(fp_cp, "X");
					}
				}
			}
			//テストパターンごとに改行
			fprintf(fp_cp, "\n");
		}
	}

}


//------------------------------------------------------------------------
//File name : file_outpuf_all_net_value.c
//Date : 2013/10/25
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
//関数名：file_output_all_net_value
//機能　：全信号線の論理値をファイル出力
//引数　：なし
//戻り値：なし
//--------------------------------------------------------------------
void file_output_all_net_value(FILE *fp){

	int		i,j;

		
	fprintf(stderr,"\n\n File Output All Net Value\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
		
	//==============================================================
	// 縮退故障
	//=============================================================
	if(opt.fmodel == SAF){
		//---------------------------------------------
		// 2値テスト集合の論理SIM(nval)
		//---------------------------------------------
		if(n_before_x==0 && opt.fsim==YES){
			for(i=0; i<n_tp; i++){

				//プログレスバー
				printf("%d回目/%d\r", i+1, n_tp);

				//ファイル出力
				fprintf(fp, "tp[%d]\n", i);
				for(j=0; j<n_net; j++){
					fprintf(fp, "%s %d\n", nl[j].name, Get_NBit_Xbuf(nl[j].nval,i));
				}
				fprintf(fp, "\n");
			}
		}
		
		//---------------------------------------------
		// 3値テスト集合の論理SIM(nval)
		//---------------------------------------------
		else if(n_before_x>0 && opt.fsim==YES){
			for(i=0; i<n_tp; i++){
				
				//プログレスバー
				printf("%d回目/%d\r", i+1, n_tp);

				//ファイル出力
				fprintf(fp, "tp[%d]\n", i);
				for(j=0; j<n_net; j++){
					fprintf(fp, "%s %d\n", nl[j].name, Get_NBit(nl[j].nval,i));
				}
				fprintf(fp, "\n");
			}
		}
		
		//---------------------------------------------
		// XIDモード時のテスト集合(xid_nval)
		//---------------------------------------------
		else if(opt.xid == YES){
			for(i=0; i<n_tp; i++){

				//プログレスバー
				printf("%d回目/%d\r", i+1, n_tp);

				//ファイル出力
				fprintf(fp, "tp[%d]\n", i);
				for(j=0; j<n_net; j++){
					fprintf(fp, "%s %d\n", nl[j].name, Get_NBit(nl[j].xid_nval,i));
				}
				fprintf(fp, "\n");
			}
		}
	}
	
	//==============================================================
	// 遷移故障
	//==============================================================
	else if(opt.fmodel == TDF){
		//---------------------------------------------
		// 2値テスト集合の論理SIM(nval)
		//---------------------------------------------
		if(n_before_x==0 && opt.fsim==YES){
			for(i=0; i<n_tp; i++){

				//プログレスバー
				printf("%d回目/%d\r", i+1, n_tp);

				//ファイル出力
				fprintf(fp, "tp[%d]\n", i);
				for(j=0; j<n_snet; j++){
					fprintf(fp, "%s %d %d\n", s_nl[j].name, Get_NBit_Xbuf(s_nl[j].nval_t1,i), Get_NBit_Xbuf(s_nl[j].nval_t2, i));
				}
				fprintf(fp, "\n");
			}
		}
		
		//---------------------------------------------
		// 3値テスト集合の論理SIM(nval)
		//---------------------------------------------
		else if(n_before_x>0 && opt.fsim==YES){
			for(i=0; i<n_tp; i++){

				//プログレスバー
				printf("%d回目/%d\r", i+1, n_tp);

				//ファイル出力
				fprintf(fp, "tp[%d]\n", i);
				for(j=0; j<n_snet; j++){
					fprintf(fp, "%s %d %d\n", s_nl[j].name, Get_NBit(s_nl[j].nval_t1,i), Get_NBit(s_nl[j].nval_t2, i));
				}
				fprintf(fp, "\n");
			}
		}
		
		//---------------------------------------------
		// XIDモード時のテスト集合(xid_nval)
		//---------------------------------------------
		else if(opt.xid==YES || opt.cpi_xid==YES){
			for(i=0; i<n_tp; i++){

				//プログレスバー
				printf("%d回目/%d\r", i+1, n_tp);

				//ファイル出力
				fprintf(fp, "tp[%d]\n", i);
				for(j=0; j<n_snet; j++){
					fprintf(fp, "%s %d %d\n", s_nl[j].name, Get_NBit(s_nl[j].xid_nval_t1,i), Get_NBit(s_nl[j].xid_nval_t2, i));
				}
				fprintf(fp, "\n");
			}
		}
	}
}

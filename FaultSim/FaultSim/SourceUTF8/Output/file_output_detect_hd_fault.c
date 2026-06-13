//------------------------------------------------------------------------
//File name : file_output_detect_hd_fault.c
//Date : 2012/2/24
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Lib/bit_int.h"
#include	"../Lib/bit_tp.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------


//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : file_output_detect_hd_fault
//  機  能 : 【解析モード】各TPの検出困難故障検出数出力
//  戻り値 : なし
//  引  数 : ファイルポインタ
//----------------------------------------------
void	file_output_detect_hd_fault(FILE* fp){

	int		i,j;
	int		*n_essfault;	//各TPの必須故障数カウンタ(代表故障のみ)
	int		*n_hdfault;		//各TPの検出困難故障数カウンタ(代表故障のみ)
	int		*n_other;		//各TPのその他故障数カウンタ(代表故障のみ)

	fprintf(stderr,"\n\n File_Output_Carebit_Analize\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	//================================================
	// メモリ確保と初期化
	//================================================
	n_essfault = (int*)malloc(sizeof(int)*n_tp);	//TP数メモリ確保
	n_hdfault = (int*)malloc(sizeof(int)*n_tp);		//TP数メモリ確保
	n_other = (int*)malloc(sizeof(int)*n_tp);		//TP数メモリ確保

	for(i=0; i<n_tp; i++){
		n_essfault[i] = 0;
		n_hdfault[i] = 0;
		n_other[i] = 0;
	}

	//================================================
	// 縮退故障の場合
	//================================================
	for(i=0; i<n_net; i++){

		//------------------------------------
		// Progress Bar
		//------------------------------------
		if(((int)i % (int)((n_net*2/100)+1)) == 0)fprintf(stderr,"*");
		

		//------------------------------------
		//0縮退故障の代表故障
		//------------------------------------
		if(nl[i].test_sf0 == YES){
			//必須故障
			if(nl[i].det_sf0 == 1){
				for(j=0; j<n_tp; j++){
					if(Get_NBit_INT(fdic_sa0[j], i) == 1){
						n_essfault[j]++;
					}
				}
			}

			//検出困難故障
			else if(nl[i].det_sf0<=opt.N_HD_fault){
				for(j=0; j<n_tp; j++){
					if(Get_NBit_INT(fdic_sa0[j], i) == 1){
						n_hdfault[j]++;
					}
				}
			}

			//その他故障
			else{
				for(j=0; j<n_tp; j++){
					if(Get_NBit_INT(fdic_sa0[j], i) == 1){
						n_other[j]++;
					}
				}
			}
		}		

		//------------------------------------
		//1縮退故障
		//------------------------------------
		if(nl[i].test_sf1 == YES){
			//必須故障
			if(nl[i].det_sf1 == 1){
				for(j=0; j<n_tp; j++){
					if(Get_NBit_INT(fdic_sa1[j], i) == 1){
						n_essfault[j]++;
					}
				}
			}

			//検出困難故障
			else if(nl[i].det_sf1<=opt.N_HD_fault){
				for(j=0; j<n_tp; j++){
					if(Get_NBit_INT(fdic_sa1[j], i) == 1){
						n_hdfault[j]++;
					}
				}
			}

			//その他故障
			else{
				for(j=0; j<n_tp; j++){
					if(Get_NBit_INT(fdic_sa1[j], i) == 1){
						n_other[j]++;
					}
				}
			}
		}		
	}

	//================================================
	// ファイル出力
	//================================================
	for(i=0; i<n_tp; i++){
		fprintf(fp, "tp \t %d \t 必須故障数: \t %d \t 検出困難故障数: \t %d \t その他故障数: \t %d \t 合計故障数: \t %d\n", i, n_essfault[i], n_hdfault[i], n_other[i], (n_essfault[i]+n_hdfault[i]+n_other[i]));
		//fprintf(fp, "tp \t %d \t 検出困難故障数: \t %d\n", i, n_hdfault[i]);
	}

}
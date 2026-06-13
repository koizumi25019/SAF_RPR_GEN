//------------------------------------------------------------------------
//File name : file_output_ndetect.c
//Date : 2012/2/3
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
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
//#define	DET_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : file_output_ndetect
//  機  能 : 検出故障回数のファイル出力
//  戻り値 : なし
//  引  数 : ファイルポインタ
//----------------------------------------------
void	file_output_ndetect(FILE* fp){

	int		i;
#ifdef DET_DEBUG
	int		j;
	int		count=0;
#endif

	//================================================
	// 縮退故障の場合(故障フラグYESのやつのみ出力)
	//================================================
	if(opt.fmodel == SAF){

		for(i=0; i<n_net; i++){

			//-------------------------------------
			// 0縮退故障
			//-------------------------------------
			if(nl[i].test_sf0==YES && nl[i].det_sf0>0){
				fprintf(fp, "%s\tsa0\t%d", nl[i].name, nl[i].det_sf0);
#ifdef DET_DEBUG
				count=0;
				for(j=0; j<n_tp; j++){
					if( Get_NBit_INT(fdic_sa0[j], i) == YES ){
						count++;
					}
				}
				fprintf(fp, "\tdic0\t%d", count);
#endif
				fprintf(fp, "\n");
			}
		
			//-------------------------------------
			// 1縮退故障
			//-------------------------------------
			if(nl[i].test_sf1==YES && nl[i].det_sf1>0){
				fprintf(fp, "%s\tsa1\t%d", nl[i].name, nl[i].det_sf1);
#ifdef DET_DEBUG
				count=0;
				for(j=0; j<n_tp; j++){
					if( Get_NBit_INT(fdic_sa1[j], i) == YES ){
						count++;
					}
				}
				fprintf(fp, "\tdic1\t%d", count);
#endif
				fprintf(fp,"\n");
			}
		}

	}
	
	//================================================
	// 遷移故障の場合
	//================================================
	else if(opt.fmodel == TDF){
		for(i=0; i<n_snet; i++){

			//-------------------------------------
			// 立上り遷移故障
			//-------------------------------------
			if(s_nl[i].test_str==YES && s_nl[i].det_str>0){
				fprintf(fp, "%s\tstr\t%d\n", s_nl[i].name, s_nl[i].det_str);
			}
		
			//-------------------------------------
			// 立下り遷移故障
			//-------------------------------------
			if(s_nl[i].test_stf==YES && s_nl[i].det_stf>0){
				fprintf(fp, "%s\tstf\t%d\n", s_nl[i].name, s_nl[i].det_stf);
			}
		}

	}

	//ファイルクローズ
	//fclose(fp);
}
//------------------------------------------------------------------------
//File name : file_output_fdictionary.c
//Date : 2012/2/3
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
	//#define DEBUG
	//#define TEST

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : file_output_fdictionary
//  機  能 : 故障辞書のファイル出力
//  戻り値 : なし
//  引  数 : ファイルポインタ
//----------------------------------------------
void	file_output_fdictionary(FILE* fp){

	int		i,j;
	
	fprintf(stderr,"\n\n File_Output_FaultDictionary\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	//================================================
	// 縮退故障の場合
	//================================================
	if(opt.fmodel == SAF){

		for(i=0; i<n_tp; i++){
			
			// Progress Bar
			if(((int)i % (int)((n_tp*2/100)+1)) == 0)fprintf(stderr,"*");

			fprintf(fp, "tp%d\n", i);

			for(j=0; j<n_net; j++){

				//-----------------------------------------
				//0縮退故障の辞書確認
				//-----------------------------------------
				//if( Get_NBit_INT(fdic_sa0[i], j) == 1 ){
				if( Get_NBit_INT(fdic_sa0[i], j) == 1 && nl[j].test_sf0 == YES){
					fprintf(fp, "SF0 %s", nl[j].name);
					
#ifdef TEST
					if(nl[j].test_sf0 == YES){

						fprintf(fp, "(test)\n");
					}
					else{
						fprintf(fp, "\n");
					}
#else
					fprintf(fp, "\n");
#endif
				}
				
				//-----------------------------------------
				//1縮退故障の辞書確認
				//-----------------------------------------
				//else if( Get_NBit_INT(fdic_sa1[i], j) == 1 ){
				else if( Get_NBit_INT(fdic_sa1[i], j) == 1 && nl[j].test_sf1 == YES){
					fprintf(fp, "SF1 %s", nl[j].name);
#ifdef TEST
					if(nl[j].test_sf1 == YES){

						fprintf(fp, "(test)\n");
					}
					else{
						fprintf(fp, "\n");
					}
#else
					fprintf(fp, "\n");
#endif
				}

			}

			fprintf(fp, "\n");
		}
	}
	
	//================================================
	// 遷移故障の場合
	//================================================
	else if(opt.fmodel == TDF){
		for(i=0; i<n_tp; i++){
			
			// Progress Bar
			if(((int)i % (int)((n_tp*2/100)+1)) == 0)fprintf(stderr,"*");

			fprintf(fp, "tp%d\n", i);
#ifdef DEBUG
			printf("tp%d\n", i);
#endif

			for(j=0; j<n_snet; j++){

				//-----------------------------------------
				//立上り遷移故障の辞書確認
				//-----------------------------------------
				//if( Get_NBit_INT(fdic_str[i], j) == 1 ){
				if( Get_NBit_INT(fdic_str[i], j) == 1 && s_nl[j].test_str == YES){
					fprintf(fp, "STR %s", s_nl[j].name);					
#ifdef DEBUG
					printf("STR %s", s_nl[j].name);
#endif
					if(s_nl[j].test_str == YES){
						//fprintf(fp, "(test)\n");
						fprintf(fp, "\n");
#ifdef DEBUG
						printf("(test)\n");
#endif
					}
					else{
						fprintf(fp, "\n");
#ifdef DEBUG
						printf("\n");
#endif
					}
				}
				
				//-----------------------------------------
				//立下り故障の辞書確認
				//-----------------------------------------
				//else if( Get_NBit_INT(fdic_stf[i], j) == 1 ){
				else if( Get_NBit_INT(fdic_stf[i], j) == 1 && s_nl[j].test_stf == YES){
					fprintf(fp, "STF %s", s_nl[j].name);
#ifdef DEBUG
					printf("STF %s", s_nl[j].name);
#endif
					if(s_nl[j].test_stf == YES){
						//fprintf(fp, "(test)\n");
						fprintf(fp, "\n");
#ifdef DEBUG
						printf("(test)\n");
#endif
					}
					else{
						fprintf(fp, "\n");
#ifdef DEBUG
						printf("\n");
#endif
					}
				}

			}

			fprintf(fp, "\n");
		}
	}

	//ファイルクローズ
	//fclose(fp);
}
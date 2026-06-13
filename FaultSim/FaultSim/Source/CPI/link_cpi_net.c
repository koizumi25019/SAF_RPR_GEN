//------------------------------------------------------------------------
//File name : link_cpi_net
//Date : 2013/11/17
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
#define _CRT_SECURE_NO_DEPRECATE
//#pragma warning ( disable : 4996 )

	//#define DEBUG
//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//----------------------------------------------
//関数名：link_cpi_net
//機能　：CPI付加信号線とCPI対象FFをポインタ接続
//引数　：なし
//戻り値：なし
//----------------------------------------------
void link_cpi_net(){
	
	int		i,j,k;
	char	temp[1000];

	
#ifdef DEBUG
		printf("\n\n//=============================================\n");
		printf("// DEBUG: CPI付加信号線のポインタ接続\n");
		printf("//=============================================\n");
#endif
	//===========================================================
	// メモリ確保
	//===========================================================
	//---------------------------------------
	// オリジナルFF：2個分メモリ確保
	//---------------------------------------
	for(i=0; i<n_scan_ff; i++){
		if(ppi_tp[i].nl_id != -1){
			ppi_tp[i].sinput->link_cpi = (S_NLIST**)malloc(sizeof(S_NLIST*)*2);
#ifdef DEBUG
			printf("メモリ確保(×2): %s\n", ppi_tp[i].sinput->name);
#endif
		}
	}


	//---------------------------------------
	// 付加FF(FFC)：1個分メモリ確保
	//---------------------------------------
	for( ; i<(n_scan_ff+(n_sdff/2)); i++){ //初期化式iの結果が未使用のため，iを削除
		if(ppi_tp[i].nl_id != -1){
			ppi_tp[i].sinput->link_cpi = (S_NLIST**)malloc(sizeof(S_NLIST*));
#ifdef DEBUG
			printf("メモリ確保(×1): %s\n", ppi_tp[i].sinput->name);
#endif
		}
	}


	//---------------------------------------
	// 付加PI(CP)：1個分メモリ確保
	//---------------------------------------
	for(i=n_se_input; i<(n_se_input+(n_sdff/2)); i++){
		if(pi_tp[i].nl_id != -1){
			pi_tp[i].sinput->link_cpi = (S_NLIST**)malloc(sizeof(S_NLIST*));
#ifdef DEBUG
			printf("メモリ確保(×1): %s\n", pi_tp[i].sinput->name);
#endif
		}
	}
	
#ifdef DEBUG
	printf("\n");
#endif
	
	//===========================================================
	// ポインタ接続
	//===========================================================	
	//---------------------------------------
	// CP(PI)->FF と FF->CPを接続
	//---------------------------------------
	for(i=n_se_input; i<(n_se_input+(n_sdff/2)); i++){
						
		//----------------------------------------
		//対応するFF名を切り出し
		//----------------------------------------
		k = strlen(pi_tp[i].sinput->name)-7;			//"_tpi_cp"分(7文字)削除
		strncpy(temp, pi_tp[i].sinput->name, k);		//temp = FF名
		temp[k] = '\0';

		//----------------------------------------
		//CP(PI)から対応するFF(PPI)を探索
		//----------------------------------------
		for(j=0; j<n_sdff; j++){
			if( strcmp(temp, ppi[j]->name)==0 ){
					
				//ポインタ接続(CP->FF)
				pi_tp[i].sinput->link_cpi[0] = ppi[j];
#ifdef DEBUG
				printf("%s(CP) -> %s(FF)\n", pi_tp[i].sinput->name, ppi[j]->name);
#endif
				//※ポインタ接続(FF->CP)
				ppi[j]->link_cpi[0] = pi_tp[i].sinput;
#ifdef DEBUG
				printf("%s(FF) -> %s(CP)\n\n",ppi[j]->name , pi_tp[i].sinput->name);
#endif
				//次のCP(PI)へ移動
				break;
			}
		}
		
		//----------------------------------------
		//見つからなかった場合：名前を変えて再探索
		//----------------------------------------
		if(j == n_sdff){

			//最後に『_stem』を連結
			strcat(temp, "_stem");

			//再探索
			for(j=0; j<n_sdff; j++){
				if( strcmp(temp, ppi[j]->name)==0 ){
					
					//ポインタ接続(CP->FF)
					pi_tp[i].sinput->link_cpi[0] = ppi[j];
#ifdef DEBUG
					printf("%s(CP) -> %s(FF)\n", pi_tp[i].sinput->name, ppi[j]->name);
#endif
					//※ポインタ接続(FF->CP)
					ppi[j]->link_cpi[0] = pi_tp[i].sinput;
#ifdef DEBUG
					printf("%s(FF) -> %s(CP)\n\n",ppi[j]->name , pi_tp[i].sinput->name);
#endif
					//次のCP(PI)へ移動
					break;
				}
			}
		}
		
		//----------------------------------------
		//それでも見つからなかった場合
		//----------------------------------------
		if(j == n_sdff){
			printf("Not found link CP-FF: %s\n", temp);

			//見つからなかった個数をカウント
			n_not_found_link_CP_FF++;
		}
	}

	//---------------------------------------
	// FFC->FF と FF->FFCを接続
	//---------------------------------------
	for(i=n_scan_ff; i<(n_scan_ff+(n_sdff/2)); i++){
						
		//----------------------------------------
		//対応するFF名を切り出し
		//----------------------------------------
		k = strlen(ppi_tp[i].sinput->name)-8;			//"_tpi_ffc"分(8文字)削除
		strncpy(temp, ppi_tp[i].sinput->name, k);		//temp = FF名
		temp[k] = '\0';

		//----------------------------------------
		//CP(PI)から対応するFF(PPI)を探索
		//----------------------------------------
		for(j=0; j<n_sdff; j++){
			if( strcmp(temp, ppi[j]->name)==0 ){
					
				//ポインタ接続(FFC->FF)
				ppi_tp[i].sinput->link_cpi[0] = ppi[j];
#ifdef DEBUG
				printf("%s(FFC) -> %s(FF)\n", ppi_tp[i].sinput->name, ppi[j]->name);
#endif
				//※ポインタ接続(FF->FFC)
				ppi[j]->link_cpi[1] = ppi_tp[i].sinput;
#ifdef DEBUG
				printf("%s(FF) -> %s(FFC)\n\n",ppi[j]->name , ppi_tp[i].sinput->name);
#endif
				//次のFFC(PPI)へ移動
				break;
			}
		}
		
		//----------------------------------------
		//見つからなかった場合：名前を変えて再探索
		//----------------------------------------
		if(j == n_sdff){

			//最後に『_stem』を連結
			strcat(temp, "_stem");

			//再探索
			for(j=0; j<n_sdff; j++){
				if( strcmp(temp, ppi[j]->name)==0 ){
					
					//ポインタ接続(FFC->FF)
					ppi_tp[i].sinput->link_cpi[0] = ppi[j];
#ifdef DEBUG
					printf("%s(FFC) -> %s(FF)\n", ppi_tp[i].sinput->name, ppi[j]->name);
#endif
					//※ポインタ接続(FF->FFC)
					ppi[j]->link_cpi[1] = ppi_tp[i].sinput;
#ifdef DEBUG
					printf("%s(FF) -> %s(FFC)\n\n",ppi[j]->name , ppi_tp[i].sinput->name);
#endif
					//次のFFC(PPI)へ移動
					break;
				}
			}
		}
		
		//----------------------------------------
		//それでも見つからなかった場合
		//----------------------------------------
		if(j == n_sdff){
			printf("Not found link FFC-FF: %s\n", temp);
			
			//見つからなかった個数をカウント
			n_not_found_link_FFC_FF++;
		}
	}

	
}
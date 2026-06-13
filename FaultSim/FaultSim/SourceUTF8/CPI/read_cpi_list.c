//------------------------------------------------------------------------
//File name : read_cpi_list
//Date : 2013/11/23
//Designer : H.Yamazaki
//Ver : 0.02
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
#pragma warning ( disable : 4996 )


//=============================
//|  ・制御ポイント挿入リスト |
//|  (FFの信号線名)			  |
//=============================
//|  a			    	      |
//|  b	      			      |
//|  c					      |
//|  					      |
//=============================
//※最後に'改行'が必要
//※インスタンス名でなく，信号線名でOK

	//#define DEBUG

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	search_priority_path		(S_NLIST*, unsigned int);
void	LSIM_only_cpi				(S_NLIST*, unsigned int);


//----------------------------------------------
//関数名：read_cpi_list
//機能　：【遷移故障】制御ポイント挿入FFファイルを読み込み対象【FFCを1に設定】
//引数　：制御ポイント挿入FFリストファイルポインタ
//戻り値：なし
//※2回目LSIMの直前で実行
//----------------------------------------------
void read_cpi_list(FILE *fp){

	int				i;
	unsigned int	j;
	char			str[MAXN];
	int				count;
#ifdef DEBUG
	int				k;
#endif

	//============================================
	// 初期化
	//============================================
	count = 0;
	
	//全信号線フラグ初期化
	for(i=0; i<n_snet; i++){
		if(s_nl[i].flag != 0){
			s_nl[i].flag = 0;
		}
	}

	//============================================
	// 制御ポイント挿入Fリスト読み込み(CP=1にしていく)
	//============================================
	while(fgets(str,MAXN,fp)!=NULL){

		//------------------------------------------------------------
		// _tpi_ffc を連結
		//------------------------------------------------------------
		//改行(\0)の削除
		str[strlen(str)-1] = '\0';

		//_tpi_ffc を連結
		strcat(str, "_tpi_ffc");
#ifdef DEBUG
		printf("探索FFC: %s\n", str);
#endif
		
		//------------------------------------------------------------
		// 対象FFCを探索
		//------------------------------------------------------------
		for(i=0; i<n_ffc; i++){
			if(!strcmp(str ,ffc[i]->name)){				
#ifdef DEBUG
				printf("対象FFC=1に設定: %s\n", ffc[i]->name);
#endif
				//----------------------------------------
				//対象FFC=1に設定
				//----------------------------------------
				for(j=0; j<ffc[i]->nval_t1->int_num; j++){
					ffc[i]->nval_t1->x_buf[j] = 0xFFFFFFFF;		//オール1
					ffc[i]->nval_t1->p_buf[j] = 0;				//オール0
				}

				//制御ポイント挿入数を更新
				count++;
				
				//----------------------------------------
				//★対象CPに2時刻目PPI値を代入(※他CPは2値なら1, 3値ならXに初期化されている)
				//----------------------------------------
				for(j=0; j<ffc[i]->nval_t2->int_num; j++){
					// CP(PI) = DFF
					ffc[i]->link_cpi[0]->link_cpi[0]->nval_t1->x_buf[j] = ffc[i]->link_cpi[0]->nval_t2->x_buf[j];
					ffc[i]->link_cpi[0]->link_cpi[0]->nval_t1->p_buf[j] = ffc[i]->link_cpi[0]->nval_t2->p_buf[j];
					ffc[i]->link_cpi[0]->link_cpi[0]->nval_t2->x_buf[j] = ffc[i]->link_cpi[0]->nval_t2->x_buf[j];
					ffc[i]->link_cpi[0]->link_cpi[0]->nval_t2->p_buf[j] = ffc[i]->link_cpi[0]->nval_t2->p_buf[j];
				}
				
				//----------------------------------------
				//★対象FFCから付加回路部分のみLSIM
				//----------------------------------------
				LSIM_only_cpi(ffc[i], 777);
				
				//----------------------------------------
				//★優先パス解析
				//----------------------------------------
				if(opt.priority_path == YES){
					//CP(PI)を信号線引数にする，優先パスフラグ値=777
					search_priority_path(ffc[i]->link_cpi[0]->link_cpi[0], 777);
				}

				break;
			}
		}
		
		//------------------------------------------------------------
		// 名前を変えてFFCを再探索 (対象FFがPOと繋がっている場合『_stem_tpi_ffc』となっている可能性がある)
		//------------------------------------------------------------
		if(i == n_ffc){
			//『stem_tpi_ffc』を削除
			str[strlen(str)-12] = '\0';

			//tpi_ffc を連結
			strcat(str, "tpi_ffc");
#ifdef DEBUG
			printf("探索FFC: %s\n", str);
#endif
			for(i=0; i<n_ffc; i++){
				if(!strcmp(str ,ffc[i]->name)){				
#ifdef DEBUG
					printf("対象FFC=1に設定: %s\n", ffc[i]->name);
#endif
					//----------------------------------------
					//対象FFC=1に設定
					//----------------------------------------
					for(j=0; j<ffc[i]->nval_t1->int_num; j++){
						ffc[i]->nval_t1->x_buf[j] = 0xFFFFFFFF;		//オール1
						ffc[i]->nval_t1->p_buf[j] = 0;				//オール0
					}

					//制御ポイント挿入数を更新
					count++;
					
					//----------------------------------------
					//★対象CPに2時刻目PPI値を代入(※他CPは2値なら1, 3値ならXに初期化されている)
					//----------------------------------------
					for(j=0; j<ffc[i]->nval_t2->int_num; j++){
						// CP(PI) = DFF
						ffc[i]->link_cpi[0]->link_cpi[0]->nval_t1->x_buf[j] = ffc[i]->link_cpi[0]->nval_t2->x_buf[j];
						ffc[i]->link_cpi[0]->link_cpi[0]->nval_t1->p_buf[j] = ffc[i]->link_cpi[0]->nval_t2->p_buf[j];
						ffc[i]->link_cpi[0]->link_cpi[0]->nval_t2->x_buf[j] = ffc[i]->link_cpi[0]->nval_t2->x_buf[j];
						ffc[i]->link_cpi[0]->link_cpi[0]->nval_t2->p_buf[j] = ffc[i]->link_cpi[0]->nval_t2->p_buf[j];
					}
					
					//----------------------------------------
					//★対象FFCから付加回路部分のみLSIM
					//----------------------------------------
					LSIM_only_cpi(ffc[i], 777);
					
					//----------------------------------------
					//★優先パス解析
					//----------------------------------------
					if(opt.priority_path == YES){
						//CP(PI)を信号線引数にする，優先パスフラグ値=777
						search_priority_path(ffc[i]->link_cpi[0]->link_cpi[0], 777);
					}

					break;
				}
			}
		}
		
		//------------------------------------------------------------
		//FFC信号線がない		
		//------------------------------------------------------------
		if(i == n_ffc){
			fprintf(stderr,"Not found FFC : %s\n",str);
			
			//見つからなかった個数をカウント
			n_not_found_ffc++;
		}
	}
	

	//============================================
	// 制御ポイント挿入数決定
	//============================================
	n_cpi = count;


#ifdef DEBUG
	printf("制御ポイント挿入数: %d\n", n_cpi);
#endif

	
#ifdef DEBUG
	printf("\nピン順番\n");
	for(k=0; k<(n_se_input+(n_sdff/2)); k++){
		if(pi_tp[k].nl_id != -1){
			printf("%s ", pi_tp[k].pin_name);
		}
	}
	for(k=(n_scan_ff+(n_sdff/2))-1; 0<=k; k--){
		printf("%s ", ppi_tp[k].pin_name);
	}

	printf("\n\n-Netlist構造体の中身-\n");
	printf("(※TP構造体はFFC=0のまま)\n");
	for(i=0; i<n_tp; i++){
		//PIのテストパターン表示
		for(k=0; k<(n_se_input+(n_sdff/2)); k++){
			if(pi_tp[k].nl_id != -1){
				if( (Get_NBit(pi_tp[k].sinput->nval_t1,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(pi_tp[k].sinput->nval_t1,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(pi_tp[k].sinput->nval_t1,i)) == 3){
					printf("X");
				}
			}
		}
		//PPIのテストパターン表示
		for(k=(n_scan_ff+(n_sdff/2))-1; 0<=k; k--){
			if(ppi_tp[k].nl_id != -1){
				if( (Get_NBit(ppi_tp[k].sinput->nval_t1,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(ppi_tp[k].sinput->nval_t1,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(ppi_tp[k].sinput->nval_t1,i)) == 3){
					printf("X");
				}
			}
		}
		printf("\n");
	}
#endif


	//============================================
	// ファイルクローズ
	//============================================
	fclose(fp);
}
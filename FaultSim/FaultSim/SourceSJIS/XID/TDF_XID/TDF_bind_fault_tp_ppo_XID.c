//------------------------------------------------------------------------
//File name : TDF_bind_fault_tp_ppo_XID.c
//Date : 2014/7/26
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	<string.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"
#include	"../Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			TDF_2v_SPPFP				(int, SXID_LIST**, int, unsigned int);
void			TDF_3v_SPPFP				(int, SXID_LIST**, int, unsigned int);
unsigned int	TDF_2v_xfilling_bind_ppo	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

	//#define DEBUG

//=========================================================
//| ・XID指定ファイル	                                  |
//| (故障タイプ 故障信号線名 テストパターン番号 検出PPO)  |
//=========================================================
//| STR c 0 a				                              |
//| STF d 7 b				                              |
//| STR e 10 c				                              |
//=========================================================
//※テストパターン番号は0～n_tp-1まで(入力テストパターンの行番号を記載)

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_bind_fault_tp_ppo_XID
//  機  能 : 【故障・パターン・PPO指定】遷移故障ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱)
//----------------------------------------------
void	TDF_bind_fault_tp_ppo_XID(char	**file_names, SXID_LIST	**t_fault){


	FILE			*fp;
	int				i=0,j,k;
	unsigned int	ed_flag=2;		//イベントドリブンフラグ
	unsigned int	xid_flag=2;		//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;		//ドントケア抽出用フラグ(限定正当化に使用)
	int				count=0;		//t_fault内に何個必須故障を入れたかのカウンタ

	char			str[MAXN];		//fgets用
	char			*flt_type;		//故障タイプ{STR, STF}
	char			*flt_name;		//故障信号線名
	char			*tp_name;		//テストパターン番号
	char			*ppo_name;		//検出PPO信号線名

	int				tp_id;			//テストパターン番号
	int				flt_id;			//故障信号線ID
	int				ppo_id;			//故障検出PPOのID

#ifdef DEBUG
	printf("\n//======================================\n");
	printf("// 故障 パターン PPO指定のX抽出\n");
	printf("//======================================\n");
#endif
	
	//===============================================
	// XID指定ファイルオープン
	//===============================================
	if((fp = fopen(file_names[F_FLT_TP_PPO],"r")) == (FILE *)NULL){
		fprintf(stderr,"Cannot open bind_fault_tp_ppo file %s\n",file_names[F_T2_TP]);
		exit(1);
	}

	//===============================================
	// フラグ初期化
	//===============================================
	for(i=0; i<n_snet; i++){
		s_nl[i].flag = 0;
		s_nl[i].xid_flag = 0;
		s_nl[i].jus_flag = 0;
	}
	
	
	//===============================================
	// ファイルを読みながら指定ドントケア抽出
	//===============================================
	fprintf(stderr,"\n\n X-Identification for Bind Faults\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	while(fgets(str, MAXN, fp) != NULL){
		
#ifdef DEBUG
		printf("\n//---------------------------------------\n");
		printf("//%d番目の故障\n", i);
		printf("//---------------------------------------\n");
		printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);
		printf("xid_flag: %d～\n", xid_flag);
#endif
		//------------------------------------------------------
		// XID回数表示
		//------------------------------------------------------
		printf("%d回目\r", i+1);
		
		//------------------------------------------------------
		// 故障タイプ読込み
		//------------------------------------------------------
		flt_type = strtok(str," \t,\n");
		
		//------------------------------------------------------
		// 故障信号線名読込み
		//------------------------------------------------------
		flt_name = strtok('\0'," \t,\n");
		
		//------------------------------------------------------
		// テストパターン番号読込み
		//------------------------------------------------------
		tp_name = strtok('\0'," ,\t\n");
		tp_id = atoi(tp_name);				//変換

		//------------------------------------------------------
		// 検出PPO信号線名読込み
		//------------------------------------------------------
		ppo_name = strtok('\0'," ,\t\n");
		
		//------------------------------------------------------
		// 信号線探索
		//------------------------------------------------------
		//故障信号線探索
		for(j=0; j<n_snet; j++){			
			if(strcmp(s_nl[j].name, flt_name) == 0){
				flt_id = j;
				break;
			}
		}
		//故障伝搬PPO信号線探索(ソート済みPPOのインデックス)
		for(j=0; j<n_ppo; j++){			
			if(strcmp(ppo[sort_propa_po[j]->po_id]->name, ppo_name) == 0){
				ppo_id = j;
				break;
			}
		}

		//------------------------------------------------------
		// ドントケア抽出
		//------------------------------------------------------
		//■立上り遷移故障
		if(strcmp(flt_type, "STR") == 0){

			//対象TPで検出する場合XID
			if( Get_NBit_INT(fdic_str[tp_id], flt_id)==YES ){

				//必須故障の場合必須故障カウンタ更新
				if(s_nl[flt_id].det_str == 1){
					n_essential_fault++;
					tp_info[i].n_essential_fault++;
				}

				//テストパターンごとの必須故障数更新				
				tp_info[i].n_xid_fault++;

				//対象故障リスト内の故障数更新
				count++;

				//対象故障を対象故障リストへ追加
				t_fault[0]->net			= &s_nl[flt_id];		//信号線名
				t_fault[0]->fault_type	= TDF_STR;				//故障タイプ(立上り遷移故障)
				
				//【2値FSIM】&【X抽出】
				if(n_before_x == 0){
					TDF_2v_SPPFP(tp_id, t_fault, 1, ed_flag);												//SPPFP故障SIM
					xid_flag = TDF_2v_xfilling_bind_ppo(tp_id, t_fault, ppo_id, ed_flag, xid_flag, &jus_flag);	//SPPFPからの2値XID
				}
				//【3値FSIM】&【X抽出】
				else{
					printf("3値XIDは未実装だお・・・\n");
					exit(-1);
				}

				//故障設置フラグ初期化
				t_fault[0]->net->nbit_fault = -1;
				
				//EDフラグ更新
				ed_flag+=2;
				jus_flag+=3;

			}

			//ERROR処理
			else{
				printf("\n//-------------------------------------------\n");
				printf("// ERROR: TDF_bind_fault_tp_ppo_XID\n");
				printf("//-------------------------------------------\n");
				printf("STR %s が テストパターン[%d]で検出できません!\n", flt_name, tp_id);
			}
		}
		
		//------------------------------------------------------
		//■立下り遷移故障
		else if(strcmp(flt_type, "STF") == 0){

			//対象TPで検出する場合XID
			if( Get_NBit_INT(fdic_stf[tp_id], flt_id)==YES ){

				//必須故障の場合必須故障カウンタ更新
				if(s_nl[flt_id].det_stf == 1){
					n_essential_fault++;
					tp_info[i].n_essential_fault++;
				}

				//テストパターンごとの必須故障数更新				
				tp_info[i].n_xid_fault++;

				//対象故障リスト内の故障数更新
				count++;

				//対象故障を対象故障リストへ追加
				t_fault[0]->net			= &s_nl[flt_id];		//信号線名
				t_fault[0]->fault_type	= TDF_STF;				//故障タイプ(立下り遷移故障)
				
				//【2値FSIM】&【X抽出】
				if(n_before_x == 0){
					TDF_2v_SPPFP(tp_id, t_fault, 1, ed_flag);												//SPPFP故障SIM
					xid_flag = TDF_2v_xfilling_bind_ppo(tp_id, t_fault, ppo_id, ed_flag, xid_flag, &jus_flag);	//SPPFPからの2値XID
				}
				//【3値FSIM】&【X抽出】
				else{
					printf("3値XIDは未実装だお・・・\n");
					exit(-1);
				}

				//故障設置フラグ初期化
				t_fault[0]->net->nbit_fault = -1;
				
				//EDフラグ更新
				ed_flag+=2;
				jus_flag+=3;

			}
			
			//ERROR処理
			else{
				printf("\n//-------------------------------------------\n");
				printf("// ERROR: TDF_bind_fault_tp_ppo_XID\n");
				printf("//-------------------------------------------\n");
				printf("STF %s が テストパターン[%d]で検出できません!\n", flt_name, tp_id);
			}
		}

		//カウンタ更新
		i++;
	}

	//===============================================
	// XID指定ファイルクローズ
	//===============================================
	fclose(fp);


}//END
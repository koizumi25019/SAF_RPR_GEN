//------------------------------------------------------------------------
//File name : double_detection.c
//Date : 2012/9/22
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_tp.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	#define DEBUG


//構造体定義：二重検出法故障テーブル(n_detect(検出故障数)分保持)
typedef struct DD_Fault_Table{
	int			nl_id;			//ネットリストID
	int			ftype;			//故障タイプ{SF0, SF1}
	int			n_det;			//検出回数(いくつのテストパターンで検出できたのか)
	int			det_flag;		//二重検出での検出状態{YES(検出), NO(未検出)}
}DDFTBL;


//---------------------------------------------------------------------
// 静的変数
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : double_detection
//  機  能 : 二重検出法
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void double_detection(){

	//未完成に見える 未使用変数max_det, max_essential, max_unessentialをコメントアウト
	int			i;
//  int			max_det = n_detect;		//初期TPで検出可能故障数(代表故障のみ : n_detectのコピー)
	//int			undetect;				//未検出故障数
	//int			red_tp;					//冗長なテストパターン数

//	int			max_essential=0;		//最大検出必須故障数
//	int			max_unessential=0;		//最大検出故障数(必須故障以外)

//	int			essential_tp;			//必須故障検出可能TP数

	int			*tp_select;				//テストパターン選択テーブル

	BIT_INT		**dddic_sa0;			//二重検出用の0縮退故障の故障辞書(1:故障検出　0:故障未検出)
	BIT_INT		**dddic_sa1;			//二重検出用の1縮退故障の故障辞書(1:故障検出　0:故障未検出)
	DDFTBL		*dd_f_tbl;				//二重検出用の検出故障テーブル


	//================================================
	// 初期化
	//================================================
	fprintf(stderr,"\n\n START Double_Detection!\n");

	//--------------------------------------
	//二重検出用の故障辞書メモリ確保
	//--------------------------------------
	dddic_sa0 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	dddic_sa1 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		dddic_sa0[i] = Alloc_Bit_INT((unsigned int)n_net);		//ヨコ
		dddic_sa1[i] = Alloc_Bit_INT((unsigned int)n_net);		//ヨコ
	}
		
	//--------------------------------------
	// 構造体メモリ確保
	//--------------------------------------
	dd_tbl = (DDTBL*)malloc(sizeof(DDTBL) * n_tp);			//二重検出用のテストパターン管理テーブル
	dd_f_tbl = (DDFTBL*)malloc(sizeof(DDFTBL) * n_detect);	//二重検出用の検出故障管理テーブル

	
	//--------------------------------------
	// テストパターン選択テーブル　メモリ確保
	//--------------------------------------
	tp_select = (int*)malloc(sizeof(int) * n_tp);


	
	//================================================
	// 構造体内部初期化
	//================================================
	for(i=0; i<n_tp; i++){

		//■二重検出用のテストパターン管理テーブル初期化
		dd_tbl[i].tp_id = i;			//ID
		dd_tbl[i].select = YES;			//テストパターン選択状態(初期化)
		dd_tbl[i].n_detect_fault=0;		//検出故障数(必須故障以外)
		dd_tbl[i].n_essential_fault=0;

		//■テストパターン選択テーブル初期化
		tp_select[i] = YES;

	}

	//■二重検出用の検出故障管理テーブル初期化
	for(i=0; i<n_net; i++){

		//0縮退故障の場合
		if(nl[i].test_sf0==YES && nl[i].det_sf0>0){
			dd_f_tbl[i].nl_id = i;
			dd_f_tbl[i].ftype = SAF0;
			dd_f_tbl[i].det_flag = NO;
			dd_f_tbl[i].n_det = 0;
		}

		//1縮退故障の場合
		if(nl[i].test_sf1==YES &&nl[i].det_sf1>0){
			dd_f_tbl[i].nl_id = i;
			dd_f_tbl[i].ftype = SAF1;
			dd_f_tbl[i].det_flag = NO;
			dd_f_tbl[i].n_det = 0;
		}
	}
	
	
	//================================================
	// 各テーブルの検出故障数等初期化
	//================================================


	/*
	//================================================
	// テストパターン選択テーブルの初期化
	//================================================
	for(i=0; i<n_tp; i++){
			
		for(j=0; j<n_net; j++){
				
			//-----------------------------------------
			//0縮退故障の辞書確認
			//-----------------------------------------
			if( Get_NBit_INT(fdic_sa0[i], j) == 1 ){
				//代表故障
				if(nl[j].test_sf0 == YES){
					//必須故障
					if(nl[j].det_sf0 == 1){
						dd_tbl[i].n_essential_fault++;
					}
					//必須故障以外
					else{
						dd_tbl[i].n_detect_fault++;
					}
				}
			}
			//-----------------------------------------
			//1縮退故障の辞書確認
			//-----------------------------------------
			if( Get_NBit_INT(fdic_sa1[i], j) == 1 ){
				//代表故障
				if(nl[j].test_sf1 == YES){
					//必須故障
					if(nl[j].det_sf1 == 1){
						dd_tbl[i].n_essential_fault++;
					}
					//必須故障以外
					else{
						dd_tbl[i].n_detect_fault++;
					}
				}
			}
		}

	}




#ifdef DEBUG
	printf("\n\n二重検出法テーブル初期値 (最大検出:%d)\n", max_det);
	for(i=0; i<n_tp; i++){
		printf("tp[%d] 必須故障:%d 必須以外:%d\n", i, dd_tbl[i].n_essential_fault, dd_tbl[i].n_detect_fault);
	}
	printf("\n");
#endif

	//================================================
	// 二重検出法
	//================================================
	while(1){	//無限ループ
		
		//-----------------------------------------------
		// 初期化
		//-----------------------------------------------
		printf("*");
#ifdef DEBUG
		printf("\n");
#endif
		undetect = max_det;			//検出故障数

		//ネットリスト初期化
		for(i=0; i<n_net; i++){
			nl[i].dd_det_sf0 = 0;
			nl[i].dd_det_sf1 = 0;
		}

		//故障辞書初期化
		for(i=0; i<n_tp; i++){
			All_INT_Zero(dddic_sa0[i]);		//ヨコ
			All_INT_Zero(dddic_sa1[i]);		//ヨコ
		}

		//最大検出必須故障数探索
		max_essential = 0;
		max_unessential = 0;
		essential_tp = 0;
		for(i=0; i<n_tp; i++){
			if(tp_select[i] == YES){
				//必須故障
				if(max_essential < dd_tbl[i].n_essential_fault){
					max_essential = dd_tbl[i].n_essential_fault;
				}
				//必須故障以外
				if(max_unessential < dd_tbl[i].n_detect_fault){
					max_unessential = dd_tbl[i].n_detect_fault;
				}
				//必須故障検出可能の場合
				if(dd_tbl[i].n_essential_fault > 0){
					essential_tp++;
				}
			}
		}

#ifdef DEBUG
		printf("最大必須故障:%d  必須故障検出可能TP数:%d  必須以外の最大:%d\n", max_essential, essential_tp, max_unessential);
#endif
		//選択テーブル初期化
		for(i=0;i<n_tp;i++)tp_select[i] = NO;


		//-----------------------------------------------
		// 二重検出
		//-----------------------------------------------
		//■必須故障TP選択
		while(max_essential != 0 && essential_tp != 0){	//探索目標必須故障数が無くなるまでループ
			
			//探索目標必須故障数のテストパターンが存在する場合
			if(ess_count[max_essential] > 0){
				
				for(i=n_tp-1; i>0; i--){

					//残っているテストパターン && 必須故障数が一致
					if(dd_tbl[i].select==YES && dd_tbl[i].n_essential_fault==max_essential){

						//テストパターン選択
						tp_select[i] = YES;
						essential_tp--;					//必須故障検出可能TPの残数更新
						ess_count[max_essential]--;		//探索目標必須故障数のテストパターンを更新
#ifdef DEBUG
						printf("TP[%d]を選択\n", i);
#endif

						//故障辞書更新
						for(j=0; (unsigned int)j<dddic_sa0[i]->int_num; j++){
							dddic_sa0[i]->flag[j] = fdic_sa0[i]->flag[j];	//0縮退故障辞書のコピー
							dddic_sa1[i]->flag[j] = fdic_sa1[i]->flag[j];	//1縮退故障辞書のコピー
						}

						//未検出故障更新
						for(j=0; j<n_net; j++){
							//-----------------------------------
							// 代表故障の0縮退
							//-----------------------------------
							if(nl[j].test_sf0 == YES){
								if( Get_NBit_INT(dddic_sa0[i], j) == YES ){
									//未検出故障だった場合
									if(nl[j].dd_det_sf0 == 0){
										undetect--;			//未検出故障数デクリメント
									}
									nl[j].dd_det_sf0++;		//検出故障数インクリメント
								}
							}
							//-----------------------------------				
							// 代表故障の1縮退
							//-----------------------------------
							if(nl[j].test_sf1 == YES){
								if( Get_NBit_INT(dddic_sa1[i], j) == YES ){
									//未検出故障だった場合
									if(nl[j].dd_det_sf1 == 0){
										undetect--;			//未検出故障数デクリメント
									}
									nl[j].dd_det_sf1++;		//検出故障数インクリメント
								}
							}
						}
					}

					//未検出故障==0 or 必須故障検出可能TP数==0 なら終了
					if(undetect==0 || essential_tp==0){
						max_essential=1;	//その後のデクリメントで0になる予定
						break;
					}

					//入れ以上探索しても目標必須故障数のTPが出てこない
					if(ess_count[max_essential]==0){
						break;
					}
				}
			}

			//探索目標必須故障数のデクリメント
			max_essential--;
		}

		printf("-");
		//■必須故障以外のTP選択 その1(検出回数多い順選択)
		while(undetect != 0){	//未検出故障が無くなるまでループ

			for(i=n_tp-1; i>0; i--){
				//残っているテストパターン && 必須故障数が一致
				if(dd_tbl[i].select==YES && dd_tbl[i].n_detect_fault==max_unessential && tp_select[i]==NO){

					//テストパターン選択
					tp_select[i] = YES;
					det_count[max_unessential]--;
#ifdef DEBUG
					printf("TP[%d]を選択\n", i);
#endif

					//故障辞書更新
					for(j=0; (unsigned int)j<dddic_sa0[i]->int_num; j++){
						dddic_sa0[i]->flag[j] = fdic_sa0[i]->flag[j];	//0縮退故障辞書のコピー
						dddic_sa1[i]->flag[j] = fdic_sa1[i]->flag[j];	//1縮退故障辞書のコピー
					}

					//未検出故障更新
					for(j=0; j<n_net; j++){
						//-----------------------------------
						// 代表故障の0縮退
						//-----------------------------------
						if(nl[j].test_sf0 == YES){
							if( Get_NBit_INT(dddic_sa0[i], j) == YES ){
								//未検出故障だった場合
								if(nl[j].dd_det_sf0 == 0){
									undetect--;			//未検出故障数デクリメント
								}
								nl[j].dd_det_sf0++;		//検出故障数インクリメント
							}
						}
						//-----------------------------------				
						// 代表故障の1縮退
						//-----------------------------------
						if(nl[j].test_sf1 == YES){
							if( Get_NBit_INT(dddic_sa1[i], j) == YES ){
								//未検出故障だった場合
								if(nl[j].dd_det_sf1 == 0){
									undetect--;			//未検出故障数デクリメント
								}
								nl[j].dd_det_sf1++;		//検出故障数インクリメント
							}
						}
					}
				}

				//これ以上探索しても無駄な場合
				if(det_count[max_unessential] == 0){
					break;
				}

				//未検出故障==0なら終了
				if(undetect==0){
					break;
				}
			}

			//探索目標必須故障数のデクリメント
			max_unessential--;
		}


		//■必須故障以外のTP選択 その2(TP下から選択)
		i=n_tp-1;

		while(undetect != 0){//未検出故障が無くなるまでループ

			//残っているテストパターンのみ && 必須故障TP以外 対象
			if(dd_tbl[i].select==YES && tp_select[i]==NO){

				//選択
				tp_select[i] = YES;
#ifdef DEBUG
				printf("TP[%d]を選択\n", i);
#endif

				//故障辞書更新
				for(j=0; j<dddic_sa0[i]->int_num; j++){
					dddic_sa0[i]->flag[j] = fdic_sa0[i]->flag[j];	//0縮退故障辞書のコピー
					dddic_sa1[i]->flag[j] = fdic_sa1[i]->flag[j];	//1縮退故障辞書のコピー
				}

				//未検出故障更新
				for(j=0; j<n_net; j++){
					//-----------------------------------
					// 代表故障の0縮退
					//-----------------------------------
					if(nl[j].test_sf0 == YES){
						if( Get_NBit_INT(dddic_sa0[i], j) == YES ){
							//未検出故障だった場合
							if(nl[j].dd_det_sf0 == 0){
								undetect--;			//未検出故障数デクリメント
							}
							nl[j].dd_det_sf0++;		//検出故障数インクリメント
						}
					}
					//-----------------------------------				
					// 代表故障の1縮退
					//-----------------------------------
					if(nl[j].test_sf1 == YES){
						if( Get_NBit_INT(dddic_sa1[i], j) == YES ){
							//未検出故障だった場合
							if(nl[j].dd_det_sf1 == 0){
								undetect--;			//未検出故障数デクリメント
							}
							nl[j].dd_det_sf1++;		//検出故障数インクリメント
						}
					}
				}
			}

			//次のテストパターンへ移行
			i--;
		}

		//■テストパターン削除
		red_tp = 0;
		for(i=0; i<n_tp; i++){
			if(dd_tbl[i].select != tp_select[i]){
				dd_tbl[i].select = tp_select[i];	//必要TP情報更新
				red_tp++;							//TP削除数更新
			}
		}

		//-----------------------------------------------
		// 終了判定
		//-----------------------------------------------
		//テストパターンがこれ以上減らない
		if(red_tp == 0){
			break;	
		}
		//減った場合
		else{

			//検出回数カウンタ初期化
			for(i=0; i<n_tp; i++){
				ess_count[i] = 0;
				det_count[i] = 0;
			}

			//dd_tbl[]内の必須故障数と検出故障数を計算
			for(i=0; i<n_tp; i++){

				//初期化
				dd_tbl[i].n_essential_fault	= 0;
				dd_tbl[i].n_detect_fault	= 0;

				//選択テストパターンのみ探索
				if(tp_select[i] == YES){
					for(j=0; j<n_net; j++){
						//-----------------------------------------
						//0縮退故障の辞書確認
						//-----------------------------------------
						if( Get_NBit_INT(dddic_sa0[i], j) == YES ){
							//代表故障
							if(nl[j].test_sf0 == YES){
								//必須故障
								if(nl[j].dd_det_sf0 == 1){
									dd_tbl[i].n_essential_fault++;
								}
								//必須故障以外
								else{
									dd_tbl[i].n_detect_fault++;
								}
							}
						}
						//-----------------------------------------
						//1縮退故障の辞書確認
						//-----------------------------------------
						if( Get_NBit_INT(dddic_sa1[i], j) == YES ){
							//代表故障
							if(nl[j].test_sf1 == YES){
								//必須故障
								if(nl[j].dd_det_sf1 == 1){
									dd_tbl[i].n_essential_fault++;
								}
								//必須故障以外
								else{
									dd_tbl[i].n_detect_fault++;
								}
							}
						}
					}

					//故障数カウントテーブル更新
					ess_count[dd_tbl[i].n_essential_fault]++;
					det_count[dd_tbl[i].n_detect_fault]++;
#ifdef DEBUG
					printf("tp[%d] 必須故障:%d 必須以外:%d\n", i, dd_tbl[i].n_essential_fault, dd_tbl[i].n_detect_fault);
#endif
				}
			}
#ifdef DEBUG
			printf("\n");
#endif
		}

	}	//while(1)

	*/
}


//----------------------------------------------
//  関数名 : dd_sort
//  機  能 : 二重検出法で使用するソート関数
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------

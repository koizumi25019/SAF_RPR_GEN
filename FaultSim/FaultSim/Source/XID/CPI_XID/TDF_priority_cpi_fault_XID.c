//------------------------------------------------------------------------
//File name : TDF_priority_cpi_fault_XID.c
//Date : 2013/12/9
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			TDF_2v_SPPFP				(int, SXID_LIST**, int, unsigned int);
void			TDF_3v_SPPFP				(int, SXID_LIST**, int, unsigned int);
unsigned int	TDF_priority_2v_xfilling	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*, unsigned int);
unsigned int	TDF_priority_3v_xfilling	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*, unsigned int);
int				TDF_3v_XID_SPPFP_FSIM		(int, unsigned int*, SXID_LIST**);
void			TDF_make_cpi_falult_list	(unsigned int);

SXID_LIST		*pop_sxid_list				();
void			delete_sxid_list			();
void			sort_cpi_tp					();


//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define TP_DEBUG
	//#define DEBUG
	//#define DEBUG_FLT

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_priority_cpi_fault_XID
//  機  能 : ★優先パスモード【遷移故障】制御ポイントコーン内故障の優先ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱), priroty(優先パスフラグ)
//----------------------------------------------
void	TDF_priority_cpi_fault_XID(SXID_LIST **t_fault, unsigned int priority){

	int				i,j;
	unsigned int	ed_flag=2;								//イベントドリブンフラグ
	unsigned int	xid_flag=2;								//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;								//ドントケア抽出用フラグ(限定正当化に使用)
	int				count=0;								//
//	int				min_xid;								//各テストパターンのXID故障数の最小値 //結果を未使用のため削除
	int				min_tp;									//min_xidのテストパターンID
	SXID_LIST		*temp;


#ifdef DEBUG
	int				k;

	printf("\n//======================================\n");
	printf("// CPI_corn_fault X抽出\n");
	printf("//======================================\n");
	printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);
	printf("xid_flag: %d〜\n", xid_flag);	
#endif

	//printf("\n検出故障数:%d\n", n_detect);for(i=0; i<n_tp; i++)printf("tp[%d]: 必須:%d 検出:%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);

	//===================================================================
	// フラグ初期化
	//===================================================================
	for(i=0; i<n_snet; i++){
		s_nl[i].flag = 0;
		s_nl[i].jus_flag = 0;
		s_nl[i].xid_flag = 0;
	}	
	
	//===================================================================
	// 制御ポイントコーン内故障リストの作成
	//===================================================================
	TDF_make_cpi_falult_list(priority);


	//===================================================================
	// 制御ポイントコーン内故障数の計算
	//===================================================================
#ifdef DEBUG
	//テストパターンごとに計算
	for(i=0; i<n_tp; i++){

		//初期化
		n_cpi_corn_fault = 0;

		//探索
		for(j=0; j<n_snet; j++){
			//立上り遷移故障
			if(s_nl[j].test_str==YES && s_nl[j].det_str>0 && s_nl[j].pri_path==priority){
				if( Get_NBit_INT(fdic_str[i], j)==YES ){	//対象TPで検出するか？
					n_cpi_corn_fault++;
				}
			}

			//立下り遷移故障
			if(s_nl[j].test_stf==YES && s_nl[j].det_stf>0 && s_nl[j].pri_path==priority){
				if( Get_NBit_INT(fdic_stf[i], j)==YES ){	//対象TPで検出するか？
					n_cpi_corn_fault++;
				}
			}
		}
		printf("tp[%d]: %d故障\n", i, n_cpi_corn_fault);
	}
#endif
	

	n_cpi_corn_fault = 0;
	for(i=0; i<n_snet; i++){
		//立上り遷移故障
		if(s_nl[i].test_str==YES && s_nl[i].det_str>0 && s_nl[i].pri_path==priority){
			
			//CPIコーン内故障数の更新
			n_cpi_corn_fault++;

			//各テストパターンでのCPIコーン内故障の検出数を調査
			for(j=0; j<n_tp; j++){
				if(Get_NBit_INT(fdic_str[j], i) == YES){
					tp_info[j].n_cpi_fault++;
				}
			}
		}

		//立下り遷移故障
		if(s_nl[i].test_stf==YES && s_nl[i].det_stf>0 && s_nl[i].pri_path==priority){
			
			//CPIコーン内故障数の更新
			n_cpi_corn_fault++;

			//各テストパターンでのCPIコーン内故障の検出数を調査
			for(j=0; j<n_tp; j++){
				if(Get_NBit_INT(fdic_stf[j], i) == YES){
					tp_info[j].n_cpi_fault++;
				}
			}
		}
	}

#ifdef DEBUG_FLT
	printf("\nCPIコーン内故障数: %d\n", n_cpi_corn_fault);
#endif
	
	//===================================================================
	// 制御ポイントコーン内故障数でのテストパターンのソート
	//===================================================================
	sort_cpi_tp();


	//===================================================================
	// 制御ポイントコーン内故障の優先ドントケア抽出
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Undetected Faults (制御ポイント故障優先)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
	
	while((temp=pop_sxid_list()) != NULL){
		
		//----------------------------------------------------------------------------
		//■立上り遷移故障
		//----------------------------------------------------------------------------
		if(temp->fault_type==TDF_STR && temp->net->xid_det_str==NO && temp->net->n_str_xid<opt.n_xid){

			//------------------------------------------------
			//初期化
			//------------------------------------------------
//			min_xid = n_snet*2; //代入結果未使用のため削除
			
			//カウンタ更新
			count++;
			
			//------------------------------------------------
			//一番XID故障数が少ないテストパターンを探索
			//------------------------------------------------
			/*
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_str[j], temp->net->n) == YES){
					if(tp_info[j].n_xid_fault < min_xid){
						min_xid = tp_info[j].n_xid_fault;	//最小XID故障数更新
						min_tp = j;							//最小XID故障数のテストパターンID更新
					}
				}

				//最小XID故障数==0の場合はbreak
				if(min_xid == 0){
					break;
				}
			}
			*/
			//***************************************************************
			//一番初めのパターンを探索
			/*
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_str[j], temp->net->n) == YES){
					min_tp = j;
					break;
				}
			}
			*/
			//***************************************************************
			//【ソート情報利用】沢山検出できるパターンを探索
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_str[(sort_tp[j]->tp_id)], temp->net->n) == YES){
					min_tp = sort_tp[j]->tp_id;
					break;
				}
			}
			//***************************************************************
			
			
			//------------------------------------------------
			//ドントケア抽出
			//------------------------------------------------
			//テストパターンごとのXID故障数更新
			tp_info[min_tp].n_xid_fault++;

			//対象信号線立上り遷移故障のX抽出実行回数更新
			temp->net->n_str_xid++;
			
			//32個目の対象故障を対象故障リストへ追加
			t_fault[0]->net			= temp->net;		//信号線名
			t_fault[0]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)
				
			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				//SPPFP故障SIM
				TDF_2v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス2値XID
				xid_flag = TDF_priority_2v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
			}
			//【3値FSIM】&【X抽出】
			else{
				//SPPFP故障SIM
				TDF_3v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス3値XID
				xid_flag = TDF_priority_3v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
							
			}

			//故障設置フラグ初期化
			t_fault[0]->net->nbit_fault = -1;

			//EDフラグ更新
			ed_flag+=2;
			jus_flag+=3;
			
			//------------------------------------------------
			//X抽出後テストパターンでFSIM(偶発的故障検出)
			//------------------------------------------------
			TDF_3v_XID_SPPFP_FSIM(min_tp, &ed_flag, t_fault);		//X抽出対象故障を更新
		}

		//----------------------------------------------------------------------------
		//■立下り遷移故障
		//----------------------------------------------------------------------------
		else if(temp->fault_type==TDF_STF && temp->net->xid_det_stf==NO && temp->net->n_stf_xid<opt.n_xid){

			//------------------------------------------------
			//初期化
			//------------------------------------------------
//			min_xid = n_snet*2; //代入結果未使用のため削除
			
			//カウンタ更新
			count++;
			
			//------------------------------------------------
			//一番XID故障数が少ないテストパターンを探索
			//------------------------------------------------
			/*
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_stf[j], temp->net->n) == YES){
					if(tp_info[j].n_xid_fault < min_xid){
						min_xid = tp_info[j].n_xid_fault;	//最小XID故障数更新
						min_tp = j;							//最小XID故障数のテストパターンID更新
					}
				}

				//最小XID故障数==0の場合はbreak
				if(min_xid == 0){
					break;
				}
			}
			*/
			//***************************************************************
			/*
			//一番初めのパターンを探索
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_stf[j], temp->net->n) == YES){
					min_tp = j;
					break;
				}
			}
			*/
			//***************************************************************
			//【ソート情報利用】沢山検出できるパターンを探索
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_stf[(sort_tp[j]->tp_id)], temp->net->n) == YES){
					min_tp = sort_tp[j]->tp_id;
					break;
				}
			}
			//***************************************************************
			
			
			//------------------------------------------------
			//ドントケア抽出
			//------------------------------------------------
			//テストパターンごとのXID故障数更新
			tp_info[min_tp].n_xid_fault++;

			//対象信号線立下り遷移故障のX抽出実行回数更新
			temp->net->n_stf_xid++;
			
			//32個目の対象故障を対象故障リストへ追加
			t_fault[0]->net			= temp->net;		//信号線名
			t_fault[0]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)
				
			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				//SPPFP故障SIM
				TDF_2v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス2値XID
				xid_flag = TDF_priority_2v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
			}
			//【3値FSIM】&【X抽出】
			else{
				//SPPFP故障SIM
				TDF_3v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス3値XID
				xid_flag = TDF_priority_3v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
							
			}

			//故障設置フラグ初期化
			t_fault[0]->net->nbit_fault = -1;

			//EDフラグ更新
			ed_flag+=2;
			jus_flag+=3;
			
			//------------------------------------------------
			//X抽出後テストパターンでFSIM(偶発的故障検出)
			//------------------------------------------------
			TDF_3v_XID_SPPFP_FSIM(min_tp, &ed_flag, t_fault);		//X抽出対象故障を更新

		}
	}


	//printf("\n検出故障数:%d\n", n_detect);for(i=0; i<n_tp; i++)printf("tp[%d]: 必須:%d 検出:%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);
	printf("%d回/%d故障 回XID実行\n", count, n_cpi_corn_fault);


	
	//===============================================
	// XID後の検出故障数を計算
	//===============================================
	for(i=0; i<n_tp; i++){

		//カウンタ初期化
		count = 0;

		//検出故障数計算
		for(j=0; j<n_snet; j++){

			//■立上り遷移故障
			if(s_nl[j].test_str==YES && s_nl[j].xid_det_str==YES){
			}

			//■立下り遷移故障
			if(s_nl[j].test_stf==YES && s_nl[j].xid_det_stf==YES){
			}
		}

	}

	//===============================================
	// 故障リストスタックの解放
	//===============================================
	delete_sxid_list();


	//===============================================
	// DEBUG：XID後のテストパターン確認
	//===============================================
#ifdef TP_DEBUG
	printf("\n//======================================\n");
	printf("// XID後のテストパターン確認\n");
	printf("//======================================\n");
	for(i=0; i<n_tp; i++){
		printf("tp[%d]\t", i);
		
		//初期テストパターンPI
		for(j=0; j<n_spi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(spi[j]->nval_t1, i);
			}
			//3値
			else{
				k = Get_NBit(spi[j]->nval_t1, i);
			}

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}
		
		//初期テストパターンPPI
		for(j=0; j<n_ppi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(ppi[j]->nval_t1, i);
			}
			//3値
			else{
				k = Get_NBit(ppi[j]->nval_t1, i);
			}

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}

		printf(" -> ");

		//X抽出後テストパターンPI
		for(j=0; j<n_spi; j++){
			k = Get_NBit(spi[j]->xid_nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}

		//X抽出後テストパターンPPI
		for(j=0; j<n_ppi; j++){
			k = Get_NBit(ppi[j]->xid_nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}
		printf("\n");
	}


#endif


}

/*
//------------------------------------------------------------------------
//File name : TDF_priority_cpi_fault_XID.c
//Date : 2013/12/9
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			TDF_2v_SPPFP				(int, SXID_LIST**, int, unsigned int);
void			TDF_3v_SPPFP				(int, SXID_LIST**, int, unsigned int);
unsigned int	TDF_priority_2v_xfilling	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*, unsigned int);
unsigned int	TDF_priority_3v_xfilling	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*, unsigned int);
int				TDF_3v_XID_SPPFP_FSIM		(int, unsigned int*, SXID_LIST**);
void			TDF_make_cpi_falult_list	(unsigned int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define TP_DEBUG
	//#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_priority_cpi_fault_XID
//  機  能 : ★優先パスモード【遷移故障】制御ポイントコーン内故障の優先ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱), priroty(優先パスフラグ)
//----------------------------------------------
void	TDF_priority_cpi_fault_XID(SXID_LIST **t_fault, unsigned int priority){

	int				i,j;
	unsigned int	ed_flag=2;								//イベントドリブンフラグ
	unsigned int	xid_flag=2;								//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;								//ドントケア抽出用フラグ(限定正当化に使用)
	int				count=0;								//
	int				min_xid;								//各テストパターンのXID故障数の最小値
	int				min_tp;									//min_xidのテストパターンID

#ifdef DEBUG
	int				k;

	printf("\n//======================================\n");
	printf("// CPI_corn_fault X抽出\n");
	printf("//======================================\n");
	printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);
	printf("xid_flag: %d〜\n", xid_flag);	
#endif

	//printf("\n検出故障数:%d\n", n_detect);for(i=0; i<n_tp; i++)printf("tp[%d]: 必須:%d 検出:%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);

	//===================================================================
	// フラグ初期化
	//===================================================================
	for(i=0; i<n_snet; i++){
		s_nl[i].flag = 0;
		s_nl[i].xid_flag = 0;
	}	
	
	//===================================================================
	// 制御ポイントコーン内故障リストの作成
	//===================================================================
	TDF_make_cpi_falult_list(priority);


	//===================================================================
	// 制御ポイントコーン内故障数の計算
	//===================================================================
#ifdef DEBUG
	//テストパターンごとに計算
	for(i=0; i<n_tp; i++){

		//初期化
		n_cpi_corn_fault = 0;

		//探索
		for(j=0; j<n_snet; j++){
			//立上り遷移故障
			if(s_nl[j].test_str==YES && s_nl[j].det_str>0 && s_nl[j].pri_path==priority){
				if( Get_NBit_INT(fdic_str[i], j)==YES ){	//対象TPで検出するか？
					n_cpi_corn_fault++;
				}
			}

			//立下り遷移故障
			if(s_nl[j].test_stf==YES && s_nl[j].det_stf>0 && s_nl[j].pri_path==priority){
				if( Get_NBit_INT(fdic_stf[i], j)==YES ){	//対象TPで検出するか？
					n_cpi_corn_fault++;
				}
			}
		}
		printf("tp[%d]: %d故障\n", i, n_cpi_corn_fault);
	}
#endif
	

	n_cpi_corn_fault = 0;
	for(i=0; i<n_snet; i++){
		//立上り遷移故障
		if(s_nl[i].test_str==YES && s_nl[i].det_str>0 && s_nl[i].pri_path==priority){
			n_cpi_corn_fault++;
		}

		//立下り遷移故障
		if(s_nl[i].test_stf==YES && s_nl[i].det_stf>0 && s_nl[i].pri_path==priority){
			n_cpi_corn_fault++;
		}
	}

	printf("\nCPIコーン内故障数: %d\n", n_cpi_corn_fault);


	//===================================================================
	// 制御ポイントコーン内故障の優先ドントケア抽出
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Undetected Faults (制御ポイント故障優先)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	for(i=0; i<n_snet; i++){

		printf("%d回目/%d\r", i+1, n_snet);

		//----------------------------------------------------------------------------
		//■立上り遷移故障
		//----------------------------------------------------------------------------
		if(s_nl[i].test_str==YES && s_nl[i].det_str>0 && s_nl[i].pri_path==priority && s_nl[i].xid_det_str==NO && s_nl[i].n_str_xid<opt.n_xid){
			
			//------------------------------------------------
			//初期化
			//------------------------------------------------
			min_xid = n_snet*2;
			
			//カウンタ更新
			count++;
			
			//------------------------------------------------
			//一番XID故障数が少ないテストパターンを探索
			//------------------------------------------------
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_str[j], i) == YES){
					if(tp_info[j].n_xid_fault < min_xid){
						min_xid = tp_info[j].n_xid_fault;	//最小XID故障数更新
						min_tp = j;							//最小XID故障数のテストパターンID更新
					}
				}

				//最小XID故障数==0の場合はbreak
				if(min_xid == 0){
					break;
				}
			}
			
			//------------------------------------------------
			//ドントケア抽出
			//------------------------------------------------
			//テストパターンごとのXID故障数更新
			tp_info[min_tp].n_xid_fault++;

			//対象信号線立上り遷移故障のX抽出実行回数更新
			s_nl[i].n_str_xid++;
			
			//32個目の対象故障を対象故障リストへ追加
			t_fault[0]->net			= &s_nl[i];		//信号線名
			t_fault[0]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)
				
			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				//SPPFP故障SIM
				TDF_2v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス2値XID
				xid_flag = TDF_priority_2v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
			}
			//【3値FSIM】&【X抽出】
			else{
				//SPPFP故障SIM
				TDF_3v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス3値XID
				xid_flag = TDF_priority_3v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
							
			}

			//故障設置フラグ初期化
			t_fault[0]->net->nbit_fault = -1;

			//EDフラグ更新
			ed_flag+=2;
			jus_flag+=3;
			
			//------------------------------------------------
			//X抽出後テストパターンでFSIM(偶発的故障検出)
			//------------------------------------------------
			TDF_3v_XID_SPPFP_FSIM(min_tp, &ed_flag, t_fault);		//X抽出対象故障を更新

		}
		
		//----------------------------------------------------------------------------
		//■立下り遷移故障
		//----------------------------------------------------------------------------
		if(s_nl[i].test_stf==YES && s_nl[i].det_stf>0 && s_nl[i].pri_path==priority && s_nl[i].xid_det_stf==NO && s_nl[i].n_stf_xid<opt.n_xid){
			
			//------------------------------------------------
			//初期化
			//------------------------------------------------
			min_xid = n_snet*2;
			
			//カウンタ更新
			count++;
			
			//------------------------------------------------
			//一番XID故障数が少ないテストパターンを探索
			//------------------------------------------------
			for(j=0; j<n_tp; j++){
				//対象テストパターンで検出可能
				if(Get_NBit_INT(fdic_stf[j], i) == YES){
					if(tp_info[j].n_xid_fault < min_xid){
						min_xid = tp_info[j].n_xid_fault;	//最小XID故障数更新
						min_tp = j;							//最小XID故障数のテストパターンID更新
					}
				}

				//最小XID故障数==0の場合はbreak
				if(min_xid == 0){
					break;
				}
			}
			
			//------------------------------------------------
			//ドントケア抽出
			//------------------------------------------------
			//テストパターンごとのXID故障数更新
			tp_info[min_tp].n_xid_fault++;

			//対象信号線立下り遷移故障のX抽出実行回数更新
			s_nl[i].n_stf_xid++;
			
			//32個目の対象故障を対象故障リストへ追加
			t_fault[0]->net			= &s_nl[i];		//信号線名
			t_fault[0]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)
				
			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				//SPPFP故障SIM
				TDF_2v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス2値XID
				xid_flag = TDF_priority_2v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
			}
			//【3値FSIM】&【X抽出】
			else{
				//SPPFP故障SIM
				TDF_3v_SPPFP(min_tp, t_fault, 1, ed_flag);
							
				//優先パス3値XID
				xid_flag = TDF_priority_3v_xfilling(min_tp, t_fault, 1, ed_flag, xid_flag, &jus_flag, priority);
							
			}

			//故障設置フラグ初期化
			t_fault[0]->net->nbit_fault = -1;

			//EDフラグ更新
			ed_flag+=2;
			jus_flag+=3;
			
			//------------------------------------------------
			//X抽出後テストパターンでFSIM(偶発的故障検出)
			//------------------------------------------------
			TDF_3v_XID_SPPFP_FSIM(min_tp, &ed_flag, t_fault);		//X抽出対象故障を更新

		}
	}

	printf("\n検出故障数:%d\n", n_detect);for(i=0; i<n_tp; i++)printf("tp[%d]: 必須:%d 検出:%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);
	printf("%d回/%d故障 回XID実行\n", count, n_cpi_corn_fault);

	//===============================================
	// DEBUG：XID後のテストパターン確認
	//===============================================
#ifdef TP_DEBUG
	printf("\n//======================================\n");
	printf("// XID後のテストパターン確認\n");
	printf("//======================================\n");
	for(i=0; i<n_tp; i++){
		printf("tp[%d]\t", i);
		
		//初期テストパターンPI
		for(j=0; j<n_spi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(spi[j]->nval_t1, i);
			}
			//3値
			else{
				k = Get_NBit(spi[j]->nval_t1, i);
			}

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}
		
		//初期テストパターンPPI
		for(j=0; j<n_ppi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(ppi[j]->nval_t1, i);
			}
			//3値
			else{
				k = Get_NBit(ppi[j]->nval_t1, i);
			}

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}

		printf(" -> ");

		//X抽出後テストパターンPI
		for(j=0; j<n_spi; j++){
			k = Get_NBit(spi[j]->xid_nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}

		//X抽出後テストパターンPPI
		for(j=0; j<n_ppi; j++){
			k = Get_NBit(ppi[j]->xid_nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}
		printf("\n");
	}


#endif


}
*/
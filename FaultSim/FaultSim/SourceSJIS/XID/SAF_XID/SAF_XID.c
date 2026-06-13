//------------------------------------------------------------------------
//File name : SAF_XID.c
//Date : 2011/7/11
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Xidentification.h"
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	SAF_3v_XID_PPSFP_FSIM				();
void	SAF_make_xid_falult_list					();
void	Make_SAF_Missed_XID_List			();
void	SAF_search_influence_corn			();
void	SAF_Essential_XID					(XID_LIST**);
void	SAF_EX_XID							(XID_LIST**);
void	SAF_m2004_XID						(XID_LIST**);
void	SAF_m2008_XID						(XID_LIST**);
void	SAF_HD_XID							(XID_LIST**);
void	SAF_diff_path_XID					(XID_LIST**);
void	SAF_Missed_XID						(XID_LIST**);

void	FPointer_SAF_Fault_Path				();
void	FPointer_SAF_Justification			();
void	FPointer_SAF_3v_Justification		();
void	FPointer_SAF_2v_SPPFP				();
void	FPointer_SAF_3v_SPPFP				();
void	FPointer_SAF_3v_XID_SPPFP			();
void	FPointer_SAF_3v_XID_ED_PPSFP		();

void	check_missed_fault					();
//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_XID
//  機  能 : 縮退故障ドントケア抽出
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	SAF_XID(){
	
	int			i;
	XID_LIST	**t_fault;		//故障リスト配列(MAX32故障)

	
	//====================================================
	// 初期化とメモリ確保
	//====================================================	
	//---------------------------------------------------------
	//32個分の故障リストメモリ確保
	//---------------------------------------------------------
	t_fault = (XID_LIST**)malloc(sizeof(XID_LIST*) * 32);
	for(i=0; i<32; i++)t_fault[i] = (XID_LIST*)malloc(sizeof(XID_LIST));

	//---------------------------------------------------------
	//テストパターンソートテーブルのメモリ確保
	//---------------------------------------------------------
	sort_tp = (TPINFO**)malloc(sizeof(TPINFO*) * n_tp);

	//---------------------------------------------------------
	//SPPFPで各POで検出した故障した故障情報を保持する構造体のメモリ確保
	//---------------------------------------------------------
	propa_po = (XID_PROPA_PO*)malloc(sizeof(XID_PROPA_PO) * n_po);	//PO数分メモリ確保

	//外部出力いろいろ初期化
	for(i=0; i<n_po; i++){
		propa_po[i].po_id = i;						//POのID値を代入
		propa_po[i].n_det = 0;						//初期化
		nl[po[i]->n].po_id = i;						//po[i]のi番目情報を保持
		propa_po[i].det_flag = Alloc_Bit_INT(32);	//メモリ確保
	}

	//ドントケア抽出後のテストパターンの正常値計算用メモリ確保
	for(i=0; i<n_net; i++)nl[i].xid_nval = Alloc_Bit_TP(n_tp);
	

	//---------------------------------------------------------
	//X抽出後TPのテストパターンごとのビット情報構造体メモリ確保
	//---------------------------------------------------------
	tp_bit = (TPBIT*)malloc(sizeof(TPBIT) * n_tp);
	
	//初期化(オールドントケア状態)
	for(i=0; i<n_tp; i++){
		tp_bit[i].tp_id = i;
		tp_bit[i].ui_num = i / (8 * sizeof(unsigned int)) ;	//MASKのunsigned int何番目か
		tp_bit[i].bit_num = i % (8 * sizeof(unsigned int));	//MASKの何bit目か;
		tp_bit[i].n_cbit = 0;
		tp_bit[i].n_0bit = 0;
		tp_bit[i].n_1bit = 0;
	}
	
	//---------------------------------------------------------
	//X抽出後TPの外部入力ごとのビット情報構造体メモリ確保
	//---------------------------------------------------------
	pi_bit = (PIBIT*)malloc(sizeof(PIBIT) * n_pi);
	
	//初期化(オールドントケア状態)
	for(i=0; i<n_pi; i++){
		//外部入力のケアビット情報構造体
		pi_bit[i].pi_id = i;
		pi_bit[i].n_cbit = 0;
		pi_bit[i].n_0bit = 0;
		pi_bit[i].n_1bit = 0;

		//PIのIDもついでに初期化
		pi[i]->pi_id = i;
	}

		
	//---------------------------------------------------------
	//X抽出後TPの故障検出情報構造体メモリ確保
	//---------------------------------------------------------
	tp_info = (TPINFO*)malloc(sizeof(TPINFO) * n_tp);

	//初期化
	for(i=0; i<n_tp; i++){
		//テストパターン情報初期化
		tp_info[i].tp_id = i;
		tp_info[i].ui_num = i / (8 * sizeof(unsigned int)) ;	//MASKのunsigned int何番目か
		tp_info[i].bit_num = i % (8 * sizeof(unsigned int));	//MASKの何bit目か
		tp_info[i].n_xid_fault = 0;								//XIDした故障数
		tp_info[i].n_essential_fault = 0;						//必須故障数
		tp_info[i].n_HD_fault = 0;								//検出困難故障数
		tp_info[i].n_many_carebit_fault = 0;					//検出するのに多ケアビットが必要だった故障数
		tp_info[i].n_xid_det = 0;								//XID後パターンでの検出故障数
	}

	//---------------------------------------------------------
	// X抽出後テスト集合の故障辞書のメモリ確保
	//---------------------------------------------------------
	xid_fdic_sa0 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	xid_fdic_sa1 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		xid_fdic_sa0[i] = Alloc_Bit_INT(n_net);		//ヨコ
		xid_fdic_sa1[i] = Alloc_Bit_INT(n_net);		//ヨコ
	}
		
	//---------------------------------------------------------
	// 各FFRの検出故障数を計算(※1故障1カウント)
	//---------------------------------------------------------
	for(i=0; i<n_net; i++){
		//0縮退
		if(nl[i].test_sf0==YES && nl[i].det_sf0>0){
			ffr[nl[i].ffr_id].n_det_fault++;
		}
		//1縮退
		if(nl[i].test_sf1==YES && nl[i].det_sf1>0){
			ffr[nl[i].ffr_id].n_det_fault++;
		}
	}

	//====================================================
	// 各POの影響範囲内PI数をカウント
	//====================================================
	SAF_search_influence_corn();

	
	//====================================================
	// 関数ポインタセット
	//====================================================
	FPointer_SAF_Fault_Path();				//故障伝搬関数

	FPointer_SAF_3v_XID_SPPFP();			//X抽出後テストパターン用の3値FSIM(SPPFP)
	FPointer_SAF_3v_XID_ED_PPSFP();			//X抽出後テスト集合用の3値FSIM(PPSFP)
	FPointer_SAF_Justification();			//正当化関数

	if(n_before_x == 0){
		FPointer_SAF_2v_SPPFP();			//2値-SPPFP
	}
	else{
		FPointer_SAF_3v_SPPFP();			//3値-SPPFP
	}
	


	//********************************************************************
	// ★ドントケア抽出★
	//********************************************************************
	//====================================================
	// 必須故障ドントケア抽出
	//====================================================
	SAF_Essential_XID(t_fault);				//必須故障ドントケア抽出
	SAF_3v_XID_PPSFP_FSIM();				//3値故障シミュレーション


	//====================================================
	// 検出困難故障ドントケア抽出
	//====================================================
	if(opt.M_HD_tp != 0){
		SAF_HD_XID(t_fault);				//検出困難故障ドントケア抽出
		SAF_3v_XID_PPSFP_FSIM();			//3値故障シミュレーション
	}


	//====================================================
	// ノーマルドントケア抽出(Miyase2004)
	//====================================================
	// X抽出モードオプションが全てNOの場合
	if(opt.miyase2004 == YES){	
		SAF_m2004_XID(t_fault);		//Miyase2004
	}
	
	//====================================================
	// ドントケア抽出(モード別処理)
	//====================================================
	else{		
		//----------------------------------------------------
		// X抽出対象故障リスト作成 (未検出故障をリスト化)
		//----------------------------------------------------
		SAF_make_xid_falult_list();
		
		//----------------------------------------------------
		// 未検出故障X抽出(Miyase2008)
		//----------------------------------------------------
		if (opt.miyase2008 == YES){
			//SAF_m2008_XID(t_fault);		//Miyase2008
			printf("Miyase2008 XID is not supported anymore.\n");
			exit(1);
		}

	}


	//====================================================
	// 見逃し故障X抽出
	//====================================================
	//見逃し故障数計算
	n_missed_fault = n_detect - n_xid_detect;

	//未検出故障が存在する場合
	if(n_missed_fault > 0){
		
		//見逃し故障チェック
		if(opt.miss_list == YES){
			check_missed_fault();
		}

		//故障リスト更新
		//Make_SAF_Missed_XID_List();
		
		//--------------------------------------------------
		// 通常の見逃し故障X抽出
		//--------------------------------------------------
		if(opt.diff_path_n_xid > 0){

			SAF_diff_path_XID(t_fault);									//異パスXID
			
			n_dpxid_fault = n_missed_fault - (n_detect - n_xid_detect);	//異パスXIDで検出できた故障数
			n_missed_fault = n_detect - n_xid_detect;					//見逃し故障数再計算
		}

		//--------------------------------------------------
		// 通常の見逃し故障X抽出(拡張含意)
		//--------------------------------------------------
		if(n_missed_fault > 0){
			SAF_EX_XID(t_fault);
			//SAF_Missed_XID(t_fault);
		}
	}
}
//------------------------------------------------------------------------
//File name : CPI_XID.c
//Date : 2013/11/15
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Xidentification.h"
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			TDF_3v_XID_PPSFP_FSIM				();
void			Make_TDF_Missed_XID_List			();
unsigned int	TDF_search_influence_corn			();
void			TDF_Essential_XID					(SXID_LIST**);
void			TDF_EX_XID							(SXID_LIST**);
void			TDF_m2004_XID						(SXID_LIST**);
void			TDF_m2008_XID						(SXID_LIST**);
void			TDF_HD_XID							(SXID_LIST**);
void			TDF_diff_path_XID					(SXID_LIST**);
void			TDF_Missed_XID						(SXID_LIST**);
void			TDF_priority_Essential_XID			(SXID_LIST**, unsigned int);
void			TDF_priority_m2004_XID				(SXID_LIST**, unsigned int);
void			TDF_priority_m2008_XID				(SXID_LIST**, unsigned int);
void			TDF_priority_cpi_fault_XID			(SXID_LIST**, unsigned int);
void			LSIM_only_cpi_xidnval				(unsigned int);


void			FPointer_TDF_Priority_and_Cost_Justification();
void			FPointer_TDF_Priority_Cost_Fault_Path		();
void			FPointer_TDF_Priority_Fault_Path			();
void			FPointer_TDF_Fault_Path						();
void			FPointer_TDF_Justification					();
void			FPointer_TDF_Priority_Justification			();
void			FPointer_TDF_2v_SPPFP						();
void			FPointer_TDF_3v_SPPFP						();
void			FPointer_TDF_3v_XID_SPPFP					();
void			FPointer_TDF_3v_XID_ED_PPSFP				();
void			Make_FP_XID_LSIM_3v_sequential				();

void			check_missed_fault							();
void			sort_priority_ppo							(unsigned int);
void			make_fault_tbl								(unsigned int);
void			XID_LSIM_3v_sequential						();

void			LSIM_2v_sequential();
//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG
	//#define	INITIAL_XID_MODE		//疑似的な初期回路XID

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : CPI_XID
//  機  能 : 【遷移故障】制御ポイント挿入用X抽出
//  戻り値 : なし
//  引  数 : pri_flag(優先パスフラグ)
//----------------------------------------------
void	CPI_XID(unsigned int pri_flag){


	int				i;
	SXID_LIST		**t_fault;		//故障リスト配列(MAX32故障)
	unsigned int	flag;
#ifdef DEBUG
	int				j;
#endif
		
	//====================================================
	// 初期化とメモリ確保
	//====================================================	
	//---------------------------------------------------------
	//故障テーブルのメモリ確保
	//---------------------------------------------------------
	flt_stbl = (S_NLIST**)malloc(sizeof(S_NLIST*) * n_snet);
	
	//---------------------------------------------------------
	//32個分の故障リストメモリ確保
	//---------------------------------------------------------
	t_fault = (SXID_LIST**)malloc(sizeof(SXID_LIST*) * 32);
	for(i=0; i<32; i++)t_fault[i] = (SXID_LIST*)malloc(sizeof(SXID_LIST));
	
	//---------------------------------------------------------
	//テストパターンソートテーブルのメモリ確保
	//---------------------------------------------------------
	sort_tp = (TPINFO**)malloc(sizeof(TPINFO*) * n_tp);

	//---------------------------------------------------------
	//SPPFPで各PPOで検出した故障した故障情報を保持する構造体のメモリ確保
	//---------------------------------------------------------
	propa_po = (XID_PROPA_PO*)malloc(sizeof(XID_PROPA_PO) * n_ppo);	//PPO数分メモリ確保

	//外部出力いろいろ初期化
	for(i=0; i<n_ppo; i++){
		propa_po[i].po_id = i;						//PPOのID値を代入
		propa_po[i].n_det = 0;						//初期化
		s_nl[ppo[i]->n].ppo_id = i;					//ppo[i]のi番目情報を保持
		propa_po[i].det_flag = Alloc_Bit_INT(32);	//メモリ確保
	}

	//ドントケア抽出後のテストパターンの正常値計算用メモリ確保
	for(i=0; i<n_snet; i++){
		s_nl[i].xid_nval_t1 = Alloc_Bit_TP(n_tp);
		s_nl[i].xid_nval_t2 = Alloc_Bit_TP(n_tp);
	}

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
	//X抽出後TPの外部入力と疑似外部入力ごとのビット情報構造体メモリ確保
	//---------------------------------------------------------
	pi_bit = (PIBIT*)malloc(sizeof(PIBIT) * n_spi);
	ppi_bit = (PIBIT*)malloc(sizeof(PIBIT) * n_ppi);
	
	//---------------------------------
	//PI初期化(オールドントケア状態)
	//---------------------------------
	for(i=0; i<n_spi; i++){
		//外部入力のケアビット情報構造体
		pi_bit[i].pi_id = i;
		pi_bit[i].n_cbit = 0;
		pi_bit[i].n_0bit = 0;
		pi_bit[i].n_1bit = 0;

		//PIのIDもついでに初期化
		spi[i]->pi_id = i;
	}
	
	//---------------------------------
	//PPI初期化(オールドントケア状態)
	//---------------------------------
	for(i=0; i<n_ppi; i++){
		//疑似外部入力のケアビット情報構造体
		ppi_bit[i].pi_id = i;
		ppi_bit[i].n_cbit = 0;
		ppi_bit[i].n_0bit = 0;
		ppi_bit[i].n_1bit = 0;

		//PPIのIDもついでに初期化
		ppi[i]->ppi_id = i;
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
		tp_info[i].n_cpi_fault = 0;								//制御ポイント挿入範囲内故障数
		tp_info[i].n_xid_det = 0;								//XID後パターンでの検出故障数
	}

	//---------------------------------------------------------
	// X抽出後テスト集合の故障辞書のメモリ確保
	//---------------------------------------------------------
	xid_fdic_str = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	xid_fdic_stf = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		xid_fdic_str[i] = Alloc_Bit_INT(n_snet);		//ヨコ
		xid_fdic_stf[i] = Alloc_Bit_INT(n_snet);		//ヨコ
	}
		
	//---------------------------------------------------------
	// 各FFRの検出故障数を計算(※1故障1カウント)
	//---------------------------------------------------------
	for(i=0; i<n_snet; i++){
		//立上り遷移故障
		if(s_nl[i].test_str==YES && s_nl[i].det_str>0){
			sffr[s_nl[i].ffr_id].n_det_fault++;
		}
		//立下り遷移故障
		if(s_nl[i].test_stf==YES && s_nl[i].det_stf>0){
			sffr[s_nl[i].ffr_id].n_det_fault++;
		}
	}

	//====================================================
	// 各PPOの影響範囲内PI+PPI数をカウント(PPOソート)
	//====================================================
	flag = TDF_search_influence_corn();
	

	//====================================================
	// CPI_XID優先パス情報でPPOソート
	//====================================================
	if(opt.priority_path == YES){

		//PPOソート
		sort_priority_ppo(pri_flag);
				
		//優先パスより故障テーブル作成
		make_fault_tbl(pri_flag);
	}
	

	//====================================================
	// 関数ポインタセット
	//====================================================
	//------------------------------------
	// 優先パスXIDモードの関数ポインタセット
	//------------------------------------
	if(opt.priority_path == YES){
		
		FPointer_TDF_Priority_and_Cost_Justification();
		FPointer_TDF_Priority_Cost_Fault_Path		();

		//FPointer_TDF_Priority_Fault_Path();		//故障伝搬関数
		//FPointer_TDF_Priority_Justification();	//優先パス正当化
	}
	//------------------------------------
	// 通常モード(最小レベル選択)の関数ポインタセット
	//------------------------------------
	else{
		FPointer_TDF_Fault_Path();				//故障伝搬関数
		FPointer_TDF_Justification();			//最小レベル正当化
	}
	
	//------------------------------------
	// X抽出後用のFSIM関数の関数ポインタセット
	//------------------------------------
	FPointer_TDF_3v_XID_SPPFP();			//X抽出後テストパターン用の3値FSIM(SPPFP)
	FPointer_TDF_3v_XID_ED_PPSFP();			//X抽出後テスト集合用の3値FSIM(PPSFP)
	
	//------------------------------------
	// 初期テスト集合のSPPFP関数の関数ポインタセット
	//------------------------------------
	if(n_before_x == 0){
		FPointer_TDF_2v_SPPFP();			//2値-SPPFP
	}
	else{
		FPointer_TDF_3v_SPPFP();			//3値-SPPFP
	}
	
	//------------------------------------
	// 全信号線のxid_nval値でLSIM関数の関数ポインタセット
	//------------------------------------
	Make_FP_XID_LSIM_3v_sequential();

	
	//====================================================
	// 強制的に疑似初期回路XIDモード
	//====================================================
#ifdef INITIAL_XID_MODE
	//ちょい改造(CPI回路で初期回路XIDをする)
	for(i=0; i<n_ffc; i++){
		//----------------------------------------
		//FFC=1の箇所を0に戻す
		//----------------------------------------
		if(Get_NBit(ffc[i]->nval_t1, 0) == 1){
			for(j=0; j<ffc[i]->nval_t1->int_num; j++){
				ffc[i]->nval_t1->x_buf[j] = 0;				//オール0
				ffc[i]->nval_t1->p_buf[j] = 0xFFFFFFFF;		//オール1
			}
		}
		
		//----------------------------------------
		//ついでにCPもXに戻す
		//----------------------------------------
		for(j=0; j<ffc[i]->nval_t2->int_num; j++){
			// CP(PI) = DFF
			ffc[i]->link_cpi[0]->link_cpi[0]->nval_t1->x_buf[j] = 0xFFFFFFFF;		//オール1
			ffc[i]->link_cpi[0]->link_cpi[0]->nval_t1->p_buf[j] = 0xFFFFFFFF;		//オール1
			ffc[i]->link_cpi[0]->link_cpi[0]->nval_t2->x_buf[j] = 0xFFFFFFFF;		//オール1
			ffc[i]->link_cpi[0]->link_cpi[0]->nval_t2->p_buf[j] = 0xFFFFFFFF;		//オール1
		}
	}
	//----------------------------------------
	//初期テスト集合で再LSIM
	//----------------------------------------
	LSIM_2v_sequential();
#endif


	//====================================================
	// 制御ポイント部のxid_nvalでLSIM
	//====================================================
	flag += 4;
	LSIM_only_cpi_xidnval(flag);

	
	//********************************************************************
	//====================================================
	//★xid_nval値でLSIM
	//====================================================
	/*
	//CPの値設置
	for(i=0; i<n_cp; i++){
		for(j=0; j<cp[i]->xid_nval_t1->int_num; j++){
			cp[i]->xid_nval_t1->x_buf[j] = cp[i]->nval_t1->x_buf[j];
			cp[i]->xid_nval_t1->p_buf[j] = cp[i]->nval_t1->p_buf[j];
			cp[i]->xid_nval_t2->x_buf[j] = cp[i]->nval_t2->x_buf[j];
			cp[i]->xid_nval_t2->p_buf[j] = cp[i]->nval_t2->p_buf[j];
		}
	}

	//LSIM
	XID_LSIM_3v_sequential();
	*/
	//********************************************************************
	
	
	//********************************************************************
	// ★ドントケア抽出★
	//********************************************************************
	//====================================================
	// 制御御ポイントコーン内故障の優先ドントケア抽出
	//====================================================
	if(opt.pri_xid == YES){
		TDF_priority_cpi_fault_XID(t_fault, pri_flag);	
	}

	//====================================================
	// 必須故障ドントケア抽出
	//====================================================
	if(opt.priority_path == YES){
		TDF_priority_Essential_XID(t_fault, pri_flag);		//優先パス必須故障ドントケア抽出
	}
	else{
		TDF_Essential_XID(t_fault);						//必須故障ドントケア抽出
	}

	TDF_3v_XID_PPSFP_FSIM();						//3値故障シミュレーション


	//====================================================
	// 未検出故障ドントケア抽出
	//====================================================
	//--------------------------------------
	// Miyase2004モード
	//--------------------------------------
	if(opt.miyase2004 == YES){
		//優先パスXID
		if(opt.priority_path == YES){
			TDF_priority_m2004_XID(t_fault, pri_flag);
		}
		//ノーマルXID
		else{
			TDF_m2004_XID(t_fault);
		}
	}
	//--------------------------------------
	// Miyase2008モード
	//--------------------------------------
	else if(opt.miyase2008 == YES){
		//優先パスXID
		if(opt.priority_path == YES){
			TDF_priority_m2008_XID(t_fault, pri_flag);
		}
		//ノーマルXID
		else{
			TDF_m2008_XID(t_fault);
		}
	}

	//printf("\n検出故障数:%d\n", n_detect);
	//for(i=0; i<n_tp; i++)printf("tp[%d]: 必須:%d 検出:%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);


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

		//--------------------------------------------------
		// 通常の見逃し故障X抽出(拡張含意)
		//--------------------------------------------------
		if(n_missed_fault > 0){
			TDF_EX_XID(t_fault);
		}
	}
	

#ifdef DEBUG
	//PI
	printf("■PI\n");
	for(i=0; i<n_spi; i++){
		printf("%s 初期(%d, %d) XID(", spi[i]->name, Get_NBit_Xbuf(spi[i]->nval_t1, 1), Get_NBit_Xbuf(spi[i]->nval_t2, 1));
		//1時刻目
		j = Get_NBit(spi[i]->xid_nval_t1, 1);
		if(j == 3){
			printf("X, ");
		}
		else{
			printf("%d, ", j);
		}
		//2時刻目
		j = Get_NBit(spi[i]->xid_nval_t2, 1);
		if(j == 3){
			printf("X)");
		}
		else{
			printf("%d)", j);
		}
		printf("\n");
	}

	//PPI
	printf("■PPI\n");
	for(i=0; i<n_ppi; i++){
		printf("%s 初期(%d, %d) XID(", ppi[i]->name, Get_NBit_Xbuf(ppi[i]->nval_t1, 1), Get_NBit_Xbuf(ppi[i]->nval_t2, 1));
		//1時刻目
		j = Get_NBit(ppi[i]->xid_nval_t1, 1);
		if(j == 3){
			printf("X, ");
		}
		else{
			printf("%d, ", j);
		}
		//2時刻目
		j = Get_NBit(ppi[i]->xid_nval_t2, 1);
		if(j == 3){
			printf("X)");
		}
		else{
			printf("%d)", j);
		}
		printf("\n");
	}
#endif

}
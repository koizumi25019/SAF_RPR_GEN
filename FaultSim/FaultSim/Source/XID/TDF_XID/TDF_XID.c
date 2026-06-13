//------------------------------------------------------------------------
//File name : TDF_XID.c
//Date : 2013/10/10
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
void			TDF_bind_fault_tp_ppo_XID			(char**, SXID_LIST**);
void			TDF_distribution_XID				();
void			TDF_make_falult_list				();

void			FPointer_TDF_Fault_Path				();
void			FPointer_TDF_Long_Fault_Path		();
void			FPointer_TDF_Cost_Fault_Path		();
void			FPointer_TDF_Justification			();
void			FPointer_TDF_Long_Justification		();
void			FPointer_TDF_Cost_Justification		();
void			FPointer_TDF_2v_SPPFP				();
void			FPointer_TDF_3v_SPPFP				();
void			FPointer_TDF_3v_XID_SPPFP			();
void			FPointer_TDF_3v_XID_ED_PPSFP		();

void			Make_FP_Dis_LSIM_2v_sequential		();
//void			Make_FP_Dis_LSIM_3v_sequential		();
void			Make_FP_TDF_2v_dis_ED_PPSFP			();
//void			Make_FP_TDF_3v_dis_ED_PPSFP			();

void			check_missed_fault					();
//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_XID
//  機  能 : 遷移故障ドントケア抽出
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	TDF_XID(char	**file_names){


	int			i;
	SXID_LIST	**t_fault;		//故障リスト配列(MAX32故障)
#ifdef DEBUG
	int			j;
#endif
	
	//====================================================
	// 初期化とメモリ確保
	//====================================================
	//---------------------------------------------------------
	// 分散制御XID用の正常値格納変数のメモリ確保
	//---------------------------------------------------------
	if(opt.pi_vari==YES || opt.pi_x==YES || opt.tp_vari==YES || opt.tp_x==YES){
		for(i=0; i<n_snet; i++){
			s_nl[i].dis_nval_t1 = Alloc_Bit_TP(32);
			s_nl[i].dis_nval_t2 = Alloc_Bit_TP(32);
		}
	}

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
	// 各PPOの影響範囲内PI+PPI数をカウント
	//====================================================
	TDF_search_influence_corn();

	
	//====================================================
	// 関数ポインタセット
	//====================================================
	//------------------------------------------
	// 故障伝搬関数セット
	//------------------------------------------
	//ゲート段数の小さい順に選択
	if(opt.xid_fpath == LEVEL){
		FPointer_TDF_Fault_Path();
	}

	//ゲート段数の大きい順に選択
	else if(opt.xid_fpath == MAX_LEVEL){
		FPointer_TDF_Long_Fault_Path();
	}

	//可制御性コストの小さい順に選択
	else if(opt.xid_fpath == COST){
		FPointer_TDF_Cost_Fault_Path();
	}

	//------------------------------------------
	// 正当化関数セット
	//------------------------------------------
	//ゲート段数の小さい順に選択
	if(opt.xid_jus == LEVEL){
		FPointer_TDF_Justification();
	}

	//ゲート段数の大きい順に選択
	else if(opt.xid_jus == MAX_LEVEL){
		FPointer_TDF_Long_Justification();
	}

	//可制御性コストの小さい順に選択
	else if(opt.xid_jus == COST){
		FPointer_TDF_Cost_Justification();
	}
	
	//------------------------------------------
	// 故障シミュレーター関数セット
	//------------------------------------------
	FPointer_TDF_3v_XID_SPPFP();			//X抽出後テストパターン用の3値FSIM(SPPFP)
	FPointer_TDF_3v_XID_ED_PPSFP();			//X抽出後テスト集合用の3値FSIM(PPSFP)
	
	if(n_before_x == 0){
		FPointer_TDF_2v_SPPFP();			//2値-SPPFP
	}
	else{
		FPointer_TDF_3v_SPPFP();			//3値-SPPFP
	}
	

	//********************************************************************
	// ★ドントケア抽出★
	//********************************************************************
	//====================================================
	// 特殊ドントケア抽出
	//====================================================
	//故障とパターンと検出PPOの指定XID
	if(opt.xid_flt_tp_ppo == YES){

		//故障とパターンと検出PPO指定ドントケア抽出
		TDF_bind_fault_tp_ppo_XID(file_names, t_fault);

		//X抽出後のテスト集合で3値FSIM
		TDF_3v_XID_PPSFP_FSIM();
	}

	//故障とパターンの指定XID


	//====================================================
	// 必須故障ドントケア抽出
	//====================================================
	TDF_Essential_XID(t_fault);				//必須故障ドントケア抽出

	//X抽出後のテスト集合で3値FSIM
	TDF_3v_XID_PPSFP_FSIM();

	
	//====================================================
	// 未検出故障ドントケア抽出
	//====================================================
	//-------------------------------------------------
	// 未検出故障リストの作成
	//-------------------------------------------------
	TDF_make_falult_list();

	//-------------------------------------------------
	// ノーマルドントケア抽出(Miyase2004)
	//-------------------------------------------------
	if(opt.miyase2004 == YES){	
		TDF_m2004_XID(t_fault);
	}
	
	//-------------------------------------------------
	// 検出故障数均一化(Miyase2008)
	//-------------------------------------------------
	else if(opt.miyase2008 == YES){
		TDF_m2008_XID(t_fault);
	}

	//-------------------------------------------------
	// 分散制御XID (PI分散，PI分散とX数，TP分散，TP分散とX数)
	//-------------------------------------------------
	else if(opt.pi_vari==YES || opt.pi_x==YES || opt.tp_vari==YES || opt.tp_x==YES){

		//分散制御XID用の2値関数ポインタセット
		if(n_before_x == 0){
			Make_FP_Dis_LSIM_2v_sequential();	//2値LSIM関数ポインタ
			Make_FP_TDF_2v_dis_ED_PPSFP();		//2値FSIM関数ポインタ
		}
		else{
			//Make_FP_Dis_LSIM_3v_sequential();	//3値LSIM関数ポインタ
			//Make_FP_TDF_3v_dis_ED_PPSFP();	//3値FSIM関数ポインタ
		}

		//分散制御XID
		TDF_distribution_XID();
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
		//Make_TDF_Missed_XID_List();
		
		//--------------------------------------------------
		// 通常の見逃し故障X抽出
		//--------------------------------------------------
		/*if(opt.diff_path_n_xid > 0){

			TDF_diff_path_XID(t_fault);									//異パスXID
			
			n_dpxid_fault = n_missed_fault - (n_detect - n_xid_detect);	//異パスXIDで検出できた故障数
			n_missed_fault = n_detect - n_xid_detect;					//見逃し故障数再計算
		}
		*/
		//--------------------------------------------------
		// 通常の見逃し故障X抽出(拡張含意)
		//--------------------------------------------------
		if(n_missed_fault > 0){
			TDF_EX_XID(t_fault);
			//TDF_Missed_XID(t_fault);
		}
	}
	

	//printf("tp[207] %s: (%d, %d)\n", s_nl[1824].name, Get_NBit(s_nl[1824].xid_nval_t1, 207), Get_NBit(s_nl[1824].xid_nval_t2, 207));


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
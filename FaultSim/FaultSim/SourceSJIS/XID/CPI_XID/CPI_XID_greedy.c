//------------------------------------------------------------------------
//File name : CPI_XID_greedy.c
//Date : 2013/11/24
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
void			search_priority_path						(S_NLIST*, unsigned int);
void			LSIM_only_cpi								(S_NLIST*, unsigned int);
void			file_outpuf_tdf_txt							(FILE*);

void			Greedy_XID									(SXID_LIST**, unsigned int);

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

void			TDF_3v_XID_PPSFP_FSIM				();
void			Make_TDF_Missed_XID_List			();
unsigned int	TDF_search_influence_corn			();
void			TDF_Essential_XID					(SXID_LIST**);
void			TDF_EX_XID							(SXID_LIST**);
void			TDF_m2004_XID						(SXID_LIST**);
void			TDF_HD_XID							(SXID_LIST**);
void			TDF_diff_path_XID					(SXID_LIST**);
void			TDF_Missed_XID						(SXID_LIST**);
void			TDF_priority_Essential_XID			(SXID_LIST**, unsigned int);
void			TDF_priority_m2004_XID				(SXID_LIST**, unsigned int);
void			check_missed_fault					();
void			sort_priority_ppo					(unsigned int);
void			count_xbit_rate						();
void			make_fault_tbl						(unsigned int);
void			LSIM_only_cpi_xidnval				(unsigned int);
void			Make_FP_XID_LSIM_3v_sequential		();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

	#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : CPI_XID_greedy
//  機  能 : 【遷移故障】制御ポイント挿入用X抽出(制御ポイント挿入箇所探索WRTLT2012ver)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	CPI_XID_greedy(){

	int				i,j;
	unsigned int	k;
	FILE			*tp;				//X抽出後テスト集合ファイルポインタ
	FILE			*log;				//ログファイル
	unsigned int	lsim_flag=0;		//LSIM用フラグ
	unsigned int	pri_flag=0;			//優先パスフラグ
	int				n_com_tp;			//圧縮後テストパターン数
	int				n_min_tp;			//圧縮後テストパターン数の最小値
	int				min_ffc;			//圧縮後テストパターン数が最小だったFFCのID
	int				*decision_ff;		//制御ポイント挿入に決定したFFリスト
	int				*decision_flag;		//制御ポイント挿入に決定したかのフラグ(n_ffc分メモリ確保) {YES, NO}

	SXID_LIST		**t_fault;			//XIDで使用する故障リスト配列(MAX32故障)


#ifdef DEBUG
	printf("\n//=============================================\n");
	printf("// DEBUG: CPI_XID貪欲探索モード\n");
	printf("//=============================================\n");
#endif

	//=============================================================
	// 制御ポイント挿入数計算
	//=============================================================
	n_cpi = (int)(((double)n_sdff/(double)2)*((double)opt.p_cpi/(double)100));	//FFCのせいでFF数が2倍になっているのでn_dff/2する

	if(n_cpi == 0){
		n_cpi = 1;
	}
#ifdef DEBUG
	printf("制御ポイント挿入FF数：%d (割合%d%%)\n", n_cpi, opt.p_cpi);
#endif

	
	//=============================================================
	// 初期化
	//=============================================================
	//------------------------------------------
	// 探索関係
	//------------------------------------------
	//ログファイルオープン
	if((log = fopen("log.txt","w")) == (FILE *)NULL){
		fprintf(stderr,"Cannot open Log file %s\n");
		exit(1);
	}

	//メモリ確保
	decision_ff = (int*)malloc(sizeof(int) * n_cpi);
	decision_flag = (int*)malloc(sizeof(int) * n_ffc);

	//決定済みFFリスト内を初期化
	for(i=0; i<n_cpi; i++)decision_ff[i] = -1;

	//制御ポイント挿入フラグ初期化
	for(i=0; i<n_ffc; i++)decision_flag[i] = NO;

	//全信号線フラグ初期化
	for(i=0; i<n_snet; i++){
		s_nl[i].cpi_lsim = 0;
	}

	//------------------------------------------
	// X抽出関係
	//------------------------------------------
	//故障テーブルのメモリ確保
	flt_stbl = (S_NLIST**)malloc(sizeof(S_NLIST*) * n_snet);

	//32故障リスト
	t_fault = (SXID_LIST**)malloc(sizeof(SXID_LIST*) * 32);
	for(i=0; i<32; i++)t_fault[i] = (SXID_LIST*)malloc(sizeof(SXID_LIST));

	//【以下外部変数】
	//SPPFPで各PPOで検出した故障した故障情報を保持する構造体のメモリ確保
	propa_po = (XID_PROPA_PO*)malloc(sizeof(XID_PROPA_PO) * n_ppo);

	//X抽出後TPのテストパターンごとのビット情報構造体メモリ確保
	tp_bit = (TPBIT*)malloc(sizeof(TPBIT) * n_tp);

	//X抽出後TPの外部入力と疑似外部入力ごとのビット情報構造体メモリ確保
	pi_bit = (PIBIT*)malloc(sizeof(PIBIT) * n_spi);
	ppi_bit = (PIBIT*)malloc(sizeof(PIBIT) * n_ppi);

	//X抽出後TPの故障検出情報構造体メモリ確保
	tp_info = (TPINFO*)malloc(sizeof(TPINFO) * n_tp);

	// X抽出後テスト集合の故障辞書のメモリ確保
	xid_fdic_str = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	xid_fdic_stf = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	for(i=0;i<n_tp;i++){
		xid_fdic_str[i] = Alloc_Bit_INT(n_snet);		//ヨコ
		xid_fdic_stf[i] = Alloc_Bit_INT(n_snet);		//ヨコ
	}

	//ドントケア抽出後のテストパターンの正常値計算用メモリ確保
	for(i=0; i<n_snet; i++){
		s_nl[i].xid_nval_t1 = Alloc_Bit_TP(n_tp);
		s_nl[i].xid_nval_t2 = Alloc_Bit_TP(n_tp);
	}
	
	//------------------------------------------
	// 構造体初期化
	//------------------------------------------
	//PPO関係
	for(i=0; i<n_ppo; i++){
		propa_po[i].po_id = i;						//PPOのID値を代入
		propa_po[i].n_det = 0;						//初期化
		s_nl[ppo[i]->n].ppo_id = i;					//ppo[i]のi番目情報を保持
		propa_po[i].det_flag = Alloc_Bit_INT(32);	//メモリ確保
	}

	//テストパターン関係
	for(i=0; i<n_tp; i++){
		tp_bit[i].tp_id = i;
		tp_bit[i].ui_num = i / (8 * sizeof(unsigned int)) ;	//MASKのunsigned int何番目か
		tp_bit[i].bit_num = i % (8 * sizeof(unsigned int));	//MASKの何bit目か;
	}

	//PI関係
	for(i=0; i<n_spi; i++){
		//外部入力のケアビット情報構造体
		pi_bit[i].pi_id = i;

		//PIのIDもついでに初期化
		spi[i]->pi_id = i;
	}

	//PPI関係
	for(i=0; i<n_ppi; i++){
		//疑似外部入力のケアビット情報構造体
		ppi_bit[i].pi_id = i;

		//PPIのIDもついでに初期化
		ppi[i]->ppi_id = i;
	}

	//テストパターン情報構造体
	for(i=0; i<n_tp; i++){
		//テストパターン情報初期化
		tp_info[i].tp_id = i;
		tp_info[i].ui_num = i / (8 * sizeof(unsigned int)) ;	//MASKのunsigned int何番目か
		tp_info[i].bit_num = i % (8 * sizeof(unsigned int));	//MASKの何bit目か
		tp_info[i].n_xid_fault = 0;								//検出故障数
		tp_info[i].n_essential_fault = 0;						//必須故障数
		tp_info[i].n_HD_fault = 0;								//検出困難故障数
		tp_info[i].n_many_carebit_fault = 0;					//検出するのに多ケアビットが必要だった故障数
		tp_info[i].n_cpi_fault = 0;								//制御ポイント挿入範囲内故障数
	}
	
	//------------------------------------------
	// 関数ポインタセット
	//------------------------------------------
	// 優先パスXIDモード
	if(opt.priority_path == YES){
		FPointer_TDF_Priority_Fault_Path();		//故障伝搬関数
		FPointer_TDF_Priority_Justification();	//優先パス正当化
	}
	// 通常モード(最小レベル選択)
	else{
		FPointer_TDF_Fault_Path();				//故障伝搬関数
		FPointer_TDF_Justification();			//最小レベル正当化
	}
	
	// X抽出後用のFSIM関数
	FPointer_TDF_3v_XID_SPPFP();			//X抽出後テストパターン用の3値FSIM(SPPFP)
	FPointer_TDF_3v_XID_ED_PPSFP();			//X抽出後テスト集合用の3値FSIM(PPSFP)
	
	// 初期テスト集合のSPPFP関数
	if(n_before_x == 0){
		FPointer_TDF_2v_SPPFP();			//2値-SPPFP
	}
	else{
		FPointer_TDF_3v_SPPFP();			//3値-SPPFP
	}

	// 制御ポイント部のLSIM関数
	Make_FP_XID_LSIM_3v_sequential();


	//=============================================================
	// 制御ポイント挿入数回ループ(貪欲探索)
	//=============================================================
	for(i=0; i<n_cpi; i++){
				
		printf("//=============================================\n");
		printf("// 【%d/%d】CPI_XID貪欲探索モード\n", i+1, n_cpi);
		printf("//=============================================\n");

		//-----------------------------------------------------
		// 初期化
		//-----------------------------------------------------
		//圧縮後テストパターン数の最小値(初期テストパターン数で初期化)
		n_min_tp = n_tp+1;

		
		//-----------------------------------------------------
		// 制御ポイント挿入箇所の貪欲探索(XID, Dsatur)
		//-----------------------------------------------------
		for(j=0; j<n_ffc; j++){

			//------------------------------------------
			// j番目のFFCが決定済みでない場合
			//------------------------------------------
			if(decision_flag[j] == NO){

#ifdef DEBUG
				printf("選択FFC: %s\n", ffc[j]->name);
#endif
				//------------------------------------------
				// j番目のFFCを1に設定
				//------------------------------------------
				for(k=0; k<ffc[j]->nval_t1->int_num; k++){
					ffc[j]->nval_t1->x_buf[k] = 0xFFFFFFFF;		//オール1
					ffc[j]->nval_t1->p_buf[k] = 0;				//オール0
				}

				//CPに2時刻目PPI値を代入
				for(k=0; k<ffc[j]->nval_t2->int_num; k++){
					// CP(PI) = DFF
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t1->x_buf[k] = ffc[j]->link_cpi[0]->nval_t2->x_buf[k];
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t1->p_buf[k] = ffc[j]->link_cpi[0]->nval_t2->p_buf[k];
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t2->x_buf[k] = ffc[j]->link_cpi[0]->nval_t2->x_buf[k];
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t2->p_buf[k] = ffc[j]->link_cpi[0]->nval_t2->p_buf[k];
				}

				//論理シミュレーションフラグ更新
				lsim_flag += 4;

				//論理シミュレーション
				LSIM_only_cpi(ffc[j], lsim_flag);

				//優先パスフラグ更新
				pri_flag += 2;

				//優先パス解析
				if(opt.priority_path == YES){
					//決定済みのFFC
					for(k=0; k<(unsigned int)i; k++){
						search_priority_path(ffc[(decision_ff[k])]->link_cpi[0]->link_cpi[0], pri_flag);
					}

					//今のFFC
					search_priority_path(ffc[j]->link_cpi[0]->link_cpi[0], pri_flag);
				}

				//------------------------------------------
				// X抽出
				//------------------------------------------
				//CPI_X抽出
				Greedy_XID(t_fault, pri_flag);

				//ファイルオープン
				if((tp = fopen("tp_temp.txt","w")) == (FILE *)NULL){
					fprintf(stderr,"Cannot open TP_temp file %s\n");
					exit(1);
				}


				//X抽出後テスト集合ファイル出力
				file_outpuf_tdf_txt(tp);
			
				//ファイルクローズ
				fclose(tp);

				//優先パスフラグ更新
				pri_flag += 2;

				//------------------------------------------
				// テスト圧縮(Dsatur)
				//------------------------------------------
				//Dsatur実行
				n_com_tp = system("Dsatur.exe tp_temp.txt -conti_com log_dsatur_s5378.txt");

				//テストパターン数の比較
				if(n_com_tp < n_min_tp){
					n_min_tp = n_com_tp;		//最小テストパターン数の更新
					min_ffc = j;				//最小テストパターン数のFFC_IDの更新
				}
							
				//------------------------------------------
				// ログ出力
				//------------------------------------------
				//圧縮後テストパターン数
				fprintf(log, "%d	", n_com_tp);

				//X抽出率
				fprintf(log, "%.2lf%	", ((double)n_after_x/(double)(n_tp*(n_spi+n_ppi)))*100);

				//決定済みFF名
				for(k=0; k<(unsigned int)i; k++){
					fprintf(log, "%s	", ffc[(decision_ff[k])]->link_cpi[0]->name);
				}
				
				//現在のFF名
				fprintf(log, "%s\n", ffc[j]->link_cpi[0]->name);

				//------------------------------------------
				// j番目のFFCを0に設定(元に戻す)
				//------------------------------------------
				for(k=0; k<ffc[j]->nval_t1->int_num; k++){
					ffc[j]->nval_t1->x_buf[k] = 0;				//オール0
					ffc[j]->nval_t1->p_buf[k] = 0xFFFFFFFF;		//オール1
				}

				//CP=Xに戻す
				for(k=0; k<ffc[j]->nval_t2->int_num; k++){
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t1->x_buf[k] = 0xFFFFFFFF;	//オール1;
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t1->p_buf[k] = 0xFFFFFFFF;	//オール1;
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t2->x_buf[k] = 0xFFFFFFFF;	//オール1;
					ffc[j]->link_cpi[0]->link_cpi[0]->nval_t2->p_buf[k] = 0xFFFFFFFF;	//オール1;
				}

				//論理シミュレーションフラグ更新
				lsim_flag += 4;

				//論理シミュレーション
				LSIM_only_cpi(ffc[j], lsim_flag);

			}
		}
		
		//-----------------------------------------------------
		// 圧縮後テストパターン数が最小だったFFCを保存
		//-----------------------------------------------------		
#ifdef DEBUG
		printf("最小FFC: %s (圧縮後パターン数%d)\n", ffc[min_ffc]->name, n_min_tp);
#endif

		//FFC_IDを保存
		decision_flag[min_ffc] = YES;
		decision_ff[i] = min_ffc;
		
		//今決まったFFCを1に設定
		for(k=0; k<ffc[min_ffc]->nval_t1->int_num; k++){
			ffc[min_ffc]->nval_t1->x_buf[k] = 0xFFFFFFFF;	//オール1
			ffc[min_ffc]->nval_t1->p_buf[k] = 0;			//オール0
		}

		//論理シミュレーションフラグ更新
		lsim_flag += 4;

		//論理シミュレーション
		LSIM_only_cpi(ffc[min_ffc], lsim_flag);

		//CPに2時刻目PPI値を代入
		for(k=0; k<ffc[min_ffc]->nval_t2->int_num; k++){
			// CP(PI) = DFF
			ffc[min_ffc]->link_cpi[0]->link_cpi[0]->nval_t1->x_buf[k] = ffc[min_ffc]->link_cpi[0]->nval_t2->x_buf[k];
			ffc[min_ffc]->link_cpi[0]->link_cpi[0]->nval_t1->p_buf[k] = ffc[min_ffc]->link_cpi[0]->nval_t2->p_buf[k];
			ffc[min_ffc]->link_cpi[0]->link_cpi[0]->nval_t2->x_buf[k] = ffc[min_ffc]->link_cpi[0]->nval_t2->x_buf[k];
			ffc[min_ffc]->link_cpi[0]->link_cpi[0]->nval_t2->p_buf[k] = ffc[min_ffc]->link_cpi[0]->nval_t2->p_buf[k];
		}

	}
	
	//=============================================================
	//テストパターン数最小の組合せでもう一度XID
	//=============================================================
	//------------------------------------------
	//優先パス解析
	//------------------------------------------
	pri_flag += 2;

	if(opt.priority_path == YES){
		//決定済みのFFC
		for(k=0; k<(unsigned int)n_cpi; k++){
			search_priority_path(ffc[(decision_ff[k])]->link_cpi[0]->link_cpi[0], pri_flag);
		}
	}

	//------------------------------------------
	// X抽出
	//------------------------------------------
	//CPI_X抽出
	Greedy_XID(t_fault, pri_flag);

	//ファイルオープン
	if((tp = fopen("tp_temp.txt","w")) == (FILE *)NULL){
		fprintf(stderr,"Cannot open TP_temp file %s\n");
		exit(1);
	}

	//X抽出後テスト集合ファイル出力
	file_outpuf_tdf_txt(tp);
			
	//ファイルクローズ
	fclose(tp);

	//優先パスフラグ更新
	pri_flag += 2;

	//------------------------------------------
	// テスト圧縮(Dsatur)
	//------------------------------------------
	//Dsatur実行
	n_com_tp = system("Dsatur.exe tp_temp.txt -conti_com log_dsatur_s5378.txt");

	//テストパターン数の比較
	if(n_com_tp < n_min_tp){
		n_min_tp = n_com_tp;		//最小テストパターン数の更新
		min_ffc = j;				//最小テストパターン数のFFC_IDの更新
	}

	//------------------------------------------
	// ログ出力
	//------------------------------------------
	//圧縮後テストパターン数
	fprintf(log, "\n【最小TP組合せ】\n");
	fprintf(log, "%d	", n_com_tp);

	//X抽出率
	fprintf(log, "%.2lf%	", ((double)n_after_x/(double)(n_tp*(n_spi+n_ppi)))*100);

	//決定済みFF名
	for(k=0; k<(unsigned int)i; k++){
		fprintf(log, "%s	", ffc[(decision_ff[k])]->link_cpi[0]->name);
	}
	

	//=============================================================
	//最終結果表示
	//=============================================================
	printf("\n最小圧縮後テストパターン数: %d\n", n_min_tp);


	//=============================================================
	//メモリ解放
	//=============================================================
	free(decision_ff);
	free(decision_flag);

	//ファイルクローズ
	fclose(log);
}


//----------------------------------------------
//  関数名 : Greedy_XID
//  機  能 : CPI_XID_greedy用の専用XID関数
//  戻り値 : なし
//  引  数 : t_fault(32故障格納ポインタ), pri_flag(優先パスフラグ)
//----------------------------------------------
void	Greedy_XID(SXID_LIST **t_fault, unsigned int pri_flag){

	int				i;
	unsigned int	flag;

	//====================================================
	// 初期化
	//====================================================
	//テストパターンに含まれるケアビット数初期化
	for(i=0; i<n_tp; i++){
		tp_bit[i].n_cbit = 0;
		tp_bit[i].n_0bit = 0;
		tp_bit[i].n_1bit = 0;
	}

	//PIに含まれるケアビット数初期化
	for(i=0; i<n_spi; i++){
		pi_bit[i].n_cbit = 0;
		pi_bit[i].n_0bit = 0;
		pi_bit[i].n_1bit = 0;
	}

	//PPIに含まれるケアビット数初期化
	for(i=0; i<n_ppi; i++){
		ppi_bit[i].n_cbit = 0;
		ppi_bit[i].n_0bit = 0;
		ppi_bit[i].n_1bit = 0;
	}

	//テストパターン情報構造体初期化
	for(i=0; i<n_tp; i++){
		tp_info[i].n_xid_fault = 0;							//検出故障数
		tp_info[i].n_essential_fault = 0;						//必須故障数
		tp_info[i].n_HD_fault = 0;								//検出困難故障数
		tp_info[i].n_many_carebit_fault = 0;					//検出するのに多ケアビットが必要だった故障数
	}
	
	//全信号線の変数初期化
	for(i=0; i<n_snet; i++){
		s_nl[i].xid_det_str = NO;
		s_nl[i].xid_det_stf = NO;
		s_nl[i].n_str_xid = 0;
		s_nl[i].n_stf_xid = 0;
		s_nl[i].n_str_diff_xid = 0;
		s_nl[i].n_stf_diff_xid = 0;
		s_nl[i].n_carebit = 0;

		All_Bit_X(s_nl[i].xid_nval_t1);		//全信号線のX抽出後値をXに初期化
		All_Bit_X(s_nl[i].xid_nval_t2);		//全信号線のX抽出後値をXに初期化
	}

	//外部変数初期化
	n_after_x = 0;			//ドントケア抽出後テストパターン中のドントケア数
	n_missed_fault = 0;		//見逃し故障数
	n_missed_cbit = 0;		//見逃し故障ドントケア抽出で増加したPIのケアビット数
	n_xid_detect = 0;		//X抽出後テスト集合での故障検出数(1故障1回でカウント)

	//X抽出後故障辞書初期化
	for(i=0; i<n_tp; i++){
		All_INT_Zero(xid_fdic_str[i]);
		All_INT_Zero(xid_fdic_stf[i]);
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
	// 制御ポイント部のxid_nvalでLSIM
	//====================================================
	flag += 4;
	LSIM_only_cpi_xidnval(flag);


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
	// ノーマルドントケア抽出(Miyase2004)
	//====================================================
	if(opt.priority_path == YES){
		TDF_priority_m2004_XID(t_fault, pri_flag);
	}
	else{
		TDF_m2004_XID(t_fault);
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

		//--------------------------------------------------
		// 通常の見逃し故障X抽出(拡張含意)
		//--------------------------------------------------
		if(n_missed_fault > 0){
			TDF_EX_XID(t_fault);
		}
	}
	
	//====================================================
	// X抽出率計算
	//====================================================
	count_xbit_rate();
}
//------------------------------------------------------------------------
//File name : main.c
//Date : 2011/6/13
//Designer : H.Yamazaki
//Ver : 0.01
//
//memo:
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<time.h>

#include	"./StandardHead.h"
#include	"./Netlist/netlist.h"
#include	"./Netlist/s_netlist.h"
#include	"./option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	file_open							(FILE *[], char **);
void	option								(int, char *[], char **);
int		read_nl								(char *);
int		read_snl							(char *);
void	set_ppi_ppo							();
void	search_tdf_testable_net				();
void	Initialize							();
void	read_tp_txt							(FILE*, FILE*);
void	read_tp_stil						(FILE*);
void	random_pattern_generation			();
void	read_flist							(FILE*);
void	rep_flist							();
void	search_fstem_combinational			();
void	search_fstem_sequential				();
void	levelize_combinational				();
void	levelize_sequential					();
void	search_ffr_combinational			();
void	search_ffr_sequential				();
void	search_reconv_combinational			();
void	search_reconv_sequential			();
void	rep_levelize_combinational			();
void	rep_levelize_sequential				();
void	LSIM_2v_combinational				();
void	LSIM_3v_combinational				();
void	LSIM_2v_sequential					();
void	LSIM_3v_sequential					();
void	SAF_XID								();
void	TDF_XID								(char *[]);
void	SAF_2v_PPSFP_Ndrop					();
void	SAF_3v_PPSFP_Ndrop					();
void	SAF_2v_PPSFP						();
void	SAF_3v_PPSFP						();
void	TDF_2v_PPSFP_Ndrop					();
void	TDF_3v_PPSFP_Ndrop					();
void	TDF_2v_PPSFP						();
void	TDF_3v_PPSFP						();
void	SAF_2v_repPPSFP						();
void	SAF_3v_repPPSFP						();
void	TDF_2v_repPPSFP						();
void	TDF_3v_repPPSFP						();
void	make_fsim_det_num					();
void	fault_diagnosis_fsim				(FILE*, FILE*);
void	double_detection					();
void	count_xbit_rate						();
void	comp_exp_2v_combinational			(FILE*);
void	comp_exp_3v_combinational			(FILE*);
void	comp_exp_2v_sequential				(FILE*);
void	comp_exp_3v_sequential				(FILE*);
void	read_cpi_list						(FILE*);
void	CPI_XID								(unsigned int);
void	CPI_XID_greedy						();
void	make_pointer_cpi					();
void	link_cpi_net						();
void	count_cpi_xbit_rate					();
int		run_dsatur							(char**);
void	count_dff_xbit						();
void	change_xbit							();
void	cal_cpi_utilization					();
void	initial_com_controllability			();
void	initial_seq_controllability			();
void	cal_com_controllability				();
void	cal_seq_controllability				();
void	cpi_analyze_main					(FILE*, FILE*);
void	trans_ffc_list						(FILE*, char*);
void	sort_legal_state_tp					(FILE*, FILE*, char*[]);
void	find_reachable_state				(FILE*, FILE*);


//ファイル出力系
void	file_outpuf_xid_test_set			(FILE*, FILE*, FILE*);
void	file_output_stil_to_txt				(FILE*, FILE*);
void	file_output_fdictionary				(FILE*);
void	file_output_ndetect					(FILE*);
void	file_output_carebit_analysis		(FILE*);
void	file_output_detect_hd_fault			(FILE*);
void	file_output_double_dtection_tp		(FILE*);
void	file_output_undetect				(FILE*);
void	file_output_xid_log					(FILE*);
void	file_output_miss_log				(FILE*);
void	file_output_add_log					(FILE*, char*, double);
void	file_output_log						(FILE*, char**, double);
void	file_output_expectation_value		(FILE*);
void	file_output_po_pin					(FILE*);
void	file_output_lsim					(FILE*);
void	file_output_pin						(FILE*);
void	file_outpuf_missed_fault			(FILE*);
void	file_output_all_net_value			(FILE*);
void	file_output_rep_flist				(FILE*);
void	file_outpuf_cpi_xid_test_set		(FILE*, FILE*);
void	file_output_time2_tp				(FILE*, char *[]);

//DEBUG関数形
void	DEBUG_SAF_SPPFP						();
void	DEBUG_TDF_SPPFP						();
void	debug_output_min_level_net			(FILE* fp);
//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
//#pragma warning ( disable : 4996 )

	#define	DEBUG
	//#define	COMP_DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------
static	clock_t	cput;


//--------------------------------------------------------------------
//関数名：main
//機能　：X抽出 main関数
//引数　：なし
//戻り値：なし
//--------------------------------------------------------------------
int main(int argc, char *argv[]){
	
	FILE		*fp[N_FILE];
	char		*file_names[N_FILE];

	//==============================================================================
	// CPUタイム計測開始
	//==============================================================================
	cput = clock();
#ifdef DEBUG
	printf("//===============================================\n");
	printf("// XID : DEBUG-mode\n");
	printf("//===============================================\n");
	printf("START CPU-Time\n");
#endif
	
	
	//==============================================================================
	// コマンドライン処理
	//==============================================================================
	//オプション設定
	option(argc, argv, file_names);
#ifdef DEBUG
	printf("Completion of Option_Setting\n");
#endif

	// ファイルのオープン
	file_open(fp,file_names);
#ifdef DEBUG
	printf("Completion of File_Open\n");
#endif
	
	//==============================================================================
	// ネットリスト読み込み
	//==============================================================================
	//【縮退故障】組合せ回路の場合
	if(opt.fmodel == SAF){

		//組合せ回路用のNetlist配列作成
		read_nl(file_names[F_NET]);

		//WARNING
		if(n_dff > 0){
			printf("\n//-------------------------------\n");
			printf("// WARNING !! \n");
			printf("//-------------------------------\n");
			printf("組合せ回路・縮退故障なのにDFFが存在します！\n");
			printf("入力回路か故障モデル指定コマンド(-fm)を確認\n");
			exit(-1);
		}
	}

	//【遷移故障】順序回路の場合
	else if(opt.fmodel == TDF){

		//順序回路用のNetlist配列作成
		read_snl(file_names[F_NET]);
		
		//★PPIとPPOポインタ配列作成
		set_ppi_ppo();

		//WARNING
		if(n_sdff == 0){
			printf("\n//-------------------------------\n");
			printf("// WARNING !! \n");
			printf("//-------------------------------\n");
			printf("順序回路・遷移故障なのにDFFが存在しません！\n");
			printf("入力回路か故障モデル指定コマンド(-fm)を確認\n");
			exit(-1);
		}
	}
	
	//==============================================================================
	// 初期化
	//==============================================================================
	//外部変数等の初期化
	Initialize();
#ifdef DEBUG
	printf("Completion of Initialize\n");
#endif
	
	//==============================================================================
	// 構造的テスト可能遷移故障信号線判定
	//==============================================================================
	//PPIからのコーンとPPOからのコーンを抽出
	if(opt.fmodel == TDF){
		search_tdf_testable_net();

#ifdef DEBUG
		printf("Completion of testable transition fault\n");
#endif

	}

	//==============================================================================
	// テストパターン読み込み
	//==============================================================================
	//.txtファイル形式
	if(opt.tp_txt == YES){
		read_tp_txt(fp[F_TPTX], fp[F_PIN]);
	}

	//.stilファイル形式
	else if(opt.tp_stil == YES){
		read_tp_stil(fp[F_TPST]);
	}

	//ランダムパターン生成
	if(opt.rpg > 0){
		random_pattern_generation();
	}
	
#ifdef DEBUG
	printf("Completion of Read_Test_Pattern_File\n");
#endif
	
	//==============================================================================
	// 故障設置
	//==============================================================================
	//故障リスト読み込み
	if(opt.flt==YES){
		read_flist(fp[F_FLT]);
	}
	//代表故障
	else{
		rep_flist();
	}
#ifdef DEBUG
	printf("Completion of Faults_Set\n");
#endif
	
	//==============================================================================
	// レベライズ
	//==============================================================================
	//--------------------------------
	//組合せ回路用レベライズ
	//--------------------------------
	if(opt.fmodel == SAF){
		
		//ノーマルレベライズ
		levelize_combinational();
#ifdef DEBUG
	printf("Completion of Normal_Levelize\n");
#endif

		//代表信号線レベライズ
		if(opt.rep_levelize == YES){
			rep_levelize_combinational();
#ifdef DEBUG
	printf("Completion of Rep_Levelize\n");
#endif
		}
	}
	//--------------------------------
	//順序回路用レベライズ
	//--------------------------------
	else if(opt.fmodel == TDF){
		
		//ノーマルレベライズ
		levelize_sequential();
		
		//代表信号線レベライズ
		if(opt.rep_levelize == YES){
			rep_levelize_sequential();
		}
	}	
#ifdef DEBUG
	printf("Completion of Levelize\n");
#endif
	
	//==============================================================================
	// FOUT-STEM解析 [レベル降順で作成]
	//==============================================================================
	//組合せ回路用
	if(opt.fmodel == SAF){
		search_fstem_combinational();
	}
	//順序回路用
	else if(opt.fmodel == TDF){
		search_fstem_sequential();
	}	
#ifdef DEBUG
	printf("Completion of Search_Fout_Stem\n");
#endif
	
	//==============================================================================
	// FFR解析 [ PO⇒STEM(レベル降順)で作成 ]
	//==============================================================================
	//組合せ回路用
	if(opt.fmodel == SAF){
		search_ffr_combinational();
	}
	//順序回路用
	else if(opt.fmodel == TDF){
		search_ffr_sequential();
	}
#ifdef DEBUG
	printf("Completion of Search_FFR\n");
#endif

	
	//==============================================================================
	// 再収斂解析
	//==============================================================================
	//組合せ回路用
	if(opt.fmodel == SAF){
		search_reconv_combinational();
	}
	//順序回路用
	else if(opt.fmodel == TDF){
		search_reconv_sequential();
	}
#ifdef DEBUG
	printf("Completion of Rconvergence_Block\n");
#endif
	
	//==============================================================================
	// 可制御性計算
	//==============================================================================
	//組合せ回路
	if(opt.fmodel == SAF){

		//PIの可制御性初期化
		initial_com_controllability();

		//回路内可制御性の計算
		cal_com_controllability();
	}

	//順序回路
	else if(opt.fmodel == TDF){

		//PI，PPIの可制御性初期化
		initial_seq_controllability();

		//回路内可制御性の計算
		cal_seq_controllability();
	}


	//==============================================================================
	// 制御ポイント挿入回路とのポインタ接続
	//==============================================================================
	//CPI_XIDモード時は付加信号線をCP挿入FFとポインタ接続
	if(opt.cpi_xid == YES){
		make_pointer_cpi();
		link_cpi_net();
#ifdef DEBUG
		printf("Completion of Initialize CPI-net\n");
#endif
	}
	
	//==============================================================================
	// ★解析・変換モード (※関数終了後にプログラム終了)
	//==============================================================================
	//---------------------------------------------------
	// 制御ポイント挿入箇所解析モード
	//---------------------------------------------------
	if(opt.cpi_analyze == YES){
		cpi_analyze_main(fp[F_CPI_ANA], fp[F_CPI_ANA_LOG]);
	}

	//---------------------------------------------------
	// 制御ポイント挿入箇所解析モード
	//---------------------------------------------------
	if(opt.trans_ffc_list == YES){
		trans_ffc_list(fp[F_TRANS_FFC], file_names[F_TRANS_FFC]);
	}
	
	//---------------------------------------------------
	// 有効状態テストパターン分類モード
	//---------------------------------------------------
	if(opt.rtl_state == YES){
		if(opt.rtl_pin == YES){
			//有効状態テストパターン分類
			sort_legal_state_tp(fp[F_STATE], fp[F_STATE_PIN], file_names);
			
		}
		else{
			printf("ERROR: 有効状態PINファイルがありません！\n");
			exit(-1);
		}
	}
	
	//---------------------------------------------------
	// 到達可能状態同定モード
	//---------------------------------------------------
	if(opt.r_state == YES){
		if(opt.r_pin == YES){
			//到達可能状態の同定
			find_reachable_state(fp[F_RSTATE], fp[F_RSTATE_PIN]);
			
		}
		else{
			printf("ERROR: 到達可能状態PINファイルがありません！\n");
			exit(-1);
		}
	}


	//==============================================================================
	// 【SPPFP】組合せ回路用 論理・故障シミュレーション
	//==============================================================================
	if(opt.debug_sppfp == YES){
		
		//-----------------------------------------
		// 縮退故障
		//-----------------------------------------
		if(opt.fmodel == SAF){

			// 論理シミュレーション
			if(n_before_x == 0){
				LSIM_2v_combinational();
			}
			else{
				LSIM_3v_combinational();
			}

			// 故障シミュレーション	
			DEBUG_SAF_SPPFP();
		}
				
		//-----------------------------------------
		// 遷移故障
		//-----------------------------------------
		else if(opt.fmodel == TDF){

			// 論理シミュレーション
			if(n_before_x == 0){
				LSIM_2v_sequential();
			}
			else{
				LSIM_3v_sequential();
			}

			// 故障シミュレーション	
			DEBUG_TDF_SPPFP();
		}

		//-------------------------------------
		// 検出故障数調査
		//-------------------------------------
		make_fsim_det_num();	//(1故障1検出：対象故障のみ)
	}
	//==============================================================================
	// 【PPSFP】組合せ回路用 論理・故障シミュレーション
	//==============================================================================
	else if((opt.fsim==YES || opt.xid==YES) && opt.fmodel==SAF){

		//-------------------------------------
		// 2値シミュレーション(0,1)
		//-------------------------------------
		if(n_before_x == 0){
			LSIM_2v_combinational();	//論理SIM
			
#ifdef COMP_DEBUG
			if(opt.tp_stil == YES){
				comp_exp_2v_combinational(fp[F_TPST]);	//出力期待値比較
			}
#endif
			//-------------------------------------
			// 2値故障シミュレーション(PPSFP)
			//-------------------------------------
			if(opt.xid==YES && opt.miyase2004==YES){
				SAF_2v_PPSFP_Ndrop();	//Miyase2004用の故障シミュレーション
			}
			else if(opt.rep_levelize == YES){
				SAF_2v_repPPSFP();		//Xバッファのみ計算(代表信号線レベライズ使用)
			}
			else if(opt.drop_fsim==YES && opt.n_drop>0){
				SAF_2v_PPSFP_Ndrop();	//Nドロップ故障シミュレーション
			}
			else{
				SAF_2v_PPSFP();			//Xバッファのみ計算
			}
		}
		
		//-------------------------------------
		// 3値シミュレーション(0,1,X)
		//-------------------------------------
		else{
			LSIM_3v_combinational();	//論理SIM
						
#ifdef COMP_DEBUG
			if(opt.tp_stil == YES){
				comp_exp_3v_combinational(fp[F_TPST]);	//出力期待値比較
			}
#endif
			//-------------------------------------
			// 3値故障シミュレーション(PPSFP)
			//-------------------------------------
			if(opt.xid==YES && opt.miyase2004 == YES){
				SAF_3v_PPSFP_Ndrop();	//Miyase2004用の故障シミュレーション
			}
			else if(opt.rep_levelize == YES){
				SAF_3v_repPPSFP();
			}
			else if(opt.drop_fsim==YES && opt.n_drop>0){
				SAF_3v_PPSFP_Ndrop();	//Nドロップ故障シミュレーション
			}
			else{
				SAF_3v_PPSFP();
			}
		}
				
		//-------------------------------------
		// 検出故障数調査
		//-------------------------------------
		make_fsim_det_num();	//(1故障1検出：対象故障のみ)

	}
	//==============================================================================
	// 【PPSFP】順序回路用 論理・故障シミュレーション
	//==============================================================================
	else if((opt.fsim==YES || opt.xid==YES || opt.cpi_xid==YES) && opt.fmodel==TDF){

		//-------------------------------------
		// 2値シミュレーション(0,1)
		//-------------------------------------
		if(n_before_x == 0){

			LSIM_2v_sequential();		//論理SIM

#ifdef COMP_DEBUG
			if(opt.tp_stil == YES){
				comp_exp_2v_sequential(fp[F_TPST]);	//出力期待値比較
			}
#endif
			//-------------------------------------
			// ★CPI_XIDモード★
			//-------------------------------------
			if(opt.cpi_xid == YES){
				
				//制御ポイント挿入リスト読込み & LSIM
				if(opt.cpi_list == YES){
					read_cpi_list(fp[F_CPI]);
				}
			}

			//-------------------------------------
			// 2値故障シミュレーション(PPSFP)
			//-------------------------------------
			if(opt.cpi_xid==YES){
				TDF_2v_PPSFP();			//Xバッファのみ計算(完全故障辞書生成)
			}


			else if((opt.xid==YES || opt.cpi_xid==YES) && opt.miyase2004==YES){
				TDF_2v_PPSFP_Ndrop();	//Miyase2004用の故障シミュレーション
			}
			else if(opt.rep_levelize == YES){
				TDF_2v_repPPSFP();		//Xバッファのみ計算(代表信号線レベライズ使用)
			}
			else if(opt.drop_fsim==YES && opt.n_drop>0){
				TDF_2v_PPSFP_Ndrop();	//Nドロップ故障シミュレーション
			}
			else{
				TDF_2v_PPSFP();			//Xバッファのみ計算(完全故障辞書生成)
			}
		}

		//-------------------------------------
		// 3値シミュレーション(0,1,X)
		//-------------------------------------
		else{
			LSIM_3v_sequential();	//論理SIM
			
#ifdef COMP_DEBUG
			if(opt.tp_stil == YES){
				comp_exp_3v_sequential(fp[F_TPST]);	//出力期待値比較
			}
#endif
			//-------------------------------------
			// ★CPI_XIDモード★
			//-------------------------------------
			if(opt.cpi_xid == YES){

				//制御ポイント挿入リスト読込み & LSIM
				if(opt.cpi_list == YES){
					read_cpi_list(fp[F_CPI]);
				}
			}

			//-------------------------------------
			// 3値故障シミュレーション(PPSFP)
			//-------------------------------------
			if((opt.xid==YES || opt.cpi_xid==YES) && opt.miyase2004==YES){
				TDF_3v_PPSFP_Ndrop();	//Miyase2004用の故障シミュレーション
			}
			else if(opt.rep_levelize == YES){
				TDF_3v_repPPSFP();
			}
			else if(opt.drop_fsim==YES && opt.n_drop>0){
				TDF_3v_PPSFP_Ndrop();	//Nドロップ故障シミュレーション
			}
			else{
				TDF_3v_PPSFP();
			}
		}

		//-------------------------------------
		// 検出故障数調査
		//-------------------------------------
		make_fsim_det_num();	//(1故障1検出：対象故障のみ)

	}
	
	//==============================================================================
	// X抽出
	//==============================================================================
	//-------------------------------------
	//縮退故障用X抽出
	//-------------------------------------
	if(opt.xid==YES && opt.fmodel==SAF){
		SAF_XID();
	}

	//-------------------------------------
	//遷移故障用X抽出
	//-------------------------------------
	else if(opt.xid==YES && opt.fmodel==TDF){
		TDF_XID(file_names);
	}

	//-------------------------------------
	// CPI_XID
	//-------------------------------------
	else if(opt.cpi_xid==YES && opt.fmodel==TDF){

		//制御ポイント挿入リスト読込みモード
		if(opt.cpi_list == YES){
			CPI_XID(777);			//リスト読込み時は優先パスフラグ値=777
		}
		//貪欲解法(WRTLT2012)
		else{
			CPI_XID_greedy();
		}

		//テストパターン変換
		change_xbit();

		//CPI_XID後のX数計算
		count_cpi_xbit_rate();
	}

	
	//==============================================================================
	// 二重検出法
	//==============================================================================
	if(opt.double_det == YES){
		double_detection();
	}

	
	//==============================================================================
	// ▲故障診断用故障シミュレーション(疑似テスター)
	//==============================================================================
	if(opt.f_diag == YES){
		fault_diagnosis_fsim(fp[F_FLSI], fp[F_DIAG]);
	}
	
	//==============================================================================
	// CPU-Time計測終了
	//==============================================================================
	cput = clock()-cput;
#ifdef DEBUG
	printf("\n\nEND CPU-Time\n");
#endif
	
	
	//==============================================================================
	// X抽出率の計算
	//==============================================================================
	if(opt.xid==YES || opt.cpi_xid==YES){
		count_xbit_rate();
	}
	

	//==============================================================================
	// ファイル出力
	//==============================================================================
	//PINファイル出力
	if(opt.out_pin == YES){
		file_output_pin(fp[F_OPIN]);
	}

	//X抽出後テスト集合{0,1,X}出力
	if(opt.xtp_tx==YES || opt.xtp_st==YES){
		file_outpuf_xid_test_set(fp[F_XTXT], fp[F_XSTIL], fp[F_TPST]);
	}

	//【オリジナル回路部】か【PI+CP部】のX抽出後テスト集合{0,1,X}出力
	if(opt.cpi_xid==YES && (opt.ori_tp==YES || opt.cp_tp==YES)){
		file_outpuf_cpi_xid_test_set(fp[F_ORI_TP], fp[F_CP_TP]);
	}
	
	//初期テスト集合のPIと2時刻目PPIの論理値を出力
	if(opt.t2_tp == YES){
		file_output_time2_tp(fp[F_T2_TP], file_names);
	}

	//入力テストパターンSTIL⇒txt変換出力
	if(opt.st_to_txt == YES){
		file_output_stil_to_txt(fp[F_TPST], fp[F_STTX]);
	}

	//故障辞書出力
	if(opt.dic == YES){
		file_output_fdictionary(fp[F_DIC]);
	}
	
	//代表故障リスト出力
	if(opt.rep == YES){
		file_output_rep_flist(fp[F_REP]);
	}

	//検出故障情報(検出回数)出力
	if(opt.det == YES){
		file_output_ndetect(fp[F_DET]);
	}

	//各TPの検出困難故障検出数出力
	if(opt.out_tp_hdf== YES){
		file_output_detect_hd_fault(fp[F_TP_HDF]);
	}

	//論理SIM後の出力期待値をファイル出力
	if(opt.exp_val == YES){
		file_output_expectation_value(fp[F_EXVAL]);
	}

	//論理SIM後の出力期待値に対応するPIN情報をファイル出力
	if(opt.exp_val == YES){
		file_output_po_pin(fp[F_EXPIN]);
	}	
	
	//論理SIM後の論理値をファイル出力
	if(opt.lsim_val == YES){
		file_output_lsim(fp[F_LSIM]);
	}
	
	//全信号線の論理値をファイル出力(FSIMならnval, XIDならxid_nval)
	if(opt.all_val == YES){
		file_output_all_net_value(fp[F_ALL_VAL]);
	}

	//二重検出後のテストパターン出力(txt形式)
	if(opt.double_det == YES){
		file_output_double_dtection_tp(fp[F_DDTP]);
	}

	//未検出故障リスト出力
	if(opt.undet == YES){
		file_output_undetect(fp[F_UNDET]);
	}
	
	//XIDログ出力
	if(opt.xid_log==YES && opt.xid==YES){
		file_output_xid_log(fp[F_XLOG]);
	}
	
	//見逃し故障ログ出力
	if(opt.miss_log==YES && opt.xid==YES){
		file_output_miss_log(fp[F_MISS_LOG]);
	}
	
	//見逃し故障リスト出力
	if(opt.miss_list==YES && opt.xid==YES){
		file_outpuf_missed_fault(fp[F_MISS]);
	}
	
	//追記ログ出力
	if(opt.add_log == YES){
		file_output_add_log(fp[F_A_LOG], file_names[F_NET], ((double)cput / (double)CLOCKS_PER_SEC));
	}

	
	//==============================================================================
	//X抽出後テスト集合に対するDsaturの実行
	//==============================================================================
	if(opt.run_dsatur==YES && opt.xtp_tx==YES){
		n_min_comtp = run_dsatur(file_names);
	}
	

	//==============================================================================
	//2時刻目PPIのX率計算
	//==============================================================================
	count_dff_xbit();

	//制御ポイント使用率計算
	if(opt.cpi_xid == YES){
		//cal_cpi_utilization();
	}
	

	//==============================================================================
	// ログファイル出力
	//==============================================================================
	if(opt.log == YES){
		file_output_log(fp[F_LOG], file_names, ((double)cput / (double)CLOCKS_PER_SEC));
	}


	//==============================================================================
	// レポート
	//==============================================================================
	printf("\n\n//-------------------------------------------------------\n");
	printf("// Report\n");
	printf("//-------------------------------------------------------\n");
	if(opt.fsim == YES){
		printf("Mode                            : Fault Simulation\n");
	}
	else if(opt.xid == YES){
		printf("Mode                            : X-Identification\n");
	}
	else if(opt.cpi_xid == YES){
		printf("Mode                            : CPI X-Identification\n");
	}

	if(opt.fmodel == SAF){
		printf("Target Fault Model              : SAF\n");
	}
	else if(opt.fmodel == TDF){
		printf("Target Fault Model              : TDF\n");
	}
	printf("Target Circuit                  : %s\n",file_names[F_NET]);
	
	if(opt.tp_stil == YES){
		printf("Target Test Set                 : %s\n",file_names[F_TPST]);
	}
	else if(opt.tp_txt == YES){
		printf("Target Test Set                 : %s\n",file_names[F_TPTX]);
	}

	printf("#of Test Pattern                : %d\n",n_tp);
	
	if(opt.fmodel == SAF){
		printf("#of Primary Input               : %d\n",n_pi);
		printf("#of Pseudo Primary Input        : %d\n",n_dff);
		printf("#of Initial X-bit Ratio         : %.2lf%%\n",((double)n_before_x/(double)(n_tp*n_pi))*100 );
		if(opt.fault_coverage_rep == YES){
			printf("#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_rep)*100 );
		}
		else{
			printf("#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_net)*100 );
		}
	}
	else if(opt.fmodel == TDF){
		printf("#of Primary Input               : %d\n",n_spi);
		printf("#of Pseudo Primary Input        : %d\n",n_ppi);
		if(opt.cpi_xid == YES){
			printf("#of number of CPI               : %d\n", n_cpi);
		}
		printf("#of Initial X-bit Ratio         : %.2lf%%\n",((double)n_before_x/(double)(n_tp*(n_spi+n_ppi)))*100 );
		if(opt.fault_coverage_rep == YES){
			printf("#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_rep)*100 );
		}
		else{
			printf("#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_snet)*100 );
		}
	}

	printf("#of Target Fault                : %d\n",n_rep );
	printf("#of Detected Fault              : %d\n",n_detect );
	printf("#of Undetected Fault            : %d\n",n_rep-n_detect );

	if(opt.xid==YES || opt.cpi_xid==YES){
		printf("#of Essential Fault             : %d\n",n_essential_fault );
		//printf("#of Hard_to_detect Fault        : %d\n",n_hd_fault );
		//printf("#of Diff Parh Fault             : %d\n",n_dpxid_fault );
		printf("#of Missed Fault                : %d\n",n_missed_fault );
		printf("#of Increase Care bit by EX-XID : %d\n",n_missed_cbit );
		//printf("#of Number of Run XID           : %d\n",opt.n_xid );
		//printf("#of Number of Run Diff-Path XID : %d\n",opt.diff_path_n_xid );
		if(opt.diff_path_n_xid>0){
			printf("#of Diff-Path XID mode          : ");
			if(opt.diff_path_mode == RAND_FIX){
			printf("RAND_FIX\n");
			}
			else if(opt.diff_path_mode == RAND){
			printf("RAND\n");
			}
			else if(opt.diff_path_mode == DIFF){
			printf("DIFF\n");
			}
		}
		
		if(opt.cpi_xid == YES){
			printf("#of After X-bit Ratio (All_PIN) : %.2lf%%\n",((double)n_after_x/(double)(n_tp*(n_spi+n_ppi)))*100 );
			printf("  X-bit Ratio of PI+PPI         : %.2lf%%\n", ((double)n_ori_x/(double)(n_tp*(n_ori_pi+n_ori_dff)))*100 );
			printf("  X-bit Ratio of PI+CP          : %.2lf%%\n", ((double)n_pi_cp_x/(double)(n_tp*(n_ori_pi+n_cpi)))*100 );
			printf("  X-bit Ratio of PI+PPI+CP      : %.2lf%%\n", ((double)n_cpi_x/(double)(n_tp*(n_ori_pi+n_ori_dff+n_cpi)))*100 );
			printf("#of Minimum Test Pattern        : %d\n", n_min_comtp);
		}
		else if(opt.fmodel == SAF){
			printf("#of After X-bit Ratio           : %.2lf%%\n",((double)n_after_x/(double)(n_tp*n_pi))*100 );
			printf("#of Minimum Test Pattern        : %d\n", n_min_comtp);
		}
		else if(opt.fmodel == TDF){
			printf("#of After X-bit Ratio           : %.2lf%%\n",((double)n_after_x/(double)(n_tp*(n_spi+n_ppi)))*100 );
		}
		
	}
	printf("Total CPU time                  : %.2lf (sec)\n", (double)cput / (double)CLOCKS_PER_SEC);
	printf("//-------------------------------------------------------\n");

	
	//==============================================================================
	// WARNING系
	//==============================================================================
	if(n_not_found_fault != 0){
		printf("WARNING: %d個の故障が故障リスト読込み時に故障設置できませんでした\n", n_not_found_fault);
	}
	if(n_not_found_ffc != 0){
		printf("WARNING: %d個のFFがCPIリスト読込み時にFFC=1にできませんでした\n", n_not_found_ffc);
	}
	if(n_not_found_link_CP_FF != 0){
		printf("WARNING: %d個のCPとFFがポインタ接続できませんでした\n", n_not_found_link_CP_FF);
	}
	if(n_not_found_link_FFC_FF != 0){
		printf("WARNING: %d個のFFCとFFがポインタ接続できませんでした\n", n_not_found_link_FFC_FF);
	}

}//END



//---------------------------------------------------------------------
// 内部関数
//---------------------------------------------------------------------
//------------------------------------------------------------
// 関数名 : file_open
// 機能   : 必須ファイルのオープン
// 戻り値 : なし
// 引数   : 
//    fp        : ファイルポインタ
//    net_name  : ネットリストファイル名
//    vect_name : テストベクトルファイル名
//------------------------------------------------------------
void file_open(FILE	*fp[], char **file_names)
{
	int i;
	
	//-------------------------------------------------------------------------------------------
	// ファイル読込み
	//-------------------------------------------------------------------------------------------
	// 初期化(標準出力)
	for(i=0 ; i<N_FILE ; i++)fp[i] = stdout;

	// PINファイルオープン
	if(opt.pin == YES){
		if((fp[F_PIN] = fopen(file_names[F_PIN],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open PIN file %s\n",file_names[F_PIN]);
			exit(1);
		}
	}
	
	// テストパターン(.txt) ファイルオープン
	if(opt.tp_txt == YES){
		if((fp[F_TPTX] = fopen(file_names[F_TPTX],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Testpattern file(.txt) %s\n",file_names[F_TPTX]);
			exit(1);
		}
	}

	// テストパターン(.stil) ファイルオープン
	if(opt.tp_stil == YES){
		if((fp[F_TPST] = fopen(file_names[F_TPST],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Testpattern file(.stil) %s\n",file_names[F_TPST]);
			exit(1);
		}
	}

	// 故障リスト ファイルオープン
	if(opt.flt == YES){
		if((fp[F_FLT] = fopen(file_names[F_FLT],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open fault_list file %s\n",file_names[F_FLT]);
			exit(1);
		}
	}
	
	// 故障リスト ファイルオープン
	if(opt.cpi_list == YES){
		if((fp[F_CPI] = fopen(file_names[F_CPI],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open CPI list file %s\n",file_names[F_CPI]);
			exit(1);
		}
	}

	// 故障LSI情報 ファイルオープン
	if(opt.flsi == YES){
		if((fp[F_FLSI] = fopen(file_names[F_FLSI],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open fault_LSI_list file %s\n",file_names[F_FLSI]);
			exit(1);
		}
	}
	
	//-------------------------------------------------------------------------------------------
	// ファイル書き込み
	//-------------------------------------------------------------------------------------------
	// テストパターン書き込み(.txt) ファイルオープン
	if(opt.xtp_tx == YES){
		if((fp[F_XTXT] = fopen(file_names[F_XTXT],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Xid_Test_Pattern file(.txt) %s\n",file_names[F_XTXT]);
			exit(1);
		}
	}

	// テストパターン書き込み(.stil) ファイルオープン
	if(opt.xtp_st == YES){
		if((fp[F_XSTIL] = fopen(file_names[F_XSTIL],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Xid_Test_Pattern file(.stil) %s\n",file_names[F_XSTIL]);
			exit(1);
		}
	}

	// テストパターン書き込み(.stil⇒.txt変換) ファイルオープン
	if(opt.st_to_txt == YES){
		if((fp[F_STTX] = fopen(file_names[F_STTX],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open STIL to txt Test pattern file() %s\n",file_names[F_STTX]);
			exit(1);
		}
	}

	// 故障辞書ファイル書き込み ファイルオープン
	if(opt.dic == YES){
		if((fp[F_DIC] = fopen(file_names[F_DIC],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Fault_Dictionary file %s\n",file_names[F_DIC]);
			exit(1);
		}
	}

	// 代表故障リスト書き込み ファイルオープン
	if(opt.rep == YES){
		if((fp[F_REP] = fopen(file_names[F_REP],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open rep faults list file %s\n",file_names[F_REP]);
			exit(1);
		}
	}

	// 検出故障リスト書き込み ファイルオープン
	if(opt.det == YES){
		if((fp[F_DET] = fopen(file_names[F_DET],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open DETECT_List file %s\n",file_names[F_DET]);
			exit(1);
		}
	}

	// 未検出故障リスト書き込み ファイルオープン
	if(opt.undet == YES){
		if((fp[F_UNDET] = fopen(file_names[F_UNDET],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open UNDETECT_List file %s\n",file_names[F_UNDET]);
			exit(1);
		}
	}

	// ログファイルオープン
	if(opt.log == YES){
		if((fp[F_LOG] = fopen(file_names[F_LOG],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Log file %s\n",file_names[F_LOG]);
			exit(1);
		}
	}
	
	// XIDログファイルオープン
	if(opt.xid_log == YES){
		if((fp[F_XLOG] = fopen(file_names[F_XLOG],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open XID Log file %s\n",file_names[F_XLOG]);
			exit(1);
		}
	}
		
	// 見逃し故障数ログファイルオープン
	if(opt.miss_log == YES){
		if((fp[F_MISS_LOG] = fopen(file_names[F_MISS_LOG],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Missed Faults Log file %s\n",file_names[F_MISS_LOG]);
			exit(1);
		}
	}
	
	// 見逃し故障数リストファイルオープン
	if(opt.miss_list == YES){
		if((fp[F_MISS] = fopen(file_names[F_MISS],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Missed Faults List file %s\n",file_names[F_MISS]);
			exit(1);
		}
	}

	// 追記ログファイルオープン
	if(opt.add_log == YES){
		if((fp[F_A_LOG] = fopen(file_names[F_A_LOG],"a")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Add Log file %s\n",file_names[F_A_LOG]);
			exit(1);
		}
	}
		
	//-------------------------------------------------------------------------------------------
	// 解析用オプション
	//-------------------------------------------------------------------------------------------
	// 各TPが含んでいる検出困難故障数調査　書き込みファイルオープン
	if(opt.out_tp_hdf == YES){
		if((fp[F_TP_HDF] = fopen(file_names[F_TP_HDF],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Hard to Detect Faults Analyze file %s\n",file_names[F_TP_HDF]);
			exit(1);
		}
	}
	
	// 制御ポイント挿入箇所解析リスト　読込みファイルオープン
	if(opt.cpi_ana_list == YES){
		if((fp[F_CPI_ANA] = fopen(file_names[F_CPI_ANA],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open CPI Analyze file %s\n",file_names[F_CPI_ANA]);
			exit(1);
		}
	}
	
	// 制御ポイント挿入箇所解析結果　書き込みファイルオープン
	if(opt.cpi_ana_log == YES){
		if((fp[F_CPI_ANA_LOG] = fopen(file_names[F_CPI_ANA_LOG],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open CPI Analyze LOG file %s\n",file_names[F_CPI_ANA_LOG]);
			exit(1);
		}
	}
	
	// 変換対象FFCリスト　読込みファイルオープン
	if(opt.trans_ffc_list == YES){
		if((fp[F_TRANS_FFC] = fopen(file_names[F_TRANS_FFC],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Trans FFC List file %s\n",file_names[F_TRANS_FFC]);
			exit(1);
		}
	}
	

	// RTLの有効状態リスト　読込みファイルオープン
	if(opt.rtl_state == YES){
		if((fp[F_STATE] = fopen(file_names[F_STATE],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open RTL Legal State List file %s\n",file_names[F_STATE]);
			exit(1);
		}
	}
	
	// RTLの有効状態リストのPINファイル　読込みファイルオープン
	if(opt.rtl_pin == YES){
		if((fp[F_STATE_PIN] = fopen(file_names[F_STATE_PIN],"r")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open RTL Legal State Pin file %s\n",file_names[F_STATE_PIN]);
			exit(1);
		}
	}
	
	//-------------------------------------------------------------------------------------------
	// その他
	//-------------------------------------------------------------------------------------------
	// 故障診断用FSIM結果書き込み ファイルオープン
	if(opt.f_diag == YES){
		if((fp[F_DIAG] = fopen(file_names[F_DIAG],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open result of fault-diagnosis output file %s\n",file_names[F_DIAG]);
			exit(1);
		}
	}

	// 二重検出後のテストパターンファイル書き込み ファイルオープン
	if(opt.double_det == YES){
		if((fp[F_DDTP] = fopen(file_names[F_DDTP],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Double Detection Test Set output file %s\n",file_names[F_DDTP]);
			exit(1);
		}
	}
		
	// 論理SIM後の出力期待値ファイル書き込み ファイルオープン
	if(opt.exp_val == YES){
		if((fp[F_EXVAL] = fopen(file_names[F_EXVAL],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open LSIM expectatin value file %s\n",file_names[F_EXVAL]);
			exit(1);
		}
	}
		
	// 論理SIM後の出力期待値に対するPIN情報ファイル書き込み ファイルオープン
	if(opt.exp_pin == YES){
		if((fp[F_EXPIN] = fopen(file_names[F_EXPIN],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open PIN file of LSIM expectatin file %s\n",file_names[F_EXPIN]);
			exit(1);
		}
	}

	// 論理SIM後の論理値ファイル書き込み ファイルオープン
	if(opt.lsim_val == YES){
		if((fp[F_LSIM] = fopen(file_names[F_LSIM],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open LSIM value file %s\n",file_names[F_LSIM]);
			exit(1);
		}
	}
	
	// PINファイル書き込み ファイルオープン
	if(opt.out_pin == YES){
		if((fp[F_OPIN] = fopen(file_names[F_OPIN],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput PIN file %s\n",file_names[F_OPIN]);
			exit(1);
		}
	}

	// 全信号線の論理値ファイル書き込み ファイルオープン
	if(opt.all_val == YES){
		if((fp[F_ALL_VAL] = fopen(file_names[F_ALL_VAL],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput All_Net_Value file %s\n",file_names[F_ALL_VAL]);
			exit(1);
		}
	}	

	// PI+PPI部のテスト集合をファイル書き込み ファイルオープン
	if(opt.ori_tp == YES){
		if((fp[F_ORI_TP] = fopen(file_names[F_ORI_TP],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput Original TP file %s\n",file_names[F_ORI_TP]);
			exit(1);
		}
	}

	// PI+CP部のテスト集合をファイル書き込み ファイルオープン
	if(opt.cp_tp == YES){
		if((fp[F_CP_TP] = fopen(file_names[F_CP_TP],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput CP TP file %s\n",file_names[F_CP_TP]);
			exit(1);
		}
	}

	// 初期テスト集合のPIと2時刻目PPI値をファイル書き込み ファイルオープン
	if(opt.t2_tp == YES){
		if((fp[F_T2_TP] = fopen(file_names[F_T2_TP],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput PI + Time2_PPI Test set file %s\n",file_names[F_T2_TP]);
			exit(1);
		}
	}
	
	// 判定した到達可能状態をファイル書き込み ファイルオープン
	if(opt.r_state == YES){
		if((fp[F_RSTATE] = fopen(file_names[F_RSTATE],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput reachable state file %s\n",file_names[F_RSTATE]);
			exit(1);
		}
	}

	// 判定した到達可能状態のPIN情報をファイル書き込み ファイルオープン
	if(opt.r_pin == YES){
		if((fp[F_RSTATE_PIN] = fopen(file_names[F_RSTATE_PIN],"w")) == (FILE *)NULL){
			fprintf(stderr,"Cannot open Ooutput reachable state pin file %s\n",file_names[F_RSTATE_PIN]);
			exit(1);
		}
	}


}//END
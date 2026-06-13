//-----------------------------------------------------------------------
// option.c
//-----------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	"./option.h"
#include	"./Lib/alloc.h"
#include	"./Netlist/netlist.h"
#include	"./Netlist/s_netlist.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
static	void	usage			(void);

//--------------------------------------------------------------------
//定義
//--------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

//---------------------------------------------------------------------
// 外部宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 静的変数
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 外部関数
//---------------------------------------------------------------------
//------------------------------------------------------------
// 関数名 : option
// 機能   : コマンドライン処理
// 戻り値 : なし
// 引数   : 
//     argc      : 引数の数
//     argv      : 引数
//------------------------------------------------------------
void option(int argc, char *argv[], char **file_names)
{

	int i;	

	for(i=0 ; i<N_FILE ; i++){
		file_names[i] = CHAR_alloc(10000);
        file_names[i][0] = '\0';
	}	
	
	//========================================
	// 初期化
	//========================================
	// DEBUGオプション初期化(基本的に全てNO)
	opt.debug_sppfp				= NO;	//SPPFPのDEBUG

	// 動作モード初期化(どれか1つのみ動作)
	opt.fmodel					= SAF;			//故障モデル(デフォルト：縮退故障)
	opt.xid						= NO;			//ドントケア抽出
	opt.fsim					= NO;			//故障シミュレーション
	opt.f_diag					= NO;			//故障診断用故障シミュレーション
	opt.double_det				= NO;			//二重検出法
	opt.cpi_xid					= NO;			//制御ポイント挿入XID(遷移故障専用)
	opt.cpi_analyze				= NO;			//制御ポイント挿入箇所解析モード
	opt.trans_ffc_list			= NO;			//FFCリストの変換モード

	// X抽出経路選択モード初期化
	opt.xid_fpath				= LEVEL;		//故障伝搬経路決定	{LEVEL, COST, MAX_LEVEL}
	opt.xid_jus					= LEVEL;		//正当化経路決定	{LEVEL, COST, MAX_LEVEL}

	// X抽出モード初期化
	opt.miyase2004				= YES;			//Miyase2004モード
	opt.miyase2008				= NO;			//Miyase2008モード(検出故障数均一化)
	opt.n_xid					= 1;			//各故障に対するXID最大実行回数(※デフォルト1回)
	opt.diff_path_n_xid			= 0;			//見逃し故障に対して異なるパスでXIDする回数(※デフォルト0回)
	opt.diff_path_mode			= RAND;			//見逃し故障に対して異なるパスでXIDするときのパス選択衣基準(※デフォルト:ランダム選択)
	opt.tp_vari					= NO;			//テストパターンケアビット数均一化
	opt.tp_x					= NO;			//テストパターンケアビット数均一化 & X抽出率考慮(WRTLT2011論文のコスト関数MW()のTP版)
	opt.pi_vari					= NO;			//(疑似)外部入力ケアビット数均一化
	opt.pi_x					= NO;			//(疑似)外部入力ケアビット数均一化 & X抽出率考慮(WRTLT2011論文のコスト関数MW())
	opt.pitp_dc					= NO;			//テストパターンと(疑似)外部入力のケアビット数均一化
	opt.pitp_mw					= NO;			//テストパターンと(疑似)外部入力のケアビット数均一化 & X抽出率考慮

	opt.M_HD_tp					= 0;			//何%の検出困難故障TPを犠牲にするか(HD = Hard_to_Detect fault)
	opt.N_HD_fault				= 2;			//検出困難故障の定義(検出回数 ※デフォルト=2 オプションで変更可能)
	
	// 故障SIMオプション
	opt.rpg						= 0;			//ランダムパターン生成数
	opt.n_drop					= 2;			//初期テスト集合FSIM時の各故障のFSIM回数(Miyase2004時に使用)
	opt.drop_fsim				= NO;			//FSIM単体動作時に故障ドロップするか

	// CPI_XID用オプション
	opt.p_cpi					= 0;			//制御ポイント挿入割合
	opt.pri_xid					= YES;			//制御ポイント影響範囲内故障の優先XID
	opt.priority_path			= YES;			//優先パスの使用
	opt.cpi_fsim				= YES;			//制御ポイント挿入回路用のFSIM(故障リスト読込みが特殊処理)

	// X抽出解析用オプション
	opt.out_tp_hdf				= NO;			//各TPが含んでいる検出困難故障数調査

	// テスト圧縮オプション
	opt.run_dsatur				= NO;

	// 故障シミュレーションの高速化オプション
	opt.rep_levelize			= NO;			//代表信号線レベライズ

	// 表示オプション
	opt.fault_coverage_rep		= YES;			//故障検出率表示方法(YES:分母=代表故障数,  NO:分母=全信号線数*2)

	// ファイルオプション初期化
	//読込み
	opt.tp_stil					= NO;
	opt.tp_txt					= NO;
	opt.pin						= NO;
	opt.flt						= NO;	//故障リスト
	opt.flsi					= NO;	//故障LSI情報
	opt.cpi_list				= NO;	//制御ポイント挿入リスト
	opt.cpi_ana_list			= NO;	//【解析モード時使用】制御ポイント挿入箇所解析リスト
	opt.trans_ffc_list			= NO;	//変換するFFCリストファイル名
	opt.rtl_state				= NO;
	opt.rtl_pin					= NO;

	//書き込み
	opt.xtp_tx					= NO;
	opt.xtp_st					= NO;
	opt.st_to_txt				= NO;
	opt.dic						= NO;
	opt.det						= NO;
	opt.undet					= NO;
	opt.log						= NO;
	opt.xid_log					= NO;
	opt.miss_log				= NO;
	opt.miss_list				= NO;
	opt.exp_val					= NO;
	opt.exp_pin					= NO;
	opt.lsim_val				= NO;
	opt.out_pin					= NO;
	opt.all_val					= NO;
	opt.t2_tp					= NO;
	opt.cpi_ana_log				= NO;
	opt.r_state					= NO;
	opt.r_pin					= NO;

	//============================================================
	// コマンドライン処理
	//============================================================
	for(i=1; i<argc; i++){

		//--------------------------------------------------------
		// 必須ファイル
		//--------------------------------------------------------
		// Verilog-HDL回路 ファイル読込み
		if(!strcmp(argv[i],"-c")){
			i++;
			strcpy(file_names[F_NET],argv[i]);
		}

		// テストパターン(.stil) ファイル読込み
		else if(!strcmp(argv[i],"-st")){
			i++;
			strcpy(file_names[F_TPST],argv[i]);
			opt.tp_stil = YES;
		}

		// テストパターン(.txt) ファイル読込み
		else if(!strcmp(argv[i],"-tx")){
			i++;
			strcpy(file_names[F_TPTX],argv[i]);
			opt.tp_txt = YES;
		}

		// PINファイル読み込み(※-txモード時必須)
		else if(!strcmp(argv[i],"-pin")){
			i++;
			strcpy(file_names[F_PIN],argv[i]);
			opt.pin = YES;
		}

		//--------------------------------------------------------
		// 　必須コマンド
		//--------------------------------------------------------
		// 故障モデル設定
		else if(!strcmp(argv[i],"-fm")){
			i++;
			if( !strcmp(argv[i] , "SAF") ){
				opt.fmodel  = SAF;
			}
			else if( !strcmp(argv[i] , "TDF") ){
				opt.fmodel  = TDF;
			}
		}

		//--------------------------------------------------------
		//  動作モード設定(併用不可)
		//--------------------------------------------------------
		// X抽出モード設定
		else if(!strcmp(argv[i],"-xid")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.xid = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.xid = NO;
			}
		}

		// 制御ポイント挿入X抽出モード設定
		else if(!strcmp(argv[i],"-cpixid")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.cpi_xid = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.cpi_xid = NO;
			}
		}

		// 故障シミュレーションモード設定
		else if(!strcmp(argv[i],"-fsim")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.fsim = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.fsim = NO;
			}
		}

		// 二重検出法設定
		else if(!strcmp(argv[i],"-dd")){
			i++;
			strcpy(file_names[F_DDTP],argv[i]);	//二重検出後のテストパターン出力ファイル名
			opt.double_det = YES;				//-fismオプションとの併用必須
		}

		// 故障診断用故障シミュレーション設定
		else if(!strcmp(argv[i],"-fdiag")){
			i++;
			strcpy(file_names[F_DIAG],argv[i]);	//診断用FSIM結果ファイル名
			opt.f_diag = YES;
		}

		// X抽出後テスト集合でのDsatur実行
		else if(!strcmp(argv[i],"-com")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.run_dsatur = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.run_dsatur = NO;
			}
		}

		// 制御ポイント挿入箇所解析モード
		else if(!strcmp(argv[i],"-cpi_ana")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.cpi_analyze = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.cpi_analyze = NO;
			}

			//故障モデルを遷移故障に変更
			opt.fmodel = TDF;
		}
				
		// FFCリストの変換モード
		else if(!strcmp(argv[i],"-trans_ffc")){
			
			i++;
			opt.trans_ffc_list = YES;
			strcpy(file_names[F_TRANS_FFC],argv[i]);

			//故障モデルを遷移故障に変更
			opt.fmodel = TDF;
		}

		//--------------------------------------------------------
		// 故障シミュレーションオプション設定
		//--------------------------------------------------------
		// 疑似ランダムパターン生成
		else if(!strcmp(argv[i],"-rpg")){
			i++;
			opt.rpg = atoi(argv[i]);
		}

		//FSIM時に何回検出で故障ドロップするか
		else if(!strcmp(argv[i],"-n_drop")){
			i++;
			opt.n_drop = atoi(argv[i]);
			
			//Miyase2004モード時のドロップ値確認
			if(opt.miyase2004==YES && opt.n_drop!=2){
				printf("\n//------------------------------\n");
				printf("//WARNING: 故障ドロップ値 \n");
				printf("//------------------------------\n");
				printf("//Miyase2004モードのため故障ドロップ値を2に変更します \n");
				opt.n_drop = 2;
			}
		}

		//FSIM単体実行時に故障ドロップするか
		else if(!strcmp(argv[i],"-drop")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.drop_fsim = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.drop_fsim = NO;
			}
		}

		//--------------------------------------------------------
		//  X抽出オプション設定
		//--------------------------------------------------------
		// 各故障に対するXID最大実行回数
		else if(!strcmp(argv[i],"-n_xid")){
			i++;
			opt.n_xid = atoi(argv[i]);
		}

		// 異なるパスでのX抽出する実行回数
		else if(!strcmp(argv[i],"-n_dpxid")){
			i++;
			opt.diff_path_n_xid = atoi(argv[i]);
		}
		
		// 異なるパスでのX抽出(見逃し故障用？)
		else if(!strcmp(argv[i],"-dpxid")){
			i++;
			if( !strcmp(argv[i] , "fix") ){
				opt.diff_path_mode = RAND_FIX;
			}
			else if( !strcmp(argv[i] , "diff") ){
				opt.diff_path_mode = DIFF;
			}
			else if( !strcmp(argv[i] , "rand") ){
				opt.diff_path_mode = RAND;
			}
		}
		
		// 検出故障数均一化(Miyase2004)
		else if(!strcmp(argv[i],"-m2004")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.miyase2004 = YES;
				opt.n_drop = 2;					//FSIM時は2回検出でドロップに固定
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.miyase2004 = NO;
			}
		}


		// 検出故障数均一化(Miyase2008)
		else if(!strcmp(argv[i],"-m2008")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.miyase2008 = YES;				//miyase2008モードON
				opt.miyase2004 = NO;			//miyase2004モードOFF
				opt.n_drop = 0;					//完全故障辞書FSIM
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.miyase2008 = NO;
			}
		}

		// テストパターンケアビット数均一化
		else if(!strcmp(argv[i],"-tp_vari")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.tp_vari = YES;
				opt.miyase2004 = NO;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.tp_vari = NO;
			}
		}

		// (疑似)外部入力ケアビット数均一化
		else if(!strcmp(argv[i],"-pi_vari")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.pi_vari = YES;
				opt.miyase2004 = NO;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.pi_vari = NO;
			}
		}

		// テストパターンケアビット数均一化 & X抽出率考慮(WRTLT2011論文のコスト関数MW()のTP版)
		else if(!strcmp(argv[i],"-tp_x")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.tp_x = YES;
				opt.miyase2004 = NO;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.tp_x = NO;
			}
		}

		// (疑似)外部入力ケアビット数均一化 & X抽出率考慮(WRTLT2011論文のコスト関数MW())
		else if(!strcmp(argv[i],"-pi_x")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.pi_x = YES;
				opt.miyase2004 = NO;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.pi_x = NO;
			}
		}
		
		// テストパターンと(疑似)外部入力のケアビット数均一化
		else if(!strcmp(argv[i],"-pitp_dc")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.pitp_dc = YES;
				opt.miyase2004 = NO;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.pitp_dc = NO;
			}
		}

		// テストパターンと(疑似)外部入力のケアビット数均一化 & X抽出率考慮(WRTLT2011論文のコスト関数MW())
		else if(!strcmp(argv[i],"-pitp_mw")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.pitp_mw = YES;
				opt.miyase2004 = NO;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.pitp_mw = NO;
			}
		}

		// 何%の検出困難故障TPを犠牲にするか
		else if(!strcmp(argv[i],"-M_HD")){
			i++;
			opt.M_HD_tp = atoi(argv[i]);
		}

		// 検出困難故障の定義(検出回数：-N_HD未満)
		else if(!strcmp(argv[i],"-N_HD")){
			i++;
			opt.N_HD_fault = atoi(argv[i]);
		}

		// 【解析オプション】各TPが含んでいる検出困難故障数調査
		else if(!strcmp(argv[i],"-out_tp_hdf")){
			i++;
			strcpy(file_names[F_TP_HDF],argv[i]);
			opt.out_tp_hdf = YES;
		}
		
		//--------------------------------------------------------
		//  CPI_XID用オプション設定
		//--------------------------------------------------------
		//制御ポイント挿入割合(%)
		else if(!strcmp(argv[i],"-p_cpi")){
			i++;
			opt.p_cpi = atoi(argv[i]);
		}

		//制御ポイント優先XIDモード
		else if(!strcmp(argv[i],"-pri_xid")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.pri_xid = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.pri_xid = NO;
			}
		}

		//制御ポイント優先パスXID
		else if(!strcmp(argv[i],"-pri_path")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.priority_path = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.priority_path = NO;
			}
		}

		//オリジナル回路部(PI+PPI)のテスト集合を ファイル書き込み
		else if(!strcmp(argv[i],"-ori_tp")){
			i++;
			strcpy(file_names[F_ORI_TP],argv[i]);
			opt.ori_tp = YES;
		}
		
		//PI+CP部(PPIとFFCは排除)のテスト集合を ファイル書き込み
		else if(!strcmp(argv[i],"-cp_tp")){
			i++;
			strcpy(file_names[F_CP_TP],argv[i]);
			opt.cp_tp = YES;
		}
		

		//--------------------------------------------------------
		//  高速化オプション設定
		//--------------------------------------------------------
		// 代表信号線レベライズ
		else if(!strcmp(argv[i],"-rep_lev")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.rep_levelize = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.rep_levelize = NO;
			}
		}
		
		//--------------------------------------------------------
		//  DEBUGモード設定
		//--------------------------------------------------------
		// SPPFP-DEBUGモード
		else if(!strcmp(argv[i],"-sppfp")){
			i++;
			if( !strcmp(argv[i] , "YES") ){
				opt.debug_sppfp = YES;
			}
			else if( !strcmp(argv[i] , "NO") ){
				opt.debug_sppfp= NO;
			}
		}

		//--------------------------------------------------------
		//  オプション設定
		//--------------------------------------------------------
		// 故障リスト ファイル読込み
		else if(!strcmp(argv[i],"-flist")){
			i++;
			strcpy(file_names[F_FLT],argv[i]);
			opt.flt = YES;
		}

		// 制御ポイント挿入リスト ファイル読込み
		else if(!strcmp(argv[i],"-cpilist")){
			i++;
			strcpy(file_names[F_CPI],argv[i]);
			opt.cpi_list = YES;
		}

		// 故障LSI情報 ファイル読込み
		else if(!strcmp(argv[i],"-flsi")){
			i++;
			strcpy(file_names[F_FLSI],argv[i]);
			opt.flsi = YES;
		}

		// X抽出後TPファイル(.txt) ファイル書き込み
		else if(!strcmp(argv[i],"-otx")){
			i++;
			strcpy(file_names[F_XTXT],argv[i]);
			opt.xtp_tx = YES;
		}

		// X抽出後TPファイル(.stil) ファイル書き込み
		else if(!strcmp(argv[i],"-ost")){
			i++;
			strcpy(file_names[F_XSTIL],argv[i]);
			opt.xtp_st = YES;
		}

		// 入力STILファイルをtxt形式で ファイル書き込み
		else if(!strcmp(argv[i],"-stil_to_txt")){
			i++;
			strcpy(file_names[F_STTX],argv[i]);
			opt.st_to_txt = YES;
		}

		//故障辞書 ファイル書き込み
		else if(!strcmp(argv[i],"-dic")){
			i++;
			strcpy(file_names[F_DIC],argv[i]);
			opt.dic = YES;
		}
		
		//代表故障リスト ファイル書き込み
		else if(!strcmp(argv[i],"-rep")){
			i++;
			strcpy(file_names[F_REP],argv[i]);
			opt.rep = YES;
		}

		//検出故障リスト(検出回数) ファイル書き込み
		else if(!strcmp(argv[i],"-det")){
			i++;
			strcpy(file_names[F_DET],argv[i]);
			opt.det = YES;
		}

		//未検出故障リスト ファイル書き込み
		else if(!strcmp(argv[i],"-undet")){
			i++;
			strcpy(file_names[F_UNDET],argv[i]);
			opt.undet = YES;
		}

		// ログファイル ファイル書き込み
		else if(!strcmp(argv[i],"-log")){
			i++;
			strcpy(file_names[F_LOG],argv[i]);
			opt.log = YES;
		}

		// XIDログファイル ファイル書き込み
		else if(!strcmp(argv[i],"-add_log")){
			i++;
			strcpy(file_names[F_A_LOG],argv[i]);
			opt.add_log = YES;
		}
		
		// XIDログファイル ファイル書き込み
		else if(!strcmp(argv[i],"-xid_log")){
			i++;
			strcpy(file_names[F_XLOG],argv[i]);
			opt.xid_log = YES;
		}
		
		// 見逃し故障ログファイル ファイル書き込み
		else if(!strcmp(argv[i],"-miss_log")){
			i++;
			strcpy(file_names[F_MISS_LOG],argv[i]);
			opt.miss_log = YES;
		}
		
		// 見逃し故障リストファイル ファイル書き込み
		else if(!strcmp(argv[i],"-miss")){
			i++;
			strcpy(file_names[F_MISS],argv[i]);
			opt.miss_list = YES;
		}

		// 論理SIM後の出力期待値ファイル ファイル書き込み
		else if(!strcmp(argv[i],"-exval")){
			i++;
			strcpy(file_names[F_EXVAL],argv[i]);
			opt.exp_val = YES;
		}

		// 論理SIM後の出力期待値ファイル ファイル書き込み
		else if(!strcmp(argv[i],"-expin")){
			i++;
			strcpy(file_names[F_EXPIN],argv[i]);
			opt.exp_pin = YES;
		}
		
		// 論理SIM後の論理値ファイル ファイル書き込み
		else if(!strcmp(argv[i],"-lsim")){
			i++;
			strcpy(file_names[F_LSIM],argv[i]);
			opt.lsim_val = YES;
		}
		
		// PINファイル出力 ファイル書き込み
		else if(!strcmp(argv[i],"-opin")){
			i++;
			strcpy(file_names[F_OPIN],argv[i]);
			opt.out_pin = YES;
		}
		
		// 全信号線の論理値ファイル出力 ファイル書き込み
		else if(!strcmp(argv[i],"-all_val")){
			i++;
			strcpy(file_names[F_ALL_VAL],argv[i]);
			opt.all_val = YES;
		}
				
		// 初期テスト集合のPIと2時刻目PPI値のファイル出力 ファイル書き込み
		else if(!strcmp(argv[i],"-t2_tp")){
			i++;
			strcpy(file_names[F_T2_TP],argv[i]);
			opt.t2_tp = YES;
		}

		// 制御ポイント挿入箇所解析リスト ファイル読込み
		else if(!strcmp(argv[i],"-cpi_ana_list")){
			i++;
			strcpy(file_names[F_CPI_ANA],argv[i]);
			opt.cpi_ana_list = YES;
		}

		// 制御ポイント挿入箇所解析結果 ファイル書き込み
		else if(!strcmp(argv[i],"-cpi_ana_log")){
			i++;
			strcpy(file_names[F_CPI_ANA_LOG],argv[i]);
			opt.cpi_ana_log = YES;
		}

		
		// RTLの有効状態ファイル ファイル読込み
		else if(!strcmp(argv[i],"-state")){
			i++;
			strcpy(file_names[F_STATE],argv[i]);
			opt.rtl_state = YES;
		}
		
		// RTLの有効状態のPINファイル ファイル読込み
		else if(!strcmp(argv[i],"-state_pin")){
			i++;
			strcpy(file_names[F_STATE_PIN],argv[i]);
			opt.rtl_pin = YES;
		}
				
		//  ファイル書込み
		else if(!strcmp(argv[i],"-r_state")){
			i++;
			strcpy(file_names[F_RSTATE],argv[i]);
			opt.r_state = YES;
		}
		
		//  ファイル書込み
		else if(!strcmp(argv[i],"-r_pin")){
			i++;
			strcpy(file_names[F_RSTATE_PIN],argv[i]);
			opt.r_pin = YES;
		}
		
		//----------------------------------------------
		else{
			fprintf(stderr,"Unrecognized option : %s\n",argv[i]);
			usage();
		}
	}
	
	
	//============================================================
	// 必須ファイルの設定があるかチェック
	//============================================================
	if(file_names[F_NET][0]=='\0'){
		printf("//-----------------------------------\n");
		printf("// FILE ERROR\n");
		printf("//-----------------------------------\n");
		printf("回路ファイル(.v)を読み込ませてください\n\n");
		exit(-1);
	}
	if(opt.r_state==NO && opt.cpi_analyze==NO && opt.trans_ffc_list==NO && file_names[F_TPTX][0]=='\0' && file_names[F_TPST][0]=='\0'){
		printf("//-----------------------------------\n");
		printf("// FILE ERROR\n");
		printf("//-----------------------------------\n");
		printf("テストパターンファイルを読み込ませてください\n\n");
		exit(-1);
	}
	if(file_names[F_TPTX][0]!='\0' && file_names[F_TPST][0]!='\0'){
		printf("//-----------------------------------\n");
		printf("// FILE ERROR\n");
		printf("//-----------------------------------\n");
		printf("テストパターンファイルを2種類(txtとstil)読み込ませないでください\n\n");
		exit(-1);
	}
	
	//============================================================
	// 【PINファイルチェック】
	//============================================================
	if(opt.tp_txt==YES && opt.pin==NO){
		printf("\n//-----------------------------------\n");
		printf("// ヽ(*ﾟдﾟ)ノ ERROR: PIN-FILE\n");
		printf("//-----------------------------------\n");
		printf("TXTテストパターンに対するPINファイルがありません!\n");
		printf("下記2つのコマンドを両方入れてください\n");
		printf("   -tx  : テストパターンファイル(TXT形式)\n");
		printf("   -pin : PINファイル\n");
	}


	//============================================================
	// 【コマンドエラーチェック】動作モード
	//============================================================
	if( (i = (int)opt.fsim + (int)opt.xid) > 1){
		printf("\n//-----------------------------------\n");
		printf("// ヽ(*ﾟдﾟ)ノ ERROR: Operation Mode\n");
		printf("//-----------------------------------\n");
		printf("下記のコマンドは併用不可!\n");
		printf("-fsim : 故障シミュレーション\n");
		printf("-xid  : ドントケア抽出\n");
	}

	//============================================================
	// 【コマンドエラーチェック】XIDモード
	//============================================================
	if( opt.xid==YES && opt.cpi_xid==YES){
		printf("\n//-----------------------------------\n");
		printf("// ヽ(*ﾟдﾟ)ノ ERROR: XID Mode\n");
		printf("//-----------------------------------\n");
		printf("下記のコマンドは併用不可!\n");
		printf("-xid    : ドントケア抽出\n");
		printf("-cpixid : 制御ポイント挿入ドントケア抽出\n");
	}

	//============================================================
	// 【コマンドエラーチェック】CPI-XIDモード
	//============================================================
	if( opt.cpi_xid==YES && opt.fmodel==SAF){
		printf("\n//-----------------------------------\n");
		printf("// ヽ(*ﾟдﾟ)ノ ERROR: CPI-XID Mode\n");
		printf("//-----------------------------------\n");
		printf("-cpixidは遷移故障モデルでのみ動作します\n");
	}

	/*
	//============================================================
	// 【コマンドエラーチェック】X抽出モード
	//============================================================
	if((i = (int)opt.miyase2008 + (int)opt.tp_vari + (int)opt.tp_x + (int)opt.pi_vari + (int)opt.pi_x + (int)opt.pitp_dc + (int)opt.pitp_mw) > 2){
		printf("\n//-----------------------------------\n");
		printf("// ヽ(*ﾟдﾟ)ノ ERROR: XID Mode\n");
		printf("//-----------------------------------\n");
		printf("下記のコマンドは併用不可!\n");
		printf("-f_dc    : 検出故障数均一化(Miyase2008)\n");
		printf("-tp_vari   : TPのケアビット数均一化\n");
		printf("-tp_x   : TPのケアビット数均一化(X抽出率考慮)\n");
		printf("-pi_vari   : PIのケアビット数均一化\n");
		printf("-pi_x   : PIのケアビット数均一化(X抽出率考慮)\n");
		printf("-pitp_dc : TPとPIのケアビット数均一化\n");
		printf("-pitp_mw : TPとPIのケアビット数均一化(X抽出率考慮)\n");
		exit(-1);
	}
	*/
	
}

//------------------------------------------------------------
// 関数名 : usage
// 機能   : Usageの表示
// 戻り値 : なし
// 引数   : なし
//------------------------------------------------------------
static void usage(void)
{
	fprintf(stderr,"\n//--------------------------------------------------\n");
	fprintf(stderr,"// ヽ(*ﾟдﾟ)ノ Command Help \n");
	fprintf(stderr,"//--------------------------------------------------\n");
	fprintf(stderr,"-c       [filename]  : 回路ファイル(.v)\n");
	fprintf(stderr,"-st      [filename]  : テストパターン(STIL形式)\n");
	fprintf(stderr,"-tp      [filename]  : テストパターン(txt形式)\n");
	fprintf(stderr,"-fsim    (saf / tdf) : 故障シミュレーションモード\n");
	fprintf(stderr,"-xid     (saf / tdf) : X抽出モード\n");
	fprintf(stderr,"-flist   [filename]  : 故障リスト読み込み\n");
	fprintf(stderr,"-ost     [filename]  : X抽出後のテストパターン出力(STIL形式)\n");
	fprintf(stderr,"-otx     [filename]  : X抽出後のテストパターン出力(txt形式)\n");
	fprintf(stderr,"-dic     [filename]  : 故障辞書出力\n");
	fprintf(stderr,"-det     [filename]  : 検出故障リスト出力\n");
	fprintf(stderr,"-nodet   [filename]  : 未検出故障リスト出力\n");
	fprintf(stderr,"-log     [filename]  : ログファイル出力\n");
	fprintf(stderr,"-f_dc    (YES / NO)  : 検出故障数均一化(Miyase2008)\n");
	fprintf(stderr,"-tp_vari   (YES / NO)  : TPのケアビット数均一化\n");
	fprintf(stderr,"-tp_x   (YES / NO)  : TPのケアビット数均一化(X抽出率考慮)\n");
	fprintf(stderr,"-pi_vari   (YES / NO)  : PIのケアビット数均一化\n");
	fprintf(stderr,"-pi_x   (YES / NO)  : PIのケアビット数均一化(X抽出率考慮)\n");
	fprintf(stderr,"-pitp_dc (YES / NO)  : TPとPIのケアビット数均一化\n");
	fprintf(stderr,"-pitp_mw (YES / NO)  : TPとPIのケアビット数均一化(X抽出率考慮)\n");
	fprintf(stderr,"-M_HD    [数値]      : [数値]%の検出困難故障TPを犠牲にするか\n");
	fprintf(stderr,"-N_HD    [数値]      : 検出困難故障の定義(検出回数：[数値]未満)\n");
	fprintf(stderr,"//--------------------------------------------------\n\n");

	exit(1);
}
//---------------------------------------------------------------------


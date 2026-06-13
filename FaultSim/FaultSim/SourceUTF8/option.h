//------------------------------------------------------------------------
//File name : oprion.h
//Date : 2011/6/13
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//----------------------------------------------------------
// diff_path_xidのモード定義
//----------------------------------------------------------
#define		RAND_FIX			0		//ランダムで異なるパスを選択(※固定済み信号線が存在した場合は固定済み信号線を選択）
#define		RAND				1		//ランダムで異なるパスを選択(※固定済み信号線が存在しても異なる信号線を選択）
#define		DIFF				2		//絶対異なるパスを選択(※固定済み信号線が存在しても異なる信号線を選択）


//----------------------------------------------------------
// その他
//----------------------------------------------------------
#define		YES				1
#define		NO				0
#define		UK				(-1)
#define		MAXN			100000


//----------------------------------------------------------
// 故障モデル
//----------------------------------------------------------
#define		SAF				11	//縮退故障
#define		TDF				22	//遷移故障


//----------------------------------------------------------
// ファイル
//----------------------------------------------------------
#define		F_NET			0	// ネットリスト						読込み(必須)
#define		F_TPTX			1	// テストパターン(.txt)				読込み(どちらか必須)
#define		F_TPST			2	// テストパターン(.stil)			読込み(どちらか必須)
#define		F_PIN			3	// PINファイル						読込み(txtモード時必須)
#define		F_FLT			4	// 故障リスト　						読込み
#define		F_FLSI			5	// 故障LSI情報　						読込み
#define		F_CPI			6	// 制御ポイント挿入リスト			読込み

#define		F_XTXT			7	// X抽出後テストパターン(.txt)		書き込み
#define		F_XSTIL			8	// X抽出後テストパターン(.stil)		書き込み
#define		F_STTX			9	// STIL⇒txt変換					書き込み
#define		F_DIC			10	// 故障辞書							書き込み
#define		F_DET			11	// 検出故障リスト					書き込み
#define		F_UNDET			12	// 未検出故障リスト					書き込み
#define		F_LOG			13	// 全体ログファイル					書き込み
#define		F_XLOG			14	// XIDログファイル					書き込み
#define		F_MISS_LOG		15	// 見逃し故障XIDログファイル		書き込み
#define		F_A_LOG			16	// XIDログファイル					※追記
						
#define		F_TP_HDF		17	// 各TPが含んでいる検出困難故障数	書き込み

#define		F_DIAG			18	// 故障診断用FISM結果				書き込み
#define		F_DDTP			19	// 二重検出後のテストパターン		書き込み
#define		F_EXVAL			20	// 論理SIM後の出力期待値			書き込み
#define		F_EXPIN			21	// 出力期待値の(P)POピン情報		書き込み
#define		F_LSIM			22	// 論理SIM後の論理値				書き込み
#define		F_OPIN			23	// PINファイル						書き込み
#define		F_MISS			24	// 見逃し故障リスト					書き込み
#define		F_ALL_VAL		25	// 全信号線の論理値(DEBUG用)		書き込み
#define		F_REP			26	// 代表故障リスト					書き込み

#define		F_ORI_TP		27	// PI+PPIのみのテスト集合(CPI_XID用)書き込み
#define		F_CP_TP			28	// PI+CPのみのテスト集合(CPI_XID用) 書き込み
#define		F_T2_TP			29	// PIと2時刻目PPI値の出力			書き込み

//解析用
#define		F_CPI_ANA		30	// 制御ポイント挿入個所解析リスト	読込み
#define		F_CPI_ANA_LOG	31	// 制御ポイント挿入箇所解析結果		書き込み
#define		F_TRANS_FFC		32	// 返還対象のFFCファイル			読込み

#define		F_STATE			33	// RTLの有効状態					読込み
#define		F_STATE_PIN		34	// 有効状態のPINファイル			読込み

#define		F_RSTATE		35	// 到達可能状態						読込み
#define		F_RSTATE_PIN	36	// 到達可能状態のPINファイル		読込み

#define		F_FLT_TP_PPO	37	// TP指定XID(故障_TP_PPOファイル)	読込み

#define		N_FILE			38	// ファイルの数


//----------------------------------------------------------
// X抽出の経路選択オプション
//----------------------------------------------------------
#define		LEVEL			10	// 最小レベルの経路を選択
#define		COST			11	// 最小可制御性の経路を選択
#define		PRI_LEVEL		12	// 優先パスもしくは最小レベルの経路を選択
#define		PRI_COST		13	// 優先パスもしくは最小可制御性の経路を選択
#define		MAX_LEVEL		14	// 最大レベルの経路を選択(なるべく最長パスにしたい)　※2014/07/25飯山用


//----------------------------------------------------------
// コマンドラインオプション
//----------------------------------------------------------
typedef struct _option_ {
	
	//--------------------------
	// 基本設定
	//--------------------------
	char	fmodel;				// 故障モデル									{SAF,TDF}
	char 	xid;				// X抽出										{YES,NO}
	char 	fsim;				// 故障シミュレーション							{YES,NO}
	char 	f_diag;				// 故障診断FSIM(疑似テスター)					{YES,NO}
	char 	double_det;			// 二重検出法									{YES,NO}
	char	cpi_xid;			// 制御ポイント挿入X抽出						{YES,NO}
	char	cpi_fsim;			// 制御ポイント挿入回路のFSIM					{YES,NO}
	char	cpi_analyze;		// 制御ポイント挿入箇所解析モード				{YES,NO}
	char	trans_ffc_list;		// FFCリストの変換モード						{YES,NO}

	//--------------------------
	// FSIMオプション
	//--------------------------
	int		rpg;				// ランダムパターン生成							{0:OFF 1以上:ON}
	int		n_drop;				// 初期テスト集合FSIM時に何回検出まで計算するか	{1以上}			(Miyase2004モード時に使用)
	char	drop_fsim;			// FSIM単体動作時に故障ドロップするか			{YES,NO}

	//--------------------------
	// X抽出オプション
	//--------------------------
	int		n_xid;				// 各故障に対するXIDの最大実行回数				{1以上}			(見逃し故障に対して何回頑張るか)
	int		diff_path_n_xid;	// 見逃し故障に対して異なるパスでXIDする回数	{0以上}			(XIDは拡張含意は使わない)
	char	diff_path_mode;		// 見逃し故障他パスXIDのモード					{RAND_FIX, RAND_DIFF:ランダム, DIFF_FIX, DIFF_DIFF:異なるパス}
	char	miyase2004;			// Miyase2004モード								{YES,NO}
	char	miyase2008;			// 検出故障数均一化 (Miyase2008)				{YES,NO}
	char	tp_vari;			// TPケビット均一化								{YES,NO}
	char	tp_x;				// TPケビット均一化 & X率考慮					{YES,NO}
	char	pi_vari;			// (疑似)外部入力ケアビット均一化				{YES,NO}
	char	pi_x;				// (疑似)外部入力ケアビット均一化 & X率考慮		{YES,NO}
	char	pitp_dc;			// TPと(疑似)外部入力ケアビット均一化			{YES,NO}
	char	pitp_mw;			// TPと(疑似)外部入力ケアビット均一化 & X率考慮	{YES,NO}
	double	M_HD_tp;			// M:何%の検出困難故障TPを犠牲にするか			{0:OFF 0超過:ON}
	int		N_HD_fault;			// N:検出困難故障の定義(検出回数:以下)			{0:OFF 1以上:ON}
	char	run_dsatur;			// X抽出後テスト集合に対するDsaturの実行		{YES,NO}
	
	//経路選択オプション
	char	xid_jus;			// 正当化経路選択オプション		{LEVEL, COST, PRI_LEVEL, PRI_COST, MAX_LEVEL}
	char	xid_fpath;			// 故障伝搬経路選択オプション	{LEVEL, COST, PRI_LEVEL, PRI_COST, MAX_LEVEL}

	//--------------------------
	// CPI_XID用オプション
	//--------------------------
	int		p_cpi;				// 制御ポイント挿入割合(%)						{0(%)〜100(%)}
	char	pri_xid;			// 制御ポイント影響範囲内故障故障の優先XID		{YES,NO}
	char	priority_path;		// 制御ポイント挿入による優先パスXIDの利用		{YES,NO}

	//--------------------------
	// X抽出解析用オプション
	//--------------------------
	char	out_tp_hdf;			// 各TPが含んでいる検出困難故障数 ファイル出力	{YES,NO}

	//--------------------------
	// 高速化オプション
	//--------------------------
	int		rep_levelize;		// 代表信号線レベライズ						{YES,NO}
	
	//--------------------------
	// DEBUGオプション
	//--------------------------
	char	debug_sppfp;		// SPPFPのデバッグオプション					{YES,NO}
	
	//--------------------------
	// 表示オプション
	//--------------------------
	char	fault_coverage_rep;	//故障検出率の分母を代表故障に設定				{YES,NO}	//NOの場合は『全信号線*2』が分母

	//--------------------------
	// ファイル入力
	//--------------------------
	char	flt;				// 故障リスト読込み　			ファイル入力	{YES,NO}
	char	flsi;				// 故障LSI読み込み(故障診断用)	ファイル入力	{YES,NO}
	char	tp_txt;				// TPファイル読込み(.txt)		ファイル入力	{YES,NO}
	char	tp_stil;			// TPファイル読込み(.stil)		ファイル入力	{YES,NO}
	char	pin;				// PINファイル読込み			ファイル入力	{YES,NO}
	char	cpi_list;			// 制御ポイント挿入リスト		ファイル入力	{YES,NO}
	char	cpi_ana_list;		// 制御ポイント挿入箇所解析リストファイル入力	{YES,NO}
	char	rtl_state;			// RTLの有効状態リスト			ファイル入力	{YES,NO}
	char	rtl_pin;			// 有効状態リストのPIN			ファイル入力	{YES,NO}
	char	xid_flt_tp_ppo;		// TP指定XID(故障_TP_PPOファイル)ファイル入力	{YES,NO}

	//--------------------------
	// ファイル出力
	//--------------------------
	char	xtp_tx;				// テストパターン(.txt)			ファイル出力	{YES,NO}
	char	xtp_st;				// テストパターン(.stil)		ファイル出力	{YES,NO}
	char	st_to_txt;			// テストパターン(STIL⇒txt)	ファイル出力	{YES,NO}
	char	dic;				// 故障辞書						ファイル出力	{YES,NO}
	char	rep;				// 代表故障リスト				ファイル出力	{YES,NO｝
	char	det;				// 検出故障リスト				ファイル出力	{YES,NO｝
	char	undet;				// 未検出故障リスト				ファイル出力	{YES,NO｝
	char	log;				// ログ							ファイル出力	{YES,NO}
	char	xid_log;			// XIDログ						ファイル出力	{YES,NO}
	char	miss_log;			// 見逃し故障ログ				ファイル出力	{YES,NO}
	char	miss_list;			// 見逃し故障リスト				ファイル出力	{YES,NO}
	char	add_log;			// XIDログの追記モード			ファイル出力	{YES,NO}
	char	exp_val;			// 論理SIM後の出力期待値		ファイル出力	{YES,NO}
	char	exp_pin;			// 出力期待値に対応するPIN		ファイル出力	{YES,NO}
	char	lsim_val;			// 論理SIM後の論理値値			ファイル出力	{YES,NO}
	char	out_pin;			// PINファイル出力				ファイル出力	{YES,NO}
	char	all_val;			// 全信号線の論理値				ファイル出力	{YES,NO}
	char	ori_tp;				// PI+PPIのみのテスト集合		ファイル出力	{YES,NO}
	char	cp_tp;				// PI+CPのみのテスト集合		ファイル出力	{YES,NO}
	char	t2_tp;				// 初期テスト集合のPIと2時刻目PPI値のファイル出力{YES,NO}
	char	cpi_ana_log;		// 制御ポイント挿入箇所解析結果 ファイル出力	{YES,NO}
	char	r_state;			// 到達可能状態					ファイル出力	{YES,NO}
	char	r_pin;				// 到達可能状態のPIN			ファイル出力	{YES,NO}

} OPTION;

OPTION opt;

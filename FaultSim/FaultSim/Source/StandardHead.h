//------------------------------------------------------------------------
//File name : StandardHead.h
//Date : 2011/06/14
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	"./Netlist/netlist.h"
#include	"./Netlist/s_netlist.h"
#include	"./Lib/bit_tp.h"
#include	"./Lib/bit_int.h"
#include	"./Lib/alloc.h"
#include	"./Lib/queue.h"

//========================================================================
// 定義
//========================================================================
#ifndef StandardHead_C
#define StandardHead_C

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//========================================================================
// プロトタイプ宣言
//========================================================================
void	NL_FlagReset();
void	SNL_FlagReset();

//========================================================================
// 構造体
//========================================================================
//---------------------------------------------------
//テストパターン格納 (n_pi数分保持)
//---------------------------------------------------
typedef struct TestPatternHead_{
	BIT_TP			*tp;					//テストパターン
	unsigned int	flag;					//多目的フラグ
	int				nl_id;					//ネットリストID
	char			pin_name[1024];			//(疑似)外部入力名
	NLIST			*input;					//対象外部入力へのポインタ【縮退用】
	S_NLIST			*sinput;				//対象(疑似)外部入力へのポインタ【遷移用】
}TPH;


//---------------------------------------------------
//X抽出後テストパターンの故障検出情報 (n_tp数分保持)
//---------------------------------------------------
typedef struct TestPattern_Info{
	int				tp_id;					//テストパターン番号
	int				ui_num;					//対象テストパターンがunsigned intの何個目か
	int				bit_num;				//対象テストパターンがunsigned intの何bit目か
	int				n_xid_fault;			//テストパターンごとのXIDした故障数	(代表故障のみ)
	int				n_essential_fault;		//テストパターンごとの必須故障XIDした故障数		(代表故障のみ)
	int				n_HD_fault;				//テストパターンごとの検出可能検出困難故障数	(代表故障のみ)
	int				n_many_carebit_fault;	//テストパターンごとの多量ケアビット必要故障数	(代表故障のみ)
	int				n_cpi_fault;			//制御ポイント挿入範囲内故障数
	int				n_xid_det;				//XID後パターンでの検出故障数
}TPINFO;



//---------------------------------------------------
//二重検出法テーブル(n_tp数分保持)
//---------------------------------------------------
typedef struct DD_Table{
	int				tp_id;					//テストパターン番号
	int				select;					//テストパターン選択状態{YES(残す), NO(削除するパターン)}
	int				n_detect_fault;			//テストパターンごとの検出可能故障数			(代表故障のみ)
	int				n_essential_fault;		//テストパターンごとの検出可能必須故障数		(代表故障のみ)
}DDTBL;


//========================================================================
// 関数ポインタ用外部変数
//========================================================================
//【縮退故障用】
void(*func_ed_ppsfp[20])(NLIST*, int, unsigned int);	//PPSFP用の関数ポインタ
void(*func_cpt0[20])(int, unsigned int, NLIST*);		//出力値0からのCPTの関数ポインタ
void(*func_cpt1[20])(int, unsigned int, NLIST*);		//出力値1からのCPTの関数ポインタ

//【遷移故障用】
void(*sfunc_ed_ppsfp[20])(S_NLIST*, int, unsigned int);	//PPSFP用の関数ポインタ
void(*sfunc_cpt0[20])(int, unsigned int, S_NLIST*);		//出力値0からのCPTの関数ポインタ
void(*sfunc_cpt1[20])(int, unsigned int, S_NLIST*);		//出力値1からのCPTの関数ポインタ

//LSIM用
void(*sfunc_t1_lsim[20])(S_NLIST*);						//1時刻目LSIM用の関数ポインタ
void(*sfunc_t2_lsim[20])(S_NLIST*);						//2時刻目LSIM用の関数ポインタ

//XID時のLSIM用
void(*sfunc_t1_xid_lsim[20])(S_NLIST*);					//1時刻目XID_LSIM用の関数ポインタ
void(*sfunc_t2_xid_lsim[20])(S_NLIST*);					//2時刻目XID_LSIM用の関数ポインタ

//分散制御XID時のLSIM用
void(*sfunc_t1_dis_lsim[20])(S_NLIST*);					//※分散制御XID用の1時刻目LSIM用の関数ポインタ
void(*sfunc_t2_dis_lsim[20])(S_NLIST*);					//※分散制御XID用の2時刻目LSIM用の関数ポインタ
void(*sfunc_dis_ppsfp[20])(S_NLIST*, unsigned int);		//※分散制御XID用のPPSFP関数ポインタ


//========================================================================
// 外部変数
//========================================================================
//キュー
QUEUE			*nl_queue;			//n_net*2サイズのキュー(正当化やCPTなど)
QUEUE			*fimpl_queue;		//n_net*2サイズのキュー(前方含意専用)
QUEUE			*bimpl_queue;		//n_net*2サイズのキュー(後方含意専用)

//32ビットMASK
unsigned int	*MASKbit;

//テストパターン
TPINFO			*tp_info;			//X抽出後テストパターンの故障検出情報
TPINFO			**sort_tp;			//制御ポイント挿入箇所範囲内故障数でソートしたポインタ


TPH				**pin_tbl;			//PIN順序格納順テーブル(テストパターン入出力時に使用)
TPH				*pi_tp;				//外部入力
TPH				*ppi_tp;			//疑似外部入力
int				n_tp;				//テストパターン数
unsigned int	n_tp_int;			//X,Pバッファの配列の数{1+（n_tp/32）}

//回路情報
int			n_rep;					//テスト対象故障数
int			n_ffr;					//FFR数 (ステム数+PO数(+PPO数))
int			n_essential_fault;		//必須故障数
int			n_dpxid_fault;			//異パスXIDで検出できた故障数(≒見逃し故障)
int			n_missed_fault;			//見逃し故障数
int			n_hd_fault;				//検出困難故障数
int			n_cpi_corn_fault;		//制御ポイントコーン内の故障数

//WARNINGカウンタ
int			n_not_found_fault;		//故障リスト読込みモード時に見つからなかった故障数
int			n_not_found_ffc;		//制御ポイント挿入リスト読込みモード時に見つからなかったFFC数
int			n_not_found_link_CP_FF;	//CPとFF間のポインタ接続時に見つからなかった個数
int			n_not_found_link_FFC_FF;//FFCとFF間のポインタ接続時に見つからなかった個数

//X抽出
int			n_before_x;				//テストパターン集合中のドントケア数
int			n_after_x;				//ドントケア抽出後テストパターン中のドントケア数
int			n_missed_cbit;			//見逃し故障ドントケア抽出で増加したPIのケアビット数
NLIST		**flt_tbl;				//XID用故障ソートテーブル(組合せ回路用)
S_NLIST		**flt_stbl;				//XID用故障ソートテーブル(順序回路用)

//CPI_X抽出
int			n_ori_x;				//PI,オリジナルPPIに含まれるX数
int			n_cpi_x;				//PI,オリジナルPPI,制御ポイント挿入部(FFC=1の所のみ)に含まれるX数
int			n_pi_cp_x;				//PIとCP(FFC=1の所のみ)に含まれるX数

//故障シミュレーション
int			n_detect;				//初期テスト集合での故障検出数(1故障1回でカウント)
int			n_xid_detect;			//X抽出後テスト集合での故障検出数(1故障1回でカウント)
int			max_level;				//最大ゲート段数(レベライズ)

//制御ポイント
int			n_cpi;					//制御ポイント挿入数(opt.p_cpiより計算)
int			n_dff_x;				//X抽出後の2時刻目PPIに含まれているX数
int			n_rep_x;				//X抽出後の1時刻目回路応答に含まれているX数
int			n_min_comtp;			//圧縮後テストパターン数の最小値

//故障辞書
BIT_INT		**fdic_sa0;				//0縮退故障の故障辞書(1:故障検出　0:故障未検出)
BIT_INT		**fdic_sa1;				//1縮退故障の故障辞書(1:故障検出　0:故障未検出)
BIT_INT		**fdic_str;				//立上り遷移故障の故障辞書(1:故障検出　0:故障未検出)
BIT_INT		**fdic_stf;				//立下り遷移故障の故障辞書(1:故障検出　0:故障未検出)

BIT_INT		**xid_fdic_sa0;			//X抽出後テスト集合の0縮退故障の故障辞書(1:故障検出　0:故障未検出)
BIT_INT		**xid_fdic_sa1;			//X抽出後テスト集合の1縮退故障の故障辞書(1:故障検出　0:故障未検出)
BIT_INT		**xid_fdic_str;			//X抽出後テスト集合の立上り遷移故障の故障辞書(1:故障検出　0:故障未検出)
BIT_INT		**xid_fdic_stf;			//X抽出後テスト集合の立下り遷移故障の故障辞書(1:故障検出　0:故障未検出)

BIT_INT		**fdic_po;				//故障検出したTPとPOの辞書(1:故障検出　0:故障未検出)


//STILからの情報
int			n_allocation_input;		//割当て入力ピン数
int			n_scan_ff;				//順序(STIL)のときのスキャンチェインFF数(※≠n_dff)
int			n_se_input;				//順序(STIL)のときの外部入力(PI)数

//二重検出法
DDTBL		*dd_tbl;			//二重検出法のテーブル


#endif
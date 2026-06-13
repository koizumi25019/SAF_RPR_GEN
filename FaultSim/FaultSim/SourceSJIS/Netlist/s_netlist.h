//------------------------------------------------------------------------
//Netlist 定義, 外部変数, プロトタイプ宣言(パクリ)
//File name : s_nelist.h
//Date : 2009/06/26
//Designer : S.Kashiwazaki
//Ver : 0.00（s_netへの改編Ver）
//------------------------------------------------------------------------
#include	"../Lib/bit_tp.h"
#include	"../Lib/bit_int.h"

#ifndef SNetlist_C
#define SNetlist_C

//========================================================================
// 定義
//========================================================================
//---------------------------------------------------
//ゲートタイプ
//---------------------------------------------------
#define  IN			0
#define  BUF		10
#define  INV		11
#define  AND		12
#define  NAND		13
#define  OR			14
#define  NOR		15
#define  EXOR		16	//2入力のみ可
#define  EXNOR		17	//2入力のみ可
#define  FOUT		18
#define  DFF		19	//D-フリップフロップ
#define  RDFF		21	//D-フリップフロップ（リセット有）
#define  DFFS		23	//D-スキャンフリップフロップ
#define  RDFFS		24	//D-スキャンフリップフロップ（リセット有）
#define  GND		25	//グランド
#define  ACC		26	//電源


//---------------------------------------------------
//故障タイプ
//---------------------------------------------------
#define  TDF_STR	0	//立上り遷移故障(≒0縮退)
#define  TDF_STF	1	//立下り遷移故障(≒1縮退)

//---------------------------------------------------
// コスト
//---------------------------------------------------
#define		C_0			0	// 0可制御性コスト
#define		C_1			1	// 1可制御性コスト

#define		N_COST		2	// コストの種類(配列数)


//========================================================================
// 構造体
//========================================================================
//---------------------------------------------------
//【順序回路用】遷移故障用ネットリスト構造体定義
//---------------------------------------------------
typedef struct S_Netlist_Format_ {

	char					*name;				//名前
	int						n;					//ID（ユニーク）
	int						type;				//タイプ
	int						n_in;				//入力数
	struct S_Netlist_Format_ **in;				//入力ポインタ配列
	int						n_out;				//出力数
	struct S_Netlist_Format_ **out;				//出力ポインタ配列
	char					*name_port;			//端子名
	char					*name_ins;			//インスタンス名

	
	//---------------------------------------------------
	//山崎追加	
	//---------------------------------------------------
	//テストパターン関係	
	BIT_TP					*nval_t1;			//1時刻目正常値(論理SIM値: X-buff P-buff)
	BIT_TP					*nval_t2;			//2時刻目正常値(論理SIM値: X-buff P-buff)
	BIT_TP					*xid_nval_t1;		//1時刻目X抽出後のテストパターン正常値(論理SIM値: X-buff P-buff)
	BIT_TP					*xid_nval_t2;		//2時刻目X抽出後のテストパターン正常値(論理SIM値: X-buff P-buff)
	unsigned int			x_fault;			//X-buffer故障値(2時刻目のみ)
	unsigned int			p_fault;			//P-buffer故障値(2時刻目のみ)

	//故障シミュレーション関係
	int						level;				//ゲート段数(レベライズ)
	int						ffr_id;				//FFR ID
	struct S_Netlist_Format_ *fout_stem;		//FFRの先頭(ステム)へのポインタ
	int						f_stem;				//FOUT_STEM								{ YES, NO }
	int						ppo_flag;			//PPOフラグ								{ YES, NO }
	int						test_str;			//立上り遷移故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int						test_stf;			//立下り遷移故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int						det_str;			//立上り遷移故障の検出回数				{ 0～検出回数 }
	int						det_stf;			//立下り遷移故障の検出回数				{ 0～検出回数 }
	int						testable_net;		//遷移故障テスト可能信号線				{ YES(構造的テスト可能信号線), NO(構造的テスト不可能信号線)}

	//SPPFP
	int						nbit_fault;			//故障が何ビット目に割当てられてるか(0～31)
	int						one_tp_nval;		//シングルパターン正常値(1tpのSPPFPで使用)	{0:0, 1:1, X:3}

	//X抽出関係
	unsigned int			xid_flag;			//X抽出用のフラグ
	int						xid_det_str;		//X抽出後TPでの立上り遷移故障の検出状態	{ YES(検出), NO(未検出) }
	int						xid_det_stf;		//X抽出後TPでの立下り遷移故障の検出状態	{ YES(検出), NO(未検出) }
	int						n_str_xid;			//立上り遷移故障をX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_stf_xid;			//立下り遷移故障をX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_str_diff_xid;		//立上り遷移故障を他パスX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_stf_diff_xid;		//立下り遷移故障を他パスX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_str_carebit;		//立上り遷移故障をX抽出した際に必要だったケアビット数
	int						n_stf_carebit;		//立下り遷移故障をX抽出した際に必要だったケアビット数
	int						n_carebit;			//トータル何回ケアビット割当てされたか(0～対象故障数)
	struct S_XID_LIST_		*xidlist_str;		//立上り遷移故障のX抽出対象リストポインタ
	struct S_XID_LIST_		*xidlist_stf;		//立下り遷移故障のX抽出対象リストポインタ
	int						n_corn_pi;			//【PPOのみ】対象PPOのコーン内PI+PPI数(最大で何本のPI+PPIが必要か)※少ないほどX抽出に最適なPOかも
	int						jus_flag;			//限定正当化エンキュー済みフラグ{1:1時刻目, 2:2時刻目, 3:1，2時刻目を限定正当化}

	//分散制御XID関係
	BIT_TP					*dis_nval_t1;		//分散制御XID用の1時刻目正常値(バラバラの32パターンを固めてLSIM)
	BIT_TP					*dis_nval_t2;		//分散制御XID用の2時刻目正常値(バラバラの32パターンを固めてLSIM)

	//CPI_XID関係
	struct S_Netlist_Format_ **link_cpi;		//CPI付加回路ポインタ { FF:([0]->CP, [1]->FFC) , CP:([0]->FF) , FFC:([0]->FF) }
	unsigned int			pri_path;			//CPI_XID用の優先パスフラグ
	unsigned int			cpi_lsim;			//CPI_XID用のLSIMフラグ

	//可制御性(SCOAP)
	unsigned int			cost[N_COST][2];	// コスト([種類][時刻])		2時刻分

	//代表信号線関係
	int						rep_net;			//代表信号線						{ YES, NO }
	int						inv_flag;			//反転フラグ						{ YES, NO }
	struct S_Netlist_Format_ *influence_net;	//影響信号線
	struct S_Netlist_Format_ **next_event;		//次イベントドリブン信号線
	int						n_next_event;		//次イベントドリブン信号線数
	
	//二重検出関係
	int						dd_det_str;			//二重検出用の立上り遷移故障の検出回数	{ 0～検出回数 }
	int						dd_det_stf;			//二重検出用の立下り遷移故障の検出回数	{ 0～検出回数 }

	//その他
	unsigned int			flag;				//多目的フラグ
	int						pi_id;				//pi[i]のi番目
	int						ppi_id;				//ppi[i]のi番目
	int						ppo_id;				//ppo[i]のi番目
	int						missed_flag;		//見逃し故障フラグ {0:見逃し故障でない， 1:STR見逃し， 2:STF見逃し， 3:両方見逃し}
	BIT_INT					*cpi_ana_out_flag;	//出力側用の制御ポイント挿入箇所解析用フラグ
	BIT_INT					*cpi_ana_in_flag;	//入力側用の制御ポイント挿入箇所解析用フラグ

}S_NLIST;



//========================================================================
// 【順序回路用】外部変数
//========================================================================
//遷移故障用ネットリスト構造体配列
S_NLIST *s_nl;

//遷移故障テスト対象信号線数
int	n_testable_net;

//モジュール名
char *smodule_name;

//信号数
int n_snet;

//外部入力数, 外部入力
S_NLIST **spi;
int n_spi;

//外部出力数, 外部出力
S_NLIST **spo;
int n_spo;

//PPI, PPI数(※PPI=DFF)
S_NLIST **ppi;
int n_ppi;

//PPO, PPO数
S_NLIST **ppo;
int n_ppo;

//DFF, DFF数(※DFF=PPI)
S_NLIST **sdff;
int n_sdff;

//RDFF, RDFF数
S_NLIST **srdff;
int n_srdff;

//DFFS, DFFS数
S_NLIST **sdffs;
int n_sdffs;

//RDFFS, RDFFS数
S_NLIST **srdffs;
int n_srdffs;

//assign
S_NLIST **sassign;
int n_sassign;

//---------------------------------------------------
// 【順序回路用】CPI_XID用外部変数
//---------------------------------------------------
S_NLIST **ori_pi;		//オリジナルPIポインタ
int		n_ori_pi;		//オリジナルPI数 (n_spi - n_cp)

S_NLIST **ori_dff;		//オリジナルDFFポインタ
int		n_ori_dff;		//オリジナルDFF数 (=n_cp =n_ffc)

S_NLIST **cp;			//制御ポイント(CP(PI))ポインタ
int		n_cp;			//制御ポイント数 (=n_ffc)

S_NLIST **ffc;			//FFCポインタ
int		n_ffc;			//FFC数 (=n_cp)


//---------------------------------------------------
// 【順序回路用】FOUTステムポインタ
//---------------------------------------------------
S_NLIST **sfstem;		//FOUT-STEMポインタ
int		n_sfstem;		//FOUT-STEM数

//---------------------------------------------------
// 【順序回路用】計算順序配列
//---------------------------------------------------
typedef struct SLevelize{
	int		n_net;		//信号線数
	S_NLIST **net;		//信号線ポインタ
}SLEV;

SLEV	*nml_slev;		//ノーマルレベライズ
SLEV	*rep_slev;		//代表信号線レベライズ

SLEV	*slev_temp;		//レベライズスタック


//---------------------------------------------------
// 【順序回路用】FFR構造体
//---------------------------------------------------
typedef struct SFoutFreeRegion{
	int						ffr_id;				//FFRのID
	int						Reconv;				//再収斂フラグ	{YES, NO, UK(経路によっては再収斂したりしなかったり)}
	int						n_out;				//他FFRへの出力数
	struct SFoutFreeRegion	**out;				//他出力FFRへのポインタ
	int						n_in;				//他FFRへの入力数
	struct SFoutFreeRegion	**in;				//他入力FFRへのポインタ
	int						n_pi;				//FFR内の外部入力数
	int						n_ppi;				//FFR内の疑似外部入力数
	int						n_in_net;			//FFR内の入力数(入力はINとFOUTブランチとDFF(PPI)の3種類　※他FFRからの入力では無い)
	S_NLIST					*FoutStem;			//FOUT_STEMへのポインタ
	int						n_fault;			//FFR内に含まれる故障数						※立上り遷移と立下り遷移の両方をカウント
	int						n_det_fault;		//FFR内で検出した総故障数(初期テスト集合)	※1故障複数カウント
	int						n_detect;			//FFR内で検出した故障数(初期テスト集合)		※1故障1カウント
	int						n_xid_detect;		//FFR内で検出した故障数(X抽出後テスト集合)	※1故障1カウント
	BIT_INT					*det_tp;			//FFR内の故障が検出されたパターンにフラグ
	unsigned int			flag;				//再収斂解析で使用するフラグ
}SFFR;

SFFR *sffr;


//---------------------------------------------------
// 【順序回路用】X抽出対象故障リスト 線形リスト構造(検出回数の昇順でソート)
//---------------------------------------------------
typedef struct S_XID_LIST_{
	S_NLIST					*net;				//ネットリストへのポインタ
	struct S_XID_LIST_		*next;				//後続ノードポインタ
	int						fault_type;			//故障タイプ{立上り遷移故障:0 , 立下り遷移故障:1}
	int						n_detect;			//初期テスト集合での検出回数
	int						detect;				//検出済みか{YES:X抽出しない(他TPの故障SIMで落ちた) , NO:X抽出する}
}SXID_LIST;


//---------------------------------------------------
// 【順序回路用】X抽出対象故障リストの先頭アドレス
//---------------------------------------------------
typedef struct S_XID_LIST_HEAD{
	SXID_LIST	*xid_list_head;		//線形リストの先頭アドレス
	int			n_fault;			//対象故障数(X抽出ごとに変動)
}SXID_LIST_H;

//先頭アドレス構造体
SXID_LIST_H	sxid_head;



//----------------------------------------------------------
// 【順序回路用】スタック(なんかで使うかも)
//----------------------------------------------------------
typedef struct s_l_stack_ {
	S_NLIST					*net;	//データ
	struct s_l_stack_		*next;	//後続ノードポインタ
} SN_STACK;


#endif
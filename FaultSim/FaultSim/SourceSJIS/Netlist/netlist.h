//------------------------------------------------------------------------
//Netlist 定義, 外部変数, プロトタイプ宣言
//File name : nelist.h
//Date : 2007/04/03
//Designer : R.Inoue (Y.Omori)
//Ver : 3.01（改良Ver）
//------------------------------------------------------------------------
#include	"../Lib/bit_tp.h"
#include	"../Lib/bit_int.h"

#ifndef Netlist_C
#define Netlist_C

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
#define  SAF0		0	//0縮退故障
#define  SAF1		1	//1縮退故障


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
//【組合せ回路用】縮退故障用ネットリスト構造体定義
//---------------------------------------------------
typedef struct _Netlist_Format_ {

	char					*name;				//名前
	int						n;					//ID（ユニーク）
	int						type;				//タイプ
	int						n_in;				//入力数
	struct _Netlist_Format_ **in;				//入力ポインタ配列
	int						n_out;				//出力数
	struct _Netlist_Format_ **out;				//出力ポインタ配列
	char					*name_port;			//端子名
	char					*name_ins;			//インスタンス名
	
	//---------------------------------------------------
	//山崎追加	
	//---------------------------------------------------
	//テストパターン関係	
	BIT_TP					*nval;				//正常値(論理SIM値: X-buff P-buff)
	BIT_TP					*xid_nval;			//X抽出後のテストパターン正常値(論理SIM値: X-buff P-buff)
	unsigned int			x_fault;			//X-buffer故障値
	unsigned int			p_fault;			//P-buffer故障値

	//故障シミュレーション関係
	int						level;				//ゲート段数(レベライズ)
	int						ffr_id;				//FFR ID
	struct _Netlist_Format_ *fout_stem;			//FFRの先頭(ステム)へのポインタ
	int						f_stem;				//FOUT_STEM							{ YES, NO }
	int						test_sf0;			//0縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int						test_sf1;			//1縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int						det_sf0;			//0縮退故障の検出回数				{ 0～検出回数 }
	int						det_sf1;			//1縮退故障の検出回数				{ 0～検出回数 }

	//SPPFP
	int						nbit_fault;			//故障が何ビット目に割当てられてるか(0～31)
	int						one_tp_nval;		//シングルパターン正常値(1tpのSPPFPで使用)	{0:0, 1:1, X:3}

	//X抽出関係
	unsigned int			xid_flag;			//X抽出用のフラグ
	int						xid_det_sf0;		//X抽出後TPでの0縮退故障の検出状態	{ YES(検出), NO(未検出) }
	int						xid_det_sf1;		//X抽出後TPでの1縮退故障の検出状態	{ YES(検出), NO(未検出) }
	int						n_sa0_xid;			//0縮退故障をX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_sa1_xid;			//1縮退故障をX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_sa0_diff_xid;		//0縮退故障を他パスX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_sa1_diff_xid;		//1縮退故障を他パスX抽出した数(0はFSIMで検出，1は1回で検出，2以上は見逃し故障)
	int						n_sa0_carebit;		//0縮退故障をX抽出した際に必要だったケアビット数
	int						n_sa1_carebit;		//1縮退故障をX抽出した際に必要だったケアビット数
	int						n_carebit;			//トータル何回ケアビット割当てされたか(0～対象故障数)
	struct _XID_LIST_		*xidlist_sa0;		//0縮退故障のX抽出対象リストポインタ
	struct _XID_LIST_		*xidlist_sa1;		//1縮退故障のX抽出対象リストポインタ
	int						n_corn_pi;			//【POのみ】対象POのコーン内PI数(最大で何本のPIが必要か)※少ないほどX抽出に最適なPOかも
	
	//可制御性(SCOAP)
	unsigned int			cost[N_COST];		// コスト([種類][時刻])

	//代表信号線関係
	int						rep_net;			//代表信号線						{ YES, NO }
	int						inv_flag;			//反転フラグ						{ YES, NO }
	struct _Netlist_Format_ *influence_net;		//影響信号線
	struct _Netlist_Format_ **next_event;		//次イベントドリブン信号線
	int						n_next_event;		//次イベントドリブン信号線数
	
	//二重検出関係
	int						dd_det_sf0;			//二重検出用の0縮退故障の検出回数	{ 0～検出回数 }
	int						dd_det_sf1;			//二重検出用の1縮退故障の検出回数	{ 0～検出回数 }

	//その他
	unsigned int			flag;				//多目的フラグ
	int						po_id;				//po[i]のi番目
	int						pi_id;				//pi[i]のi番目
	int						missed_flag;		//見逃し故障フラグ {0:見逃し故障でない， 1:0縮退見逃し， 2:1縮退見逃し， 3:両方見逃し}

	
} NLIST;


//========================================================================
// 【組合せ回路用】外部変数
//========================================================================
//縮退故障用ネットリスト構造体配列
NLIST *nl;

//モジュール名
char *module_name;

//信号数
int n_net;

//外部入力数, 外部入力
NLIST **pi;
int n_pi;

//外部出力数, 外部出力
NLIST **po;
int n_po;

//DFF, DFF数
NLIST **dff;
int n_dff;

//RDFF, RDFF数
NLIST **rdff;
int n_rdff;

//DFFS, DFFS数
NLIST **dffs;
int n_dffs;

//RDFFS, RDFFS数
NLIST **rdffs;
int n_rdffs;

//assign
NLIST **assign;
int n_assign;

//---------------------------------------------------
// 【組合せ回路用】FOUTステムポインタ
//---------------------------------------------------
NLIST	**fstem;		//FOUT-STEMポインタ(レベル降順で接続)
int		n_fstem;		//FOUT-STEM数

//---------------------------------------------------
// 【組合せ回路用】計算順序配列
//---------------------------------------------------
typedef struct Levelize{
	int		n_net;		//信号線数
	NLIST **net;		//信号線ポインタ
}LEV;

LEV *nml_lev;		//ノーマルレベライズ
LEV	*rep_lev;		//代表信号線レベライズ

LEV *lev_temp;		//レベライズスタック


//---------------------------------------------------
// 【組合せ回路用】FFR構造体 [PO⇒STEM(レベル降順)の順番]
//---------------------------------------------------
typedef struct FoutFreeRegion{
	int						ffr_id;				//FFRのID
	int						Reconv;				//再収斂フラグ	{YES, NO, UK(経路によっては再収斂したりしなかったり)}
	int						n_out;				//他FFRへの出力数
	struct FoutFreeRegion	**out;				//他出力FFRへのポインタ
	int						n_in;				//他FFRへの入力数
	struct FoutFreeRegion	**in;				//他入力FFRへのポインタ
	int						n_pi;				//FFR内の外部入力数
	int						n_in_net;			//FFR内の入力数(入力はINとFOUTブランチの2種類　※他FFRからの入力では無い)
	NLIST					*FoutStem;			//FOUT_STEMへのポインタ
	int						n_fault;			//FFR内に含まれる故障数						※0縮退と1縮退の両方をカウント
	int						n_det_fault;		//FFR内で検出した総故障数(初期テスト集合)	※1故障複数カウント
	int						n_detect;			//FFR内で検出した故障数(初期テスト集合)		※1故障1カウント
	int						n_xid_detect;		//FFR内で検出した故障数(X抽出後テスト集合)	※1故障1カウント
	BIT_INT					*det_tp;			//FFR内の故障が検出されたパターンにフラグ
	unsigned int			flag;				//再収斂解析で使用するフラグ
}FFR;

FFR *ffr;


//---------------------------------------------------
// 【組合せ回路用】X抽出対象故障リスト 線形リスト構造(検出回数の昇順でソート)
//---------------------------------------------------
typedef struct _XID_LIST_{
	NLIST					*net;				//ネットリストへのポインタ
	struct _XID_LIST_		*next;				//後続ノードポインタ
	int						fault_type;			//故障タイプ{0縮退故障:0 , 1縮退故障:1}
	int						n_detect;			//初期テスト集合での検出回数
	int						detect;				//検出済みか{YES:X抽出しない(他TPの故障SIMで落ちた) , NO:X抽出する}
}XID_LIST;


//---------------------------------------------------
// 【組合せ回路用】X抽出対象故障リストの先頭アドレス
//---------------------------------------------------
typedef struct _XID_LIST_HEAD{
	XID_LIST	*xid_list_head;		//線形リストの先頭アドレス
	int			n_fault;			//対象故障数(X抽出ごとに変動)
}XID_LIST_H;

//先頭アドレス構造体
XID_LIST_H	xid_head;


//---------------------------------------------------
// 【組合せ回路・順序回路共用】SPPFPで各(P)POで検出した故障した故障情報を保持
//---------------------------------------------------
typedef struct _Propagation_PO_{
	int			po_id;				//(p)po[i]のi番目
	int			n_det;				//検出した故障信号線数(最大32個)
	BIT_INT		*det_flag;			//検出した各故障信号線のSPPFPの故障番号割当ID(0～31) {1:検出　0:未検出}
	int			n_corn_pi;			//各(P)POの影響範囲内PI+PPI数
}XID_PROPA_PO;

XID_PROPA_PO	*propa_po;			//(P)PO数分確保 ((p)po[i]順)
XID_PROPA_PO	**sort_propa_po;	//(P)PO数分確保 (各(P)POの影響範囲内PI数順でソート)


//----------------------------------------------------------
// 【組合せ回路用】スタック(なんかで使うかも)
//----------------------------------------------------------
typedef struct _l_stack_ {
	NLIST					*net;	//データ
	struct _l_stack_		*next;	//後続ノードポインタ
} N_STACK;



#endif

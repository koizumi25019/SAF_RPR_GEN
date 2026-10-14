//------------------------------------------------------------------------
//Netlist 定義, 外部変数, プロトタイプ宣言
//File name : Nelist.h
//Date : 2006/03/26
//Designer : R.Inoue
//Ver : 3.00（配布Ver）
//------------------------------------------------------------------------
//------------------------------------------------------------------------
// 定義
//------------------------------------------------------------------------
//ゲートタイプ
#pragma once


#define  IN		0
#define  BUF    10
#define  INV	11
#define  AND	12
#define  NAND	13
#define  OR		14
#define  NOR	15
#define  EXOR	16 //2入力のみ可
#define  EXNOR	17 //2入力のみ可
#define  FOUT	18
#define  DFF	19 //D-フリップフロップ
#define  RDFF	21 //D-フリップフロップ（リセット有）
#define  DFFS	23 //D-スキャンフリップフロップ
#define  RDFFS	24 //D-スキャンフリップフロップ（リセット有）
#define  GND	25 //グランド
#define  ACC	26 //電源

//ネットリスト構造体定義
typedef struct _Netlist_Format_ {//一つの信号線のデータを管理する
	char *name;						//名前
	int n;							//ID（ユニーク）
	int type;						//タイプ
	int n_in;						//入力数
	struct _Netlist_Format_ **in;	//入力ポインタ配列
	int n_out;						//出力数
	struct _Netlist_Format_ **out;	//出力ポインタ配列
	char *name_port;				//端子名 pin名
	char *name_ins;					//インスタンス名
	int level;						//レベル付け
	int flag;						//属性フラグ
	int target;						//ターゲット値、目標値。決定していない値。
	int value;						//正常値
	int signal;						//故障値

	struct _Fault0_Table_* f0ptr;	//0縮退故障テーブルポインタ
	struct _Fault0_Table_* f1ptr;	//1縮退故障テーブルポインタ

	int sa0;						//S.A.0値　０縮退故障かどうか 
	int sa1;						//S.A.1値　１縮退故障かどうか
	int cc0;						//可制御費、論理値０を設定するためのコスト
	int cc1;						//可制御費、論理値１を設定するためのコスト
	int cco;						//可観測費、論理値を観測する為のコスト
	//-1は未定値(初期値)、0は論理０、1は論理１を示す
	//可制御値で無限大の場合は -2 を入れる
	//何か追加した際はteamsにて連絡をして下さい
} NLIST;

//0縮退故障テーブル構造体定義
typedef struct _Fault0_Table_ {
	struct _Netlist_Format_* net;	//故障信号ポインタ
	int detection;				//検出フラグ 0:未検出 1:検出済み
	
} FAULT_0;

//------------------------------------------------------------------------
// 外部変数
//------------------------------------------------------------------------
//ネットリスト
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

//------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------
int read_nl(char *);

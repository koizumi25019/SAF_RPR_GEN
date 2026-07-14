//--------------------------------------------------------------------------------------------------------------------
//Netlist 構造体, グローバル変数, PROTOTYPE
//ファイル名 : nelist.h
//日付 : 2006/03/26
//設計者 : R.Inoue
//Ver : 3.00（以前のVer）
//--------------------------------------------------------------------------------------------------------------------
#pragma once
#include<stdbool.h>

//--------------------------------------------------------------------------------------------------------------------
// 構造体
//--------------------------------------------------------------------------------------------------------------------
//ゲートタイプ
#define  IN		0
#define  BUF    10
#define  INV	11
#define  AND	12
#define  NAND	13
#define  OR		14
#define  NOR	15
#define  EXOR	16
#define  EXNOR	17
#define  FOUT	18
#define  DFF	19
#define  RDFF	21
#define  DFFS	23
#define  RDFFS	24
#define  GND	25
#define  ACC	26

typedef struct _Netlist_Format_ {
	char* name;							//信号線名
	int	n;		           				//ユニークID
	int	type;	          				//ゲートタイプ
	int	n_in;	            			//
	struct _Netlist_Format_** in;		//
	int				n_out;	            //
	struct _Netlist_Format_** out;      //
	char* name_port;					//
	char* name_ins;						//
	int				flag;			    /**< フラグ */
	unsigned int	varsgc;			    /**< 正常回路用変数  */
	unsigned int	varsfc;				/**< 故障回路用変数  */
	unsigned int	varprop;			/**< 故障伝搬用変数 (Dチェーン) */
	int				logic_value;		/**< 必須割当ての論理値 (-1=不明) */
	int				ea_flag;			/**< 必須割当てフラグ (0=未割当, 1=割当済み) */
	int				unique_flag;		/**< ユニーク化フラグ (0=down, 1=up, 2=middle) */
	int				level;				/**< トポロジカルレベル (0=PI) */
	int* consgc;						/**< 正常回路用制約 (フラットなint配列、0終端の節) */
	int consgc_len;						/**< consgc に格納されているint数 */
	char** consfc;						/**< 故障回路用制約  */
	unsigned int suf_fc;				//
	unsigned int fault_pass;			//
	int	test_sa0;						//0縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int	test_sa1;						//1縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない)
}NLIST;

//--------------------------------------------------------------------------------------------------------------------
// グローバル変数
//--------------------------------------------------------------------------------------------------------------------
//ネットリスト
NLIST* nl;

//モジュール名
char* module_name;

//信号数
int n_net;

//外部入力線, 外部入力数
NLIST** pi;
int n_pi;

//外部出力線, 外部出力数
NLIST** po;
int n_po;

//疑似外部出力線, 外部出力数
NLIST** ppo;
int n_ppo;

//DFF, DFF数
NLIST** dff;
int n_dff;

//RDFF, RDFF数
NLIST** rdff;
int n_rdff;

//DFFS, DFFS数
NLIST** dffs;
int n_dffs;

//RDFFS, RDFFS数
NLIST** rdffs;
int n_rdffs;

//assign
NLIST** assign;
int n_assign;

//--------------------------------------------------------------------------------------------------------------------
// プロトタイプ宣言
//--------------------------------------------------------------------------------------------------------------------
int		read_nl(char*);
void	free_netlist(void);

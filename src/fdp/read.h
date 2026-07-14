#pragma once
//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>

#include "./target_fault.h"
#include "./cube_set.h"
#include "../netlist/netlist.h"
#include "../lib/lib.h"


//-------------------------------------------------------------------------------------------------------------
//	定義
//-------------------------------------------------------------------------------------------------------------
#define READ_OKAY		  true		          /**< 戻り値   = true */
#define READ_ERROR		  false				  /**< 戻り値   = false */

#define MAXSIZE_BUFFER	  500000	          /**< バッファの最大サイズ */
#define MAXSIZE_BITINT	  32		          /**< bitintの最大サイズ */
#define MAXSIZE_CHAR	  5000

#define FOUND			  true		          /**< 戻り値   = true */
#define NOT_FOUND		  false				  /**< 戻り値   = false */

/** 文字列と「ファイル終端(EOF)」を比較する */
#define COMP_EOF(string)		string != (char*)NULL

/** 文字列と「改行」を比較する */
#define COMP_NEWLINE(string)	strcmp(string, "\n")!=0

#define	MAXSIZE_HASH	  100				  /**< 故障リストのサイズ */
#define TFR				  2			          /**< 遷移故障モデル   = rise */
#define	TFF				  4			          /**< 遷移故障モデル   = fall */
#define SF0				  8			          /**< 縮退故障モデル   = 0 */
#define	SF1				  16			      /**< 縮退故障モデル   = 1 */

#define	FP				  1					  /**< フラグ種別   = 故障点 */
#define	TFO				  2			          /**< フラグ種別   = 伝搬先(TFO) */
#define	TPO				  4					  /**< フラグ種別   = 伝搬先外部出力(TPO) */
#define	TFO_TPO			  (TFO|TPO)			  /**< フラグ種別   = TFO & TPO */
#define	FP_TPO			  (FP|TPO)			  /**< フラグ種別   = FP & TPO */

#define	DETECTED		  7					  /**< 故障検出済み */
#define	UNDETECTED		  17				  /**< 故障未検出 */
#define	REDEUNDANT		  27				  /**< 冗長故障 */


//-------------------------------------------------------------------------------------------------------------
//	構造体
//-------------------------------------------------------------------------------------------------------------

/** 故障ノード構造体 */
typedef struct FaultNode
{
	char* string;					 /**< 文字列 */
	char* name;						 /**< 名前 */
	int					  type;		/**< 故障タイプ */
	int					  detect;			  /**< 検出済みか */
	NLIST* netptr;					 /**< ネットリストへのポインタ */
	struct FaultNode* nextptr;				  /**< 次ノードへのポインタ */

	struct FaultNode** subset_faults;   /**< テスト集合がこの故障の部分集合になる故障（ゲート入力故障）。これらのキューブはこの故障の正当なテストなので種＋禁止節として流用できる */
	int                n_subset_faults; /**< subset_faults の要素数 */
	CubeSet            cubes;           /**< この故障のテストキューブ集合（後続の支配故障が流用する） */
	int                n_pending;       /**< まだこのキューブを流用する未処理の親故障の数。0 になれば cubes を解放してよい */
}
FNODE;

/** 故障リスト構造体 */
typedef struct FaultList
{
	int					  numrema;			  /**< 残り故障数 */
	int					  numinit;			  /**< 初期故障数 */
	int					  numdete;			  /**< 検出済み故障数 */
	int					  numred;			  /**< 冗長故障数 */
	struct FaultNode* list[MAXSIZE_HASH]; /**< 故障リスト */
}
FLIST;


/** 読み込みデータ構造体 */
typedef struct ReadData
{
	struct FaultList	  fault;			  /**< 故障リストデータ */
}
READDATA;


//-------------------------------------------------------------------------------------------------------------
//	グローバル変数
//-------------------------------------------------------------------------------------------------------------
READDATA			      readdata;			  /**< 読み込みデータ */

//-------------------------------------------------------------------------------------------------------------
//	プロトタイプ宣言
//-------------------------------------------------------------------------------------------------------------

/** 故障を読み込む */
bool ReadFault(
	void
);

/** 故障リストを作成する */
bool CreateFaultList(
	char* buffer			  /**< バッファ */
);

/** 故障ノードを探索する */
bool searchFnode(
	char* buffer,			  /**< バッファ（キー） */
	FNODE* fnode				  /**< 故障ノードへのポインタ */
);

/** 文字列から故障ノードのポインタを探す */
FNODE* searchFnodePtr(
	char* buffer,			  /**< バッファ（キー） */
	FNODE* head				  /**< ハッシュ故障リストの先頭 */
);

/** 故障ノードを作成する */
FNODE* CreateFaultNode(
	char* buffer			  /**< バッファ */
);

//等価故障解析
void AnalyzeEquivalenceFaults(
	void
);

//支配故障解析
void AnalyzeDominanceFaults(
	void
);

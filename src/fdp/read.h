#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <time.h>

#include "./target_fault.h"
#include "./cube_set.h"
#include "../netlist/netlist.h"
#include "../lib/lib.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define READ_OKAY		  true		          /**< return code   = ture */
#define READ_ERROR		  false				  /**< return code   = false */

#define MAXSIZE_BUFFER	  500000	          /**< maximum size of buffer */
#define MAXSIZE_BITINT	  32		          /**< maximum size of bitint */
#define MAXSIZE_CHAR	  5000

#define FOUND			  true		          /**< return code   = true */
#define NOT_FOUND		  false				  /**< return code   = false */

/** compare string and "End-Of-File" */
#define COMP_EOF(string)		string != (char*)NULL

/** compare string and "New-Line" */
#define COMP_NEWLINE(string)	strcmp(string, "\n")!=0	

#define	MAXSIZE_HASH	  100				  /**< size of fault list */
#define TFR				  2			          /**< transition fault model   = rise */
#define	TFF				  4			          /**< transition fault model   = fall */
#define SF0				  8			          /**< stack-at fault model   = 0 */
#define	SF1				  16			      /**< stack-at fault model   = 1 */

#define	FP				  1					  /**< flag type   = fault point */
#define	TFO				  2			          /**< flag type   = transitive fout */
#define	TPO				  4					  /**< flag type   = transitive priamary-output */
#define	TFO_TPO			  (TFO|TPO)			  /**< flag type   = TFO & TPO */
#define	FP_TPO			  (FP|TPO)			  /**< flag type   = FP & TPO */

#define	DETECTED		  7					  /**< fault detected */
#define	UNDETECTED		  17				  /**< fault undetected */
#define	REDEUNDANT		  27				  /**< fault redeundant */


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------

/** fault node structre */
typedef struct FaultNode
{
	char* string;					 /**< string */
	char* name;						 /**< name */
	int					  type;		/**< fault type */
	int					  detect;			  /**< detected??? */
	NLIST* netptr;					 /**< pointer to netlist */
	NLIST* exc_netptr;				 /**< TDF専用: 励起条件を課す1時刻目コピー（STR: =0, STF: =1）。SAF時はNULL */
	struct FaultNode* nextptr;				  /**< pointer to next node */

	struct FaultNode** subset_faults;   /**< テスト集合がこの故障の部分集合になる故障（ゲート入力故障）。これらのキューブはこの故障の正当なテストなので種＋禁止節として流用できる */
	int                n_subset_faults; /**< subset_faults の要素数 */
	CubeSet            cubes;           /**< この故障のテストキューブ集合（後続の支配故障が流用する） */
	int                n_pending;       /**< まだこのキューブを流用する未処理の親故障の数。0 になれば cubes を解放してよい */
}
FNODE;

/** fault list structres */
typedef struct FaultList
{
	int					  numrema;			  /**< number of remaining faults */
	int					  numinit;			  /**< number of initial faults */
	int					  numdete;			  /**< number of detected faults */
	int					  numred;			  /**< number of redundant faults */
	struct FaultNode* list[MAXSIZE_HASH]; /**< fault list */
}
FLIST;


/** read data structres */
typedef struct ReadData
{
	struct FaultList	  fault;			  /**< fault list data */
}
READDATA;


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
READDATA			      readdata;			  /**< reading data */

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------

/** read the fault */
bool ReadFault(
	void
);

/** create the fault lists */
bool CreateFaultList(
	char* buffer			  /**< buffer */
);

/** search for fault node */
bool searchFnode(
	char* buffer,			  /**< buffer (key) */
	FNODE* fnode				  /**< pointer to fault node */
);

/** find fault node pointer by string */
FNODE* searchFnodePtr(
	char* buffer,			  /**< buffer (key) */
	FNODE* head				  /**< head of hash-fault list */
);

/** create the fault node */
FNODE* CreateFaultNode(
	char* buffer			  /**< buffer */
);

//等価故障解析
void AnalyzeEquivalenceFaults(
	void
);

//等価故障解析（遷移故障: BUF/INV のみ、DFF境界は跨がない）
void AnalyzeEquivalenceFaultsTDF(
	void
);

/** 故障タイプの表示名: SAF="sa0"/"sa1", TDF="STR"/"STF"（CSV・ログ共通） */
const char* FaultTypeName(
	int type
);

//支配故障解析
void AnalyzeDominanceFaults(
	void
);

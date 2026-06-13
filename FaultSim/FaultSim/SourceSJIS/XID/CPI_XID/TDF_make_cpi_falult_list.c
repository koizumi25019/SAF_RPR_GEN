//------------------------------------------------------------------------
//File name : TDF_make_cpi_falult_list.c
//Date : 2013/12/11
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
SXID_LIST*		TDF_push_xid_list(S_NLIST*, int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
		//#define		XID_LIST_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------

//----------------------------------------------
//  関数名 : TDF_make_cpi_falult_list
//  機  能 : CPI用のX抽出対象故障リストの作成(線形リスト構造，検出回数の昇順)
//  戻り値 : なし
//  引  数 : priority(優先パスフラグ)
//----------------------------------------------
void TDF_make_cpi_falult_list(unsigned int priority){

	int		i;

#ifdef XID_LIST_DEBUG
	SXID_LIST *temp;
#endif

	//====================================================
	//初期化
	//====================================================
	sxid_head.xid_list_head = NULL;
	sxid_head.n_fault		= 0;


	//====================================================
	// 制御ポイント影響範囲内故障リストの作成
	//====================================================
	for(i=0; i<n_snet; i++){

		//-------------------------------------
		// 【立上り遷移故障】テスト対象 && 制御ポイント影響範囲内故障
		//-------------------------------------
		if(s_nl[i].pri_path==priority && s_nl[i].test_str==YES && s_nl[i].det_str>0 && s_nl[i].xid_det_str==NO){
			s_nl[i].xidlist_str = TDF_push_xid_list(&s_nl[i], TDF_STR, s_nl[i].det_str);	//ノード追加
			sxid_head.n_fault++;							//保持故障信号線数インクリメント
		}
		
		//-------------------------------------
		// 【立下り遷移故障】テスト対象
		//-------------------------------------
		if(s_nl[i].pri_path==priority && s_nl[i].test_stf==YES && s_nl[i].det_stf>0 && s_nl[i].xid_det_stf==NO){
			s_nl[i].xidlist_stf = TDF_push_xid_list(&s_nl[i], TDF_STF, s_nl[i].det_stf);	//ノード追加
			sxid_head.n_fault++;							//保持故障信号線数インクリメント
		}
	}
	
	//====================================================
	// DEBUG：リスト表示
	//====================================================
#ifdef XID_LIST_DEBUG
		printf("\n//-----------------------------------\n");
		printf("// DEBUG: XID対象リスト表示\n");
		printf("//-----------------------------------\n");
		printf("%s f:%d ndete:%d\n", sxid_head.xid_list_head->net->name, sxid_head.xid_list_head->fault_type, sxid_head.xid_list_head->n_detect);
		temp = sxid_head.xid_list_head->next;
		for(i=1; i<sxid_head.n_fault; i++){
			printf("[%d] %s f:%d ndet:%d\n",i,  temp->net->name, temp->fault_type, temp->n_detect);
			temp = temp->next;
		}
		printf("\n");
#endif


}//END

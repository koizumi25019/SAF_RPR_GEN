//------------------------------------------------------------------------
//File name : Make_TDF_XID_List.c
//Date : 2011/9/27
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/s_netlist.h"
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
	//#define XID_LIST_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_make_falult_list
//  機  能 : 未検出故障のX抽出対象故障リストの作成(線形リスト構造，検出回数の昇順)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void TDF_make_falult_list(){

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
	// 未検出故障リストの作成
	//====================================================
	for(i=0; i<n_snet; i++){

		//-------------------------------------
		// 【立上り遷移故障】テスト対象 && 制御ポイント影響範囲内故障
		//-------------------------------------
		if(s_nl[i].xid_det_str==NO && s_nl[i].test_str==YES && s_nl[i].det_str>0){
			s_nl[i].xidlist_str = TDF_push_xid_list(&s_nl[i], TDF_STR, s_nl[i].det_str);	//ノード追加
			sxid_head.n_fault++;							//保持故障信号線数インクリメント
		}
		
		//-------------------------------------
		// 【立下り遷移故障】テスト対象
		//-------------------------------------
		if(s_nl[i].xid_det_stf==NO && s_nl[i].test_stf==YES && s_nl[i].det_stf>0){
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


//--------------------------------------------------
//  関数名 : TDF_push_xid_list
//  機  能 : X抽出対象故障リストに故障信号線追加
//  戻り値 : なし
//  引  数 : net(対象信号線), ftype(故障情報[0:立上り遷移故障  1:立下り遷移故障]), ndet(検出回数)
//--------------------------------------------------
SXID_LIST* TDF_push_xid_list(S_NLIST* net, int ftype, int ndet){

	SXID_LIST *ptr , *head , *add_element;

	//===========================================
    // スタック要素確保
	//===========================================
	add_element = (SXID_LIST *)malloc(sizeof (SXID_LIST));
	

	//===========================================
	// メンバ値設定
	//===========================================
	add_element->net		= net;		//ネットリストへのポインタ
	add_element->next		= NULL;		//後続ノードポインタ
	add_element->fault_type = ftype;	//故障タイプ{立上り遷移故障:0 , 1縮退故障:1}
	add_element->n_detect	= ndet;		//検出回数
	add_element->detect		= NO;		//検出済みか{YES:X抽出しない(他TPの故障SIMで落ちた) , NO:X抽出する}
	

	//===========================================
	// リストに要素を追加 (n_detectの低い順に並ぶように挿入)
	//===========================================
	head = NULL;
	ptr = sxid_head.xid_list_head;	//現在の先頭アドレス代入

	//------------------------------------
	// 挿入箇所探索
	//------------------------------------
	while(ptr != NULL){
		if(ptr->n_detect > ndet){
			break;	// 検出回数が少ない順
		}
		head = ptr;
		ptr = ptr->next;
	}
	
	//------------------------------------
	// 要素挿入
	//------------------------------------
	// 先頭への要素挿入
	if(head == NULL){
		add_element->next = ptr;
        sxid_head.xid_list_head = add_element;
	}
	// 要素間への挿入
	else{
        add_element->next = ptr;
		head->next = add_element;
	}
	
	//===========================================
	// 追加した要素へのアドレス返却
	//===========================================
	return(add_element);
}

//--------------------------------------------------
//  関数名 : pop_sxid_list
//  機  能 :  X抽出対象故障リストから先頭pop
//  戻り値 : 信号線
//  引  数 : なし
//--------------------------------------------------
SXID_LIST *pop_sxid_list(){

	SXID_LIST		*ptr;

	//ptrに故障リストの先頭を入れる
	ptr = sxid_head.xid_list_head;

	//空だった場合NULLを返す
	if(ptr == NULL)return NULL;

	//故障リストの先頭を変更
	sxid_head.xid_list_head = ptr->next;

	//X抽出対象故障リスト内要素数をデクリメント
	sxid_head.n_fault--;

	//先頭だった故障リストを返す
	return(ptr);

}

//--------------------------------------------------
//  関数名 : num_n_sxid_list
//  機  能 :  X抽出対象故障リスト内要素数をかえす
//  戻り値 : 
//		0 : 要素0個
//		1 : 要素1個以上
//  引  数 : なし
//--------------------------------------------------
int num_n_sxid_list(){

    if(sxid_head.xid_list_head == NULL)return 0;
	return 1;
}

//------------------------------------------------------------------------
//  内部関数
//------------------------------------------------------------------------
//--------------------------------------------------
//  関数名 : delete_sxid_list
//  機  能 :  X抽出対象故障リスト解放
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------
void delete_sxid_list(){

	SXID_LIST		*ptr;
	SXID_LIST		*temp;

    ptr = sxid_head.xid_list_head;
	while(ptr != NULL){
		temp = ptr->next;
        free((void *)ptr);
		ptr = temp;
	}

    sxid_head.xid_list_head = NULL;	
}

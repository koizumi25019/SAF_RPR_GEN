//------------------------------------------------------------------------
//File name : SAF_make_xid_falult_list.c
//Date : 2012/2/5
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
XID_LIST*		SAF_push_xid_list(NLIST*, int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
//#define		XID_LIST_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------

//----------------------------------------------
//  関数名 : SAF_make_xid_falult_list
//  機  能 : X抽出対象故障リストの作成(線形リスト構造，検出回数の昇順)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void SAF_make_xid_falult_list(){

	int		i;

#ifdef XID_LIST_DEBUG
	XID_LIST *temp;
#endif

	//====================================================
	//初期化
	//====================================================
	xid_head.xid_list_head = NULL;
	xid_head.n_fault		= 0;


	//====================================================
	// X抽出対象故障リストの作成
	//====================================================
	for(i=0; i<n_net; i++){

		//-------------------------------------
		// 【0縮退故障】テスト対象かつ2回以上検出
		//-------------------------------------
		// ※必須故障以外
		if(nl[i].test_sf0==YES && nl[i].det_sf0>1){
			nl[i].xidlist_sa0 = SAF_push_xid_list(&nl[i], SAF0, nl[i].det_sf0);	//ノード追加
			xid_head.n_fault++;							//保持故障信号線数インクリメント
		}
		
		//-------------------------------------
		// 【1縮退故障】テスト対象かつ2回以上検出
		//-------------------------------------
		// ※必須故障以外
		if(nl[i].test_sf1==YES && nl[i].det_sf1>1){
			nl[i].xidlist_sa1 = SAF_push_xid_list(&nl[i], SAF1, nl[i].det_sf1);	//ノード追加
			xid_head.n_fault++;							//保持故障信号線数インクリメント
		}
	}
	
	//====================================================
	// DEBUG：リスト表示
	//====================================================
#ifdef XID_LIST_DEBUG
		printf("\n//-----------------------------------\n");
		printf("// DEBUG: XID対象リスト表示\n");
		printf("//-----------------------------------\n");
		printf("%s f:%d ndete:%d\n", xid_head.xid_list_head->net->name, xid_head.xid_list_head->fault_type, xid_head.xid_list_head->n_detect);
		temp = xid_head.xid_list_head->next;
		for(i=1; i<xid_head.n_fault; i++){
			printf("%s f:%d ndet:%d\n", temp->net->name, temp->fault_type, temp->n_detect);
			temp = temp->next;
		}
		printf("\n");
#endif


}//END


//----------------------------------------------
//  関数名 : Make_SAF_Missed_XID_List
//  機  能 : 見逃し故障X抽出用の対象故障リストの作成(線形リスト構造，検出回数の昇順)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void Make_SAF_Missed_XID_List(){

	int		i;

#ifdef XID_LIST_DEBUG
	XID_LIST *temp;
#endif

	//====================================================
	//初期化
	//====================================================
	xid_head.xid_list_head = NULL;
	xid_head.n_fault		= 0;


	//====================================================
	// X抽出対象故障リストの作成
	//====================================================
	for(i=0; i<n_net; i++){

		//-------------------------------------
		// 【0縮退故障】テスト対象かつ見逃し故障
		//-------------------------------------
		if(nl[i].test_sf0==YES && nl[i].xid_det_sf0==NO){
			nl[i].xidlist_sa0 = SAF_push_xid_list(&nl[i], SAF0, nl[i].det_sf0);	//ノード追加
			xid_head.n_fault++;												//保持故障信号線数インクリメント
		}
		
		//-------------------------------------
		// 【1縮退故障】テスト対象かつ見逃し故障
		//-------------------------------------
		if(nl[i].test_sf1==YES && nl[i].xidlist_sa1==NO){
			nl[i].xidlist_sa1 = SAF_push_xid_list(&nl[i], SAF1, nl[i].det_sf1);	//ノード追加
			xid_head.n_fault++;												//保持故障信号線数インクリメント
		}
	}
	
	//====================================================
	// DEBUG：リスト表示
	//====================================================
#ifdef XID_LIST_DEBUG
		printf("\n//-----------------------------------\n");
		printf("// DEBUG: 見逃し故障XID対象リスト表示\n");
		printf("//-----------------------------------\n");
		printf("%s f:%d ndete:%d\n", xid_head.xid_list_head->net->name, xid_head.xid_list_head->fault_type, xid_head.xid_list_head->n_detect);
		temp = xid_head.xid_list_head->next;
		for(i=1; i<xid_head.n_fault; i++){
			printf("%s f:%d ndet:%d\n", temp->net->name, temp->fault_type, temp->n_detect);
			temp = temp->next;
		}
		printf("\n");
#endif


}//END

//--------------------------------------------------
//  関数名 : SAF_push_xid_list
//  機  能 : X抽出対象故障リストに故障信号線追加
//  戻り値 : なし
//  引  数 : net(対象信号線), ftype(故障情報[0:0縮退  1:1縮退]), ndet(検出回数)
//--------------------------------------------------
XID_LIST* SAF_push_xid_list(NLIST* net, int ftype, int ndet){

	XID_LIST *ptr , *head , *add_element;

	//===========================================
    // スタック要素確保
	//===========================================
	add_element = (XID_LIST *)malloc(sizeof (XID_LIST));
	

	//===========================================
	// メンバ値設定
	//===========================================
	add_element->net		= net;		//ネットリストへのポインタ
	add_element->next		= NULL;		//後続ノードポインタ
	add_element->fault_type = ftype;	//故障タイプ{0縮退故障:0 , 1縮退故障:1}
	add_element->n_detect	= ndet;		//検出回数
	add_element->detect		= NO;		//検出済みか{YES:X抽出しない(他TPの故障SIMで落ちた) , NO:X抽出する}
	

	//===========================================
	// リストに要素を追加 (n_detectの低い順に並ぶように挿入)
	//===========================================
	head = NULL;
	ptr = xid_head.xid_list_head;	//現在の先頭アドレス代入

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
        xid_head.xid_list_head = add_element;
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
//  関数名 : pop_xid_list
//  機  能 :  X抽出対象故障リストから先頭pop
//  戻り値 : 信号線
//  引  数 : なし
//--------------------------------------------------
XID_LIST *pop_xid_list(){

	XID_LIST		*ptr;

	//ptrに故障リストの先頭を入れる
	ptr = xid_head.xid_list_head;

	//空だった場合NULLを返す
	if(ptr == NULL)return NULL;

	//故障リストの先頭を変更
	xid_head.xid_list_head = ptr->next;

	//X抽出対象故障リスト内要素数をデクリメント
	xid_head.n_fault--;

	//先頭だった故障リストを返す
	return(ptr);

}

//--------------------------------------------------
//  関数名 : num_n_xid_list
//  機  能 :  X抽出対象故障リスト内要素数をかえす
//  戻り値 : 
//		0 : 要素0個
//		1 : 要素1個以上
//  引  数 : なし
//--------------------------------------------------
int num_n_xid_list(){

    if(xid_head.xid_list_head == NULL)return 0;
	return 1;
}

//------------------------------------------------------------------------
//  内部関数
//------------------------------------------------------------------------
//--------------------------------------------------
//  関数名 : delete_xid_list
//  機  能 :  X抽出対象故障リスト解放
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------
void delete_xid_list(){

	XID_LIST		*ptr;
	XID_LIST		*temp;

    ptr = xid_head.xid_list_head;
	while(ptr != NULL){
		temp = ptr->next;
        free((void *)ptr);
		ptr = temp;
	}

    xid_head.xid_list_head = NULL;	
}

//------------------------------------------------------------------------
//File name : search_priority_path
//Date : 2013/11/15
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>

#include	"../Netlist/s_netlist.h"
#include	"../option.h"
#include	"../StandardHead.h"

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//----------------------------------------------
//関数名：search_priority_path
//機能　：制御ポイント(PI)から構造的に到達可能信号線(XID優先パス)を特定
//引数　：net(制御ポイント), priority(優先フラグ値)
//戻り値：なし
//----------------------------------------------
void search_priority_path(S_NLIST *net, unsigned int priority){

	int		i;
	S_NLIST	*temp;


	//============================================
	// 制御ポイント(net)にXID優先パスフラグを設置
	//============================================
	//PI(CP)を優先パス設置
	net->pri_path = priority;
	
	//対応するDFFを優先パス設置
	net->link_cpi[0]->pri_path = priority;

	//PI(CP)の全出力をエンキュー
	for(i=0; i<net->n_out; i++){
		//DFF以外 && フラグ設置済み以外ならエンキュー
		if(net->out[i]->type!=DFF && net->out[i]->pri_path!=priority){
			enqueue(nl_queue, (void*)net->out[i]);
		}
	}

	//対応するDFFの全出力をエンキュー
	for(i=0; i<net->link_cpi[0]->n_out; i++){
		if(net->link_cpi[0]->out[i]->type!=DFF && net->link_cpi[0]->out[i]->pri_path!=priority){
			enqueue(nl_queue, (void*)net->link_cpi[0]->out[i]);
		}
	}

	//============================================
	// 構造的に到達可能信号線(XID優先パス)を特定
	//============================================
	while((temp = (S_NLIST *)dequeue(nl_queue)) != (S_NLIST *)NULL){	//空になるまでデキュー
		for(i=0; i<temp->n_out; i++){
			if(temp->out[i]->type!=DFF && temp->out[i]->pri_path!=priority){
				
				//優先パスフラグ設置
				temp->out[i]->pri_path = priority;

				//エンキュー
				enqueue(nl_queue, (void*)temp->out[i]);
			}
		}
	}


	//============================================
	// キュー初期化
	//============================================
	reset_queue(nl_queue);
}
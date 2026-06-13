//------------------------------------------------------------------------
//File name : search_reconv_combinational.c
//Date : 2011/12/31
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Lib/alloc.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
/*
void		push_n_stack(NLIST *);
NLIST		*pop_n_stack();
void		delete_n_stack();
int			num_n_stack();
*/

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		RECONV_DEBUG

//---------------------------------------------------------------------
//　静的変数
//---------------------------------------------------------------------
/*
static	N_STACK	*n_stack = NULL;	// レベルスタック先頭
*/

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : search_reconv_combinational
//  機  能 : 再収斂構造解析[組合せ回路用] {YES, NO, UKの判定}
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void search_reconv_combinational(){

	int					i,j;
	unsigned int		count=0;			//計算済み信号線フラグ値(1〜2^32まで使用)
	FFR					*t_ffr;				//デキューしたFFRポインタを格納
	int					reconv_po;			//再収斂していたPO数をカウント
	int					not_reconv_po;		//再収斂してないPO数をカウント
	

	//=================================================
	// フラグ初期化
	//=================================================
	for(i=0; i<n_ffr; i++){
		ffr[i].flag = 0;
	}

	//=================================================
	// 再収斂探索
	//=================================================
	for(i=n_po; i<n_ffr; i++){
		
		//-------------------------------------
		// フラグ値更新
		//-------------------------------------
		count += 2;

		//フラグ値が限界に達したらFFRフラグを再初期化
		if(count == 4294967294){
			for(i=0; i<n_ffr; i++)ffr[i].flag=0;
			count=2;
		}
		
		//-------------------------------------
		// 判定対象FFRをエンキュー
		//-------------------------------------
		ffr[i].flag = count;	//対象FFRにフラグ値セット
		enqueue(nl_queue, &ffr[i]);		//エンキュー
#ifdef RECONV_DEBUG
		printf("\n【対象FFR】ffr[%d]:%s count:%d\n", ffr[i].ffr_id, ffr[i].FoutStem->name, count);
#endif
		
		//-------------------------------------
		// 再収斂探索
		//-------------------------------------
		while( (t_ffr = (FFR*)dequeue(nl_queue) ) != NULL){

#ifdef RECONV_DEBUG
					printf("デキューしたffr[%d]:%s \n", t_ffr->ffr_id, t_ffr->FoutStem->name);
#endif

			//デキューしたFFRの出力FFRにフラグ値設定しつつエンキュー
			for(j=0; j<t_ffr->n_out; j++){

				//------------------
				// 次FFRエンキュー
				//------------------
				if(t_ffr->out[j]->flag != count+1){
					enqueue(nl_queue, t_ffr->out[j]);		//2回通過済み以外ならエンキュー
#ifdef RECONV_DEBUG
					printf("【1回目通過エンキュー】ffr[%d]:%s \n", t_ffr->out[j]->ffr_id, t_ffr->out[j]->FoutStem->name);
#endif
				}

				//------------------
				// フラグセット
				//------------------
				if(t_ffr->flag==count && t_ffr->out[j]->flag!=count && t_ffr->out[j]->flag!=count+1){	//自分は非再収斂 && 出力は未通過
					t_ffr->out[j]->flag = count;	//1回目の通過
#ifdef RECONV_DEBUG
					if(t_ffr->out[j]->n_out == 0){
						printf("※PO ");
					}
					printf("【1回目通過flag=%d】ffr[%d]:%s \n", count, t_ffr->out[j]->ffr_id, t_ffr->out[j]->FoutStem->name);
#endif
				}

				else if(t_ffr->flag==count+1 || t_ffr->out[j]->flag==count){	//自分が再収斂　or 出力が1回カウント済み
					t_ffr->out[j]->flag = count+1;	//2回目以降の通過(※再収斂)
#ifdef RECONV_DEBUG
					if(t_ffr->out[j]->n_out == 0){
						printf("※PO ");
					}
					printf("【2回目通過flag=%d】ffr[%d]:%s \n", count+1, t_ffr->out[j]->ffr_id, t_ffr->out[j]->FoutStem->name);
#endif
				}

			}

		}
		
		//-------------------------------------
		// キュー再初期化
		//-------------------------------------
		reset_queue(nl_queue);

		//-------------------------------------
		// 再収斂判定
		//-------------------------------------
		reconv_po = 0;		//初期化
		not_reconv_po = 0;	//初期化

		for(j=0; j<n_po; j++){
			if(ffr[j].flag == count+1){
				reconv_po++;		//再収斂していたPO数を更新
			}
			else if(ffr[j].flag == count)
				not_reconv_po++;	//再収斂してないPO数を更新
		}

		
		//-------------------------------------
		// 再収斂状態を決定{YES, NO, UK}
		//-------------------------------------
		if(reconv_po == 0){
			ffr[i].Reconv = NO;		//どのPOも再収斂してない
		}
		else if(not_reconv_po == 0){
			ffr[i].Reconv = YES;	//どのPOでも再収斂する
		}
		else{
			ffr[i].Reconv = UK;		//再収斂してるPOもあれば，しないPOもある
		}

	}

#ifdef RECONV_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG: 再収斂確認【組合せ回路】\n");
	printf("//------------------------------\n");
	for(i=0; i<n_ffr; i++){
		printf("FFR[%d] - STEM:%s  再収斂:", i, ffr[i].FoutStem->name);
		if(ffr[i].Reconv == YES){
			printf("YES\n");
		}
		else if(ffr[i].Reconv == NO){
			printf("NO\n");
		}
		else if(ffr[i].Reconv == UK){
			printf("UK\n");
		}
		else{
			printf("ERROR\n");
		}
	}
	printf("\n");
#endif
	

}//END

/*
//--------------------------------------------------
//  関数名 : push_n_stack
//  機  能 : レベルスタックpush
//  戻り値 : なし
//  引  数 : 信号線
//--------------------------------------------------
void push_n_stack(NLIST* net){

	N_STACK *ptr , *head , *add_element;

    // スタック要素確保
	add_element = (N_STACK *)malloc(sizeof (N_STACK));

	// メンバ値設定
	add_element->net = net;
	add_element->next = NULL;

	// リストに要素を追加
	// (LEVELの低い順に並ぶように挿入)
	head = NULL;
	ptr = n_stack;
	while(ptr != NULL){
		// 挿入箇所探索
		if(ptr->net == net){
			// 同要素発見
            add_element->net = NULL;
			free((void *)add_element);            			
			return;
		}
		if(ptr->net->level > net->level){
			break;}	// 可観測費の安い順
		head = ptr;
		ptr = ptr->next;
	}
	if(head == NULL){
		// 先頭への要素挿入
		add_element->next = ptr;
        n_stack = add_element;
	}else{
		// 要素間への挿入
        add_element->next = ptr;
		head->next = add_element;
	}
}

//--------------------------------------------------
//  関数名 : pop_n_stack
//  機  能 : レベルスタックから先頭pop
//  戻り値 : 信号線
//  引  数 : なし
//--------------------------------------------------
NLIST *pop_n_stack(){

	N_STACK		*ptr;
	NLIST			*net;

    ptr = n_stack;
	if(ptr == NULL)return NULL;

	net = ptr->net;	
	n_stack = ptr->next;
	ptr->net = NULL;
	ptr->next = NULL;
	free((void *)ptr);

	return (net);
}

//--------------------------------------------------
//  関数名 : num_n_stack
//  機  能 : リスト内要素数をかえす
//  戻り値 : 
//		0 : 要素0個
//		1 : 要素1個以上
//  引  数 : なし
//--------------------------------------------------
int num_n_stack(){

    if(n_stack == NULL)return 0;
	return 1;
}

//------------------------------------------------------------------------
//  内部関数
//------------------------------------------------------------------------
//--------------------------------------------------
//  関数名 : delete_n_stack
//  機  能 : スタック解放
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------
void delete_n_stack(){

	N_STACK		*ptr;
	N_STACK		*temp;

    ptr = n_stack;
	while(ptr != NULL){
		temp = ptr->next;
        free((void *)ptr);
		ptr = temp;
	}

    n_stack = NULL;	
}
*/
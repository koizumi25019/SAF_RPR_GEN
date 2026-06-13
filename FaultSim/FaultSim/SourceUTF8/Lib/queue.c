//------------------------------------------------------------------------
//File name : queue.c
//Date : 2012/2/26
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"./alloc.h"
#include	"./queue.h"
#include	"../Netlist/netlist.h"


//--------------------------------------------------
//  関数名 : init_queue
//  機  能 : キューの初期化
//  戻り値 : キュー構造体のポインタ
//  引  数 : q_size(キューサイズ)
//--------------------------------------------------
QUEUE	*init_queue (int q_size)
{
	QUEUE	*temp;

	temp = (QUEUE*)malloc(sizeof(QUEUE));
	temp->front = 0;
	temp->rear	= 0;
	temp->size	= q_size;
	temp->queue = PVOID_alloc(q_size);

	return temp;
}

//--------------------------------------------------
//  関数名 : enqueue
//  機  能 : キューへ取入れ
//  戻り値 : なし
//  引  数 : temp(キュー構造体ポインタ), p(取り入れるポインタ)
//--------------------------------------------------
void enqueue(QUEUE *temp, void  *p)
{
    int now_size = 0;

	if(temp->rear >= temp->front){
        now_size = (temp->rear - temp->front)+1;
	}else{
        now_size = (temp->size - temp->front)+(temp->rear + 1);
	}

	if (now_size >= temp->size) {
		fprintf(stderr, "Queue overflow\n");
		exit( 1 );
	}

	temp->queue[temp->rear++] = p;
	if(temp->rear == temp->size)temp->rear=0;
}

//--------------------------------------------------
//  関数名 : dequeue
//  機  能 : キューから取り出し
//  戻り値 : 取り出されたポインタ(空ならNULL)
//  引  数 : temp(キュー構造体ポインタ)
//--------------------------------------------------
void  *dequeue (QUEUE *temp)
{
	if(temp->front == temp->size)temp->front=0;
	if (temp->front == temp->rear) {
		return( (void *)NULL );
	}
    
	return( temp->queue[temp->front++] );
}

//--------------------------------------------------
//  関数名 : free_queue
//  機  能 : キュー解放
//  戻り値 : なし
//  引  数 : temp(キュー構造体ポインタ)
//--------------------------------------------------
void free_queue(QUEUE *temp){
	free(temp->queue);
}

//--------------------------------------------------
//  関数名 : reset_queue
//  機  能 : キューの再初期化
//  戻り値 : なし
//  引  数 : temp(キュー構造体ポインタ)
//--------------------------------------------------
void reset_queue(QUEUE *temp){
	temp->front = 0;
	temp->rear = 0;
}



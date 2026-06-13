//------------------------------------------------------------------------
//File name : TDF_priority_3v_xfilling
//Date : 2013/11/22
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_tp.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"
#include	"../Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void		dtob						(unsigned int, int);
void		TDF_XID_fpath_push			(S_NLIST*, int, int);
void		TDF_0_Backward_Implication	(S_NLIST*, int, int, unsigned int);
void		TDF_1_Backward_Implication	(S_NLIST*, int, int, unsigned int);
void		TDF_0_Forward_Implication	(S_NLIST*, int, int);
void		TDF_1_Forward_Implication	(S_NLIST*, int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//
//  xid_flag			: X抽出関数用のフラグ(故障伝搬経路(xid_flag) or 含意操作済み(xid_flag-1) )

	/*
	#define ED_DEBUG	//イベントドリブンデバッグ
	#define PATH_DEBUG	//伝搬経路デバッグ
	#define ECX_DEBUG	//正当化デバッグ
	#define JUS_DEBUG	//正当化キューデバッグ
	#define FIMPL_DEBUG	//前方含意デバッグ
	*/

//※s_nl[i].x_faultはPPSFP時の値がそのままなので注意(故障伝搬してない信号線も何か値が入りっぱなし)
// s_nl[i].p_faultも同様に初期化してない
//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------
static  unsigned int     all_zero = 0;				//32bit全て『0』
static  unsigned int     all_one  = 0xFFFFFFFF;		//32bit全て『1』

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_priority_3v_xfilling
//  機  能 : 【限定含意・限定正当化】3値SPPFP結果からの必要なケアビット決定(要はX抽出部)
//  戻り値 : ed_flag(イベントドリブン用フラグ)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ), xid_flag(X抽出用フラグ), jus_flag(限定正当化用フラグ), priority(優先パスフラグ)
//----------------------------------------------
unsigned int	TDF_priority_3v_xfilling(int tp_id, SXID_LIST **t_fault, int n_fault, unsigned int ed_flag, unsigned int xid_flag, unsigned int *jus_flag, unsigned int priority){
	
	return xid_flag;
}
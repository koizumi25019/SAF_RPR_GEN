//------------------------------------------------------------------------
//File name : TDF_3v_EX_xfilling
//Date : 2013/10/18
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
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
//
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
//static  unsigned int     all_zero = 0;				//32bit全て『0』//未使用のため削除
//static  unsigned int     all_one  = 0xFFFFFFFF;		//32bit全て『1』//未使用のため削除

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_2v_EX_xfilling
//  機  能 : 【見逃し故障用】3値SPPFP結果からの必要なケアビット決定(要はX抽出部)
//  戻り値 : ed_flag(イベントドリブン用フラグ)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ), xid_flag(X抽出用フラグ), jus_flag(正当化フラグ)
//----------------------------------------------
unsigned int	TDF_3v_EX_xfilling(int tp_id, XID_LIST **t_fault, int n_fault, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){

	
	//======================================================================================
	//xid_flag(ケアビット固定フラグ)の返却
	//======================================================================================
	return xid_flag;
}
//------------------------------------------------------------------------
//File name : SAF_3v_EXrepED_PPSFP.c
//Date : 2012/4/1
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_tp.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void		SAF_3v_repEDpush	(NLIST*,int);
void		SAF_FOUT_3v_CPT			(int, NLIST*, int, unsigned int*, NLIST*);
void		SAF_3v_CPT0			(int, unsigned int, NLIST*);
void		SAF_3v_CPT1			(int, unsigned int, NLIST*);

static void SAF_3v_ED_fout		(NLIST*, int, unsigned int);
static void SAF_3v_ED_buf		(NLIST*, int, unsigned int);
static void SAF_3v_ED_inv		(NLIST*, int, unsigned int);
static void SAF_3v_ED_and		(NLIST*, int, unsigned int);
static void SAF_3v_ED_nand		(NLIST*, int, unsigned int);
static void SAF_3v_ED_or		(NLIST*, int, unsigned int);
static void SAF_3v_ED_nor		(NLIST*, int, unsigned int);
static void SAF_3v_ED_exor		(NLIST*, int, unsigned int);
static void SAF_3v_ED_exnor		(NLIST*, int, unsigned int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define ED_DEBUG

//---------------------------------------------------------------------
// 静的変数
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_3v_EXrepED_PPSFP
//  機  能 : 【代表信号線】※STEM(故障個所)が非代表信号線の場合の3値縮退故障PPSFPイベントドリブン
//  戻り値 : なし
//  引  数 : ffr_id(ステムのFFR番号), stem_net(ステム信号線), ed_flag(計算済みフラグ値), ui_num(unsigned int番目)
//----------------------------------------------
void	SAF_3v_EXrepED_PPSFP (int ffr_id, NLIST* stem_net, unsigned int ed_flag, int ui_num){
}
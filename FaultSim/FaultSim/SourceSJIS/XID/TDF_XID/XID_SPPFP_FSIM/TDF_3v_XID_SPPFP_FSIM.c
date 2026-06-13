//------------------------------------------------------------------------
//File name : TDF_3v_XID_SPPFP_FSIM.c
//Date : 2013/10/13
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../../Netlist/s_netlist.h"
#include	"../../../Lib/bit_int.h"
#include	"../../../StandardHead.h"
#include	"../../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	TDF_3v_XID_SPPFP_LSIM	(int);
int		TDF_3v_XID_SPPFP		(int, SXID_LIST**, int, unsigned int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//------------------
//    one_tp_nval  |
//-----------------|
// 0 |     0       | 
// 1 |     1       |
// X |     3       |
// -----------------

	//#define	SPPFP_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_3v_XID_SPPFP_FSIM
//  機  能 : X抽出後テストパターンで3値故障シミュレーション
//  戻り値 : n_det_fault(検出故障数)
//  引  数 : tp_id(故障SIMの対象となるテストパターン), ed_flag(イベントドリブンフラグ), t_fault(故障SIMの対象とする故障リスト)
//----------------------------------------------
int	TDF_3v_XID_SPPFP_FSIM(int tp_id,  unsigned int *ed_flag, SXID_LIST** t_fault){

	int			i;
	int			count=0;				//t_fault内に何個対象故障を入れたかのカウンタ	
	int			n_det_fault=0;			//検出故障数(1故障1回カウント)
#ifdef SPPFP_DEBUG
	int			j;
#endif

	//=========================================================
	// DEBUG:テストパターン表示
	//=========================================================
#ifdef SPPFP_DEBUG
	printf("\nX抽出後テストパターン[%d]\n", tp_id);
	//PI
	for(i=0; i<n_spi; i++){
		//----------------------------------
		//3値の場合{0,1,X}
		//----------------------------------
		j = Get_NBit(spi[i]->xid_nval_t1, tp_id);

		if(j==0){
			printf("0");
		}
		else if(j==1){
			printf("1");
		}
		else if(j==3){
			printf("X");
		}
	}
	//PPI
	for(i=0; i<n_ppi; i++){
		//----------------------------------
		//3値の場合{0,1,X}
		//----------------------------------
		j = Get_NBit(ppi[i]->xid_nval_t1, tp_id);

		if(j==0){
			printf("0");
		}
		else if(j==1){
			printf("1");
		}
		else if(j==3){
			printf("X");
		}
	}
	printf("\n");
#endif
	
	//=========================================================
	// X抽出後テストパターンの3値SPPFP
	//=========================================================
	for(i=0; i<n_snet; i++){

		//--------------------------------------------------
		// 32個SPPFPの対象となる故障を集める
		//--------------------------------------------------
		//■立上り遷移故障(0→1/0)
		if(s_nl[i].test_str==YES && s_nl[i].det_str>=1 && s_nl[i].xid_det_str==NO){	//代表故障 && 1回以上検出してる && X抽出後で未検出
			
			//対象テストパターンの初期テストパターンで検出可能
			if(Get_NBit_INT(fdic_str[tp_id], i) == YES){

				//対象テスパターンのX抽出後テストパターンでは未検出
				if(Get_NBit_INT(xid_fdic_str[tp_id], i) == NO){

					//対象テストパターンで1時刻目=0 && 2時刻目=1の場合設置
					if(Get_NBit(s_nl[i].xid_nval_t1, tp_id)==0 && Get_NBit(s_nl[i].xid_nval_t2, tp_id)==1){
#ifdef SPPFP_DEBUG
						printf("%d個目 : %s  STR\n", count, s_nl[i].name);
#endif
						//対象故障リストへ追加
						t_fault[count]->net			= &s_nl[i];	//信号線名
						t_fault[count]->fault_type	= TDF_STR;	//故障タイプ(立上り遷移故障)

						//対象故障リスト内の故障数更新
						count++;
					}
				}
			}
		}

		//■立下り遷移故障(1→0/1)
		if(s_nl[i].test_stf==YES && s_nl[i].det_stf>=1 && s_nl[i].xid_det_stf==NO){	//代表故障 && 1回以上検出してる && X抽出後で未検出
		
			//対象テストパターンの初期テストパターンで検出可能
			if(Get_NBit_INT(fdic_stf[tp_id], i) == YES){

				//対象テスパターンのX抽出後テストパターンでは未検出
				if(Get_NBit_INT(xid_fdic_stf[tp_id], i) == NO){

					//対象テストパターンで1時刻目=1 && 2時刻目=0の場合設置
					if(Get_NBit(s_nl[i].xid_nval_t1, tp_id)==1 && Get_NBit(s_nl[i].xid_nval_t2, tp_id)==0){
#ifdef SPPFP_DEBUG
						printf("%d個目 : %s  SF1\n", count, s_nl[i].name);
#endif
						//対象故障リストへ追加
						t_fault[count]->net			= &s_nl[i];	//信号線名
						t_fault[count]->fault_type	= TDF_STF;	//故障タイプ(立下り遷移故障)

						//対象故障リスト内の故障数更新
						count++;
					}
				}
			}
		}

		//--------------------------------------------------
		// 32故障が集まったらSPPFP
		//--------------------------------------------------
		if(count == 31){
			//故障シミュレーション
			n_det_fault += TDF_3v_XID_SPPFP(tp_id, t_fault, count, *ed_flag);	//※故障辞書変数が違うためTDF_3v_SPPFPとは関数(ほとんど一緒だが・・・)

			//対象故障リスト内の故障数カウンタ初期化
			count = 0;

			//EDフラグ更新
			*ed_flag+=2;
		}

	}

	//=========================================================
	// 3値SPPFP (故障が32未満でループ抜けた場合実行)
	//=========================================================
	if(count > 0){
		//故障シミュレーション
		n_det_fault += TDF_3v_XID_SPPFP(tp_id, t_fault, count, *ed_flag);		//※故障辞書変数が違うためTDF_3v_SPPFPとは別関数(ほとんど一緒だが・・・)

		//EDフラグ更新
		*ed_flag+=2;
	}

	return n_det_fault;

}
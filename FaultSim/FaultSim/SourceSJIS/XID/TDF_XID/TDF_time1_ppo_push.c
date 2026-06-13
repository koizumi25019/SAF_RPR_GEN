//------------------------------------------------------------------------
//File name : TDF_time1_ppo_push.c
//Date : 2013/11/8
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
void		TDF_0_Forward_Implication			(S_NLIST*, int, int);
void		TDF_1_Forward_Implication			(S_NLIST*, int, int);

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
//  ed_flag		: FSIMでの故障伝搬フラグ(SPPFPで決定)
//  ed_flag-1	: FSIMでイベント計算されたが故障伝搬しなかった信号線(SPPFPで決定)
//  xid_flag	: ケアビットに固定したフラグ(故障伝搬経路or正当化経路)
//
	//#define PATH_DEBUG
	//#define ED_DEBUG

//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_time1_ppo_push
//  機  能 : 【拡張含意用】1時刻目PPOをスタックに積む(slev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), xid_flag(X抽出用フラグ値), tp_id(テストパターン番号)
//  memo   : ★対象信号線自信を積む★
//----------------------------------------------
void TDF_time1_ppo_push(S_NLIST *e_net, int xid_flag, int tp_id){
	
#ifdef ED_DEBUG
	int				j;
#endif
	int				i;
	int				buff_nval;
	
	//=========================================================================
	// 【DEBUG】slev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:slev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, slev_temp[i].n_net);
		for(j=0; j<slev_temp[i].n_net; j++){
			printf("%s (flag:%d)\n", slev_temp[i].net[j]->name, slev_temp[i].net[j]->xid_flag);
		}
		printf("\n");
	}
#endif

	//=======================================================================
	//対象信号線に故障伝搬経路フラグ(xid_flag)が立ってない
	//=======================================================================
	if(e_net->xid_flag != xid_flag){

		//故障伝搬経路フラグ(xid_flag)を設置
		e_net->xid_flag = xid_flag;

		//★DFF以外ならプッシュ
		if(e_net->type != DFF){
			//slev_tempに対象信号線をプッシュ
			slev_temp[e_net->level].net[slev_temp[e_net->level].n_net] = e_net;
		
			//slev_tempの保持信号線数をインクリメント
			slev_temp[e_net->level].n_net++;
		}
#ifdef PATH_DEBUG
		else{
			printf("%s: DFFなのでプッシュしない\n", e_net->name);
		}
#endif

		//-----------------------------------------------------------------------------
		// ■2値ケアビット固定
		//-----------------------------------------------------------------------------
		if(n_before_x == 0){
			//X抽出後値(xid_nval_t1)がドントケアの場合
			if(Get_NBit(e_net->xid_nval_t1, tp_id) == 3){

				//2値正常値(nval_t1)を取得
				buff_nval = Get_NBit_Xbuf(e_net->nval_t1, tp_id);

				//--------------------------------------------------------
				// 0固定
				//--------------------------------------------------------
				if(buff_nval == 0){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t2, tp_id) == 3){
							//--------------------
							//2時刻目0固定
							//--------------------
							Set_NBit_Zero(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【1時刻目PPO】※PI %s : 0固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPO以外：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == NO){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
						}

						//テストパターンに対するケアビット情報
						tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
						tp_bit[tp_id].n_0bit++;		//対象信号線(PI)のテストパターン[tp_id]の0数をインクリメント

						//PIに対するケアビット情報
						pi_bit[e_net->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
						pi_bit[e_net->pi_id].n_0bit++;	//対象信号線(PI)の0数をインクリメント
					}

					//ケアビット固定処理
					Set_NBit_Zero(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
					printf("【1時刻目PPO】%s : 0固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
				}
				//--------------------------------------------------------
				// 1固定
				//--------------------------------------------------------
				else if(buff_nval ==1){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t2, tp_id) == 3){
							//--------------------
							//2時刻目1固定
							//--------------------
							Set_NBit_One(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【1時刻目PPO】※PI %s : 2固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPO以外：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == NO){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
						}

						//テストパターンに対するケアビット情報
						tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
						tp_bit[tp_id].n_1bit++;		//対象信号線(PI)のテストパターン[tp_id]の1数をインクリメント

						//PIに対するケアビット情報
						pi_bit[e_net->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
						pi_bit[e_net->pi_id].n_1bit++;	//対象信号線(PI)の1数をインクリメント
					}

					//ケアビット固定処理
					Set_NBit_One(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
					printf("【1時刻目PPO】%s : 1固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
				}
				else{
					printf("//=================================\n");
					printf("//ERROR : TDF_XID_fpath_push (2値)\n");
					printf("//=================================\n");
					printf("buff_nvalに変な値が入ってる！ (buff_nval = %u)\n", buff_nval);
				}
			}
#ifdef PATH_DEBUG
			else{
				printf("【1時刻目PPO】ケアビット固定済み(1時刻目): %s (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
			}
#endif
		}
		
		//-----------------------------------------------------------------------------
		// ■3値ケアビット固定
		//-----------------------------------------------------------------------------
		else{
			//X抽出後値(xid_nval_t1)がドントケアの場合
			if(Get_NBit(e_net->xid_nval_t1, tp_id) == 3){
			
				//3値正常値(nval_t1)を取得
				buff_nval = Get_NBit(e_net->nval_t1, tp_id);
				
				//--------------------------------------------------------
				// 0固定
				//--------------------------------------------------------
				if(buff_nval == 0){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t2, tp_id) == 3){
							//--------------------
							//2時刻目0固定
							//--------------------
							Set_NBit_Zero(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【1時刻目PPO】※PI %s : 0固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPO以外：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == NO){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
						}

						//テストパターンに対するケアビット情報
						tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
						tp_bit[tp_id].n_0bit++;		//対象信号線(PI)のテストパターン[tp_id]の0数をインクリメント

						//PIに対するケアビット情報
						pi_bit[e_net->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
						pi_bit[e_net->pi_id].n_0bit++;	//対象信号線(PI)の0数をインクリメント
					}

					//ケアビット固定処理
					Set_NBit_Zero(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
					printf("【1時刻目PPO】%s : 0固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
				}
				//--------------------------------------------------------
				// 1固定
				//--------------------------------------------------------
				else if(buff_nval ==1){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t2, tp_id) == 3){
							//--------------------
							//2時刻目1固定
							//--------------------
							Set_NBit_One(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【1時刻目PPO】※PI %s : 1固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPO以外：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == YES){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
						}

						//テストパターンに対するケアビット情報
						tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
						tp_bit[tp_id].n_1bit++;		//対象信号線(PI)のテストパターン[tp_id]の1数をインクリメント

						//PIに対するケアビット情報
						pi_bit[e_net->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
						pi_bit[e_net->pi_id].n_1bit++;	//対象信号線(PI)の1数をインクリメント
					}

					//ケアビット固定処理
					Set_NBit_One(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
					printf("【1時刻目PPO】%s : 1固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
				}
				else{
					printf("//=================================\n");
					printf("//ERROR : TDF_XID_fpath_push (3値)\n");
					printf("//=================================\n");
					printf("buff_nvalに変な値が入ってる！ (buff_nval = %u)\n", buff_nval);
				}
			}
#ifdef PATH_DEBUG
			else{
				printf("【1時刻目PPO】ケアビット固定済み(1時刻目): %s (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
			}
#endif
		}
	}
	//=======================================================================
	//対象信号線に伝搬経路(ケアビット固定)フラグが立ってた！！！
	//=======================================================================
#ifdef PATH_DEBUG
	else{
		printf("【1時刻目PPO】xid_flag設置済み：%s(再収斂？)\n", e_net->name);
	}
#endif
	

	//=========================================================================
	// 【DEBUG】slev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:挿入後slev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, slev_temp[i].n_net);
		for(j=0; j<slev_temp[i].n_net; j++){
			printf("%s (flag:%d)\n", slev_temp[i].net[j]->name, slev_temp[i].net[j]->flag);
		}
		printf("\n");
	}
	
	printf("\n");
#endif

}
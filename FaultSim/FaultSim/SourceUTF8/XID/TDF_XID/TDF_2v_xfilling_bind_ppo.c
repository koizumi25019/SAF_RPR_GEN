//------------------------------------------------------------------------
//File name : TDF_2v_xfilling_bind_ppo
//Date : 2014/7/29
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
	#define TIME1_DEBUG	//1時刻目前方含意・後方含意デバッグ
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
//  関数名 : TDF_2v_xfilling_bind_ppo
//  機  能 : 【限定含意・限定正当化】2値SPPFP結果からの必要なケアビット決定(要はX抽出部)
//  戻り値 : ed_flag(イベントドリブン用フラグ)
//  引  数 : tp_id(テストパターンID, t_fault(1個の故障リスト), ppo_id(故障検出保障PPO(ソート済みPPOのインデックス)), ed_flag(イベントドリブン用フラグ), xid_flag(X抽出用フラグ), jus_flag(限定正当化用フラグ)
//----------------------------------------------
unsigned int	TDF_2v_xfilling_bind_ppo(int tp_id, SXID_LIST **t_fault, int ppo_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int *jus_flag){

	int			i,j,k;
	int			event_lev;					//故障伝搬経路を決めるレベル
	int			level_flag=0;				//event_lev更新時にbreakしないようにする
	S_NLIST		*temp_net=NULL;				//スタックから取り出した信号線を一時的に格納
	
	
	//======================================================================================
	// SPPFPドントケア抽出 (1故障対象，故障検出保障PPO指定)
	//======================================================================================
	//指定ビット目(i番目故障)に『1』が立っている⇒経路追跡OK (i番目故障が検出可能)
	if(Get_NBit_INT(sort_propa_po[ppo_id]->det_flag, 0) == 1){

#ifdef PATH_DEBUG
		printf("\n\n【X抽出開始】\n");
		printf("★%d番目の故障：%s  ",0, t_fault[0]->net->name);
		if(t_fault[0]->fault_type == TDF_STR){
			printf("STR");
		}
		else if(t_fault[0]->fault_type == TDF_STF){
			printf("STF");
		}
		printf(" ed_flag:%d xid_flag:%d jus_flag:%d\n\n",ed_flag, xid_flag, *jus_flag);
		printf("故障伝搬PPO: %s\n", ppo[sort_propa_po[ppo_id]->po_id]->name);
#endif

		//======================================================================================
		// 【1時刻目初期値の値固定・前方含意・後方含意】(1時刻目)
		//======================================================================================
		//--------------------------------------------------
		//立上り遷移故障(0→1/0)
		//--------------------------------------------------
		if(t_fault[0]->fault_type == TDF_STR){

			//1時刻目対象信号線がドントケアの場合
			if(Get_NBit(t_fault[0]->net->xid_nval_t1, tp_id) == 3){
#ifdef TIME1_DEBUG
				printf("初期値0固定(時刻1):%s\n", t_fault[0]->net->name);
#endif
				//--------------------
				//1時刻目0固定
				//--------------------
				Set_NBit_Zero(t_fault[0]->net->xid_nval_t1, tp_id);
							
				//--------------------
				//後方含意判定
				//--------------------
				//PIの場合
				if(t_fault[0]->net->type == IN){
					printf("\n//----------------------------------\n");
					printf("// WARNING : TDF_2v_xfilling.c \n");
					printf("//----------------------------------\n");
					printf("PIに遷移故障が設置されています．\n");
				}
				//PPIの場合
				else if(t_fault[0]->net->type == DFF){
					//テストパターンに対するケアビット情報
					tp_bit[tp_id].n_cbit++;						//対象信号線(PPI)のテストパターン[tp_id]のケアビット数をインクリメント
					tp_bit[tp_id].n_0bit++;						//対象信号線(PPI)のテストパターン[tp_id]の0数をインクリメント

					//PPIに対するケアビット情報
					ppi_bit[t_fault[0]->net->ppi_id].n_cbit++;	//対象信号線(PPI)のケアビット数をインクリメント
					ppi_bit[t_fault[0]->net->ppi_id].n_0bit++;	//対象信号線(PPI)の0数をインクリメント
				}
				//その他→後方含意
				else{
					TDF_0_Backward_Implication(t_fault[0]->net, tp_id, 1, *jus_flag);
				}						

				//--------------------
				//PPOの場合：2時刻目前方含意
				//--------------------
				if(t_fault[0]->net->ppo_flag == YES){
					for(k=0; k<t_fault[0]->net->n_out; k++){
						if(Get_NBit(t_fault[0]->net->out[k]->xid_nval_t2, tp_id) == 3){
							TDF_0_Forward_Implication(t_fault[0]->net->out[k], tp_id, 2);
						}
					}
				}

				//--------------------
				//それ以外：1時刻目前方含意
				//--------------------
				else{
					for(k=0; k<t_fault[0]->net->n_out; k++){
						if(Get_NBit(t_fault[0]->net->out[k]->xid_nval_t1, tp_id) == 3){
							TDF_0_Forward_Implication(t_fault[0]->net->out[k], tp_id, 1);
						}
					}
				}
			}
		}

		//--------------------------------------------------
		//立下り遷移故障(1→0/1)
		//--------------------------------------------------
		else if(t_fault[0]->fault_type == TDF_STF){
						
			//1時刻目対象信号線がドントケアの場合
			if(Get_NBit(t_fault[0]->net->xid_nval_t1, tp_id) == 3){
#ifdef TIME1_DEBUG
				printf("初期値1固定(時刻1):%s\n", t_fault[0]->net->name);
#endif
				//--------------------
				//1時刻目1固定
				//--------------------
				Set_NBit_One(t_fault[0]->net->xid_nval_t1, tp_id);
							
				//--------------------
				//後方含意判定
				//--------------------
				//PIの場合
				if(t_fault[0]->net->type == IN){								
					printf("\n//----------------------------------\n");
					printf("// WARNING : TDF_2v_xfilling.c \n");
					printf("//----------------------------------\n");
					printf("PIに遷移故障が設置されています．\n");
				}
				//PPIの場合
				else if(t_fault[0]->net->type == DFF){
					//テストパターンに対するケアビット情報
					tp_bit[tp_id].n_cbit++;						//対象信号線(PPI)のテストパターン[tp_id]のケアビット数をインクリメント
					tp_bit[tp_id].n_1bit++;						//対象信号線(PPI)のテストパターン[tp_id]の1数をインクリメント

					//PPIに対するケアビット情報
					ppi_bit[t_fault[0]->net->ppi_id].n_cbit++;	//対象信号線(PPI)のケアビット数をインクリメント
					ppi_bit[t_fault[0]->net->ppi_id].n_1bit++;	//対象信号線(PPI)の1数をインクリメント
				}
				//その他→後方含意
				else{
					TDF_1_Backward_Implication(t_fault[0]->net, tp_id, 1, *jus_flag);
				}
							
				//--------------------
				//PPOの場合：2時刻目前方含意
				//--------------------
				if(t_fault[0]->net->ppo_flag == YES){
					for(k=0; k<t_fault[0]->net->n_out; k++){
						if(Get_NBit(t_fault[0]->net->out[k]->xid_nval_t2, tp_id) == 3){
							TDF_1_Forward_Implication(t_fault[0]->net->out[k], tp_id, 2);
						}
					}
				}

				//--------------------
				//それ以外：1時刻目前方含意
				//--------------------
				else{
					for(k=0; k<t_fault[0]->net->n_out; k++){
						if(Get_NBit(t_fault[0]->net->out[k]->xid_nval_t1, tp_id) == 3){
							TDF_1_Forward_Implication(t_fault[0]->net->out[k], tp_id, 1);
						}
					}
				}
			}
		}

		//======================================================================================
		// 【故障伝搬PPO決定】※本モードではファイルから指定済み
		//======================================================================================
#ifdef PATH_DEBUG
		printf("\n\n【故障伝搬経路決定】\n");
		printf("故障伝搬PPO: %s\n", ppo[sort_propa_po[ppo_id]->po_id]->name);
		printf("対象故障：%s  TDF%d ed_flag:%d xid_flag:%d jus_flag:%d\n\n",t_fault[0]->net->name, t_fault[0]->fault_type, ed_flag, xid_flag, *jus_flag);
#endif
		//PPOをプッシュ
		TDF_XID_fpath_push(ppo[sort_propa_po[ppo_id]->po_id], xid_flag, tp_id);

		//イベントレベルの決定
		event_lev = ppo[sort_propa_po[ppo_id]->po_id]->level;	//プッシュしたPOのレベル


		//======================================================================================
		// 【伝搬経路決定】(2時刻目)
		//======================================================================================
		while(1){

			//-------------------------------------------
			// レベライズスタックから信号線を取り出す
			//-------------------------------------------
			temp_net = slev_temp[event_lev].net[slev_temp[event_lev].n_net-1];	//ケツから取り出す
			slev_temp[event_lev].n_net--;										//信号線保持数デクリメント

#ifdef PATH_DEBUG
			printf("故障伝搬信号線:%s (ed_flag=%d  xid_flag=%d)\n", temp_net->name, temp_net->flag, temp_net->xid_flag);
#endif
			//-------------------------------------------
			// ◆対象信号線がFOUT-STEMの場合前方含意
			//-------------------------------------------
			if(temp_net->n_out >= 2){	//出力信号線数が2本以上(= FOUT-STEM)
					
				for(k=0; k<temp_net->n_out; k++){

					//X抽出後値がドントケア
					if(Get_NBit(temp_net->out[k]->xid_nval_t2, tp_id) == 3){
									
						//入力(STEM)値0からの前方含意(2時刻目)
						if(Get_NBit(temp_net->xid_nval_t2, tp_id) == 0){
							TDF_0_Forward_Implication(temp_net->out[k], tp_id, 2);
						}
						//入力(STEM)値1からの前方含意(2時刻目)
						else{
							TDF_1_Forward_Implication(temp_net->out[k], tp_id, 2);
						}
					}
				}
			}


			//-------------------------------------------
			// 故障個所だったらbreak
			//-------------------------------------------
			if(temp_net->nbit_fault == i){
#ifdef PATH_DEBUG
				printf("故障個所break!!\n");
#endif
				break;
			}
						
			//-------------------------------------------
			// 【関数ポインタ使用】temp_netの伝搬経路計算
			//-------------------------------------------
			(*sfunc_path[temp_net->type])(temp_net, tp_id, i, ed_flag, xid_flag, *jus_flag);

			//-------------------------------------------
			// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
			//-------------------------------------------
			if(slev_temp[event_lev].n_net == 0){

				//レベライズスタック内を探索(PO⇒PI側へ)
				for(k=event_lev-1; k>=0; k--){
					//保持信号線数≠0のレベルスタック発見
					if(slev_temp[k].n_net != 0){
						event_lev = k;		//event_lev更新
						level_flag++;		//break阻止
						break;
					}
				}

				//レベライズスタックの中身が全部空だった
				if(level_flag == 0){
					break;	//イベントドリブン終了
				}

				//次のevent_lev更新時に備えてlevel_flagを初期化
				level_flag = 0;

			}
#ifdef PATH_DEBUG
				printf("\n");
#endif
		}
#ifdef PATH_DEBUG
		printf("\n");
#endif

		//======================================================================================
		// 【故障励起】(2時刻目)
		//======================================================================================
		//※故障箇所信号線自身はケアビット固定済み
#ifdef ECX_DEBUG
		printf("\n\n【故障励起開始】\n");
#endif
		//temp_net == 故障箇所信号線(ケアビット固定済み)
		switch(temp_net->type){
			//================================================================
			case IN:
#ifdef ECX_DEBUG
				printf("\n//----------------------------------\n");
				printf("// WARNING : TDF_2v_xfilling.c \n");
				printf("//----------------------------------\n");
				printf("【故障励起】PIに遷移故障が設置されています．\n");
#endif
			break;
			//================================================================
			case DFF:
			case BUF:
#ifdef ECX_DEBUG
				printf("【DFF・BUF】");
#endif
				//出力信号線=0
				if(Get_NBit_Xbuf(temp_net->xid_nval_t2, tp_id) == 0){
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力信号線=1
				else{
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}

			break;
			//================================================================
			case INV:
#ifdef ECX_DEBUG
				printf("【INV】");
#endif						

				//出力信号線=0
				if(Get_NBit_Xbuf(temp_net->xid_nval_t2, tp_id) == 0){
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力信号線=1
				else{
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}

			break;
			//================================================================
			case FOUT:
						
				//-------------------------------------------
				//ブランチを前方含意
				//-------------------------------------------
				for(k=0; k<temp_net->in[0]->n_out; k++){

					//X抽出後値がドントケア
					if(Get_NBit(temp_net->in[0]->out[k]->xid_nval_t2, tp_id) == 3){
									
						//0からの前方含意(ステムは現時点でXなので故障信号線値(ブランチ)で値判定)
						if(Get_NBit(temp_net->xid_nval_t2, tp_id) == 0){
							TDF_0_Forward_Implication(temp_net->in[0]->out[k], tp_id, 2);
						}
						//1からの前方含意(ステムは現時点でXなので故障信号線値(ブランチ)で値判定)
						else{
							TDF_1_Forward_Implication(temp_net->in[0]->out[k], tp_id, 2);
						}
					}
				}
								
				//-------------------------------------------
				//後方含意エンキュー&ケアビット固定関数
				//-------------------------------------------
#ifdef ECX_DEBUG
				printf("【FOUT】");
#endif
				//出力信号線=0
				if(Get_NBit_Xbuf(temp_net->xid_nval_t2, tp_id) == 0){
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力信号線=1
				else{
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}

			break;
			//================================================================
			case AND:
				//出力の正常値が1(入力が全て非制御値)
				if((temp_net->x_fault & MASKbit[i]) == 0){	//故障値だと=0
#ifdef ECX_DEBUG
					printf("【AND】出力値1(非制御値入力)\n");
#endif
					//出力値1からの後方含意
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力の正常値が0(入力に制御値がある)
				else{
#ifdef ECX_DEBUG
					printf("【AND】出力値0(制御値入力)\n");
#endif
					//出力値0からの後方含意
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
			break;
			//================================================================
			case NAND:
				//出力の正常値が0(入力が全て非制御値)
				if((temp_net->x_fault & MASKbit[i]) != 0){	//故障値だと=1
#ifdef ECX_DEBUG
					printf("【NAND】出力値0(非制御値入力)\n");
#endif
					//出力値0からの後方含意
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力の正常値が1(入力に制御値がある)
				else{
#ifdef ECX_DEBUG
					printf("【NAND】出力値1(制御値入力)\n");
#endif
					//出力値1からの後方含意
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
			break;
			//================================================================
			case OR:
				//出力の正常値が0(入力が全て非制御値)
				if((temp_net->x_fault & MASKbit[i]) != 0){	//故障値だと=1
#ifdef ECX_DEBUG
					printf("【OR】出力値0(非制御値入力)\n");
#endif
					//出力値0からの後方含意
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力の正常値が1(入力に制御値がある)
				else{
#ifdef ECX_DEBUG
					printf("【OR】出力値1(制御値入力)\n");
#endif
					//出力値1からの後方含意
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
			break;
			//================================================================
			case NOR:
				//出力の正常値が1(入力が全て非制御値)
				if((temp_net->x_fault & MASKbit[i]) == 0){	//故障値だと=0
#ifdef ECX_DEBUG
					printf("【NOR】出力値1(非制御値入力)\n");
#endif
					//出力値1からの後方含意
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力の正常値が0(入力に制御値がある)
				else{
#ifdef ECX_DEBUG
					printf("【NOR】出力値0(制御値入力)\n");
#endif
					//出力値0からの後方含意
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
			break;
			//================================================================
			case EXOR:
			case EXNOR:
				//出力信号線=0
				if(Get_NBit_Xbuf(temp_net->xid_nval_t2, tp_id) == 0){
					TDF_0_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
				//出力信号線=1
				else{
					TDF_1_Backward_Implication(temp_net, tp_id, 2, *jus_flag);
				}
			break;
			//================================================================
		}


		//======================================================================================
		// 【限定正当化】
		//======================================================================================
#ifdef ECX_DEBUG			
		printf("\n【限定正当化開始】\n");
#endif
		while((temp_net = (S_NLIST *)dequeue(nl_queue)) != (S_NLIST *)NULL){	//空になるまでデキュー

			//-------------------------------------------
			// 【関数ポインタ使用】temp_netに対して限定含意・限定正当化
			//-------------------------------------------
			if(temp_net->n_in != 0){
#ifdef ECX_DEBUG
				printf("限定正当化: %s\n", temp_net->name);
#endif
				//正当化(関数ポインタ)
				(*sfunc_jus[temp_net->type])(temp_net, tp_id, xid_flag, *jus_flag);
			}
		}

		//======================================================================================
		// キュー再初期化
		//======================================================================================
		if(nl_queue->front != nl_queue->rear){
			printf("nl_queueエラー：キュー内に残りあり\n");
		}
		else if(fimpl_queue->front != fimpl_queue->rear){
			printf("fimpl_queueエラー：キュー内に残りあり\n");
		}
		else if(bimpl_queue->front != bimpl_queue->rear){
			printf("bimpl_queueエラー：キュー内に残りあり\n");
		}

		reset_queue(nl_queue);
		reset_queue(fimpl_queue);
		reset_queue(bimpl_queue);
					
		//======================================================================================
		// ケアビット固定フラグ(xid_flag)と正当化フラグ(jus_flag)を更新
		//======================================================================================
		xid_flag+=2;
		*jus_flag+=3;									
		
		//======================================================================================
		//故障伝搬PPOフラグの初期化(X抽出用のSPPFPの結果情報)
		//======================================================================================
		for(i=0; i<n_ppo; i++){
			if(sort_propa_po[i]->n_det > 0){
				sort_propa_po[i]->n_det = 0;						//対象PPOでの故障検出数初期化
				All_INT_Zero(sort_propa_po[i]->det_flag);		//対象PPOでの故障検出フラグ初期化(どの故障を検出したか)
			}
		}

	}

	//ERROR処理
	else{
		printf("\n//-------------------------------------\n");
		printf("//ERROR : TDF_2v_xfilling_bind_ppo\n");
		printf("//-------------------------------------\n");
		printf("対象故障がPPOで検出しません！\n");
	}


	//======================================================================================
	//xid_flag(ケアビット固定フラグ)の返却
	//======================================================================================
	return xid_flag;

}
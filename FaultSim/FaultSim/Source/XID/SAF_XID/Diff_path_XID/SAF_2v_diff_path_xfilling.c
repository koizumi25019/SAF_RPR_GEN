//------------------------------------------------------------------------
//File name : SAF_2v_diff_path_xfilling
//Date : 2013/7/27
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../../Netlist/netlist.h"
#include	"../../../Lib/bit_tp.h"
#include	"../../../Lib/bit_int.h"
#include	"../../../StandardHead.h"
#include	"../../../option.h"
#include	"../../Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void		dtob						(unsigned int, int);
void		SAF_XID_fpath_push			(NLIST*, int, int);
void		SAF_0_Backward_Implication	(NLIST*, int);
void		SAF_1_Backward_Implication	(NLIST*, int);
void		SAF_0_Forward_Implication	(NLIST*, int);
void		SAF_1_Forward_Implication	(NLIST*, int);

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

//※nl[i].x_faultはPPSFP時の値がそのままなので注意(故障伝搬してない信号線も何か値が入りっぱなし)
// nl[i].p_faultも同様に初期化してない
//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------
//static  unsigned int     all_zero = 0;				//32bit全て『0』未使用のため削除
//static  unsigned int     all_one  = 0xFFFFFFFF;		//32bit全て『1』未使用のため削除

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_2v_diff_path_xfilling
//  機  能 : 【通常とは異なるパスで限定含意・限定正当化】2値SPPFP結果からの必要なケアビット決定
//  戻り値 : ed_flag(イベントドリブン用フラグ)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ), xid_flag(X抽出用フラグ)
//----------------------------------------------
unsigned int	SAF_2v_diff_path_xfilling(int tp_id, XID_LIST **t_fault, int n_fault, unsigned int ed_flag, unsigned int xid_flag){

	int			i,j,k;
	int			event_lev;					//故障伝搬経路を決めるレベル
	int			level_flag=0;				//event_lev更新時にbreakしないようにする
	NLIST		*temp_net=NULL;				//スタックから取り出した信号線を一時的に格納
	
	
	///DEBUG////////////////////////////////////////////////
#ifdef PATH_DEBUG
	printf("\n//==========================================\n");
	printf("// 2値 X抽出関数\n");
	printf("//==========================================\n");
	for(i=0; i<n_net; i++)printf("%s ed_flag:%d xid_flag:%d\n",nl[i].name, nl[i].flag, nl[i].xid_flag);
	printf("\n\n");

	printf("【 テストパターン[%d] 】\n", tp_id);
	//テストパターン表示
	//2値
	if(n_before_x == 0){
		for(i=0; i<n_pi; i++){
			j = Get_NBit_Xbuf(pi[i]->nval, tp_id);
			if(j==0){
				printf("0");
			}
			else if(j==1){
				printf("1");
			}
		}
	}
	//3値
	else{
		for(i=0; i<n_pi; i++){
			j = Get_NBit_Xbuf(pi[i]->nval, tp_id);
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
	}
	printf("\n");
	printf("X抽出対象故障リスト[%d個]\n", n_fault);
	for(i=0; i<n_fault; i++)printf("%d番目 対象故障：%s  SAF%d\n",i, t_fault[i]->net->name, t_fault[i]->fault_type);
	printf("\n\n");
#endif
	////////////////////////////////////////////////////////

	//======================================================================================
	// SPPFPドントケア抽出 (1故障ずつX抽出)
	//======================================================================================
	//最大32故障に対してループ
	for(i=0; i<n_fault; i++){

		//各POに対して対象故障を検出してるか確認(※通常とは逆ループ)
		for(j=n_po-1; j>=0; j--){
			
			//直前のSPPFPでj番目POで検出した故障数が1以上存在
			if(sort_propa_po[j]->n_det > 0){

				//指定ビット目(i番目故障)に『1』が立っている⇒経路追跡OK (i番目故障が検出可能)
				if(Get_NBit_INT(sort_propa_po[j]->det_flag, i) == 1){
					
#ifdef PATH_DEBUG
					printf("\n\n【X抽出開始】\n");
					printf("★%d番目の故障：%s  SAF%d ed_flag:%d xid_flag:%d\n\n",i, t_fault[i]->net->name, t_fault[i]->fault_type, ed_flag, xid_flag);
					printf("故障伝搬PO: %s\n", po[sort_propa_po[j]->po_id]->name);
#endif
					//======================================================================================
					// 【故障伝搬PO決定】
					//======================================================================================
#ifdef PATH_DEBUG
					printf("\n\n【故障伝搬経路決定】\n");
					printf("対象故障：%s  SAF%d ed_flag:%d xid_flag:%d\n\n",t_fault[i]->net->name, t_fault[i]->fault_type, ed_flag, xid_flag);
#endif

					//POをプッシュ
					SAF_XID_fpath_push(po[sort_propa_po[j]->po_id], xid_flag, tp_id);

					//イベントレベルの決定
					event_lev = po[sort_propa_po[j]->po_id]->level;	//プッシュしたPOのレベル


					//======================================================================================
					// 【伝搬経路決定】
					//======================================================================================
					while(1){

						//-------------------------------------------
						// レベライズスタックから信号線を取り出す
						//-------------------------------------------
						temp_net = lev_temp[event_lev].net[lev_temp[event_lev].n_net-1];	//ケツから取り出す
						lev_temp[event_lev].n_net--;										//信号線保持数デクリメント

#ifdef PATH_DEBUG
						printf("故障伝搬信号線:%s (ed_flag=%d  xid_flag=%d)\n", temp_net->name, temp_net->flag, temp_net->xid_flag);
#endif
						//-------------------------------------------
						// ◆対象信号線がFOUT-STEMの場合前方含意
						//-------------------------------------------
						if(temp_net->n_out >= 2){	//出力信号線数が2本以上(= FOUT-STEM)
					
							for(k=0; k<temp_net->n_out; k++){

								//X抽出後値がドントケア
								if(Get_NBit(temp_net->out[k]->xid_nval, tp_id) == 3){
									
									//入力(STEM)値0からの前方含意
									if(Get_NBit(temp_net->xid_nval, tp_id) == 0){
										SAF_0_Forward_Implication(temp_net->out[k], tp_id);
									}
									//入力(STEM)値1からの前方含意
									else{
										SAF_1_Forward_Implication(temp_net->out[k], tp_id);
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
						(*func_path[temp_net->type])(temp_net, tp_id, i, ed_flag, xid_flag);

						//-------------------------------------------
						// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
						//-------------------------------------------
						if(lev_temp[event_lev].n_net == 0){

							//レベライズスタック内を探索(PO⇒PI側へ)
							for(k=event_lev-1; k>=0; k--){
								//保持信号線数≠0のレベルスタック発見
								if(lev_temp[k].n_net != 0){
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
					// 【故障励起】
					//======================================================================================
					//※故障箇所信号線自身はケアビット固定済み
#ifdef ECX_DEBUG
					printf("\n\n【故障励起開始】\n");
#endif
					switch(temp_net->type){
						//================================================================
						case IN:
#ifdef ECX_DEBUG
							printf("【PI】故障個所がPIのため何もしなくておk\n");	//PIの入力信号線とか無いし・・・
#endif
						break;
						//================================================================
						case BUF:
#ifdef ECX_DEBUG
							printf("【BUF】");
#endif
							//出力信号線=0
							if(Get_NBit_Xbuf(temp_net->xid_nval, tp_id) == 0){
								SAF_0_Backward_Implication(temp_net, tp_id);
							}
							//出力信号線=1
							else{
								SAF_1_Backward_Implication(temp_net, tp_id);
							}

						break;
						//================================================================
						case INV:
#ifdef ECX_DEBUG
							printf("【INV】");
#endif
							//出力信号線=0
							if(Get_NBit_Xbuf(temp_net->xid_nval, tp_id) == 0){
								SAF_1_Backward_Implication(temp_net, tp_id);
							}
							//出力信号線=1
							else{
								SAF_0_Backward_Implication(temp_net, tp_id);
							}

						break;
						//================================================================
						case FOUT:
						
							//-------------------------------------------
							//前方含意
							//-------------------------------------------
							for(k=0; k<temp_net->in[0]->n_out; k++){

								//X抽出後値がドントケア
								if(Get_NBit(temp_net->in[0]->out[k]->xid_nval, tp_id) == 3){
									
									//入力(STEM)値0からの前方含意
									if(Get_NBit(temp_net->in[0]->xid_nval, tp_id) == 0){
										SAF_0_Forward_Implication(temp_net->in[0]->out[k], tp_id);
									}
									//入力(STEM)値1からの前方含意
									else{
										SAF_1_Forward_Implication(temp_net->in[0]->out[k], tp_id);
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
							if(Get_NBit_Xbuf(temp_net->xid_nval, tp_id) == 0){
								SAF_0_Backward_Implication(temp_net, tp_id);
							}
							//出力信号線=1
							else{
								SAF_1_Backward_Implication(temp_net, tp_id);
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
								SAF_1_Backward_Implication(temp_net, tp_id);
							}
							//出力の正常値が0(入力に制御値がある)
							else{
#ifdef ECX_DEBUG
								printf("【AND】出力値0(制御値入力)\n");
#endif
								//出力値0からの後方含意
								SAF_0_Backward_Implication(temp_net, tp_id);
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
								SAF_0_Backward_Implication(temp_net, tp_id);
							}
							//出力の正常値が1(入力に制御値がある)
							else{
#ifdef ECX_DEBUG
								printf("【NAND】出力値1(制御値入力)\n");
#endif
								//出力値1からの後方含意
								SAF_1_Backward_Implication(temp_net, tp_id);
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
								SAF_0_Backward_Implication(temp_net, tp_id);
							}
							//出力の正常値が1(入力に制御値がある)
							else{
#ifdef ECX_DEBUG
								printf("【OR】出力値1(制御値入力)\n");
#endif
								//出力値1からの後方含意
								SAF_1_Backward_Implication(temp_net, tp_id);
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
								SAF_1_Backward_Implication(temp_net, tp_id);
							}
							//出力の正常値が0(入力に制御値がある)
							else{
#ifdef ECX_DEBUG
								printf("【NOR】出力値0(制御値入力)\n");
#endif
								//出力値0からの後方含意
								SAF_0_Backward_Implication(temp_net, tp_id);
							}
						break;
						//================================================================
						case EXOR:
						case EXNOR:
							//出力信号線=0
							if(Get_NBit_Xbuf(temp_net->xid_nval, tp_id) == 0){
								SAF_0_Backward_Implication(temp_net, tp_id);
							}
							//出力信号線=1
							else{
								SAF_1_Backward_Implication(temp_net, tp_id);
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

					while((temp_net = (NLIST *)dequeue(nl_queue)) != (NLIST *)NULL){	//空になるまでデキュー

						//-------------------------------------------
						// 【関数ポインタ使用】temp_netに対して限定含意・限定正当化
						//-------------------------------------------
						if(temp_net->n_in != 0){
#ifdef ECX_DEBUG
							printf("限定正当化: %s\n", temp_net->name);
#endif
							//正当化(関数ポインタ)
							(*func_jus[temp_net->type])(temp_net, tp_id, xid_flag);
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
					// ケアビット固定フラグ(xid_flag)を更新
					//======================================================================================
					xid_flag+=2;

					//======================================================================================
					// 次故障のX抽出に移行するためbreak (変数iを更新させたい)
					//======================================================================================
					break;

				}//if(i番目故障のX抽出)

			}//if(j番目POがそもそも故障検出してるか？)

		}//for(PO探索ループ[変数j])

	}//for(最大32故障ループ[変数i])
	

	//======================================================================================
	//故障伝搬POフラグの初期化(X抽出用のSPPFPの結果情報)
	//======================================================================================
	for(i=0; i<n_po; i++){
		if(sort_propa_po[i]->n_det > 0){
			sort_propa_po[i]->n_det = 0;					//対象POでの故障検出数初期化
			All_INT_Zero(sort_propa_po[i]->det_flag);		//対象POでの故障検出フラグ初期化(どの故障を検出したか)
		}
	}

	
	//======================================================================================
	//xid_flag(ケアビット固定フラグ)の返却
	//======================================================================================
	return xid_flag;

}
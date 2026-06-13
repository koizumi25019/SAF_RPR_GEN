


//------------------------------------------------------------------------
//File name : TDF_2v_EX_xfilling
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
void		TDF_time1_ppo_push			(S_NLIST*, int, int);
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
static  unsigned int     all_zero = 0;				//32bit全て『0』
static  unsigned int     all_one  = 0xFFFFFFFF;		//32bit全て『1』

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------

//----------------------------------------------
//  関数名 : TDF_2v_EX_xfilling
//  機  能 : 【見逃し故障用】2値SPPFP結果からの必要なケアビット決定(要はX抽出部)
//  戻り値 : ed_flag(イベントドリブン用フラグ)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ), xid_flag(X抽出用フラグ), jus_flag(正当化フラグ)
//----------------------------------------------
unsigned int	TDF_2v_EX_xfilling(int tp_id, SXID_LIST **t_fault, int n_fault, unsigned int ed_flag, unsigned int xid_flag, unsigned int *jus_flag){

	int			i,j,k,m;
	int			nval;
	int			event_lev;					//故障伝搬経路を決めるレベル
	int			level_flag=0;				//event_lev更新時にbreakしないようにする
	S_NLIST		*temp_net=NULL;				//スタックから取り出した信号線を一時的に格納
	
	
	///DEBUG////////////////////////////////////////////////
#ifdef PATH_DEBUG

	/*

	printf("\n//==========================================\n");
	printf("// 2値 見逃し故障X抽出関数\n");
	printf("//==========================================\n");
	for(i=0; i<n_snet; i++)printf("%s ed_flag:%d xid_flag:%d\n",s_nl[i].name, s_nl[i].flag, s_nl[i].xid_flag);
	printf("\n\n");

	printf("【 テストパターン[%d] 】\n", tp_id);
	//テストパターン表示
	//PI
	//2値
	if(n_before_x == 0){
		for(i=0; i<n_spi; i++){
			j = Get_NBit_Xbuf(spi[i]->nval_t1, tp_id);
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
		for(i=0; i<n_spi; i++){
			j = Get_NBit_Xbuf(spi[i]->nval_t1, tp_id);
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
	//PPI
	//2値
	if(n_before_x == 0){
		for(i=0; i<n_ppi; i++){
			j = Get_NBit_Xbuf(ppi[i]->nval_t1, tp_id);
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
		for(i=0; i<n_ppi; i++){
			j = Get_NBit_Xbuf(ppi[i]->nval_t1, tp_id);
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
	for(i=0; i<n_fault; i++)printf("%d番目 対象故障：%s  TDF%d\n",i, t_fault[i]->net->name, t_fault[i]->fault_type);
	printf("\n\n");
	*/
	
#endif
	////////////////////////////////////////////////////////
	
	//======================================================================================
	// SPPFPドントケア抽出 (結局は1故障ずつX抽出してる件. 何か良い案募集中)
	//======================================================================================
	//最大32故障に対してループ
	for(i=0; i<n_fault; i++){

		//各PPOに対して対象故障を検出してるか確認
		for(j=0; j<n_ppo; j++){
			
			//直前のSPPFPでj番目POで検出した故障数が1以上存在
			if(sort_propa_po[j]->n_det > 0){

				//指定ビット目(i番目故障)に『1』が立っている⇒経路追跡OK (i番目故障が検出可能)
				if(Get_NBit_INT(sort_propa_po[j]->det_flag, i) == 1){
					
#ifdef PATH_DEBUG
					printf("\n\n【X抽出開始】\n");
					printf("★%d番目の故障：%s  TDF%d ed_flag:%d xid_flag:%d\n\n",i, t_fault[i]->net->name, t_fault[i]->fault_type, ed_flag, xid_flag);
					printf("故障伝搬PPO: %s\n", ppo[sort_propa_po[j]->po_id]->name);
#endif
					//======================================================================================
					// 【故障伝搬PPO決定】
					//======================================================================================
#ifdef PATH_DEBUG
					printf("\n\n【故障伝搬経路決定】\n");
					printf("対象故障：%s  TDF%d ed_flag:%d xid_flag:%d\n\n",t_fault[i]->net->name, t_fault[i]->fault_type, ed_flag, xid_flag);
#endif
					//2時刻目PPOをプッシュ
					TDF_XID_fpath_push(ppo[sort_propa_po[j]->po_id], xid_flag, tp_id);

					//イベントレベルの決定
					event_lev = ppo[sort_propa_po[j]->po_id]->level;	//プッシュしたPOのレベル

					//PPOがDFFの場合(PPOかつPPIのとき)
					if(ppo[sort_propa_po[j]->po_id]->type == DFF){

						//PPOを1時刻目正当化キューにエンキュー
						//enqueue(nl_queue, ppo[sort_propa_po[j]->po_id]->in[0]);
						enqueue(nl_queue, ppo[sort_propa_po[j]->po_id]);

					}

					//======================================================================================
					// ◆2時刻目拡張含意操作◆PPOから到達可能な全PI・PPI(1時刻目)までを全てケアビット固定
					//======================================================================================

					//PPOかつPPIでない場合のみ2時刻目拡張含意操作(2014/05/16バグ修正：山崎)
					else{

						while(1){

						//----------------------------------------------------------------------
						// レベライズスタックから信号線を取り出す
						//----------------------------------------------------------------------
						temp_net = slev_temp[event_lev].net[slev_temp[event_lev].n_net-1];	//ケツから取り出す
						slev_temp[event_lev].n_net--;										//信号線保持数デクリメント

#ifdef PATH_DEBUG
						printf("故障伝搬信号線:%s (ed_flag=%d  xid_flag=%d)\n", temp_net->name, temp_net->flag, temp_net->xid_flag);
#endif
						//----------------------------------------------------------------------
						// 【拡張含意操作】 全入力信号線をケアビット固定
						//----------------------------------------------------------------------
						for(k=0; k<temp_net->n_in; k++){

							//xid_flag未設置の信号線のみ処理
							if(temp_net->in[k]->xid_flag != xid_flag){
																
								//------------------------------------------
								//★DFF(PPI)の場合PPOを1時刻目拡張含意キューにエンキュー
								//------------------------------------------
								if(temp_net->in[k]->type == DFF){
									enqueue(nl_queue, temp_net->in[k]->in[0]);	//PPOをエンキュー
#ifdef PATH_DEBUG
									printf("1時刻目エンキュー:%s (ed_flag=%d  xid_flag=%d)\n", temp_net->in[k]->in[0]->name, temp_net->in[k]->in[0]->flag, temp_net->in[k]->in[0]->xid_flag);
#endif
								}

								//------------------------------------------
								//レベルスタックにプッシュ
								//------------------------------------------
								else{
									//slev_tempに対象信号線をプッシュ
									slev_temp[temp_net->in[k]->level].net[slev_temp[temp_net->in[k]->level].n_net] = temp_net->in[k];
		
									//slev_tempの保持信号線数をインクリメント
									slev_temp[temp_net->in[k]->level].n_net++;
								}

								//xid_flag設置
								temp_net->in[k]->xid_flag = xid_flag;
								
								
								//------------------------------------------
								//対象信号線がXの場合
								//------------------------------------------
								if(Get_NBit(temp_net->in[k]->xid_nval_t2, tp_id) == 3){

									//対象信号線の正常値を取得
									nval = Get_NBit_Xbuf(temp_net->in[k]->nval_t2, tp_id);
								
									//------------------------------------------
									//ケアビット0固定
									//------------------------------------------
									if(nval == 0){

										//対象信号線がPIの場合ケアビット保持数情報を更新
										if(temp_net->in[k]->type == IN){

											//見逃し故障X抽出で増加したPIのケアビット数
											n_missed_cbit++;

											//テストパターンに対するケアビット情報
											tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
											tp_bit[tp_id].n_0bit++;		//対象信号線(PI)のテストパターン[tp_id]の0数をインクリメント

											//PIに対するケアビット情報
											pi_bit[temp_net->in[k]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
											pi_bit[temp_net->in[k]->pi_id].n_0bit++;	//対象信号線(PI)の0数をインクリメント

											//1時刻目もケアビット固定
											if(Get_NBit(temp_net->in[k]->xid_nval_t1, tp_id) == 3){
												//0固定
												Set_NBit_Zero(temp_net->in[k]->xid_nval_t1, tp_id);
												
#ifdef PATH_DEBUG
												printf("PI 0固定(1時刻目)：%s\n", temp_net->in[k]->name);
#endif
												//★PPOの場合：2時刻目前方含意
												if(temp_net->in[k]->ppo_flag == YES){
													for(m=0; m<temp_net->in[k]->n_out; m++){
														if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t2, tp_id) == 3){
															TDF_0_Forward_Implication(temp_net->in[k]->out[m], tp_id, 2);
														}
													}
												}
												//★それ以外：1時刻目前方含意
												else{
													for(m=0; m<temp_net->in[k]->n_out; m++){
														if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t1, tp_id) == 3){
															TDF_0_Forward_Implication(temp_net->in[k]->out[m], tp_id, 1);
														}
													}
												}
											}
										}
										
										//0固定
										Set_NBit_Zero(temp_net->in[k]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
										printf("0固定(2時刻目)：%s\n", temp_net->in[k]->name);
#endif
										//★入力信号線がSTEMの場合:2時刻目前方含意
										if(temp_net->in[k]->n_out >= 2){
											for(m=0; m<temp_net->in[k]->n_out; m++){
												if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t2, tp_id) == 3){
													TDF_0_Forward_Implication(temp_net->in[k]->out[m], tp_id, 2);
												}
											}
										}
									}
									
									//------------------------------------------
									//ケアビット1固定
									//------------------------------------------
									else if(nval == 1){
										
										//対象信号線がPIの場合ケアビット保持数情報を更新
										if(temp_net->in[k]->type == IN){
											
											//見逃し故障X抽出で増加したPIのケアビット数
											n_missed_cbit++;

											//テストパターンに対するケアビット情報
											tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
											tp_bit[tp_id].n_1bit++;		//対象信号線(PI)のテストパターン[tp_id]の1数をインクリメント

											//PIに対するケアビット情報
											pi_bit[temp_net->in[k]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
											pi_bit[temp_net->in[k]->pi_id].n_1bit++;	//対象信号線(PI)の1数をインクリメント
											
											//1時刻目もケアビット固定
											if(Get_NBit(temp_net->in[k]->xid_nval_t1, tp_id) == 3){
												//1固定
												Set_NBit_One(temp_net->in[k]->xid_nval_t1, tp_id);
												
#ifdef PATH_DEBUG
												printf("PI 1固定(1時刻目)：%s\n", temp_net->in[k]->name);
#endif
												//★PPOの場合：2時刻目前方含意
												if(temp_net->in[k]->ppo_flag == YES){
													for(m=0; m<temp_net->in[k]->n_out; m++){
														if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t2, tp_id) == 3){
															TDF_1_Forward_Implication(temp_net->in[k]->out[m], tp_id, 2);
														}
													}
												}
												//★それ以外：1時刻目前方含意
												else{
													for(m=0; m<temp_net->in[k]->n_out; m++){
														if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t1, tp_id) == 3){
															TDF_1_Forward_Implication(temp_net->in[k]->out[m], tp_id, 1);
														}
													}
												}
											}
										}
										
										//1固定
										Set_NBit_One(temp_net->in[k]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
										printf("1固定(2時刻目)：%s\n", temp_net->in[k]->name);
#endif
										//★入力信号線がSTEMの場合:2時刻目前方含意
										if(temp_net->in[k]->n_out >= 2){
											for(m=0; m<temp_net->in[k]->n_out; m++){
												if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t2, tp_id) == 3){
													TDF_1_Forward_Implication(temp_net->in[k]->out[m], tp_id, 2);
												}
											}
										}
									}
								}
							}
						}

						
						//----------------------------------------------------------------------
						// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
						//----------------------------------------------------------------------
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
					}
#ifdef PATH_DEBUG
					printf("\n");
#endif				
					//======================================================================================
					// キュー再初期化
					//======================================================================================
					if(fimpl_queue->front != fimpl_queue->rear){
						printf("fimpl_queueエラー：キュー内に残りあり\n");
					}
					reset_queue(fimpl_queue);

					//======================================================================================
					// ケアビット固定フラグ(xid_flag)を更新
					//======================================================================================
					xid_flag+=2;

					//======================================================================================
					// ◆1時刻目拡張含意操作◆PPOから到達可能な全PI・PPI(1時刻目)までを全てケアビット固定
					//======================================================================================
					//イベントレベル初期化
					event_lev = 0;

					//---------------------------------------------
					//キュー内の1時刻目PPOをスタックに入れる
					//---------------------------------------------
					while( (temp_net = (S_NLIST*)dequeue(nl_queue) ) != NULL){
						
						//1時刻目PPOをケアビット固定&プッシュ
						TDF_time1_ppo_push(temp_net, xid_flag, tp_id);

						//イベントレベルの決定]
						if(event_lev < temp_net->level){
							event_lev = temp_net->level;
						}
						
#ifdef PATH_DEBUG
						printf("1時刻目プッシュ:%s\n", temp_net->name);
#endif	

					}
					
					//---------------------------------------------
					//★2014/05/19更新：1時刻目故障信号線をプッシュ
					//---------------------------------------------
					TDF_time1_ppo_push(t_fault[i]->net, xid_flag, tp_id);

					//イベントレベルの決定
					if(event_lev < t_fault[i]->net->level){
						event_lev = t_fault[i]->net->level;
					}
#ifdef PATH_DEBUG
					printf("1時刻目プッシュ:%s\n", t_fault[i]->net->name);
#endif	

					//1時刻目拡張含意操作
					while(1){
						
						//----------------------------------------------------------------------
						// ★2014/05/14更新：s13207での落ちるバグ回避
						//----------------------------------------------------------------------
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

						//----------------------------------------------------------------------
						// レベライズスタックから信号線を取り出す
						//----------------------------------------------------------------------
						temp_net = slev_temp[event_lev].net[slev_temp[event_lev].n_net-1];	//ケツから取り出す
						slev_temp[event_lev].n_net--;										//信号線保持数デクリメント

#ifdef PATH_DEBUG
						printf("故障伝搬信号線:%s (ed_flag=%d  xid_flag=%d)\n", temp_net->name, temp_net->flag, temp_net->xid_flag);
#endif
						//----------------------------------------------------------------------
						// 【拡張含意操作】 全入力信号線をケアビット固定
						//----------------------------------------------------------------------
						for(k=0; k<temp_net->n_in; k++){

							//xid_flag未設置の信号線のみ処理
							if(temp_net->in[k]->xid_flag != xid_flag){
										
								//------------------------------------------
								//レベルスタックにプッシュ
								//------------------------------------------
								//xid_flag設置
								temp_net->in[k]->xid_flag = xid_flag;

								if(temp_net->in[k]->type != DFF){
									//slev_tempに対象信号線をプッシュ
									slev_temp[temp_net->in[k]->level].net[slev_temp[temp_net->in[k]->level].n_net] = temp_net->in[k];
		
									//slev_tempの保持信号線数をインクリメント
									slev_temp[temp_net->in[k]->level].n_net++;

									//★2014/5/19更新：PPI&&PPOの場合event_levのループ回らないバグ回避
									if(event_lev==0 && slev_temp[event_lev].n_net==0){
										if(event_lev < temp_net->in[k]->level){
											event_lev = temp_net->in[k]->level;
										}
									}

								}

								//------------------------------------------
								//対象信号線がXの場合
								//------------------------------------------
								if(Get_NBit(temp_net->in[k]->xid_nval_t1, tp_id) == 3){

									//対象信号線の正常値を取得
									nval = Get_NBit_Xbuf(temp_net->in[k]->nval_t1, tp_id);
								
									//------------------------------------------
									//ケアビット0固定
									//------------------------------------------
									if(nval == 0){

										//対象信号線がPIの場合ケアビット保持数情報を更新
										if(temp_net->in[k]->type == IN){

											//見逃し故障X抽出で増加したPIのケアビット数
											n_missed_cbit++;

											//テストパターンに対するケアビット情報
											tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
											tp_bit[tp_id].n_0bit++;		//対象信号線(PI)のテストパターン[tp_id]の0数をインクリメント

											//PIに対するケアビット情報
											pi_bit[temp_net->in[k]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
											pi_bit[temp_net->in[k]->pi_id].n_0bit++;	//対象信号線(PI)の0数をインクリメント

											//2時刻目もケアビット固定
											if(Get_NBit(temp_net->in[k]->xid_nval_t2, tp_id) == 3){
												//0固定
												Set_NBit_Zero(temp_net->in[k]->xid_nval_t2, tp_id);
												
#ifdef PATH_DEBUG
												printf("PI 0固定(2時刻目)：%s\n", temp_net->in[k]->name);
#endif											
												//前方含意
												//★PPO以外場合：2時刻目前方含意
												if(temp_net->in[k]->ppo_flag == NO){
													for(m=0; m<temp_net->in[k]->n_out; m++){
														if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t2, tp_id) == 3){
															TDF_0_Forward_Implication(temp_net->in[k]->out[m], tp_id, 2);
														}
													}
												}
											}
										}
										
										//0固定
										Set_NBit_Zero(temp_net->in[k]->xid_nval_t1, tp_id);										
#ifdef PATH_DEBUG
										printf("0固定(1時刻目)：%s\n", temp_net->in[k]->name);
#endif
										//★入力信号線がSTEMの場合:2時刻目前方含意
										if(temp_net->in[k]->n_out >= 2){
											for(m=0; m<temp_net->in[k]->n_out; m++){
												if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t1, tp_id) == 3){
													TDF_0_Forward_Implication(temp_net->in[k]->out[m], tp_id, 1);
												}
											}
										}
									}
									
									//------------------------------------------
									//ケアビット1固定
									//------------------------------------------
									else if(nval == 1){
										
										//対象信号線がPIの場合ケアビット保持数情報を更新
										if(temp_net->in[k]->type == IN){
											
											//見逃し故障X抽出で増加したPIのケアビット数
											n_missed_cbit++;

											//テストパターンに対するケアビット情報
											tp_bit[tp_id].n_cbit++;		//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
											tp_bit[tp_id].n_1bit++;		//対象信号線(PI)のテストパターン[tp_id]の1数をインクリメント

											//PIに対するケアビット情報
											pi_bit[temp_net->in[k]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
											pi_bit[temp_net->in[k]->pi_id].n_1bit++;	//対象信号線(PI)の1数をインクリメント
											
											//2時刻目もケアビット固定
											if(Get_NBit(temp_net->in[k]->xid_nval_t2, tp_id) == 3){
												//1固定
												Set_NBit_One(temp_net->in[k]->xid_nval_t2, tp_id);
												
#ifdef PATH_DEBUG
												printf("PI 1固定(2時刻目)：%s\n", temp_net->in[k]->name);
#endif
												//前方含意
												//★PPO以外場合：2時刻目前方含意
												if(temp_net->in[k]->ppo_flag == NO){
													for(m=0; m<temp_net->in[k]->n_out; m++){
														if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t2, tp_id) == 3){
															TDF_1_Forward_Implication(temp_net->in[k]->out[m], tp_id, 2);
														}
													}
												}
											}
										}
										
										//1固定
										Set_NBit_One(temp_net->in[k]->xid_nval_t1, tp_id);										
#ifdef PATH_DEBUG
										printf("1固定(1時刻目)：%s\n", temp_net->in[k]->name);
#endif
										//★入力信号線がSTEMの場合:2時刻目前方含意
										if(temp_net->in[k]->n_out >= 2){
											for(m=0; m<temp_net->in[k]->n_out; m++){
												if(Get_NBit(temp_net->in[k]->out[m]->xid_nval_t1, tp_id) == 3){
													TDF_1_Forward_Implication(temp_net->in[k]->out[m], tp_id, 1);
												}
											}
										}
									}
								}
							}
						}

						
						//----------------------------------------------------------------------
						// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
						//----------------------------------------------------------------------
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
					// キュー再初期化
					//======================================================================================
					if(nl_queue->front != nl_queue->rear){
						printf("nl_queueエラー：キュー内に残りあり\n");
					}
					if(fimpl_queue->front != fimpl_queue->rear){
						printf("fimpl_queueエラー：キュー内に残りあり\n");
					}
					reset_queue(nl_queue);
					reset_queue(fimpl_queue);

					//======================================================================================
					// ケアビット固定フラグ(xid_flag)を更新
					//======================================================================================
					xid_flag+=2;


					//======================================================================================
					// 次故障のX抽出に移行するためbreak (変数iを更新させたい)
					//======================================================================================
					break;

				}//if(i番目故障のX抽出)

			}//if(j番目PPOがそもそも故障検出してるか？)

		}//for(PPO探索ループ[変数j])

	}//for(最大32故障ループ[変数i])
	

	//======================================================================================
	//故障伝搬PPOフラグの初期化(X抽出用のSPPFPの結果情報)
	//======================================================================================
	for(i=0; i<n_ppo; i++){
		if(sort_propa_po[i]->n_det > 0){
			sort_propa_po[i]->n_det = 0;					//対象PPOでの故障検出数初期化
			All_INT_Zero(sort_propa_po[i]->det_flag);		//対象PPOでの故障検出フラグ初期化(どの故障を検出したか)
		}
	}

	
	//======================================================================================
	//xid_flag(ケアビット固定フラグ)の返却
	//======================================================================================
	return xid_flag;

}
//------------------------------------------------------------------------
//File name : SAF_3v_xfilling
//Date : 2012/5/09
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
#include	"../Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void		dtob					(unsigned int, int);
void		SAF_XID_fpath_push	(NLIST*, int, int);

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
//  ed_flag		: ケアビットに固定したフラグ(故障伝搬経路or正当化経路)
//
	#define PATH_DEBUG	//伝搬経路デバッグ
	//#define ECX_DEBUG		//正当化デバッグ
	//#define FIMPL_DEBUG		//前方含意デバッグ

//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------
//static  unsigned int     all_zero = 0;				//32bit全て『0』//未使用のため削除
//static  unsigned int     all_one  = 0xFFFFFFFF;		//32bit全て『1』//未使用のため削除

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_3v_xfilling
//  機  能 : 【限定含意・限定正当化】3値SPPFP結果からの必要なケアビット決定(要はX抽出部)
//  戻り値 : ed_flag(イベントドリブン用フラグ)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ), xid_flag(X抽出用フラグ)
//----------------------------------------------
unsigned int	SAF_3v_xfilling(int tp_id, XID_LIST **t_fault, int n_fault, unsigned int ed_flag, unsigned int xid_flag){

	int				i,j,k;
	int				exc_id;						//故障励起する信号線のID
	int				event_lev;					//故障伝搬経路を決めるレベル
	int				level_flag=0;				//event_lev更新時にbreakしないようにする
	unsigned int	buff_nval;					//nl[i].nvalの値を一時保持{0(0), 1(1), 3(X)}
	NLIST			*temp_net=NULL;				//スタックから取り出した信号線を一時的に格納
	
	
	///DEBUG////////////////////////////////////////////////
#ifdef PATH_DEBUG
	printf("\n//==========================================\n");
	printf("// 3値 X抽出関数\n");
	printf("//==========================================\n");
	for(i=0; i<n_net; i++)printf("%s ed_flag:%d xid_flag:%d\n",nl[i].name, nl[i].flag, nl[i].xid_flag);
	printf("\n\n");

	printf("X抽出対象故障リスト[%d個]\n", n_fault);
	for(i=0; i<n_fault; i++){
		printf("%d番目 対象故障：%s  SAF%d\n",i, t_fault[i]->net->name, t_fault[i]->fault_type);
	}
	printf("\n\n");
#endif
	////////////////////////////////////////////////////////

	//======================================================================================
	// SPPFPドントケア抽出 (結局は1故障ずつX抽出してる件. 何か良い案募集中)
	//======================================================================================
	for(i=0; i<n_fault; i++){
		for(j=0; j<n_po; j++){
			if(sort_propa_po[j]->n_det > 0){
				if(Get_NBit_INT(sort_propa_po[j]->det_flag, i) == 1){	//指定ビット目に『1』が立っている⇒経路追跡OK	
					
					//======================================================================================
					// 【伝搬PO決定】故障伝搬POを決定
					//======================================================================================
#ifdef PATH_DEBUG
					printf("【故障伝搬経路決定】\n");
					printf("★対象故障：%s  SAF%d ed_flag:%d\n",t_fault[i]->net->name, t_fault[i]->fault_type, ed_flag+1);
#endif
					//イベントドリブンフラグの更新
					ed_flag ++;
										
					//【ケアビット固定】xid_nvalがドントケアの場合はnaval値をxid_nvalに代入
					if(Get_NBit(po[sort_propa_po[j]->po_id]->xid_nval, tp_id) == 3){

						//nval値を得る
						buff_nval = Get_NBit(po[sort_propa_po[j]->po_id]->xid_nval, tp_id);

						//xid_nvalに同値(0 or 1)代入
						if(buff_nval == 0){
							//xid_nvalにケアビット0代入
							Set_NBit_Zero(po[sort_propa_po[j]->po_id]->xid_nval, tp_id);
						}
						else if(buff_nval == 1){
							//xid_nvalにケアビット代入
							Set_NBit_One(po[sort_propa_po[j]->po_id]->xid_nval, tp_id);
						}
						else{
							printf("//=================================\n");
							printf("//ERROR : 伝搬PO決定\n");
							printf("//=================================\n");
							printf("buff_nvalに変な値が入ってる！ (buff_nval = %u)\n", buff_nval);
						}
					}

					//POをプッシュ
					SAF_XID_fpath_push(po[sort_propa_po[j]->po_id], ed_flag, tp_id);

					//イベントレベルの決定
					event_lev = po[sort_propa_po[j]->po_id]->level;	//プッシュしたPOのレベル


					//======================================================================================
					// 【伝搬経路決定】故障個所までの伝搬経路を決定(POから故障個所まで遡る)
					//======================================================================================
					while(1){

						//-------------------------------------------
						// レベライズスタックから信号線を取り出す
						//-------------------------------------------
						temp_net = lev_temp[event_lev].net[lev_temp[event_lev].n_net-1];	//ケツから取り出す
						lev_temp[event_lev].n_net--;										//信号線保持数デクリメント

#ifdef PATH_DEBUG
						printf("故障伝搬信号線:%s\n", temp_net->name);
#endif
						//-------------------------------------------
						// ◆対象信号線がFOUT-STEMの場合前方含意キューにエンキュー
						//-------------------------------------------
						if(temp_net->n_out >= 2){					//出力信号線数が2本以上(= FOUT-STEM)
							enqueue(fimpl_queue, (void*)temp_net);	//エンキュー
						}

						//-------------------------------------------
						// 故障個所だったらbreak
						//-------------------------------------------
						if(temp_net->nbit_fault == i){
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
					}

					
					//======================================================================================
					// 故障個所の励起(入力信号線を限定含意・限定正当化キューにエンキュー)
					//======================================================================================
					switch(t_fault[i]->net->type){		//t_fault[i]->net == temp_netのはず (故障個所信号線)
						//--------------------------------------------------
						case IN:
							//何もしなくておｋ
						break;
						//--------------------------------------------------
						case BUF:
						case INV:
							//正当化経路フラグ
							temp_net->in[0]->flag = ed_flag;

							//【ケアビット固定】xid_nvalがドントケアの場合はnaval値をxid_nvalに代入
							if(Get_NBit(temp_net->in[0]->xid_nval, tp_id) == 3){

								//入力信号線をエンキュー
								enqueue(nl_queue, (void*)temp_net->in[0]);

								//nval値を得る
								buff_nval = Get_NBit(temp_net->in[0]->nval, tp_id);

								//xid_nvalに同値(0 or 1)代入
								if(buff_nval == 0){
									//xid_nvalにケアビット0代入
									Set_NBit_Zero(temp_net->in[0]->xid_nval, tp_id);
								}
								else if(buff_nval == 1){
									//xid_nvalにケアビット代入
									Set_NBit_One(temp_net->in[0]->xid_nval, tp_id);
								}
								else{
									printf("//=================================\n");
									printf("//ERROR : 故障箇所の励起\n");
									printf("//=================================\n");
									printf("buff_nvalに変な値が入ってる！ (buff_nval = %u)\n", buff_nval);
								}
							}
						//--------------------------------------------------
						case FOUT:
							//正当化経路フラグ
							temp_net->in[0]->flag = ed_flag;

							//【ケアビット固定】xid_nvalがドントケアの場合はnaval値をxid_nvalに代入
							if(Get_NBit(temp_net->in[0]->xid_nval, tp_id) == 3){

								//入力信号線をエンキュー
								enqueue(nl_queue, (void*)temp_net->in[0]);

								//nval値を得る
								buff_nval = Get_NBit(temp_net->in[0]->nval, tp_id);

								//xid_nvalに同値(0 or 1)代入
								if(buff_nval == 0){
									//xid_nvalにケアビット0代入
									Set_NBit_Zero(temp_net->in[0]->xid_nval, tp_id);
								}
								else if(buff_nval == 1){
									//xid_nvalにケアビット代入
									Set_NBit_One(temp_net->in[0]->xid_nval, tp_id);
								}
								else{
									printf("//=================================\n");
									printf("//ERROR : 故障箇所の励起\n");
									printf("//=================================\n");
									printf("buff_nvalに変な値が入ってる！ (buff_nval = %u)\n", buff_nval);
								}

								//◆FOUT-STEM信号線を前方含意キューにエンキュー
								enqueue(fimpl_queue, (void*)temp_net->in[0]);
							}

						break;
						//--------------------------------------------------
						case AND:
							//出力の正常値が1(入力が全て非制御値)
							if((temp_net->x_fault & MASKbit[i]) == 0){	//故障値だと=0
								for(k=0; k<temp_net->n_in; k++){

									temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ

									//入力信号線のxid_nvalがドントケア(X)か判定
									if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 3){																				
										enqueue(nl_queue, (void*)temp_net->in[k]);			//入力信号線をエンキュー
										Set_NBit_One(temp_net->in[k]->xid_nval, tp_id);		//【ケアビット固定】ケアビット(1)に固定
									}

								}
							}
							//出力の正常値が0(入力に制御値がある)
							else{
								//入力信号線の調査
								for(k=0; k<temp_net->n_in; k++){
									if(Get_NBit(temp_net->in[k]->nval, tp_id) == 0){	//ANDの入力正常値=0
										//他故障でケアビット(0)にした信号線があった場合
										if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 0){
											temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ
											exc_id = -1;
											break;
										}
										//違う場合
										else{
											exc_id = k;	//何か知恵を入れるべきか・・・
										}
									}
								}
								//【ケアビット固定】他故障でケアビットにした信号線が入力にない場合
								if(exc_id != -1){
									enqueue(nl_queue, (void*)temp_net->in[exc_id]);			//制御値入力を１つのみエンキュー
									temp_net->in[exc_id]->flag = ed_flag;					//正当化経路フラグ					
									Set_NBit_Zero(temp_net->in[exc_id]->xid_nval, tp_id);	//★対象信号線をケアビット固定
									break;
								}
							}
						break;
						//--------------------------------------------------
						case NAND:
							//出力の正常値が0(入力が全て非制御値)
							if((temp_net->x_fault & MASKbit[i]) != 0){	//故障値だと=1
								for(k=0; k<temp_net->n_in; k++){
									
									temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ

									//入力信号線のxid_nvalがドントケア(X)か判定
									if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 3){																				
										enqueue(nl_queue, (void*)temp_net->in[k]);			//入力信号線をエンキュー
										Set_NBit_One(temp_net->in[k]->xid_nval, tp_id);		//【ケアビット固定】ケアビット(1)に固定
									}
								}
							}
							//出力の正常値が1(入力に制御値がある)
							else{
								//入力信号線の調査
								for(k=0; k<temp_net->n_in; k++){
									if(Get_NBit(temp_net->in[k]->nval, tp_id) == 0){	//NANDの入力正常値=0
										//他故障でケアビット(0)にした信号線があった場合
										if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 0){
											temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ
											exc_id = -1;
											break;
										}
										//違う場合
										else{
											exc_id = k;	//何か知恵を入れるべきか・・・
										}
									}
								}
								//【ケアビット固定】他故障でケアビットにした信号線が入力にない場合
								if(exc_id != -1){
									enqueue(nl_queue, (void*)temp_net->in[exc_id]);			//制御値入力を１つのみエンキュー
									temp_net->in[exc_id]->flag = ed_flag;					//正当化経路フラグ					
									Set_NBit_Zero(temp_net->in[exc_id]->xid_nval, tp_id);	//★対象信号線をケアビット固定
									break;
								}
							}
						break;
						//--------------------------------------------------
						case OR:
							//出力の正常値が0(入力が全て非制御値)
							if((temp_net->x_fault & MASKbit[i]) != 0){	//故障値だと=1
								for(k=0; k<temp_net->n_in; k++){
									
									temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ

									//入力信号線のxid_nvalがドントケア(X)か判定
									if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 3){																				
										enqueue(nl_queue, (void*)temp_net->in[k]);			//入力信号線をエンキュー
										Set_NBit_Zero(temp_net->in[k]->xid_nval, tp_id);	//【ケアビット固定】ケアビット(0)に固定
									}
								}
							}
							//出力の正常値が1(入力に制御値がある)
							else{
								//入力信号線の調査
								for(k=0; k<temp_net->n_in; k++){
									if(Get_NBit(temp_net->in[k]->nval, tp_id) != 0){	//ORの入力正常値=1
										//他故障でケアビット(1)にした信号線があった場合
										if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 1){
											temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ
											exc_id = -1;
											break;
										}
										//違う場合
										else{
											exc_id = k;	//何か知恵を入れるべきか・・・
										}
									}
								}
								//【ケアビット固定】他故障でケアビットにした信号線が入力にない場合
								if(exc_id != -1){
									enqueue(nl_queue, (void*)temp_net->in[exc_id]);			//制御値入力を１つのみエンキュー
									temp_net->in[exc_id]->flag = ed_flag;					//正当化経路フラグ					
									Set_NBit_One(temp_net->in[exc_id]->xid_nval, tp_id);	//★対象信号線をケアビット固定
									break;
								}
							}
						break;
						//--------------------------------------------------
						case NOR:
							//出力の正常値が1(入力が全て非制御値)
							if((temp_net->x_fault & MASKbit[i]) == 0){	//故障値だと=0
								for(k=0; k<temp_net->n_in; k++){
									
									temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ

									//入力信号線のxid_nvalがドントケア(X)か判定
									if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 3){																				
										enqueue(nl_queue, (void*)temp_net->in[k]);			//入力信号線をエンキュー
										Set_NBit_Zero(temp_net->in[k]->xid_nval, tp_id);	//【ケアビット固定】ケアビット(0)に固定
									}
								}
							}
							//出力の正常値が0(入力に制御値がある)
							else{
								//入力信号線の調査
								for(k=0; k<temp_net->n_in; k++){
									if(Get_NBit(temp_net->in[k]->nval, tp_id) != 0){	//NORの入力正常値=1
										//他故障でケアビット(1)にした信号線があった場合
										if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 1){
											temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ
											exc_id = -1;
											break;
										}
										//違う場合
										else{
											exc_id = k;	//何か知恵を入れるべきか・・・
										}
									}
								}
								//【ケアビット固定】他故障でケアビットにした信号線が入力にない場合
								if(exc_id != -1){
									enqueue(nl_queue, (void*)temp_net->in[exc_id]);			//制御値入力を１つのみエンキュー
									temp_net->in[exc_id]->flag = ed_flag;					//正当化経路フラグ					
									Set_NBit_One(temp_net->in[exc_id]->xid_nval, tp_id);	//★対象信号線をケアビット固定
									break;
								}
							}
						break;
						//--------------------------------------------------
						case EXOR:
						case EXNOR:
							//特殊処理
							for(k=0; k<temp_net->n_in; k++){
								temp_net->in[k]->flag = ed_flag;					//正当化経路フラグ
								
								//【ケアビット固定】xid_nvalがドントケアの場合はnaval値をxid_nvalに代入
								if(Get_NBit(temp_net->in[k]->xid_nval, tp_id) == 3){
									
									//入力信号線をエンキュー
									enqueue(nl_queue, (void*)temp_net->in[k]);	

									//nval値を得る
									buff_nval = Get_NBit(temp_net->in[k]->nval, tp_id);

									//xid_nvalに同値(0 or 1)代入
									if(buff_nval == 0){
										//xid_nvalにケアビット0代入
										Set_NBit_Zero(temp_net->in[k]->xid_nval, tp_id);
									}
									else if(buff_nval == 1){
										//xid_nvalにケアビット代入
										Set_NBit_One(temp_net->in[k]->xid_nval, tp_id);
									}
									else{
										printf("//=================================\n");
										printf("//ERROR : 故障箇所の励起\n");
										printf("//=================================\n");
										printf("buff_nvalに変な値が入ってる！ (buff_nval = %u)\n", buff_nval);
									}
								}
							}
						break;
						//--------------------------------------------------
					}

					//======================================================================================
					// 【前方含意】故障伝搬経路からの前方含意
					//======================================================================================
					while((temp_net = (NLIST *)dequeue(fimpl_queue)) != (NLIST *)NULL){	//空になるまでデキュー

						//-------------------------------------------------
						// 【前方含意その1】故障伝搬経路決定後の前方含意
						//-------------------------------------------------
						for(k=0; k<temp_net->n_out; k++){
							//出力信号線(前方含意候補)のxid_nvalがドントケア(X)か判定
							if(Get_NBit(temp_net->out[k]->xid_nval, tp_id) == 3){
#ifdef FIMPL_DEBUG
								printf("前方含意: %s\n", temp_net->out[k]->name);
#endif
								//前方含意できるかも(関数ポインタ)
//								(*func_fwd_impl[temp_net->type])(temp_net->out[k], tp_id, xid_flag);
							}
						}
					}

					//======================================================================================
					// 【正当化】ケアビット信号線決定(限定含意・限定正当化)
					//======================================================================================
					while((temp_net = (NLIST *)dequeue(nl_queue)) != (NLIST *)NULL){	//空になるまでデキュー

						//-------------------------------------------
						// 【関数ポインタ使用】temp_netに対して限定含意・限定正当化
						//-------------------------------------------
						if(temp_net->n_in != 0){
#ifdef ECX_DEBUG
							printf("限定含意・限定正当化: %s\n", temp_net->name);
#endif
							//正当化(関数ポインタ)
							(*func_jus[temp_net->type])(temp_net, tp_id, xid_flag);
						}
					}

					//======================================================================================
					// キュー再初期化
					//======================================================================================
					reset_queue(nl_queue);
					reset_queue(fimpl_queue);
				}
			}
		}
	}

	

	//======================================================================================
	//故障伝搬POフラグの初期化(X抽出用のSPPFPの結果情報)
	//======================================================================================
	for(i=0; i<n_po; i++){
		if(sort_propa_po[i]->n_det > 0){
			sort_propa_po[i]->n_det = 0;					//対象POでの故障検出数初期化
			All_INT_Zero(sort_propa_po[i]->det_flag);	//対象POでの故障検出フラグ初期化(どの故障を検出したか)
		}
	}


	//======================================================================================
	//ed_flag(イベントドリブン用フラグ)の返却
	//======================================================================================
	return ed_flag;

}
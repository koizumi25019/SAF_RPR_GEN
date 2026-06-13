//------------------------------------------------------------------------
//File name : TDF_FOUT_CPT.c
//Date : 2013/2/1
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
void	TDF_2v_CPT0				(int, unsigned int, S_NLIST*);
void	TDF_2v_CPT1				(int, unsigned int, S_NLIST*);
void	TDF_3v_CPT0				(int, unsigned int, S_NLIST*);
void	TDF_3v_CPT1				(int, unsigned int, S_NLIST*);
void	dtob					(unsigned int, int);


//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define ED_DEBUG
	//#define DEBUG

//---------------------------------------------------------------------
// 静的変数
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_FOUT_2v_CPT
//  機  能 : PPOまで故障伝搬したTPが何番目か判定し，CPT関数実行
//  戻り値 : なし
//  引  数 : ffr_id(ステムのFFR番号), stem_net(ステム信号線), ui_num(unsigned int番目), det_tp_list(PPOまで故障伝搬したTPリスト【0:未伝搬 1:伝搬(=故障検出)】), t_ppo(故障伝搬したPO)
//----------------------------------------------
void	TDF_FOUT_2v_CPT	(int ffr_id, S_NLIST* stem_net, int ui_num, unsigned int* det_tp_list, S_NLIST* t_ppo){

	int				i;
	int				tp_id;		//テストパターンの何番目か
	unsigned int	x_buff;		//FOUT-STEM信号線の正常値
	unsigned int	fault_tp;	//故障伝搬したTP
#ifdef DEBUG
	int				j;
#endif
	
	//===========================================================
	// PPOの故障伝搬テストパターン判定(故障伝搬パターンに1が立つ)
	//===========================================================
	fault_tp = t_ppo->nval_t2->x_buf[ui_num] ^ t_ppo->x_fault;
	
#ifdef DEBUG
	printf("\nSTEM(%s) : PPO(%s)の故障伝搬テストパターン判定\n",stem_net->name, t_ppo->name);
	dtob(fault_tp, 6);
#endif

	//===========================================================
	// テストパターン数が32未満(確保したunsinged int が1個)
	//===========================================================
	if(n_tp_int == 1){
		for(i=0; i<n_tp; i++){
			//------------------------------------------------
			//det_tp_listのi番目=0(テストパターンi番目が未検出)
			//------------------------------------------------
			if((*det_tp_list & MASKbit[i]) == 0){
			
				//------------------------------------------------
				//fault_tpのiビット目に『1』が立っているか？(故障検出パターンか)
				//------------------------------------------------
				if( (fault_tp & MASKbit[i]) != 0){					
#ifdef DEBUG
					printf("\n%dパターン目：", i);
					for(j=0; j<n_spi; j++)printf("%d", Get_NBit_Xbuf(spi[j]->nval_t1, i));
					for(j=0; j<n_ppi; j++)printf("%d", Get_NBit_Xbuf(ppi[j]->nval_t1, i));
					printf("\n");
#endif
					//------------------------------------------------
					//FOUT-STEMの正常値計算
					//------------------------------------------------
					x_buff = stem_net->nval_t2->x_buf[ui_num] & MASKbit[i];	//ステム信号線のXbufのj番目以外全て0の状態にする
				
					//==========================================
					// 2時刻目正常値 == 1
					//==========================================
					if(x_buff != 0){	//i番目に1が立ってる
						

						*det_tp_list |= MASKbit[i];						//det_tp_listのi番目に『1』を立てる
						tp_id = i;										//テストパターン番号を求める

						//--------------------------------------
						// 1時刻目正常値 == 0
						//--------------------------------------
						if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i]) == 0){

							//【立上り遷移故障(0→1/0)を検出】検出情報更新
							sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
							sffr[ffr_id].FoutStem->det_str++;							//対象信号線の立上り遷移故障(0→1/0)の検出回数更新
							Set_NINT_One(fdic_str[tp_id], sffr[ffr_id].FoutStem->n);		//故障辞書にフラグ立て
#ifdef DEBUG
							printf("tp[%d] CPT: STR(0→1/0) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif
						}

						//正常値1のCPT開始
						TDF_2v_CPT1(ffr_id, tp_id, stem_net);
					}
								
					//==========================================
					// 2時刻目正常値 == 0
					//==========================================
					else{	//全部0になった
						
						*det_tp_list |= MASKbit[i];					//det_tp_listのi番目に『1』を立てる
						tp_id = i;									//テストパターン番号を求める

						//--------------------------------------
						// 1時刻目正常値 == 1
						//--------------------------------------
						if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i]) != 0){

							//【立下り遷移故障(1→0/1)を検出】検出情報更新
							sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
							sffr[ffr_id].FoutStem->det_stf++;							//対象信号線の立下り遷移故障(1→0/1)の検出回数更新
							Set_NINT_One(fdic_stf[tp_id], sffr[ffr_id].FoutStem->n);		//故障辞書にフラグ立て
#ifdef DEBUG
							printf("tp[%d] CPT: STF(1→0/1) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif																
						}
						
						//正常値0のCPT開始
						TDF_2v_CPT0(ffr_id, tp_id, stem_net);
					}
				}
			}
		}
	}

	//===========================================================
	// テストパターン数が32以上
	//===========================================================
	else{
		for(i=0; i<32; i++){

			//------------------------------------------------
			//det_tp_listのi番目=0(テストパターンi番目が未検出)
			//------------------------------------------------
			if((*det_tp_list & MASKbit[i]) == 0){
			
				//------------------------------------------------
				//fault_tpのiビット目に『1』が立っているか？(故障検出パターンか)
				//------------------------------------------------
				if( (fault_tp & MASKbit[i]) != 0){			

					//------------------------------------------------
					//FOUT-STEMの正常値計算
					//------------------------------------------------
					x_buff = stem_net->nval_t2->x_buf[ui_num] & MASKbit[i];	//ステム信号線のXbufのj番目以外全て0の状態にする
				

					//==========================================
					// 2時刻目正常値 == 1
					//==========================================
					if(x_buff != 0){	//i番目に1が立ってる
								
						*det_tp_list |= MASKbit[i];					//det_tp_listのi番目に『1』を立てる
						tp_id = (ui_num*32)+i;						//テストパターン番号を求める

						//--------------------------------------
						// 1時刻目正常値 == 0
						//--------------------------------------
						if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i]) == 0){

							//【立上り遷移故障(0→1/0)を検出】
							//一番最後のunsigned intの未使用bit対策
							if(tp_id < n_tp){

								//検出情報更新
								sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
								sffr[ffr_id].FoutStem->det_str++;							//対象信号線の立上り遷移故障(0→1/0)の検出回数更新
								Set_NINT_One(fdic_str[tp_id], sffr[ffr_id].FoutStem->n);		//故障辞書にフラグ立て								
#ifdef DEBUG
								printf("tp[%d] CPT: STR(0→1/0) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif
							}
						}
						
						//一番最後のunsigned intの未使用bit対策
						if(tp_id < n_tp){
							//正常値1のCPT開始
							TDF_2v_CPT1(ffr_id, tp_id, stem_net);
						}
					}
								
					//==========================================
					// 2時刻目正常値 == 0
					//==========================================
					else{	//全部0になった

						*det_tp_list |= MASKbit[i];						//det_tp_listのi番目に『1』を立てる
						tp_id = (ui_num*32)+i;							//テストパターン番号を求める
						
						//--------------------------------------
						// 1時刻目正常値 == 1
						//--------------------------------------
						if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i]) != 0){

							//【立下り遷移故障(1→0/1)を検出】
							//一番最後のunsigned intの未使用bit対策
							if(tp_id < n_tp){
								//検出情報更新
								sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
								sffr[ffr_id].FoutStem->det_stf++;							//対象信号線の立下り遷移故障(1→0/1)の検出回数更新
								Set_NINT_One(fdic_stf[tp_id], sffr[ffr_id].FoutStem->n);		//故障辞書にフラグ立て							
#ifdef DEBUG
								printf("tp[%d] CPT: STF(1→0/1) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif							
							}
						}

						//一番最後のunsigned intの未使用bit対策
						if(tp_id < n_tp){
							//正常値0のCPT開始
							TDF_2v_CPT0(ffr_id, tp_id, stem_net);
						}
					}
				}
			}
		}
	}
}


//----------------------------------------------
//  関数名 : TDF_FOUT_3v_CPT
//  機  能 : POまで故障伝搬したTPが何番目か判定し，CPT関数実行
//  戻り値 : なし
//  引  数 : ffr_id(ステムのFFR番号), stem_net(ステム信号線), ui_num(unsigned int番目), det_tp_list(PPOまで故障伝搬したTPリスト【0:未伝搬 1:伝搬(=故障検出)】), t_ppo(故障伝搬したPO)
//----------------------------------------------
void	TDF_FOUT_3v_CPT	(int ffr_id, S_NLIST* stem_net, int ui_num, unsigned int* det_tp_list, S_NLIST* t_ppo){

	int				i;
	int				tp_id;		//テストパターンの何番目か
	unsigned int	x_buff;		//正常値x_buff一時保存
	unsigned int	p_buff;		//正常値p_buff一時保存
	unsigned int	fault_tp;	//故障伝搬したTP
#ifdef DEBUG
	int				j;
#endif

	//===========================================================
	// PPOの故障伝搬テストパターン判定(故障伝搬パターンに1が立つ)
	//===========================================================
	fault_tp = (t_ppo->nval_t2->x_buf[ui_num] ^ t_ppo->x_fault) & (t_ppo->nval_t2->p_buf[ui_num] ^ t_ppo->p_fault);	//(X-buff同士のEXOR)AND(P-buff同士のEXOR)

#ifdef DEBUG
	printf("\nSTEM(%s) : PPO(%s)の故障伝搬テストパターン判定\n",stem_net->name, t_ppo->name);
	dtob(fault_tp, 32);
#endif

	//===========================================================
	// テストパターン数が32未満(確保したunsinged int が1個)
	//===========================================================
	if(n_tp_int == 1){
		for(i=0; i<n_tp; i++){
			//------------------------------------------------
			//det_tp_listのi番目=0(テストパターンi番目が未検出)
			//------------------------------------------------
			if((*det_tp_list & MASKbit[i]) == 0){	
								
				//------------------------------------------------
				//fault_tpのiビット目に『1』が立っているか？(故障検出パターンか)
				//------------------------------------------------
				if( (fault_tp & MASKbit[i]) != 0){					
#ifdef DEBUG
					printf("\n%dパターン目：", i);
					for(j=0; j<n_spi; j++)printf("%d", Get_NBit_Xbuf(spi[j]->nval_t1, i));
					for(j=0; j<n_ppi; j++)printf("%d", Get_NBit_Xbuf(ppi[j]->nval_t1, i));
					printf("\n");
#endif
					
					//------------------------------------------------
					//FOUT-STEMの正常値計算
					//------------------------------------------------
					x_buff = stem_net->nval_t2->x_buf[ui_num] & MASKbit[i];	//ステム信号線のXbufのj番目以外全て0の状態にする
					p_buff = stem_net->nval_t2->p_buf[ui_num] & MASKbit[i];	//ステム信号線のPbufのj番目以外全て0の状態にする
											
					//------------------------------------------------
					//ステム信号線の正常値がXbitじゃない場合
					//------------------------------------------------
					if((x_buff&p_buff)==0){

						//==========================================
						// 2時刻目正常値 == 1
						//==========================================
						if(x_buff != 0){	//i番目に1が立ってる
								
							*det_tp_list |= MASKbit[i];			//det_tp_listのi番目に『1』を立てる
							tp_id = i;							//テストパターン番号を求める

							//--------------------------------------
							// 1時刻目正常値 == 0
							//--------------------------------------
							if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i])==0 && (stem_net->nval_t1->p_buf[ui_num] & MASKbit[i])!=0){
								
								//【立上り遷移故障(0→1/0)を検出】検出情報更新
								sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
								sffr[ffr_id].FoutStem->det_str++;							//対象信号線の立上り遷移故障(0→1/0)の検出回数更新
								Set_NINT_One(fdic_str[tp_id], sffr[ffr_id].FoutStem->n);	//故障辞書にフラグ立て
#ifdef DEBUG
								printf("tp[%d] CPT: STR(0→1/0) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif
							}
								
							//正常値1のCPT開始
							TDF_3v_CPT1(ffr_id, tp_id, stem_net);
						}
								
						//==========================================
						// 2時刻目正常値 == 0
						//==========================================
						else{	//全部0になった

							*det_tp_list |= MASKbit[i];		//det_tp_listのi番目に『1』を立てる
							tp_id = i;							//テストパターン番号を求める
							
							//--------------------------------------
							// 1時刻目正常値 == 1
							//--------------------------------------
							if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i])!=0 && (stem_net->nval_t1->p_buf[ui_num] & MASKbit[i])==0){
								
								//【立下り遷移故障(1→0/1)を検出】検出情報更新
								sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
								sffr[ffr_id].FoutStem->det_stf++;							//対象信号線の立下り遷移故障(1→0/1)の検出回数更新
								Set_NINT_One(fdic_stf[tp_id], sffr[ffr_id].FoutStem->n);	//故障辞書にフラグ立て
#ifdef DEBUG
								printf("tp[%d] CPT: STF(1→0/1) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif																
							}
								
							//正常値0のCPT開始
							TDF_3v_CPT0(ffr_id, tp_id, stem_net);
						}
					}
				}
			}
		}
	}

	//===========================================================
	// テストパターン数が32以上
	//===========================================================
	else{
		for(i=0; i<32; i++){
			//------------------------------------------------
			//det_tp_listのi番目=0(テストパターンi番目が未検出)
			//------------------------------------------------
			if((*det_tp_list & MASKbit[i]) == 0){
								
				//------------------------------------------------
				//fault_tpのiビット目に『1』が立っているか？(故障検出パターンか)
				//------------------------------------------------
				if( (fault_tp & MASKbit[i]) != 0){
					
					//------------------------------------------------
					//FOUT-STEMの正常値計算
					//------------------------------------------------				
					x_buff = stem_net->nval_t2->x_buf[ui_num] & MASKbit[i];	//ステム信号線のXbufのj番目以外全て0の状態にする
					p_buff = stem_net->nval_t2->p_buf[ui_num] & MASKbit[i];	//ステム信号線のPbufのj番目以外全て0の状態にする
						
					//------------------------------------------------
					//ステム信号線の正常値がXbitじゃない場合
					//------------------------------------------------
					if((x_buff&p_buff)==0){

						//==========================================
						// 2時刻目正常値 == 1
						//==========================================
						if(x_buff != 0){	//i番目に1が立ってる
								
							*det_tp_list |= MASKbit[i];						//det_tp_listのi番目に『1』を立てる
							tp_id = (ui_num*32)+i;							//テストパターン番号を求める
							
							//--------------------------------------
							// 1時刻目正常値 == 0
							//--------------------------------------
							if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i])==0 && (stem_net->nval_t1->p_buf[ui_num] & MASKbit[i])!=0){
								
								//【立上り遷移故障(0→1/0)を検出】検出情報更新
								//一番最後のunsigned intの未使用bit対策
								if(tp_id < n_tp){
									//検出情報更新
									sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
									sffr[ffr_id].FoutStem->det_str++;							//対象信号線の立上り遷移故障(0→1/0)の検出回数更新
									Set_NINT_One(fdic_str[tp_id], sffr[ffr_id].FoutStem->n);	//故障辞書にフラグ立て							
#ifdef DEBUG
									printf("tp[%d] CPT: STR(0→1/0) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif
								}
							}
							
							//一番最後のunsigned intの未使用bit対策
							if(tp_id < n_tp){
								//正常値1のCPT開始
								TDF_3v_CPT1(ffr_id, tp_id, stem_net);
							}
						}
								
						//==========================================
						// 2時刻目正常値 == 0
						//==========================================
						else{	//全部0になった

							*det_tp_list |= MASKbit[i];					//det_tp_listのi番目に『1』を立てる
							tp_id = (ui_num*32)+i;							//テストパターン番号を求める

							//--------------------------------------
							// 1時刻目正常値 == 1
							//--------------------------------------
							if( (stem_net->nval_t1->x_buf[ui_num] & MASKbit[i])!=0 && (stem_net->nval_t1->p_buf[ui_num] & MASKbit[i])==0){
								//一番最後のunsigned intの未使用bit対策
								if(tp_id < n_tp){
									//【立下り遷移故障(1→0/1)を検出】検出情報更新
									sffr[ffr_id].n_detect++;									//FFR内の検出故障数更新
									sffr[ffr_id].FoutStem->det_stf++;							//対象信号線の立下り遷移故障(1→0/1)の検出回数更新
									Set_NINT_One(fdic_stf[tp_id], sffr[ffr_id].FoutStem->n);	//故障辞書にフラグ立て							
#ifdef DEBUG
									printf("tp[%d] CPT: STF(1→0/1) %s\n", tp_id, sffr[ffr_id].FoutStem->name);
#endif							
								}
							}
							
							//一番最後のunsigned intの未使用bit対策
							if(tp_id < n_tp){
								//正常値0のCPT開始
								TDF_3v_CPT0(ffr_id, tp_id, stem_net);
							}
						}
					}
				}
			}
		}
	}
	
}

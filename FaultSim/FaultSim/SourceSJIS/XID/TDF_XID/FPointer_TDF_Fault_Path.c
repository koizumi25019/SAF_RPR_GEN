//------------------------------------------------------------------------
//File name : FPointer_TDF_Fault_Path.c
//Date : 2013/10/13
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
void		dtob								(unsigned int, int);
void		TDF_XID_fpath_push					(S_NLIST*, int, int);
void		TDF_0_Backward_Implication			(S_NLIST*, int, int, unsigned int);
void		TDF_1_Backward_Implication			(S_NLIST*, int, int, unsigned int);
void		TDF_0_Forward_Implication			(S_NLIST*, int, int);
void		TDF_1_Forward_Implication			(S_NLIST*, int, int);

//伝搬経路決定
void		TDF_XID_fpath_single_gate			(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);
void		TDF_XID_fpath_and					(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);
void		TDF_XID_fpath_nand					(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);
void		TDF_XID_fpath_or					(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);
void		TDF_XID_fpath_nor					(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);
void		TDF_XID_fpath_exor					(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);

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
//  関数名 : FPointer_TDF_Fault_Path
//  機  能 : 【関数ポインタ作成】TDF用の故障伝搬経路決定の関数ポインタ作成
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	FPointer_TDF_Fault_Path(){
		
	//======================================================================================
	// 関数ポインタ作成
	//======================================================================================
	//故障伝搬経路
	sfunc_path[FOUT]	= TDF_XID_fpath_single_gate;
	sfunc_path[BUF]		= TDF_XID_fpath_single_gate;
	sfunc_path[INV]		= TDF_XID_fpath_single_gate;
	sfunc_path[AND]		= TDF_XID_fpath_and;
	sfunc_path[NAND]	= TDF_XID_fpath_nand;
	sfunc_path[OR]		= TDF_XID_fpath_or;
	sfunc_path[NOR]		= TDF_XID_fpath_nor;
	sfunc_path[EXOR]	= TDF_XID_fpath_exor;
	sfunc_path[EXNOR]	= TDF_XID_fpath_exor;
}

//----------------------------------------------
//  関数名 : TDF_XID_fpath_push
//  機  能 : 故障伝搬経路決定用スタック(slev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), xid_flag(X抽出用フラグ値), tp_id(テストパターン番号)
//  memo   : ★対象信号線自信を積む★
//----------------------------------------------
void TDF_XID_fpath_push(S_NLIST *e_net, int xid_flag, int tp_id){
	
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

		//slev_tempに対象信号線をプッシュ
		slev_temp[e_net->level].net[slev_temp[e_net->level].n_net] = e_net;
		
		//slev_tempの保持信号線数をインクリメント
		slev_temp[e_net->level].n_net++;


		//-----------------------------------------------------------------------------
		// ■2値ケアビット固定
		//-----------------------------------------------------------------------------
		if(n_before_x == 0){
			//X抽出後値(xid_nval_t2)がドントケアの場合
			if(Get_NBit(e_net->xid_nval_t2, tp_id) == 3){

				//2値正常値(nval_t2)を取得
				buff_nval = Get_NBit_Xbuf(e_net->nval_t2, tp_id);

				//--------------------------------------------------------
				// 0固定
				//--------------------------------------------------------
				if(buff_nval == 0){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t1, tp_id) == 3){
							//--------------------
							//1時刻目0固定
							//--------------------
							Set_NBit_Zero(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】※PI %s : 0固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == YES){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(e_net->out[i], tp_id, 1);
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
					Set_NBit_Zero(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
					printf("【故障伝搬経路】%s : 0固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
				}
				//--------------------------------------------------------
				// 1固定
				//--------------------------------------------------------
				else if(buff_nval ==1){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t1, tp_id) == 3){
							//--------------------
							//1時刻目1固定
							//--------------------
							Set_NBit_One(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】※PI %s : 1固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == YES){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(e_net->out[i], tp_id, 1);
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
					Set_NBit_One(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
					printf("【故障伝搬経路】%s : 1固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
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
				printf("【故障伝搬経路】ケアビット固定済み(2時刻目): %s (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
			}
#endif
		}
		
		//-----------------------------------------------------------------------------
		// ■3値ケアビット固定
		//-----------------------------------------------------------------------------
		else{
			//X抽出後値(xid_nval_t2)がドントケアの場合
			if(Get_NBit(e_net->xid_nval_t2, tp_id) == 3){
			
				//3値正常値(nval_t2)を取得
				buff_nval = Get_NBit(e_net->nval_t2, tp_id);
				
				//--------------------------------------------------------
				// 0固定
				//--------------------------------------------------------
				if(buff_nval == 0){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t1, tp_id) == 3){
							//--------------------
							//1時刻目0固定
							//--------------------
							Set_NBit_Zero(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】※PI %s : 0固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == YES){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(e_net->out[i], tp_id, 1);
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
					Set_NBit_Zero(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
					printf("【故障伝搬経路】%s : 0固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
				}
				//--------------------------------------------------------
				// 1固定
				//--------------------------------------------------------
				else if(buff_nval ==1){

					//対象信号線がPIの場合ケアビット保持数情報を更新
					if(e_net->type == IN){
						
						//ケアビット固定処理
						if(Get_NBit(e_net->xid_nval_t1, tp_id) == 3){
							//--------------------
							//1時刻目1固定
							//--------------------
							Set_NBit_One(e_net->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】※PI %s : 1固定(1時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
#endif
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(e_net->ppo_flag == YES){
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(e_net->out[i], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(i=0; i<e_net->n_out; i++){
									if(Get_NBit(e_net->out[i]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(e_net->out[i], tp_id, 1);
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
					Set_NBit_One(e_net->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
					printf("【故障伝搬経路】%s : 1固定(2時刻目) (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
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
				printf("【故障伝搬経路】ケアビット固定済み(2時刻目): %s (ed_flag:%d  xid_flag:%d)\n", e_net->name, e_net->flag, e_net->xid_flag);
			}
#endif
		}
	}
	//=======================================================================
	//対象信号線に伝搬経路(ケアビット固定)フラグが立ってた！！！
	//=======================================================================
#ifdef PATH_DEBUG
	else{
		printf("【故障伝搬経路】xid_flag設置済み：%s(再収斂？)\n", e_net->name);
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


//******************************************************************************************************
// 関数ポインタさんの中身
//******************************************************************************************************
//★【2値・3値共通】故障伝搬経路★
//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_fpath_single_gate
//  機  能 : 【FOUT・BUF・INV】故障伝搬経路決定
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), fault_id(32個中何番目の故障なのか), ed_flag(FSIMイベントフラグ値), xid_flag(X抽出(ケアビット固定)フラグ値)
//------------------------------------------------------------------------------------
void	TDF_XID_fpath_single_gate(S_NLIST *t_net, int tp_id, int fault_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){
	
	//================================================
	//FOUT・BUF・INVの入力は絶対故障伝搬経路
	//================================================
	TDF_XID_fpath_push(t_net->in[0], xid_flag, tp_id);
}


//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_fpath_and
//  機  能 : 【AND】故障伝搬経路決定
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), fault_id(32個中何番目の故障なのか), ed_flag(FSIMイベントフラグ値), xid_flag(X抽出(ケアビット固定)フラグ値)
//------------------------------------------------------------------------------------
void	TDF_XID_fpath_and(S_NLIST *t_net, int tp_id, int fault_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){

	int		i,j;
	int		in_id = -1;					//入力信号線の最小レベルの入力ID
	int		in_lev = max_level+1;		//入力信号線のレベル⇒最小レベルに更新
	int		cbit_id = -1;				//他故障でのX抽出でケアビットに固定された信号線のID
		
	//===============================================================================
	// ゲート出力正常値が制御値(0)で決定 (規則① or 規則③)
	//===============================================================================
	//         ＿＿
	//   0/1 --|   ＼
	//   0/1 --| AND│--- 0/1
	//   1/1 --|   ／
	//         ￣￣
	if(Get_NBit_Xbuf(t_net->nval_t2, tp_id) == 0){
		
		//---------------------------------------
		//正常値が制御値(0)の信号線を全て遡る
		//---------------------------------------
		//2値
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(0)
				//----------------------------------
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 0){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(1)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
		//3値
		else{
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(0)
				//----------------------------------
				if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 0){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(1)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//--------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}

						//----------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
				//※入力にXがあった場合は故障伝搬しないので大丈夫
			}
		}
		
	}
	
	//===============================================================================
	// ゲート出力正常値が非制御値(1)で決定 (規則① or 規則②)
	//===============================================================================
	else{
		//         ＿＿
		//   1/0 --|   ＼
		//   1/0 --| AND│--- 1/0
		//   1/1 --|   ／
		//         ￣￣
		//---------------------------------------
		//故障伝搬入力(1/0)を探索⇒故障値入力0を探索
		//---------------------------------------
		for(i=0; i<t_net->n_in; i++){
			//故障SIMフラグが立ってる信号線
			if(t_net->in[i]->flag == ed_flag){
				if((t_net->in[i]->x_fault & MASKbit[fault_id]) == 0){	//AND入力故障値==0

					//-----------------------------------------------------------
					//故障伝搬 && 入力信号線のxid_nval_t2にドントケア(X)が存在
					//-----------------------------------------------------------
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){	//ドントケア(X)発見
						//入力レベル更新(規則②で使用)
						if(in_lev > t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//未固定(X)信号線の最小レベルを保存
							in_id = i;						//未固定(X)信号線の入力idを保存
						}
					}
					//-----------------------------------------------------------
					//故障伝搬 && 入力に他故障でケアビット(1)固定された信号線が存在
					//-----------------------------------------------------------
					else{
						cbit_id = i;					//固定済み信号線の入力idを保存
						break;
					}
				}
			}
		}

		//---------------------------------------
		//他故障でケアビット(1)固定された信号線が存在
		//---------------------------------------
		if(cbit_id != -1){
			for(i=0; i<t_net->n_in; i++){

				//固定済み信号線を故障伝搬経路に決定
				if(i == cbit_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}

						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}

		//---------------------------------------
		//ゲート段数が最小の信号線を故障伝搬経路に決定
		//---------------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){
				
				//固定済み信号線を故障伝搬経路に決定
				if(i == in_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//----------------------------
						//1固定
						//----------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif											
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//----------------------------
						//1からの後方含意
						//----------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_fpath_nand
//  機  能 : 【NAND】故障伝搬経路決定
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), fault_id(32個中何番目の故障なのか), ed_flag(FSIMイベントフラグ値), xid_flag(X抽出(ケアビット固定)フラグ値)
//------------------------------------------------------------------------------------
void	TDF_XID_fpath_nand(S_NLIST *t_net, int tp_id, int fault_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){

	int		i,j;
	int		in_id = -1;					//入力信号線の最小レベルの入力ID
	int		in_lev = max_level+1;		//入力信号線のレベル⇒最小レベルに更新
	int		cbit_id = -1;				//他故障でのX抽出でケアビットに固定された信号線のID
	
	//===============================================================================
	// ゲート出力正常値が制御値(0)で決定 (規則① or 規則③)
	//===============================================================================
	//         ＿＿
	//   0/1 --|   ＼
	//   0/1 --|NAND│○-- 1/0
	//   1/1 --|   ／
	//         ￣￣     
	if(Get_NBit_Xbuf(t_net->nval_t2, tp_id) == 1){
		
		//---------------------------------------
		//正常値が制御値(0)の信号線を全て遡る
		//---------------------------------------
		//2値
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(0)
				//----------------------------------
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 0){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(1)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
		//3値
		else{
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(0)
				//----------------------------------
				if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 0){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(1)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){						
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
				//※入力にXがあった場合は故障伝搬しないので大丈夫
			}
		}
		
	}
	
	//===============================================================================
	// ゲート出力正常値が非制御値(1)で決定 (規則① or 規則②)
	//===============================================================================
	else{
		//         ＿＿
		//   1/0 --|   ＼
		//   1/0 --|NAND│○-- 0/1
		//   1/1 --|   ／
		//         ￣￣     
		//---------------------------------------
		//故障伝搬入力(1/0)を探索⇒故障値入力0を探索
		//---------------------------------------
		for(i=0; i<t_net->n_in; i++){
			//故障SIMフラグが立ってる信号線
			if(t_net->in[i]->flag == ed_flag){
				if((t_net->in[i]->x_fault & MASKbit[fault_id]) == 0){	//NAND入力故障値==0

					//-----------------------------------------------------------
					//故障伝搬 && 入力信号線のxid_nval_t2にドントケア(X)が存在
					//-----------------------------------------------------------
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){	//ドントケア(X)発見
						//入力レベル更新(規則②で使用)
						if(in_lev > t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//未固定(X)信号線の最小レベルを保存
							in_id = i;						//未固定(X)信号線の入力idを保存
						}
					}
					//-----------------------------------------------------------
					//故障伝搬 && 入力に他故障でケアビット(1)固定された信号線が存在
					//-----------------------------------------------------------
					else{
						cbit_id = i;					//固定済み信号線の入力idを保存
						break;
					}
				}
			}
		}

		//---------------------------------------
		//他故障でケアビット(1)固定された信号線が存在
		//---------------------------------------
		if(cbit_id != -1){
			for(i=0; i<t_net->n_in; i++){

				//固定済み信号線を故障伝搬経路に決定
				if(i == cbit_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}

		//---------------------------------------
		//ゲート段数が最小の信号線を故障伝搬経路に決定
		//---------------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){
				
				//固定済み信号線を故障伝搬経路に決定
				if(i == in_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_fpath_or
//  機  能 : 【OR】故障伝搬経路決定
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), fault_id(32個中何番目の故障なのか), ed_flag(FSIMイベントフラグ値), xid_flag(X抽出(ケアビット固定)フラグ値)
//------------------------------------------------------------------------------------
void	TDF_XID_fpath_or(S_NLIST *t_net, int tp_id, int fault_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){

	int		i,j;
	int		in_id = -1;					//入力信号線の最小レベルの入力ID
	int		in_lev = max_level+1;		//入力信号線のレベル⇒最小レベルに更新
	int		cbit_id = -1;				//他故障でのX抽出でケアビットに固定された信号線のID
	
	//===============================================================================
	// ゲート出力正常値が制御値(1)で決定 (規則① or 規則③)
	//===============================================================================
	//         ＿＿
	//   1/0 --＼   ＼
	//   1/0 ----) OR >--- 1/0
	//   0/0 --／   ／
	//         ￣￣  
	if(Get_NBit_Xbuf(t_net->nval_t2, tp_id) == 1){
		
		//---------------------------------------
		//正常値が制御値(1)の信号線を全て遡る
		//---------------------------------------
		//2値
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(1)
				//----------------------------------
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 1){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(0)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
		//3値
		else{
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(1)
				//----------------------------------
				if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 1){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(0)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif					
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
				//※入力にXがあった場合は故障伝搬しないので大丈夫
			}
		}
		
	}
	
	//===============================================================================
	// ゲート出力正常値が非制御値(0)で決定 (規則① or 規則②)
	//===============================================================================
	else{
		//         ＿＿
		//   0/1 --＼   ＼
		//   0/1 ----) OR >--- 0/1
		//   0/0 --／   ／
		//         ￣￣    
		//---------------------------------------
		//故障伝搬入力(0/1)を探索⇒故障値入力1を探索
		//---------------------------------------
		for(i=0; i<t_net->n_in; i++){
			//故障SIMフラグが立ってる信号線
			if(t_net->in[i]->flag == ed_flag){
				if((t_net->in[i]->x_fault & MASKbit[fault_id]) != 0){	//OR入力故障値==1

					//-----------------------------------------------------------
					//故障伝搬 && 入力信号線のxid_nval_t2にドントケア(X)が存在
					//-----------------------------------------------------------
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){	//ドントケア(X)発見
						//入力レベル更新(規則②で使用)
						if(in_lev > t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//未固定(X)信号線の最小レベルを保存
							in_id = i;						//未固定(X)信号線の入力idを保存
						}
					}
					//-----------------------------------------------------------
					//故障伝搬 && 入力に他故障でケアビット(0)固定された信号線が存在
					//-----------------------------------------------------------
					else{
						cbit_id = i;					//固定済み信号線の入力idを保存
						break;
					}
				}
			}
		}

		//---------------------------------------
		//他故障でケアビット(0)固定された信号線が存在
		//---------------------------------------
		if(cbit_id != -1){
			for(i=0; i<t_net->n_in; i++){

				//固定済み信号線を故障伝搬経路に決定
				if(i == cbit_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}

		//---------------------------------------
		//ゲート段数が最小の信号線を故障伝搬経路に決定
		//---------------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){
				
				//固定済み信号線を故障伝搬経路に決定
				if(i == in_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_fpath_nor
//  機  能 : 【NOR】故障伝搬経路決定
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), fault_id(32個中何番目の故障なのか), ed_flag(FSIMイベントフラグ値), xid_flag(X抽出(ケアビット固定)フラグ値)
//------------------------------------------------------------------------------------
void	TDF_XID_fpath_nor(S_NLIST *t_net, int tp_id, int fault_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){

	int		i,j;
	int		in_id = -1;					//入力信号線の最小レベルの入力ID
	int		in_lev = max_level+1;		//入力信号線のレベル⇒最小レベルに更新
	int		cbit_id = -1;				//他故障でのX抽出でケアビットに固定された信号線のID
	
	//===============================================================================
	// ゲート出力正常値が制御値(1)で決定 (規則① or 規則③)
	//===============================================================================
	//         ＿＿
	//   1/0 --＼   ＼
	//   1/0 ----)NOR >○-- 0/1
	//   0/0 --／   ／
	//         ￣￣
	if(Get_NBit_Xbuf(t_net->nval_t2, tp_id) == 0){
		
		//---------------------------------------
		//正常値が制御値(1)の信号線を全て遡る
		//---------------------------------------
		//2値
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(1)
				//----------------------------------
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 1){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(0)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
		//3値
		else{
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------
				//入力正常値：制御値(1)
				//----------------------------------
				if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 1){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
				}
				//----------------------------------
				//入力正常値：非制御値(0)
				//----------------------------------
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
				//※入力にXがあった場合は故障伝搬しないので大丈夫
			}
		}
		
	}
	
	//===============================================================================
	// ゲート出力正常値が非制御値(0)で決定 (規則① or 規則②)
	//===============================================================================
	else{
		//         ＿＿
		//   0/1 --＼   ＼
		//   0/1 ----)NOR >○-- 1/0
		//   0/0 --／   ／
		//         ￣￣
		//---------------------------------------
		//故障伝搬入力(0/1)を探索⇒故障値入力1を探索
		//---------------------------------------
		for(i=0; i<t_net->n_in; i++){
			//故障SIMフラグが立ってる信号線
			if(t_net->in[i]->flag == ed_flag){
				if((t_net->in[i]->x_fault & MASKbit[fault_id]) != 0){	//NOR入力故障値==1

					//-----------------------------------------------------------
					//故障伝搬 && 入力信号線のxid_nval_t2にドントケア(X)が存在
					//-----------------------------------------------------------
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){	//ドントケア(X)発見
						//入力レベル更新(規則②で使用)
						if(in_lev > t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//未固定(X)信号線の最小レベルを保存
							in_id = i;						//未固定(X)信号線の入力idを保存
						}
					}
					//-----------------------------------------------------------
					//故障伝搬 && 入力に他故障でケアビット(0)固定された信号線が存在
					//-----------------------------------------------------------
					else{
						cbit_id = i;					//固定済み信号線の入力idを保存
						break;
					}
				}
			}
		}

		//---------------------------------------
		//他故障でケアビット(0)固定された信号線が存在
		//---------------------------------------
		if(cbit_id != -1){
			for(i=0; i<t_net->n_in; i++){

				//固定済み信号線を故障伝搬経路に決定
				if(i == cbit_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}

		//---------------------------------------
		//ゲート段数が最小の信号線を故障伝搬経路に決定
		//---------------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){
				
				//固定済み信号線を故障伝搬経路に決定
				if(i == in_id){
					TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);	//故障伝搬入力は遡る
				}

				//その他信号線は後方含意or限定正当化
				else{
					//入力信号線がXの場合
					if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id) == 3){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
	}
}


//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_fpath_exor
//  機  能 : 【EXOR・EXNOR】故障伝搬経路決定
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), fault_id(32個中何番目の故障なのか), ed_flag(FSIMイベントフラグ値), xid_flag(X抽出(ケアビット固定)フラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
void	TDF_XID_fpath_exor(S_NLIST *t_net, int tp_id, int fault_id, unsigned int ed_flag, unsigned int xid_flag, unsigned int jus_flag){
	
	int				i,j;
	
	//★EXOR・EXNORは2入力とも故障値の場合は，出力に故障伝搬しないので注意★
	//(※出力に故障伝搬してる場合，故障伝搬入力は1本のみとなる)
	//         ＿＿＿                       ＿＿＿                        ＿＿＿
	//   0/1 --＼    ＼               1/0 --＼    ＼                1/0 --＼    ＼
	//           ))EXOR>--- 0/0               ))EXOR>--- 0/0                ))EXOR>--- 1/1
	//   0/1 --／    ／               1/0 --／    ／                0/1 --／    ／
	//         ￣￣￣                       ￣￣￣                        ￣￣￣  
	//================================================
	// EXOR・EXNORのみ特殊処理
	//================================================
	for(i=0; i<t_net->n_in; i++){

		//--------------------------------------------------------------------
		// 入力信号線に故障伝搬経路flagが立ってる(対象故障のかは分からないが)
		//--------------------------------------------------------------------
		if(t_net->in[i]->flag == ed_flag){
			//-----------------------------------------------
			// 対象故障が故障伝搬(入力正常値≠入力故障値)
			//-----------------------------------------------
			//※故障伝搬経路なので3値でもx_bufの確認だけでOK
			if( (Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id)) != (t_net->in[i]->x_fault & MASKbit[fault_id])){
				TDF_XID_fpath_push(t_net->in[i], xid_flag, tp_id);
			}			
			//-----------------------------------------------
			// 限定含意・限定正当化
			//-----------------------------------------------
			else{
				//入力信号線がXの場合
				if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id)){
					//------------------------------
					//2値
					//------------------------------
					if(n_before_x == 0){
						//入力値=0
						if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 0){
							//---------------------------------------
							//0固定
							//---------------------------------------
							Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
							//---------------------------------------
							//PIの場合
							//---------------------------------------
							if(t_net->in[i]->type == IN){
								//--------------------
								//1時刻目がXの場合
								//--------------------
								if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
									//0固定
									Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
									printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
								}
								//--------------------
								//PPOの場合：2時刻目前方含意
								//--------------------
								if(t_net->ppo_flag == YES){
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
											TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
										}
									}
								}
								//--------------------
								//その他：1時刻目前方含意
								//--------------------
								else{
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
											TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
										}
									}
								}
							}
							
							//---------------------------------------
							//0からの後方含意
							//---------------------------------------
							TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
						}

						//入力値=1
						else{
							//---------------------------------------
							//1固定
							//---------------------------------------
							Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif
							//---------------------------------------
							//PIの場合
							//---------------------------------------
							if(t_net->in[i]->type == IN){
								//--------------------
								//1時刻目がXの場合
								//--------------------
								if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
									//1固定
									Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
									printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
								}
								//--------------------
								//PPOの場合：2時刻目前方含意
								//--------------------
								if(t_net->ppo_flag == YES){
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
											TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
										}
									}
								}
								//--------------------
								//その他：1時刻目前方含意
								//--------------------
								else{
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
											TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
										}
									}
								}
							}
							
							//---------------------------------------
							//1からの後方含意
							//---------------------------------------
							TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
						}
					}
					//------------------------------
					//3値
					//------------------------------
					else{
						//入力値=0
						if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 0){
							//---------------------------------------
							//0固定
							//---------------------------------------
							Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
							//---------------------------------------
							//PIの場合
							//---------------------------------------
							if(t_net->in[i]->type == IN){
								//--------------------
								//1時刻目がXの場合
								//--------------------
								if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
									//0固定
									Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
									printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
								}
								//--------------------
								//PPOの場合：2時刻目前方含意
								//--------------------
								if(t_net->ppo_flag == YES){
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
											TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
										}
									}
								}
								//--------------------
								//その他：1時刻目前方含意
								//--------------------
								else{
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
											TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
										}
									}
								}
							}
							
							//---------------------------------------
							//0からの後方含意
							//---------------------------------------
							TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
						}

						//入力値=1
						else{
							//---------------------------------------
							//1固定
							//---------------------------------------
							Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
							printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif
							//---------------------------------------
							//PIの場合
							//---------------------------------------
							if(t_net->in[i]->type == IN){
								//--------------------
								//1時刻目がXの場合
								//--------------------
								if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
									//1固定
									Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
									printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
								}
								//--------------------
								//PPOの場合：2時刻目前方含意
								//--------------------
								if(t_net->ppo_flag == YES){
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
											TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
										}
									}
								}
								//--------------------
								//その他：1時刻目前方含意
								//--------------------
								else{
									for(j=0; j<t_net->in[i]->n_out; j++){
										if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
											TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
										}
									}
								}
							}
							
							//---------------------------------------
							//1からの後方含意
							//---------------------------------------
							TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
						}
					}
				}
			}

		}
		//----------------------------------------------------------
		//絶対故障伝搬信号線でない⇒エンキュー(後で限定含意・限定正当化)
		//----------------------------------------------------------
		else{
			//入力信号線がXの場合
			if(Get_NBit(t_net->in[i]->xid_nval_t2, tp_id)){
				//------------------------------
				//2値
				//------------------------------
				if(n_before_x == 0){
					//入力値=0
					if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 0){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}

					//入力値=1
					else{
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
				//------------------------------
				//3値
				//------------------------------
				else{
					//入力値=0
					if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 0){
						//---------------------------------------
						//0固定
						//---------------------------------------
						Set_NBit_Zero(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 0固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//0固定
								Set_NBit_Zero(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 0固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_0_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//0からの後方含意
						//---------------------------------------
						TDF_0_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}

					//入力値=1
					else{
						//---------------------------------------
						//1固定
						//---------------------------------------
						Set_NBit_One(t_net->in[i]->xid_nval_t2, tp_id);
#ifdef PATH_DEBUG
						printf("【故障伝搬経路】%s : 1固定(2時刻目)\n", t_net->in[i]->name);
#endif
						//---------------------------------------
						//PIの場合
						//---------------------------------------
						if(t_net->in[i]->type == IN){
							//--------------------
							//1時刻目がXの場合
							//--------------------
							if(Get_NBit(t_net->in[i]->xid_nval_t1, tp_id) == 3){
								//1固定
								Set_NBit_One(t_net->in[i]->xid_nval_t1, tp_id);
#ifdef PATH_DEBUG
								printf("PI【故障伝搬経路】%s : 1固定(1時刻目)\n", t_net->in[i]->name);
#endif	
							}
							//--------------------
							//PPOの場合：2時刻目前方含意
							//--------------------
							if(t_net->ppo_flag == YES){
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t2, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 2);
									}
								}
							}
							//--------------------
							//その他：1時刻目前方含意
							//--------------------
							else{
								for(j=0; j<t_net->in[i]->n_out; j++){
									if(Get_NBit(t_net->in[i]->out[j]->xid_nval_t1, tp_id) == 3){
										TDF_1_Forward_Implication(t_net->in[i]->out[j], tp_id, 1);
									}
								}
							}
						}
						
						//---------------------------------------
						//1からの後方含意
						//---------------------------------------
						TDF_1_Backward_Implication(t_net->in[i], tp_id, 2, jus_flag);
					}
				}
			}
		}
	}
	
}
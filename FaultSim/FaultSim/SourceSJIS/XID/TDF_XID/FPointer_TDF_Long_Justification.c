//------------------------------------------------------------------------
//File name : FPointer_TDF_Long_Justification.c
//Date : 2014/7/25
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
void	dtob								(unsigned int, int);

//限定正当化
void	TDF_XID_long_jus_and_nand			(S_NLIST*, int, unsigned int, unsigned int);
void	TDF_XID_long_jus_or_nor				(S_NLIST*, int, unsigned int, unsigned int);

//後方含意
void	TDF_0_Backward_Implication			(S_NLIST*, int, int, unsigned int);
void	TDF_1_Backward_Implication			(S_NLIST*, int, int, unsigned int);

//前方含意
void	TDF_0_Forward_Implication			(S_NLIST*, int, int);
void	TDF_1_Forward_Implication			(S_NLIST*, int, int);

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
//  xid_flag		: ケアビットに固定したフラグ(故障伝搬経路or正当化経路)
//

	//#define	JUS_DEBUG

//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : FPointer_TDF_Long_Justification
//  機  能 : 【関数ポインタ作成】TDF用の最大レベルでの正当化(限定含意・限定正当化)の関数ポインタ作成
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	FPointer_TDF_Long_Justification(){

	//======================================================================================
	// 関数ポインタ作成 (限定正当化なので多入力ゲートのみ ※EXORは後方含意するため排除)
	//======================================================================================
	//限定正当化
	sfunc_jus[AND]	= TDF_XID_long_jus_and_nand;	//ANDとNANDは同じ処理
	sfunc_jus[NAND]	= TDF_XID_long_jus_and_nand;	//ANDとNANDは同じ処理
	sfunc_jus[OR]	= TDF_XID_long_jus_or_nor;		//ORとNORは同じ処理
	sfunc_jus[NOR]	= TDF_XID_long_jus_or_nor;		//ORとNORは同じ処理
}

//******************************************************************************************************
// 関数ポインタさんの中身
//******************************************************************************************************
//★限定正当化★
//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_long_jus_and_nand
//  機  能 : 【AND・NAND】2値・3値限定正当化
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), xid_flag(現在のケアビット固定フラグ値), jus_flag(限定正当化フラグ)
//------------------------------------------------------------------------------------
void	TDF_XID_long_jus_and_nand(S_NLIST *t_net, int tp_id, unsigned int xid_flag, unsigned int jus_flag){

	int				i;
	int				in_id;					//入力信号線の最小レベルの入力ID
	int				in_lev = 0;				//入力信号線のレベル⇒最大レベルに更新
	int				buff_nval;				//正常値の一時取得

	//=================================================================
	// 1時刻目限定正当化
	//=================================================================
	if(t_net->jus_flag==jus_flag || t_net->jus_flag==jus_flag+2){
		//-------------------------------------------
		// 制御値(0)を保持している信号探索
		//-------------------------------------------
		//-----------------------------------
		// 初期テスト集合が2値
		//-----------------------------------
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値0
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t1, tp_id) == 0){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t1, tp_id);

					//xid_nvalが『0』の場合(他故障で固定済みだった)
					if(buff_nval == 0){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}

					//xid_nvalが『0,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【AND・NAND】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]1時刻目 0→1： %s\n",tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
	
		//-----------------------------------
		// 初期テスト集合が3値
		//-----------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値0
				if(Get_NBit(t_net->in[i]->nval_t1, tp_id) == 0){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t1, tp_id);

					//xid_nvalが『0』の場合(他故障で固定済みだった)
					if(buff_nval == 0){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}
					
					//xid_nvalが『0,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【AND・NAND】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]1時刻目 0→1： %s\n",tp_id, t_net->in[i]->name);
					}
				}
			}
		}
		
		//-------------------------------------------
		// ケアビット0固定 & 後方含意
		//-------------------------------------------
		if(in_lev != -1){
			
			//---------------------------------------------
			// 0固定
			//---------------------------------------------
			Set_NBit_Zero(t_net->in[in_id]->xid_nval_t1, tp_id);
#ifdef JUS_DEBUG
			printf("【限定正当化(時刻1)】%s : 0固定 xid_flag:%d ed_flag:%d\n", t_net->in[in_id]->name, t_net->in[in_id]->xid_flag, t_net->in[in_id]->flag);
#endif
	
			//---------------------------------------------
			//t_net->in[in_id]信号線がPIの場合ケアビット保持数情報を更新
			//---------------------------------------------
			if(t_net->in[in_id]->type == IN){
				
				//PIの場合のみ1・2時刻目両方固定
				if(Get_NBit(t_net->in[in_id]->xid_nval_t2, tp_id) == 3){
					//--------------------
					//2時刻目0固定
					//--------------------
					Set_NBit_Zero(t_net->in[in_id]->xid_nval_t2, tp_id);
#ifdef JUS_DEBUG
					printf("※PI 限定正当化0固定(時刻2):%s\n", t_net->in[in_id]->name);
#endif					
					//--------------------
					//PPOでない場合：2時刻目前方含意
					//--------------------
					if(t_net->in[in_id]->ppo_flag == NO){
						for(i=0; i<t_net->in[in_id]->n_out; i++){
							if(Get_NBit(t_net->in[in_id]->out[i]->xid_nval_t2, tp_id) == 3){
								TDF_0_Forward_Implication(t_net->in[in_id]->out[i], tp_id, 2);
							}
						}
					}
				}

				//テストパターンに対するケアビット情報
				tp_bit[tp_id].n_cbit++;			//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
				tp_bit[tp_id].n_0bit++;			//対象信号線(PI)のテストパターン[tp_id]の0数をインクリメント

				//PIに対するケアビット情報
				pi_bit[t_net->in[in_id]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
				pi_bit[t_net->in[in_id]->pi_id].n_0bit++;	//対象信号線(PI)の0数をインクリメント
			}
			//------------------------------------------------
			//入力信号線がPPIの場合ケアビット保持数情報を更新
			//------------------------------------------------
			else if(t_net->in[in_id]->type == DFF){
				//テストパターンに対するケアビット情報
				tp_bit[tp_id].n_cbit++;			//対象信号線(PPI)のテストパターン[tp_id]のケアビット数をインクリメント
				tp_bit[tp_id].n_0bit++;			//対象信号線(PPI)のテストパターン[tp_id]の0数をインクリメント

				//PPIに対するケアビット情報
				ppi_bit[t_net->in[in_id]->ppi_id].n_cbit++;	//対象信号線(PPI)のケアビット数をインクリメント
				ppi_bit[t_net->in[in_id]->ppi_id].n_0bit++;	//対象信号線(PPI)の0数をインクリメント

			}

			//---------------------------------------------
			//後方含意
			//---------------------------------------------
			else{
#ifdef JUS_DEBUG
				printf("後方含意(時刻1): %s\n", t_net->in[in_id]->name);
#endif
				TDF_0_Backward_Implication(t_net->in[in_id], tp_id, 1 ,jus_flag);
			}
		}
	}

	//=================================================================
	// 2時刻目限定正当化
	//=================================================================
	if(t_net->jus_flag==jus_flag+1 || t_net->jus_flag==jus_flag+2){
		
		//-------------------------------------------
		//入力信号線のレベルを最初期化
		//-------------------------------------------
		in_lev = max_level+1;

		//-------------------------------------------
		// 制御値(0)を保持している信号探索
		//-------------------------------------------
		//-----------------------------------
		// 初期テスト集合が2値
		//-----------------------------------
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値0
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 0){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t2, tp_id);

					//xid_nvalが『0』の場合(他故障で固定済みだった)
					if(buff_nval == 0){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}
					
					//xid_nvalが『0,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【AND・NAND】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]2時刻目 0→1： %s\n", tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
	
		//-----------------------------------
		// 初期テスト集合が3値
		//-----------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値0
				if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 0){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t2, tp_id);

					//xid_nvalが『0』の場合(他故障で固定済みだった)
					if(buff_nval == 0){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}
					
					//xid_nvalが『0,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【AND・NAND】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]2時刻目 0→1： %s\n", tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
		
		//-------------------------------------------
		// ケアビット0固定 & 後方含意
		//-------------------------------------------
		if(in_lev != -1){
			
			//---------------------------------------------
			// 0固定
			//---------------------------------------------
			Set_NBit_Zero(t_net->in[in_id]->xid_nval_t2, tp_id);
#ifdef JUS_DEBUG
			printf("【限定正当化(時刻2)】%s : 0固定 xid_flag:%d ed_flag:%d\n", t_net->in[in_id]->name, t_net->in[in_id]->xid_flag, t_net->in[in_id]->flag);
#endif
	
			//---------------------------------------------
			//t_net->in[in_id]信号線がPIの場合ケアビット保持数情報を更新
			//---------------------------------------------
			if(t_net->in[in_id]->type == IN){

				//PIの場合のみ1・2時刻目両方固定
				if(Get_NBit(t_net->in[in_id]->xid_nval_t1, tp_id) == 3){
					//--------------------
					//1時刻目0固定
					//--------------------
					Set_NBit_Zero(t_net->in[in_id]->xid_nval_t1, tp_id);
#ifdef JUS_DEBUG
					printf("※PI 限定正当化0固定(時刻1):%s\n", t_net->in[in_id]->name);
#endif
					//--------------------
					//PPOの場合：2時刻目前方含意
					//--------------------
					if(t_net->in[in_id]->ppo_flag == YES){
						for(i=0; i<t_net->in[in_id]->n_out; i++){
							if(Get_NBit(t_net->in[in_id]->out[i]->xid_nval_t2, tp_id) == 3){
								TDF_0_Forward_Implication(t_net->in[in_id]->out[i], tp_id, 2);
							}
						}
					}
					//--------------------
					//それ以外：1時刻目前方含意
					//--------------------
					else{
						for(i=0; i<t_net->in[in_id]->n_out; i++){
							if(Get_NBit(t_net->in[in_id]->out[i]->xid_nval_t1, tp_id) == 3){
								TDF_0_Forward_Implication(t_net->in[in_id]->out[i], tp_id, 1);
							}
						}
					}
				}

				//テストパターンに対するケアビット情報
				tp_bit[tp_id].n_cbit++;			//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
				tp_bit[tp_id].n_0bit++;			//対象信号線(PI)のテストパターン[tp_id]の0数をインクリメント

				//PIに対するケアビット情報
				pi_bit[t_net->in[in_id]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
				pi_bit[t_net->in[in_id]->pi_id].n_0bit++;	//対象信号線(PI)の0数をインクリメント
			}
			//---------------------------------------------
			//後方含意
			//---------------------------------------------
			else{
#ifdef JUS_DEBUG
				printf("後方含意(時刻2): %s\n", t_net->in[in_id]->name);
#endif
				TDF_0_Backward_Implication(t_net->in[in_id], tp_id, 2 ,jus_flag);
			}
		}
	}
}

//------------------------------------------------------------------------------------
//  関数名 : TDF_XID_long_jus_or_nor
//  機  能 : 【OR・NOR】2値・3値限定正当化
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), xid_flag(現在のケアビット固定フラグ値), jus_flag(限定正当化フラグ)
//------------------------------------------------------------------------------------
void	TDF_XID_long_jus_or_nor(S_NLIST *t_net, int tp_id, unsigned int xid_flag, unsigned int jus_flag){

	int				i;
	int				in_id;					//入力信号線の最小レベルの入力ID
	int				in_lev = 0;				//入力信号線のレベル⇒最大レベルに更新
	int				buff_nval;				//正常値の一時取得
	
	//=================================================================
	// 1時刻目限定正当化
	//=================================================================
	if(t_net->jus_flag==jus_flag || t_net->jus_flag==jus_flag+2){
		//-------------------------------------------
		// 制御値(1)を保持している信号探索
		//-------------------------------------------
		//-----------------------------------
		// 初期テスト集合が2値
		//-----------------------------------
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値1
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t1, tp_id) == 1){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t1, tp_id);

					//xid_nvalが『1』の場合(他故障で固定済みだった)
					if(buff_nval == 1){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}

					//xid_nvalが『1,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【OR・NOR】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]1時刻目 1→0： %s\n", tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
	
		//-----------------------------------
		// 初期テスト集合が3値
		//-----------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値1
				if(Get_NBit(t_net->in[i]->nval_t1, tp_id) == 1){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t1, tp_id);

					//xid_nvalが『1』の場合(他故障で固定済みだった)
					if(buff_nval == 1){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}

					//xid_nvalが『1,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【OR・NOR】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]1時刻目 1→0： %s\n", tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
		
		//-------------------------------------------
		// ケアビット1固定 & 後方含意
		//-------------------------------------------
		if(in_lev != -1){
			
			//---------------------------------------------
			// 1固定
			//---------------------------------------------
			Set_NBit_One(t_net->in[in_id]->xid_nval_t1, tp_id);
#ifdef JUS_DEBUG
			printf("【限定正当化(時刻1)】%s : 1固定 xid_flag:%d ed_flag:%d\n", t_net->in[in_id]->name, t_net->in[in_id]->xid_flag, t_net->in[in_id]->flag);
#endif
	
			//---------------------------------------------
			//t_net->in[in_id]信号線がPIの場合ケアビット保持数情報を更新
			//---------------------------------------------
			if(t_net->in[in_id]->type == IN){

				//PIの場合のみ1・2時刻目両方固定
				if(Get_NBit(t_net->in[in_id]->xid_nval_t2, tp_id) == 3){
					//--------------------
					//2時刻目1固定
					//--------------------
					Set_NBit_One(t_net->in[in_id]->xid_nval_t2, tp_id);
#ifdef JUS_DEBUG
					printf("※PI 限定正当化1固定(時刻2):%s\n", t_net->in[in_id]->name);
#endif
					//--------------------
					//PPOでない場合：2時刻目前方含意
					//--------------------
					if(t_net->in[in_id]->ppo_flag == NO){
						for(i=0; i<t_net->in[in_id]->n_out; i++){
							if(Get_NBit(t_net->in[in_id]->out[i]->xid_nval_t2, tp_id) == 3){
								TDF_1_Forward_Implication(t_net->in[in_id]->out[i], tp_id, 2);
							}
						}
					}
				}

				//テストパターンに対するケアビット情報
				tp_bit[tp_id].n_cbit++;			//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
				tp_bit[tp_id].n_1bit++;			//対象信号線(PI)のテストパターン[tp_id]の1数をインクリメント

				//PIに対するケアビット情報
				pi_bit[t_net->in[in_id]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
				pi_bit[t_net->in[in_id]->pi_id].n_1bit++;	//対象信号線(PI)の1数をインクリメント
			}
			//------------------------------------------------
			//入力信号線がPPIの場合ケアビット保持数情報を更新
			//------------------------------------------------
			else if(t_net->in[in_id]->type == DFF){
				//テストパターンに対するケアビット情報
				tp_bit[tp_id].n_cbit++;			//対象信号線(PPI)のテストパターン[tp_id]のケアビット数をインクリメント
				tp_bit[tp_id].n_1bit++;			//対象信号線(PPI)のテストパターン[tp_id]の1数をインクリメント

				//PPIに対するケアビット情報
				ppi_bit[t_net->in[in_id]->ppi_id].n_cbit++;	//対象信号線(PPI)のケアビット数をインクリメント
				ppi_bit[t_net->in[in_id]->ppi_id].n_1bit++;	//対象信号線(PPI)の1数をインクリメント

			}

			//---------------------------------------------
			//後方含意
			//---------------------------------------------
			else{
#ifdef JUS_DEBUG
				printf("後方含意(時刻1): %s\n", t_net->in[in_id]->name);
#endif
				TDF_1_Backward_Implication(t_net->in[in_id], tp_id, 1 ,jus_flag);
			}
		}
	}

	//=================================================================
	// 2時刻目限定正当化
	//=================================================================
	if(t_net->jus_flag==jus_flag+1 || t_net->jus_flag==jus_flag+2){
		
		//-------------------------------------------
		//入力信号線のレベルを最初期化
		//-------------------------------------------
		in_lev = max_level+1;

		//-------------------------------------------
		// 制御値(1)を保持している信号探索
		//-------------------------------------------
		//-----------------------------------
		// 初期テスト集合が2値
		//-----------------------------------
		if(n_before_x == 0){
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値1
				if(Get_NBit_Xbuf(t_net->in[i]->nval_t2, tp_id) == 1){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t2, tp_id);

					//xid_nvalが『1』の場合(他故障で固定済みだった)
					if(buff_nval == 1){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}
					
					//xid_nvalが『1,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【OR・NOR】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]2時刻目 1→0： %s\n", tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
	
		//-----------------------------------
		// 初期テスト集合が3値
		//-----------------------------------
		else{
			for(i=0; i<t_net->n_in; i++){

				//入力[i]の信号線が正常値で制御値1
				if(Get_NBit(t_net->in[i]->nval_t2, tp_id) == 1){

					//X抽出後値(xid_nval)を確認
					buff_nval = Get_NBit(t_net->in[i]->xid_nval_t2, tp_id);

					//xid_nvalが『1』の場合(他故障で固定済みだった)
					if(buff_nval == 1){
						in_lev = -1;			//正当化しなくてもOKなのでbreak
						break;
					}

					//xid_nvalが『X』の場合
					else if(buff_nval == 3){
						if(in_lev < t_net->in[i]->level){
							in_lev = t_net->in[i]->level;	//一番レベルが高い信号線を更新
							in_id = i;						//入力IDを保存
						}
					}
					
					//xid_nvalが『1,X』以外の場合
					else{
						printf("\n//--------------------------\n");
						printf("// ERROR : 【OR・NOR】限定正当化\n");
						printf("//--------------------------\n");
						printf("初期パターンとは異なる論理値が割当てられてます\n");
						printf("tp[%d]2時刻目 1→0： %s\n", tp_id,  t_net->in[i]->name);
					}
				}
			}
		}
		
		//-------------------------------------------
		// ケアビット1固定 & 後方含意
		//-------------------------------------------
		if(in_lev != -1){
			
			//---------------------------------------------
			// 1固定
			//---------------------------------------------
			Set_NBit_One(t_net->in[in_id]->xid_nval_t2, tp_id);
#ifdef JUS_DEBUG
			printf("【限定正当化(時刻2)】%s : 1固定 xid_flag:%d ed_flag:%d\n", t_net->in[in_id]->name, t_net->in[in_id]->xid_flag, t_net->in[in_id]->flag);
#endif
	
			//---------------------------------------------
			//t_net->in[in_id]信号線がPIの場合ケアビット保持数情報を更新
			//---------------------------------------------
			if(t_net->in[in_id]->type == IN){

				//PIの場合のみ1・2時刻目両方固定
				if(Get_NBit(t_net->in[in_id]->xid_nval_t1, tp_id) == 3){
					//--------------------
					//1時刻目1固定
					//--------------------
					Set_NBit_One(t_net->in[in_id]->xid_nval_t1, tp_id);
#ifdef JUS_DEBUG
					printf("※PI 限定正当化1固定(時刻1):%s\n", t_net->in[in_id]->name);
#endif
					//--------------------
					//PPOの場合：2時刻目前方含意
					//--------------------
					if(t_net->in[in_id]->ppo_flag == YES){
						for(i=0; i<t_net->in[in_id]->n_out; i++){
							if(Get_NBit(t_net->in[in_id]->out[i]->xid_nval_t2, tp_id) == 3){
								TDF_1_Forward_Implication(t_net->in[in_id]->out[i], tp_id, 2);
							}
						}
					}
					//--------------------
					//それ以外：1時刻目前方含意
					//--------------------
					else{
						for(i=0; i<t_net->in[in_id]->n_out; i++){
							if(Get_NBit(t_net->in[in_id]->out[i]->xid_nval_t1, tp_id) == 3){
								TDF_1_Forward_Implication(t_net->in[in_id]->out[i], tp_id, 1);
							}
						}
					}
				}

				//テストパターンに対するケアビット情報
				tp_bit[tp_id].n_cbit++;			//対象信号線(PI)のテストパターン[tp_id]のケアビット数をインクリメント
				tp_bit[tp_id].n_1bit++;			//対象信号線(PI)のテストパターン[tp_id]の1数をインクリメント

				//PIに対するケアビット情報
				pi_bit[t_net->in[in_id]->pi_id].n_cbit++;	//対象信号線(PI)のケアビット数をインクリメント
				pi_bit[t_net->in[in_id]->pi_id].n_1bit++;	//対象信号線(PI)の1数をインクリメント
			}
			//---------------------------------------------
			//後方含意
			//---------------------------------------------
			else{
#ifdef JUS_DEBUG
				printf("後方含意(時刻2): %s\n", t_net->in[in_id]->name);
#endif
				TDF_1_Backward_Implication(t_net->in[in_id], tp_id, 2 ,jus_flag);
			}
		}
	}
}
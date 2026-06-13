//------------------------------------------------------------------------
//File name : SAF_2v_EXrepED_PPSFP.c
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
void		SAF_2v_repEDpush		(NLIST*, int);
void		SAF_FOUT_2v_CPT				(int, NLIST*, int, unsigned int*, NLIST*);
void		SAF_2v_CPT0				(int, unsigned int, NLIST*);
void		SAF_2v_CPT1				(int, unsigned int, NLIST*);

//通常演算関数
static void SAF_2v_ED_fout			(NLIST*, int, unsigned int);
static void SAF_2v_ED_buf			(NLIST*, int, unsigned int);
static void SAF_2v_ED_inv			(NLIST*, int, unsigned int);
static void SAF_2v_ED_and			(NLIST*, int, unsigned int);
static void SAF_2v_ED_nand			(NLIST*, int, unsigned int);
static void SAF_2v_ED_or			(NLIST*, int, unsigned int);
static void SAF_2v_ED_nor			(NLIST*, int, unsigned int);
static void SAF_2v_ED_exor			(NLIST*, int, unsigned int);
static void SAF_2v_ED_exnor			(NLIST*, int, unsigned int);

//特殊演算関数
static void SAF_2v_EX_ED_fout		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_buf		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_inv		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_and		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_nand		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_or			(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_nor		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_exor		(NLIST*, int, unsigned int, NLIST*, int, int*);
static void SAF_2v_EX_ED_exnor		(NLIST*, int, unsigned int, NLIST*, int, int*);

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
//  関数名 : SAF_2v_EXrepED_PPSFP
//  機  能 : 【代表信号線】※STEM(故障個所)が非代表信号線の場合の2値縮退故障PPSFPイベントドリブン
//  戻り値 : なし
//  引  数 : ffr_id(ステムのFFR番号), stem_net(ステム信号線), ed_flag(計算済みフラグ値), ui_num(unsigned int番目)
//----------------------------------------------
void	SAF_2v_EXrepED_PPSFP (int ffr_id, NLIST* stem_net, unsigned int ed_flag, int ui_num){


	int				i;
	int				inv_flag=0;			//反転フラグ
	int				level_flag=0;		//event_lev更新時にbreakしないようにする
	int				event_lev;			//イベント計算するレベル(※計算しながら更新)
	NLIST			*temp_net;			//レベライズスタックから取り出した信号線を一時的に格納
	unsigned int	det_tp_list=0;		//POまで故障伝搬したTPリスト【0:未伝搬 1:伝搬(=故障検出)】	
	int				func_flag;			//関数ポインタの切り替えフラグ(初期値：stem信号線のn_next_event)

	//関数ポインタ
	static void(*func[20])(NLIST*, int, unsigned int);						//通常演算関数ポインタ
	static void(*exfunc[20])(NLIST*, int, unsigned int, NLIST*, int, int*);	//特殊演算関数ポインタ

	//=========================================================
	// 関数ポインタ作成
	//=========================================================
	//通常演算関数
	func[FOUT]		= SAF_2v_ED_fout;
	func[BUF]		= SAF_2v_ED_buf;
	func[INV]		= SAF_2v_ED_inv;
	func[AND]		= SAF_2v_ED_and;
	func[NAND]		= SAF_2v_ED_nand;
	func[OR]		= SAF_2v_ED_or;
	func[NOR]		= SAF_2v_ED_nor;
	func[EXOR]		= SAF_2v_ED_exor;
	func[EXNOR]		= SAF_2v_ED_exnor;

	//特殊演算関数(※故障個所が非代表信号線の特殊ver)
	exfunc[FOUT]	= SAF_2v_EX_ED_fout;
	exfunc[BUF]		= SAF_2v_EX_ED_buf;
	exfunc[INV]		= SAF_2v_EX_ED_inv;
	exfunc[AND]		= SAF_2v_EX_ED_and;
	exfunc[NAND]	= SAF_2v_EX_ED_nand;
	exfunc[OR]		= SAF_2v_EX_ED_or;
	exfunc[NOR]		= SAF_2v_EX_ED_nor;
	exfunc[EXOR]	= SAF_2v_EX_ED_exor;
	exfunc[EXNOR]	= SAF_2v_EX_ED_exnor;

	//=============================================================
	// STEM(故障箇所)の影響信号線へたどり着くまで後方探索(PI側)
	//=============================================================
	//-----------------------------------------
	//故障個所の入力信号線をエンキュー
	//-----------------------------------------
	enqueue(nl_queue, (void*)stem_net->in[0]);

	//-----------------------------------------
	//故障個所がINVの場合,反転フラグをインクリメント
	//-----------------------------------------
	if(stem_net->type == INV){
		inv_flag++;
	}
	
	//-----------------------------------------
	//影響信号線へたどり着くまで後方探索
	//-----------------------------------------
	while((temp_net = (NLIST *)dequeue(nl_queue)) != (NLIST *)NULL){	//空になるまでデキュー

		//INVの場合反転フラグをインクリメント
		if(temp_net->type == INV){
			inv_flag++;
		}

		//代表信号線じゃない場合，さらに入力信号線をエンキュー
		if(temp_net->rep_net == NO){
			enqueue(nl_queue, (void*)temp_net->in[0]);
		}
		//代表信号線の場合，一応影響信号線かチェック
		else{
			if(stem_net->influence_net->n != temp_net->n){
				printf("\n//----------------------------------\n");
				printf("// ERROR: SAF_2v_EXrepED_PPSFP.c\n");
				printf("//----------------------------------\n");
				printf("影響信号線が不一致！\n");
				exit(-1);
			}
		}

	}
	
	//-----------------------------------------
	//キュー再初期化
	//-----------------------------------------
	reset_queue(nl_queue);
	
	//-----------------------------------------
	//故障値反転情報を計算
	//-----------------------------------------
	if(inv_flag%2 == 0){
		inv_flag = NO;
	}
	else{
		inv_flag = YES;
	}
	
	//=========================================================
	//関数ポインタの切り替えフラグ値代入
	//=========================================================
	func_flag = stem_net->n_next_event;

	//=========================================================
	// FOUTステム信号線の次イベント(代表信号線)をレベライズスタックへプッシュ
	//=========================================================
	SAF_2v_repEDpush(stem_net, ed_flag);


	//========================================================= 
	// イベントドリブン開始レベル決定
	//=========================================================
	event_lev = stem_net->next_event[0]->level;

	for(i=1; i<stem_net->n_next_event; i++){
		if(event_lev > stem_net->next_event[i]->level){
			event_lev = stem_net->next_event[i]->level;	//低いレベルがあったら更新
		}
	}

	//=========================================================
	// イベントドリブン実行
	//=========================================================
	while(event_lev != (max_level+1) ){

		//-------------------------------------------
		// レベライズスタックから信号線を取り出す
		//-------------------------------------------
		temp_net = lev_temp[event_lev].net[lev_temp[event_lev].n_net-1];	//ケツから取り出す
		
		//-------------------------------------------
		// 計算済みフラグを代入
		//-------------------------------------------
		temp_net->flag = ed_flag;

		//-------------------------------------------
		// temp_netの故障値計算
		//-------------------------------------------
		//【特殊演算ver】
		if(func_flag > 0){
			(*exfunc[temp_net->type])(temp_net, ui_num, ed_flag, stem_net, inv_flag, &func_flag);
		}
		//【通常演算ver】
		else{
			(*func[temp_net->type])(temp_net, ui_num, ed_flag);
		}
		
		//-------------------------------------------
		// temp_netの正常値≠故障値(故障伝搬)
		//-------------------------------------------
		if(temp_net->nval->x_buf[ui_num] != temp_net->x_fault){
			
			//--------------------------------------------------------------------
			//temp_netが外部出力の場合CPT
			//--------------------------------------------------------------------
			if(temp_net->n_out == 0){
				SAF_FOUT_2v_CPT(ffr_id, stem_net, ui_num, &det_tp_list, temp_net);
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント

				//------------------------------------------------------
				//★det_tp_listがオール1になった(32bit分すべて故障検出)
				//------------------------------------------------------
				if(det_tp_list == 0xFFFFFFFF){

					//レベライズスタック内を初期化
					for(i=event_lev; i<max_level+1; i++)lev_temp[i].n_net = 0;
					
					//一応レベルフラグを0にしとく
					level_flag = 0;
					
					break;	//イベントドリブン終了
				}
			}
			
			//--------------------------------------------------------------------
			//内部信号線の場合は出力代表信号線をレベライズスタックへプッシュ
			//--------------------------------------------------------------------
			else{
				SAF_2v_repEDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
				lev_temp[event_lev].n_net--;			//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
		}

		//-------------------------------------------
		// temp_netの正常値==故障値(故障値消滅⇒出力代表信号線はプッシュしない)
		//-------------------------------------------
		else{
			lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
		}
		
		//-------------------------------------------
		// event_levを更新(現event_levのスタック内信号線保持数が0になったとき)
		//-------------------------------------------
		if(lev_temp[event_lev].n_net == 0){

			//レベライズスタック内を探索
			for(i=event_lev+1; i<=max_level; i++){
				//保持信号線数≠0のレベルスタック発見
				if(lev_temp[i].n_net != 0){
					event_lev = i;		//event_lev更新
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

	}//while


	//*******************************************
	// バグチェック //
	for(i=0; i<=max_level; i++){
		//保持信号線数≠0のレベルスタック発見
		if(lev_temp[i].n_net != 0){
			printf("\n\n//-------------------------------\n");
			printf("// ERROR：レベルスタック保持信号線数≠0\n");
			printf("//-------------------------------\n");
			printf("レベル:%d 個数:%d\n", i, lev_temp[i].n_net);
			exit(-1);
		}
	}
	//********************************************

}

//******************************************************************************************************
//******************************************************************************************************
// ★【通常演算関数】関数ポインタさんの中身★
//******************************************************************************************************
//******************************************************************************************************
//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_fout
//  機  能 : 【FOUT】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_fout(NLIST *t_net, int ui_num, unsigned int ed_flag){

	//※POでFOUTの場合は『代表信号線』になるため必要
	//================================================
	// 【入力の反転フラグNO】x_faultとp_faultをそのまま引き継ぐ
	//================================================
	if(t_net->in[0]->inv_flag == NO){
		t_net->x_fault = t_net->influence_net->x_fault;
	}

	//================================================
	// 【入力の反転フラグYES】x_faultの否定を引き継ぐ
	//================================================
	else if(t_net->in[0]->inv_flag == YES){
		t_net->x_fault = ~t_net->influence_net->x_fault;
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_buf
//  機  能 : 【BUF】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_buf(NLIST *t_net, int ui_num, unsigned int ed_flag){
	
	//================================================
	// 【入力の反転フラグNO】x_faultとp_faultをそのまま引き継ぐ
	//================================================
	if(t_net->in[0]->inv_flag == NO){
		t_net->x_fault = t_net->influence_net->x_fault;
	}

	//================================================
	// 【入力の反転フラグYES】x_faultの否定を引き継ぐ
	//================================================
	else if(t_net->in[0]->inv_flag == YES){
		t_net->x_fault = ~t_net->influence_net->x_fault;
	}
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_inv
//  機  能 : 【INV】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_inv(NLIST *t_net, int ui_num, unsigned int ed_flag){
	
	//================================================
	// 【入力の反転フラグNO】INV処理
	//================================================
	if(t_net->in[0]->inv_flag == NO){
		t_net->x_fault = ~t_net->influence_net->x_fault;
	}

	//================================================
	// 【入力の反転フラグYES】否定の否定⇒BUFと同じ処理
	//================================================
	else if(t_net->in[0]->inv_flag == YES){
		t_net->x_fault = t_net->influence_net->x_fault;
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_and
//  機  能 : 【AND】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_and(NLIST *t_net, int ui_num, unsigned int ed_flag){

	int				i;
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->influence_net->flag == ed_flag){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault &= t_net->in[i]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault &= ~t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault &= t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault &= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_nand
//  機  能 : 【NAND】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_nand(NLIST *t_net, int ui_num, unsigned int ed_flag){

	int				i;
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->influence_net->flag == ed_flag){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault &= t_net->in[i]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault &= ~t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault &= t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault &= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
	
	//================================================
	//出力結果を反転
	//================================================
	t_net->x_fault = ~t_net->x_fault;

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_or
//  機  能 : 【OR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_or(NLIST *t_net, int ui_num, unsigned int ed_flag){

	int				i;
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->influence_net->flag == ed_flag){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault |= t_net->in[i]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault |= ~t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault |= t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault |= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_nor
//  機  能 : 【NOR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_nor(NLIST *t_net, int ui_num, unsigned int ed_flag){

	int				i;

	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->influence_net->flag == ed_flag){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
		//入力の反転フラグYES(xの否定)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault |= t_net->in[i]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault |= ~t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault |= t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault |= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
			}
		}
	}

	//================================================
	//出力結果を反転
	//================================================
	t_net->x_fault = ~t_net->x_fault;
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_exor
//  機  能 : 【EXOR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_exor(NLIST *t_net, int ui_num, unsigned int ed_flag){

	//===========================================================
	// 2入力とも【反転フラグNO】
	//===========================================================
	if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

	//===========================================================
	// 0番目入力のみ【反転フラグYES】
	//===========================================================
	else if(t_net->in[0]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

	//===========================================================
	// 1番目入力のみ【反転フラグYES】(in[1]のxとpを入れ替えて演算)
	//===========================================================
	else if(t_net->in[1]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

	//===========================================================
	// 2入力とも【反転フラグYES】(in[0],in[1]のxとpを入れ替えて演算)
	//===========================================================
	else{
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_ED_exnor
//  機  能 : 【EXNOR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
static void	SAF_2v_ED_exnor(NLIST *t_net, int ui_num, unsigned int ed_flag){
	
	//===========================================================
	// 2入力とも【反転フラグNO】
	//===========================================================
	if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

	//===========================================================
	// 0番目入力のみ【反転フラグYES】
	//===========================================================
	else if(t_net->in[0]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

	//===========================================================
	// 1番目入力のみ【反転フラグYES】(in[1]のxとpを入れ替えて演算)
	//===========================================================
	else if(t_net->in[1]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}

	//===========================================================
	// 2入力とも【反転フラグYES】(in[0],in[1]のxとpを入れ替えて演算)
	//===========================================================
	else{
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag == ed_flag){
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
		}
	}
	
	//================================================
	//出力結果を反転
	//================================================
	t_net->x_fault = ~t_net->x_fault;

}

//******************************************************************************************************
//******************************************************************************************************
// ★【特殊演算関数】関数ポインタさんの中身★
//******************************************************************************************************
//******************************************************************************************************
//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_fout
//  機  能 : 【FOUT】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_fout(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){

	//※POでFOUTの場合は『代表信号線』になるため必要
	//========================================================
	// 【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//========================================================
	if(t_net->influence_net->n == stem_net->influence_net->n){
		
		//★func_flag(特殊演算回数)のデクリメント
		(*func_flag)--;

		//---------------------------------------------------------
		// 【入力の反転フラグNO】
		//---------------------------------------------------------
		//故障信号線の反転フラグ通りに代入
		if(t_net->in[0]->inv_flag == NO){
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;
			}		
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;
			}
		}
		//---------------------------------------------------------
		// 【入力の反転フラグYES】
		//---------------------------------------------------------
		//故障信号線の反転フラグの否定で代入(※否定の否定)
		else if(t_net->in[0]->inv_flag == YES){
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;
			}		
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;
			}
		}
	}
	//========================================================
	// 影響信号線が不一致(普通に計算)
	//========================================================
	else{
		//---------------------------------------------------------
		// 【入力の反転フラグNO】x_faultをそのまま引き継ぐ
		//---------------------------------------------------------
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->influence_net->x_fault;
		}
		
		//---------------------------------------------------------
		// 【入力の反転フラグYES】x_faultの否定を引き継ぐ
		//---------------------------------------------------------
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->influence_net->x_fault;
		}
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_buf
//  機  能 : 【BUF】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_buf(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){
	
	//========================================================
	// 【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//========================================================
	if(t_net->influence_net->n == stem_net->influence_net->n){
		//---------------------------------------------------------
		// 【入力の反転フラグNO】
		//---------------------------------------------------------
		
		//★func_flag(特殊演算回数)のデクリメント
		(*func_flag)--;

		//故障信号線の反転フラグ通りに代入
		if(t_net->in[0]->inv_flag == NO){
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;
			}		
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;
			}
		}
		//---------------------------------------------------------
		// 【入力の反転フラグYES】
		//---------------------------------------------------------
		//故障信号線の反転フラグの否定で代入(※否定の否定)
		else if(t_net->in[0]->inv_flag == YES){
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;
			}		
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;
			}
		}
	}
	//========================================================
	// 影響信号線が不一致(普通に計算)
	//========================================================
	else{
		//---------------------------------------------------------
		// 【入力の反転フラグNO】x_faultをそのまま引き継ぐ
		//---------------------------------------------------------
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->influence_net->x_fault;
		}
		
		//---------------------------------------------------------
		// 【入力の反転フラグYES】x_faultの否定を引き継ぐ
		//---------------------------------------------------------
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = ~t_net->influence_net->x_fault;
		}
	}
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_inv
//  機  能 : 【INV】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_inv(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){
	
	//========================================================
	// 【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//========================================================
	if(t_net->influence_net->n == stem_net->influence_net->n){
		
		//★func_flag(特殊演算回数)のデクリメント
		(*func_flag)--;

		//---------------------------------------------------------
		// 【入力の反転フラグNO】
		//---------------------------------------------------------
		//故障信号線の反転フラグの否定で代入
		if(t_net->in[0]->inv_flag == NO){
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;	//stem_net->故障値のINV処理
			}		
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;		//~stem_net->故障値のINV処理
			}
		}
		//---------------------------------------------------------
		// 【入力の反転フラグYES】
		//---------------------------------------------------------
		//故障信号線の反転フラグ通りに代入(※否定の否定)
		else if(t_net->in[0]->inv_flag == YES){
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;		//~stem_net->故障値のINV処理
			}		
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;	//~~stem_net->故障値のINV処理
			}
		}
	}
	//========================================================
	// 影響信号線が不一致(普通に計算)
	//========================================================
	else{
		//---------------------------------------------------------
		// 【入力の反転フラグNO】x_faultの否定を引き継ぐ
		//---------------------------------------------------------
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = ~t_net->influence_net->x_fault;
		}
		
		//---------------------------------------------------------
		// 【入力の反転フラグYES】x_faultの否定の否定を引き継ぐ
		//---------------------------------------------------------
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = t_net->influence_net->x_fault;
		}
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_and
//  機  能 : 【AND】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_and(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){

	int				i;
	int				n_equal_net=0;	//stem信号線と一致した入力信号線数

	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//--------------------------------------------
	// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//--------------------------------------------
	if(t_net->in[0]->influence_net->n == stem_net->influence_net->n){
		
		//stem信号線と一致した回数のインクリメント
		n_equal_net++;

		//-----------------------------------
		//故障信号線の故障値を代入
		//-----------------------------------
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//故障信号線の反転フラグ通りに代入
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;
			}
		}
		//入力の反転フラグYES
		else if(t_net->in[0]->inv_flag == YES){
			//故障信号線の反転フラグの否定で代入(※否定の否定)
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;
			}
		}
	}

	//--------------------------------------------
	// ■影響信号線が不一致(普通に計算)
	//--------------------------------------------
	else{
		//-----------------------------------
		//入力が計算済み ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[0]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
			}
		}
		//-----------------------------------
		//正常値を代入
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//--------------------------------------------
		// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
		//--------------------------------------------
		if(t_net->in[i]->influence_net->n == stem_net->influence_net->n){
		
			//stem信号線と一致した回数のインクリメント
			n_equal_net++;

			//-----------------------------------
			//故障信号線の故障値を代入
			//-----------------------------------
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//故障信号線の反転フラグ通りに代入
				if(inv_flag == NO){
					t_net->x_fault &= stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault &= ~stem_net->x_fault;
				}
			}
			//入力の反転フラグYES
			else if(t_net->in[i]->inv_flag == YES){
				//故障信号線の反転フラグの否定で代入(※否定の否定)
				if(inv_flag == NO){
					t_net->x_fault &= ~stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault &= stem_net->x_fault;
				}
			}
		}

		//--------------------------------------------
		// ■影響信号線が不一致(普通に計算)
		//--------------------------------------------
		else{
			//-----------------------------------
			//入力が計算済み ⇒ 故障値で計算
			//-----------------------------------
			if(t_net->in[i]->influence_net->flag == ed_flag){
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault &= t_net->in[i]->influence_net->x_fault;
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault &= ~t_net->in[i]->influence_net->x_fault;
				}
			}
		
			//-----------------------------------
			//正常値で計算
			//-----------------------------------
			else{
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault &= t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault &= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
	}
	
	//================================================
	// 特殊演算実行回数上限のデクリメント
	//================================================
	if(n_equal_net != 0){
		(*func_flag)--;
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_nand
//  機  能 : 【NAND】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_nand(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){

	int				i;
	int				n_equal_net=0;	//stem信号線と一致した入力信号線数
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//--------------------------------------------
	// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//--------------------------------------------
	if(t_net->in[0]->influence_net->n == stem_net->influence_net->n){
		
		//stem信号線と一致した回数のインクリメント
		n_equal_net++;

		//-----------------------------------
		//故障信号線の故障値を代入
		//-----------------------------------
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//故障信号線の反転フラグ通りに代入
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;
			}
		}
		//入力の反転フラグYES
		else if(t_net->in[0]->inv_flag == YES){
			//故障信号線の反転フラグの否定で代入(※否定の否定)
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;
			}
		}
	}

	//--------------------------------------------
	// ■影響信号線が不一致(普通に計算)
	//--------------------------------------------
	else{
		//-----------------------------------
		//入力が計算済み ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[0]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
			}
		}
		//-----------------------------------
		//正常値を代入
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
		
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//--------------------------------------------
		// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
		//--------------------------------------------
		if(t_net->in[i]->influence_net->n == stem_net->influence_net->n){
		
			//stem信号線と一致した回数のインクリメント
			n_equal_net++;

			//-----------------------------------
			//故障信号線の故障値を代入
			//-----------------------------------
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//故障信号線の反転フラグ通りに代入
				if(inv_flag == NO){
					t_net->x_fault &= stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault &= ~stem_net->x_fault;
				}
			}
			//入力の反転フラグYES
			else if(t_net->in[i]->inv_flag == YES){
				//故障信号線の反転フラグの否定で代入(※否定の否定)
				if(inv_flag == NO){
					t_net->x_fault &= ~stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault &= stem_net->x_fault;
				}
			}
		}

		//--------------------------------------------
		// ■影響信号線が不一致(普通に計算)
		//--------------------------------------------
		else{
			//-----------------------------------
			//入力が計算済み ⇒ 故障値で計算
			//-----------------------------------
			if(t_net->in[i]->influence_net->flag == ed_flag){
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault &= t_net->in[i]->influence_net->x_fault;
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault &= ~t_net->in[i]->influence_net->x_fault;
				}
			}
		
			//-----------------------------------
			//正常値で計算
			//-----------------------------------
			else{
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault &= t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault &= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
	}
	
	//================================================
	//出力結果を反転
	//================================================
	t_net->x_fault = ~t_net->x_fault;
	
	//================================================
	// 特殊演算実行回数上限のデクリメント
	//================================================
	if(n_equal_net != 0){
		(*func_flag)--;
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_or
//  機  能 : 【OR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_or(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){

	int				i;
	int				n_equal_net=0;	//stem信号線と一致した入力信号線数
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//--------------------------------------------
	// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//--------------------------------------------
	if(t_net->in[0]->influence_net->n == stem_net->influence_net->n){
		
		//stem信号線と一致した回数のインクリメント
		n_equal_net++;


		//-----------------------------------
		//故障信号線の故障値を代入
		//-----------------------------------
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//故障信号線の反転フラグ通りに代入
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;
			}
		}
		//入力の反転フラグYES
		else if(t_net->in[0]->inv_flag == YES){
			//故障信号線の反転フラグの否定で代入
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;	//(※否定の否定)
			}
		}
	}

	//--------------------------------------------
	// ■影響信号線が不一致(普通に計算)
	//--------------------------------------------
	else{
		//-----------------------------------
		//入力が計算済み ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[0]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
			}
		}
		//-----------------------------------
		//正常値を代入
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//--------------------------------------------
		// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
		//--------------------------------------------
		if(t_net->in[i]->influence_net->n == stem_net->influence_net->n){
		
			//stem信号線と一致した回数のインクリメント
			n_equal_net++;

			//-----------------------------------
			//故障信号線の故障値を代入
			//-----------------------------------
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//故障信号線の反転フラグ通りに代入
				if(inv_flag == NO){
					t_net->x_fault |= stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault |= ~stem_net->x_fault;
				}
			}
			//入力の反転フラグYES
			else if(t_net->in[i]->inv_flag == YES){
				//故障信号線の反転フラグの否定で代入(※否定の否定)
				if(inv_flag == NO){
					t_net->x_fault |= ~stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault |= stem_net->x_fault;
				}
			}
		}

		//--------------------------------------------
		// ■影響信号線が不一致(普通に計算)
		//--------------------------------------------
		else{
			//-----------------------------------
			//入力が計算済み ⇒ 故障値で計算
			//-----------------------------------
			if(t_net->in[i]->influence_net->flag == ed_flag){
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault |= t_net->in[i]->influence_net->x_fault;
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault |= ~t_net->in[i]->influence_net->x_fault;
				}
			}
		
			//-----------------------------------
			//正常値で計算
			//-----------------------------------
			else{
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault |= t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault |= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
	}

	//================================================
	// 特殊演算実行回数上限のデクリメント
	//================================================
	if(n_equal_net != 0){
		(*func_flag)--;
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_nor
//  機  能 : 【NOR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_nor(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){

	int				i;
	int				n_equal_net=0;	//stem信号線と一致した入力信号線数

	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//--------------------------------------------
	// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//--------------------------------------------
	if(t_net->in[0]->influence_net->n == stem_net->influence_net->n){
		
		//stem信号線と一致した回数のインクリメント
		n_equal_net++;

		//-----------------------------------
		//故障信号線の故障値を代入
		//-----------------------------------
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//故障信号線の反転フラグ通りに代入
			if(inv_flag == NO){
				t_net->x_fault = stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault;
			}
		}
		//入力の反転フラグYES
		else if(t_net->in[0]->inv_flag == YES){
			//故障信号線の反転フラグの否定で代入
			if(inv_flag == NO){
				t_net->x_fault = ~stem_net->x_fault;
			}
			else if(inv_flag == YES){
				t_net->x_fault = stem_net->x_fault;	//(※否定の否定)
			}
		}
	}

	//--------------------------------------------
	// ■影響信号線が不一致(普通に計算)
	//--------------------------------------------
	else{
		//-----------------------------------
		//入力が計算済み ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[0]->influence_net->flag == ed_flag){
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault;
			}
		}
		//-----------------------------------
		//正常値を代入
		//-----------------------------------
		else{
			//入力の反転フラグNO
			if(t_net->in[0]->inv_flag == NO){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
			//入力の反転フラグYES(xの否定)
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//--------------------------------------------
		// ■【※特殊計算】影響信号線が一致⇒故障信号線で計算!
		//--------------------------------------------
		if(t_net->in[i]->influence_net->n == stem_net->influence_net->n){
		
			//stem信号線と一致した回数のインクリメント
			n_equal_net++;

			//-----------------------------------
			//故障信号線の故障値を代入
			//-----------------------------------
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//故障信号線の反転フラグ通りに代入
				if(inv_flag == NO){
					t_net->x_fault |= stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault |= ~stem_net->x_fault;
				}
			}
			//入力の反転フラグYES
			else if(t_net->in[i]->inv_flag == YES){
				//故障信号線の反転フラグの否定で代入(※否定の否定)
				if(inv_flag == NO){
					t_net->x_fault |= ~stem_net->x_fault;
				}
				else if(inv_flag == YES){
					t_net->x_fault |= stem_net->x_fault;
				}
			}
		}

		//--------------------------------------------
		// ■影響信号線が不一致(普通に計算)
		//--------------------------------------------
		else{
			//-----------------------------------
			//入力が計算済み ⇒ 故障値で計算
			//-----------------------------------
			if(t_net->in[i]->influence_net->flag == ed_flag){
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault |= t_net->in[i]->influence_net->x_fault;
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault |= ~t_net->in[i]->influence_net->x_fault;
				}
			}
		
			//-----------------------------------
			//正常値で計算
			//-----------------------------------
			else{
				//入力の反転フラグNO
				if(t_net->in[i]->inv_flag == NO){
					t_net->x_fault |= t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
				//入力の反転フラグYES(xの否定)
				else if(t_net->in[i]->inv_flag == YES){
					t_net->x_fault |= ~t_net->in[i]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
	}

	//================================================
	//出力結果を反転
	//================================================
	t_net->x_fault = ~t_net->x_fault;

	//================================================
	// 特殊演算実行回数上限のデクリメント
	//================================================
	if(n_equal_net != 0){
		(*func_flag)--;
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_exor
//  機  能 : 【EXOR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値), stem_net(故障信号線), inv_flag(反転情報), func_flag(関数ポインタ切り替えフラグ)
//※2入力のみ対応
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_exor(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){

	//===========================================================
	// ■2入力とも【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//===========================================================
	if(t_net->in[0]->influence_net->n==stem_net->influence_net->n && t_net->in[1]->influence_net->n==stem_net->influence_net->n){
		
		//================================================
		// ★特殊演算実行回数上限のデクリメント
		//================================================
		(*func_flag)--;

		//===========================================================
		// stem信号線の反転フラグ=NO
		//===========================================================
		if(inv_flag == NO){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				t_net->x_fault = stem_net->x_fault ^ stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault ^ stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				t_net->x_fault = stem_net->x_fault ^ ~stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				t_net->x_fault = ~stem_net->x_fault ^ ~stem_net->x_fault;
			}
		}
		//===========================================================
		// stem信号線の反転フラグ=YES
		//===========================================================
		else if(inv_flag == YES){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				t_net->x_fault = ~stem_net->x_fault ^ ~stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = stem_net->x_fault ^ ~stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault ^ stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				t_net->x_fault = stem_net->x_fault ^ stem_net->x_fault;
			}
		}

	}

	//===========================================================
	// ■0番目入力のみ【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//===========================================================
	else if(t_net->in[0]->influence_net->n == stem_net->influence_net->n){
		
		//================================================
		// ★特殊演算実行回数上限のデクリメント
		//================================================
		(*func_flag)--;

		//===========================================================
		// stem信号線の反転フラグ=NO
		//===========================================================
		if(inv_flag == NO){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
		//===========================================================
		// stem信号線の反転フラグ=YES
		//===========================================================
		else if(inv_flag == YES){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
	}

	//===========================================================
	// ■1目入力のみ【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//===========================================================
	else if(t_net->in[1]->influence_net->n == stem_net->influence_net->n){
		
		//================================================
		// ★特殊演算実行回数上限のデクリメント
		//================================================
		(*func_flag)--;

		//===========================================================
		// stem信号線の反転フラグ=NO
		//===========================================================
		if(inv_flag == NO){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}
		}
		//===========================================================
		// stem信号線の反転フラグ=YES
		//===========================================================
		else if(inv_flag == YES){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}
		}
	}
	
	//===========================================================
	// ■2入力とも影響信号線が不一致⇒通常演算
	//===========================================================
	else{
		//===========================================================
		// 2入力とも【入力反転フラグNO】
		//===========================================================
		if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}

		//===========================================================
		// 0番目入力のみ【入力反転フラグYES】
		//===========================================================
		else if(t_net->in[0]->inv_flag == YES){
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}

		//===========================================================
		// 1番目入力のみ【入力反転フラグYES】(in[1]のxとpを入れ替えて演算)
		//===========================================================
		else if(t_net->in[1]->inv_flag == YES){
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}

		//===========================================================
		// 2入力とも【入力反転フラグYES】(in[0],in[1]のxとpを入れ替えて演算)
		//===========================================================
		else{
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}
	}


}

//------------------------------------------------------------------------------------
//  関数名 : SAF_2v_EX_ED_exnor
//  機  能 : 【EXNOR】2値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), ui_num(unsigned intの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
static void	SAF_2v_EX_ED_exnor(NLIST *t_net, int ui_num, unsigned int ed_flag, NLIST *stem_net, int inv_flag, int *func_flag){
	
	//===========================================================
	// ■2入力とも【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//===========================================================
	if(t_net->in[0]->influence_net->n==stem_net->influence_net->n && t_net->in[1]->influence_net->n==stem_net->influence_net->n){
		
		//================================================
		// ★特殊演算実行回数上限のデクリメント
		//================================================
		(*func_flag)--;

		//===========================================================
		// stem信号線の反転フラグ=NO
		//===========================================================
		if(inv_flag == NO){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				t_net->x_fault = stem_net->x_fault ^ stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault ^ stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				t_net->x_fault = stem_net->x_fault ^ ~stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				t_net->x_fault = ~stem_net->x_fault ^ ~stem_net->x_fault;
			}
		}
		//===========================================================
		// stem信号線の反転フラグ=YES
		//===========================================================
		else if(inv_flag == YES){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				t_net->x_fault = ~stem_net->x_fault ^ ~stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				t_net->x_fault = stem_net->x_fault ^ ~stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				t_net->x_fault = ~stem_net->x_fault ^ stem_net->x_fault;
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				t_net->x_fault = stem_net->x_fault ^ stem_net->x_fault;
			}
		}

	}

	//===========================================================
	// ■0番目入力のみ【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//===========================================================
	else if(t_net->in[0]->influence_net->n == stem_net->influence_net->n){
		
		//================================================
		// ★特殊演算実行回数上限のデクリメント
		//================================================
		(*func_flag)--;

		//===========================================================
		// stem信号線の反転フラグ=NO
		//===========================================================
		if(inv_flag == NO){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
		//===========================================================
		// stem信号線の反転フラグ=YES
		//===========================================================
		else if(inv_flag == YES){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//1番目入力が計算済み(1番目入力は故障値で計算)
				if(t_net->in[1]->influence_net->flag == ed_flag){
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = stem_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
				}
			}
		}
	}

	//===========================================================
	// ■1目入力のみ【※特殊計算】影響信号線が一致⇒故障信号線で計算!
	//===========================================================
	else if(t_net->in[1]->influence_net->n == stem_net->influence_net->n){
		
		//================================================
		// ★特殊演算実行回数上限のデクリメント
		//================================================
		(*func_flag)--;

		//===========================================================
		// stem信号線の反転フラグ=NO
		//===========================================================
		if(inv_flag == NO){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}
		}
		//===========================================================
		// stem信号線の反転フラグ=YES
		//===========================================================
		else if(inv_flag == YES){
			//--------------------------------------------------
			// 2入力とも【入力反転フラグNO】
			//--------------------------------------------------
			if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 0番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[0]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 1番目入力のみ【入力反転フラグYES】
			//--------------------------------------------------
			else if(t_net->in[1]->inv_flag == YES){
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}			
			//--------------------------------------------------
			// 2入力とも【入力反転フラグYES】
			//--------------------------------------------------
			else{
				//0番目入力が計算済み(0番目入力は故障値で計算)
				if(t_net->in[0]->influence_net->flag == ed_flag){
					t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ stem_net->x_fault;
				}
				//未計算(正常値で計算)
				else{
					t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ stem_net->x_fault;
				}
			}
		}
	}
	
	//===========================================================
	// ■2入力とも影響信号線が不一致⇒通常演算
	//===========================================================
	else{
		//===========================================================
		// 2入力とも【入力反転フラグNO】
		//===========================================================
		if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}

		//===========================================================
		// 0番目入力のみ【入力反転フラグYES】
		//===========================================================
		else if(t_net->in[0]->inv_flag == YES){
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}

		//===========================================================
		// 1番目入力のみ【入力反転フラグYES】(in[1]のxとpを入れ替えて演算)
		//===========================================================
		else if(t_net->in[1]->inv_flag == YES){
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}

		//===========================================================
		// 2入力とも【入力反転フラグYES】(in[0],in[1]のxとpを入れ替えて演算)
		//===========================================================
		else{
			//------------------------------------------------
			// 2入力とも計算済み⇒両方故障値で計算
			//------------------------------------------------
			if(t_net->in[0]->influence_net->flag==ed_flag && t_net->in[1]->influence_net->flag==ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->x_fault;
			}
			//------------------------------------------------
			// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[0]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->x_fault ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
	
			//------------------------------------------------
			// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
			//------------------------------------------------
			else if(t_net->in[1]->influence_net->flag == ed_flag){
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->x_fault;
			}
	
			//------------------------------------------------
			// 2入力とも正常値計算
			//------------------------------------------------
			else{
				t_net->x_fault = ~t_net->in[0]->influence_net->nval->x_buf[ui_num] ^ ~t_net->in[1]->influence_net->nval->x_buf[ui_num];
			}
		}
	}
	
	//===========================================================
	//出力結果を反転
	//===========================================================
	t_net->x_fault = ~t_net->x_fault;

}

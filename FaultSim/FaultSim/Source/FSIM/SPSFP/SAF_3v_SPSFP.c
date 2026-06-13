//------------------------------------------------------------------------
//File name : SAF_3v_SPPFP
//Date : 2012/2/19
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void		SAF_3v_spsfp_EDpush (NLIST*,int);

static void SAF_3v_ED_fout		(NLIST*, int, unsigned int);
static void SAF_3v_ED_buf		(NLIST*, int, unsigned int);
static void SAF_3v_ED_inv		(NLIST*, int, unsigned int);
static void SAF_3v_ED_and		(NLIST*, int, unsigned int);
static void SAF_3v_ED_nand		(NLIST*, int, unsigned int);
static void SAF_3v_ED_or		(NLIST*, int, unsigned int);
static void SAF_3v_ED_nor		(NLIST*, int, unsigned int);
static void SAF_3v_ED_exor		(NLIST*, int, unsigned int);
static void SAF_3v_ED_exnor		(NLIST*, int, unsigned int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define ED_DEBUG
	//#define ED_DEBUG2
	//#define CAL_DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//
//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_3v_SPSFP
//  機  能 : 縮退故障故障シミュレーション(SPSFP)
//  戻り値 : なし
//  引  数 : tp_id(テストパターンID, t_fault(故障信号線), fault_type(故障種類[SAF0 or SAF1]), ed_flag(イベントドリブン用フラグ)
//----------------------------------------------
void	SAF_3v_SPSFP(int tp_id, NLIST *t_fault, int fault_type, unsigned int ed_flag){


	int			i;
	int			level_flag=0;				//event_lev更新時にbreakしないようにする
	int			event_lev;					//イベント計算するレベル(※計算しながら更新)
	NLIST		*temp_net;					//レベライズスタックから取り出した信号線を一時的に格納
	int			bit_nval;					//信号線の正常値判定
	
#ifdef CAL_DEBUG
	int			j;
#endif
	
	
	//=========================================================
	// 関数ポインタ作成
	//=========================================================
	static void(*func[20])(NLIST*, int, unsigned int);
	func[FOUT]	= SAF_3v_ED_fout;
	func[BUF]	= SAF_3v_ED_buf;
	func[INV]	= SAF_3v_ED_inv;
	func[AND]	= SAF_3v_ED_and;
	func[NAND]	= SAF_3v_ED_nand;
	func[OR]	= SAF_3v_ED_or;
	func[NOR]	= SAF_3v_ED_nor;
	func[EXOR]	= SAF_3v_ED_exor;
	func[EXNOR]	= SAF_3v_ED_exnor;
	
	//================================================
	// 故障値設置
	//================================================		
#ifdef ED_DEBUG
	printf("故障:%s sa%d\n", t_fault->name, fault_type);
#endif
	
	//-------------------------------
	//故障信号線にEDフラグ設置
	//-------------------------------
	t_fault->flag = ed_flag;


	//★PO以外なら以下の処理を実行★
	if(t_fault->n_out != 0){

	//-------------------------------
	//(故障信号線の次イベントを)レベライズスタックへプッシュ
	//-------------------------------
	SAF_3v_spsfp_EDpush(t_fault, ed_flag);

	//-------------------------------
	// イベント開始レベルの決定
	//-------------------------------
	event_lev = t_fault->out[0]->level;	//初期値代入

	for(i=1; i<t_fault->n_out; i++){
		if(event_lev > t_fault->out[i]->level){
			event_lev = t_fault->out[i]->level;	//低レベルがあったら更新
		}
	}
		
	//-------------------------------
	// 0縮退故障の場合
	//-------------------------------
	if(fault_type == SAF0){

		//対象信号線の対象テストパターン番目の正常値を代入
		//正常値: 1
		t_fault->x_fault = 1;
		t_fault->p_fault = 0;

	}

	//-------------------------------
	// 1縮退故障場合
	//-------------------------------
	else if(fault_type == SAF1){

		//対象信号線の対象テストパターン番目の正常値を代入
		//正常値: 0
		t_fault->x_fault = 0;
		t_fault->p_fault = 1;
	}

#ifdef CAL_DEBUG
	printf("%s xf: %d ",t_fault->name, t_fault->x_fault);
	printf("%s pf: %d ",t_fault->name, t_fault->p_fault);
	printf("\n");
#endif
	

	//================================================
	// SPSFP故障シミュレーション(イベントドリブン)開始
	//================================================
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
		(*func[temp_net->type])(temp_net, tp_id, ed_flag);

#ifdef CAL_DEBUG
		printf("%s xf: %d ",temp_net->name, temp_net->x_fault);
		printf("%s pf: %d ",temp_net->name, temp_net->p_fault);
		printf("\n");
#endif

		//-------------------------------------------
		// イベントドリブン判定
		//-------------------------------------------
		//対象信号線の正常値を取得
		bit_nval = Get_NBit(temp_net->nval, tp_id);

		//■対象信号線の正常値0
		if(bit_nval == 0){
			//正常値≠故障値(故障伝搬)
			if(temp_net->x_fault==1 && temp_net->p_fault==0){	//故障値が1
				if(temp_net->n_out != 0){
					SAF_3v_spsfp_EDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
				}
				lev_temp[event_lev].n_net--;			//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
			//正常値=故障値(故障非伝搬)
			else{
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
		}

		//■対象信号線の正常値1
		else if(bit_nval == 1){
			//正常値≠故障値(故障伝搬)
			if(temp_net->x_fault==0 && temp_net->p_fault==1){	//故障値が0
				SAF_3v_spsfp_EDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
			//正常値=故障値(故障非伝搬)
			else{
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
		}
		//■対象信号線の正常値X
		else if(bit_nval == 3){
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

#ifdef CAL_DEBUG
	printf("\n//-----------------------------------\n");
	printf("// DEBUG: SPSFP\n");
	printf("//-----------------------------------\n");

	for(i=0; i<max_level+1; i++){
		for(j=0; j<nml_lev[i].n_net; j++){
			//故障設置信号線
			if(nml_lev[i].net[j]->flag == ed_flag){
				printf("%s 故障伝搬\n",nml_lev[i].net[j]->name);
			}
		}
		printf("\n");
	}

	printf("\n");
#endif
	}

}
//***********************************************************************************************
//----------------------------------------------
//  関数名 : SAF_3v_spsfp_EDpush
//  機  能 : イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//  memo   : ※PPSFPのとは少し違うので注意！
//----------------------------------------------
void SAF_3v_spsfp_EDpush(NLIST *e_net,int ed_flag){
	
	int				i;

#ifdef ED_DEBUG
	int				j;
#endif
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:lev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, lev_temp[i].n_net);
		for(j=0; j<lev_temp[i].n_net; j++){
			printf("%s (flag:%d)\n", lev_temp[i].net[j]->name, lev_temp[i].net[j]->flag);
		}
		printf("\n");
	}
#endif
	
	//=========================================================================
	// イベントドリブンスタックに対象信号線の【代表信号線】出力を全てプッシュ
	//=========================================================================
	for(i=0; i<e_net->n_out; i++){	//出力代表信号線数分ループ
	
		//対象信号線の出力信号線が　計算済みで無い　&&　計算候補フラグも立ってない
		if(e_net->out[i]->flag!=ed_flag && e_net->out[i]->flag!=(ed_flag-1)){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->out[i]->level].net[lev_temp[e_net->out[i]->level].n_net] = e_net->out[i];
			
			//出力信号線に『ed_flag-1』を代入
			e_net->out[i]->flag = ed_flag-1;

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->out[i]->level].n_net++;
		}			

	}
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG
	printf("//-------------------------------\n");
	printf("// DEBUG:挿入後lev_temp\n");
	printf("//-------------------------------\n");
	for(i=0; i<max_level+1; i++){
		printf("[%d] 信号線数:%d\n", i, lev_temp[i].n_net);
		for(j=0; j<lev_temp[i].n_net; j++){
			printf("%s (flag:%d)\n", lev_temp[i].net[j]->name, lev_temp[i].net[j]->flag);
		}
		printf("\n");
	}
	
	printf("\n");
#endif

}

//******************************************************************************************************
// 関数ポインタさんの中身
//******************************************************************************************************
//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_fout
//  機  能 : 【FOUT】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_fout(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	//そのまま引き継ぎ
	//================================================
	t_net->x_fault = t_net->in[0]->x_fault;
	t_net->p_fault = t_net->in[0]->p_fault;
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_buf
//  機  能 : 【BUF】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_buf(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	//そのまま引き継ぎ
	//================================================
	t_net->x_fault = t_net->in[0]->x_fault;
	t_net->p_fault = t_net->in[0]->p_fault;
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_inv
//  機  能 : 【INV】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_inv(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	// INV処理(入れ替える)
	//================================================
	t_net->x_fault = t_net->in[0]->p_fault;
	t_net->p_fault = t_net->in[0]->x_fault;

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_and
//  機  能 : 【AND】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_and(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	int				bit_nval;	//tp_id番目の各信号線正常値保持
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag == ed_flag){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->nval, tp_id);

		//正常値=0
		if(bit_nval == 0){
			t_net->x_fault = 0;
			t_net->p_fault = 1;
		}
		//正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = 1;
			t_net->p_fault = 0;
		}
		//正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->flag == ed_flag){
			t_net->x_fault &= t_net->in[i]->x_fault;
			t_net->p_fault |= t_net->in[i]->p_fault;
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->nval, tp_id);

			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault &= 0;
				t_net->p_fault |= 1;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault &= 1;
				t_net->p_fault |= 0;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault &= 1;
				t_net->p_fault |= 1;
			}

		}
	}	

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_nand
//  機  能 : 【NAND】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_nand(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	unsigned int	not_x;	//x_faultの反転値を一時保存
	int				bit_nval;	//tp_id番目の各信号線正常値保持
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag == ed_flag){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->nval, tp_id);

		//正常値=0
		if(bit_nval == 0){
			t_net->x_fault = 0;
			t_net->p_fault = 1;
		}
		//正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = 1;
			t_net->p_fault = 0;
		}
		//正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->flag == ed_flag){
			t_net->x_fault &= t_net->in[i]->x_fault;
			t_net->p_fault |= t_net->in[i]->p_fault;
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->nval, tp_id);

			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault &= 0;
				t_net->p_fault |= 1;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault &= 1;
				t_net->p_fault |= 0;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault &= 1;
				t_net->p_fault |= 1;
			}

		}
	}
	
	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_or
//  機  能 : 【OR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_or(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	int				bit_nval;	//tp_id番目の各信号線正常値保持
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag == ed_flag){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->nval, tp_id);

		//正常値=0
		if(bit_nval == 0){
			t_net->x_fault = 0;
			t_net->p_fault = 1;
		}
		//正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = 1;
			t_net->p_fault = 0;
		}
		//正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->flag == ed_flag){
			t_net->x_fault |= t_net->in[i]->x_fault;
			t_net->p_fault &= t_net->in[i]->p_fault;
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->influence_net->nval, tp_id);

			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault |= 0;
				t_net->p_fault &= 1;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault |= 1;
				t_net->p_fault &= 0;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault |= 1;
				t_net->p_fault &= 1;
			}
		}
	}
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_nor
//  機  能 : 【NOR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_nor(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	unsigned int	not_x;	//x_faultの反転値を一時保存
	int				bit_nval;	//tp_id番目の各信号線正常値保持

	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag == ed_flag){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->nval, tp_id);

		//正常値=0
		if(bit_nval == 0){
			t_net->x_fault = 0;
			t_net->p_fault = 1;
		}
		//正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = 1;
			t_net->p_fault = 0;
		}
		//正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->flag == ed_flag){
			t_net->x_fault |= t_net->in[i]->x_fault;
			t_net->p_fault &= t_net->in[i]->p_fault;
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->influence_net->nval, tp_id);

			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault |= 0;
				t_net->p_fault &= 1;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault |= 1;
				t_net->p_fault &= 0;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault |= 1;
				t_net->p_fault &= 1;
			}
		}
	}

	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_exor
//  機  能 : 【EXOR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_exor(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	int				bit_nval;	//tp_id番目の各信号線正常値保持

	//===========================================================
	// 2入力とも計算済み⇒両方故障値で計算
	//===========================================================
	if(t_net->in[0]->flag==ed_flag && t_net->in[1]->flag==ed_flag){
		t_net->x_fault = (t_net->in[0]->x_fault & t_net->in[1]->p_fault) | (t_net->in[0]->p_fault & t_net->in[1]->x_fault);
		t_net->p_fault = (t_net->in[0]->p_fault | t_net->in[1]->x_fault) & (t_net->in[0]->x_fault | t_net->in[1]->p_fault);
	}
	
	//===========================================================
	// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
	//===========================================================
	else if(t_net->in[0]->flag == ed_flag){
		
		//1番目正常値を取得
		bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

		//1番目正常値=0
		if(bit_nval == 0){
			t_net->x_fault = (t_net->in[0]->x_fault & 1) | (t_net->in[0]->p_fault & 0);
			t_net->p_fault = (t_net->in[0]->p_fault | 0) & (t_net->in[0]->x_fault | 1);
		}
		//1番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (t_net->in[0]->x_fault & 0) | (t_net->in[0]->p_fault & 1);
			t_net->p_fault = (t_net->in[0]->p_fault | 1) & (t_net->in[0]->x_fault | 0);
		}
		//1番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//===========================================================
	// 1番目入力のみ計算済み(0番目入力のみ故障値で計算)
	//===========================================================
	else if(t_net->in[1]->flag == ed_flag){

		//0番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

		//0番目正常値=0
		if(bit_nval == 0){	
			t_net->x_fault = (0 & t_net->in[1]->p_fault) | (1 & t_net->in[1]->x_fault);
			t_net->p_fault = (1 | t_net->in[1]->x_fault) & (0 | t_net->in[1]->p_fault);
		}
		//0番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (1 & t_net->in[1]->p_fault) | (0 & t_net->in[1]->x_fault);
			t_net->p_fault = (0 | t_net->in[1]->x_fault) & (1 | t_net->in[1]->p_fault);
		}
		//0番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//===========================================================
	// 2入力とも正常値計算
	//===========================================================
	else{

		//EXOR出力信号線の正常値を取得
		bit_nval = Get_NBit(t_net->nval, tp_id);

		//EXOR出力信号線正常値=0
		if(bit_nval == 0){
			t_net->x_fault = 0;
			t_net->p_fault = 1;
		}
		//EXOR出力信号線正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = 1;
			t_net->p_fault = 0;
		}
		//EXOR出力信号線正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_exnor
//  機  能 : 【EXNOR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_exnor(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	unsigned int	not_x;	//x_faultの反転値を一時保存
	int				bit_nval;	//tp_id番目の各信号線正常値保持

	//===========================================================
	// 2入力とも計算済み⇒両方故障値で計算
	//===========================================================
	if(t_net->in[0]->flag==ed_flag && t_net->in[1]->flag==ed_flag){
		t_net->x_fault = (t_net->in[0]->x_fault & t_net->in[1]->p_fault) | (t_net->in[0]->p_fault & t_net->in[1]->x_fault);
		t_net->p_fault = (t_net->in[0]->p_fault | t_net->in[1]->x_fault) & (t_net->in[0]->x_fault | t_net->in[1]->p_fault);
	}
	
	//===========================================================
	// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
	//===========================================================
	else if(t_net->in[0]->flag == ed_flag){
		
		//1番目正常値を取得
		bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

		//1番目正常値=0
		if(bit_nval == 0){
			t_net->x_fault = (t_net->in[0]->x_fault & 1) | (t_net->in[0]->p_fault & 0);
			t_net->p_fault = (t_net->in[0]->p_fault | 0) & (t_net->in[0]->x_fault | 1);
		}
		//1番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (t_net->in[0]->x_fault & 0) | (t_net->in[0]->p_fault & 1);
			t_net->p_fault = (t_net->in[0]->p_fault | 1) & (t_net->in[0]->x_fault | 0);
		}
		//1番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//===========================================================
	// 1番目入力のみ計算済み(0番目入力のみ故障値で計算)
	//===========================================================
	else if(t_net->in[1]->flag == ed_flag){

		//0番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

		//0番目正常値=0
		if(bit_nval == 0){	
			t_net->x_fault = (0 & t_net->in[1]->p_fault) | (1 & t_net->in[1]->x_fault);
			t_net->p_fault = (1 | t_net->in[1]->x_fault) & (0 | t_net->in[1]->p_fault);
		}
		//0番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (1 & t_net->in[1]->p_fault) | (0 & t_net->in[1]->x_fault);
			t_net->p_fault = (0 | t_net->in[1]->x_fault) & (1 | t_net->in[1]->p_fault);
		}
		//0番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}
	
	//===========================================================
	// 2入力とも正常値計算
	//===========================================================
	else{

		//EXOR出力信号線の正常値を取得
		bit_nval = Get_NBit(t_net->nval, tp_id);

		//EXOR出力信号線正常値=0
		if(bit_nval == 0){
			t_net->x_fault = 0;
			t_net->p_fault = 1;
		}
		//EXOR出力信号線正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = 1;
			t_net->p_fault = 0;
		}
		//EXOR出力信号線正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = 1;
			t_net->p_fault = 1;
		}
	}

	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	

}

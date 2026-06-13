//------------------------------------------------------------------------
//File name : SAF_3v_repSPPFP
//Date : 2012/2/19
//Designer : H.Yamazaki
//Ver : 0.01
//★2/21非代表信号線が故障信号線の場合バグ発生！★
//　⇒　後日アルゴリズム見直し予定
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
void		dtob				(unsigned int, int);
void		SAF_3v_sppfp_repEDpush	(NLIST*,int);
void		SAF_3v_sppfp_repED_fault_push	(NLIST*,int);

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
//	ed_flag-2	: スタックに積んだ信号線(≠故障信号線)
//  ed_flag-1	: 故障設置信号線
//	ed_flag		: 故障設置信号線&&スタックに積んだ信号線
//
//------------------------------------------------------------------------
//  静的変数
//------------------------------------------------------------------------
static  unsigned int     all_zero = 0;				//32bit全て『0』
static  unsigned int     all_one  = 0xFFFFFFFF;		//32bit全て『1』

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_3v_repSPPFP
//  機  能 : 【代表信号線使用】縮退故障故障シミュレーション(SPPFP)
//  戻り値 : なし
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ)
//----------------------------------------------
void	SAF_3v_repSPPFP(int tp_id, XID_LIST **t_fault, int n_fault, unsigned int ed_flag){


	int			i,j;
	int			level_flag=0;				//event_lev更新時にbreakしないようにする
	int			event_lev = max_level;		//イベント計算するレベル(※計算しながら更新)
	NLIST		*temp_net;					//レベライズスタックから取り出した信号線を一時的に格納
	int			bit_nval;					//信号線の正常値判定
	
	
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
	for(i=0; i<n_fault; i++){
		
#ifdef ED_DEBUG
		printf("故障[%d]:%s sa%d\n", i, t_fault[i]->net->name, t_fault[i]->fault_type);
#endif

		//-------------------------------
		//(故障信号線の次イベントを)レベライズスタックへプッシュ
		//-------------------------------
		SAF_3v_sppfp_repED_fault_push(t_fault[i]->net, ed_flag);

		//-------------------------------
		// イベント開始レベルの決定
		//-------------------------------
		for(j=0; j<t_fault[i]->net->n_next_event; j++){
			if(event_lev > t_fault[i]->net->next_event[i]->level){
				event_lev = t_fault[i]->net->next_event[i]->level;
			}
		}
		

		//-------------------------------
		// 0縮退故障の場合
		//-------------------------------
		if(t_fault[i]->fault_type == SAF0){

			//対象信号線の対象テストパターン番目の正常値を代入
			//正常値: 1
			t_fault[i]->net->x_fault = all_one;
			t_fault[i]->net->p_fault = all_zero;

			//故障値ビット目(i番目)のみ故障値に変更
			t_fault[i]->net->x_fault ^= MASKbit[i];	//x_fault  EXOR  MASKbit[i]
			t_fault[i]->net->p_fault ^= MASKbit[i];	//p_fault  EXOR  MASKbit[i]

			//ネットリストの故障情報格納
			t_fault[i]->net->nbit_fault = i;	//i番目割当て
			//t_fault[i]->net->ftype = SAF0;		//0縮退故障
		}

		//-------------------------------
		// 1縮退故障場合
		//-------------------------------
		else if(t_fault[i]->fault_type == SAF1){

			//対象信号線の対象テストパターン番目の正常値を代入
			//正常値: 0
			t_fault[i]->net->x_fault = all_zero;
			t_fault[i]->net->p_fault = all_one;
			
			//故障値ビット目(i番目)のみ故障値に変更
			t_fault[i]->net->x_fault ^= MASKbit[i];	//x_fault  EXOR  MASKbit[i]
			t_fault[i]->net->p_fault ^= MASKbit[i];	//p_fault  EXOR  MASKbit[i]

			//ネットリストの故障情報格納
			t_fault[i]->net->nbit_fault = i;	//i番目割当て
			//t_fault[i]->net->ftype = SAF1;		//1縮退故障
		}
#ifdef CAL_DEBUG
		printf("%s xf: ",t_fault[i]->net->name);
		dtob(t_fault[i]->net->x_fault,n_fault);
		printf("%s pf: ",t_fault[i]->net->name);
		dtob(t_fault[i]->net->p_fault,n_fault);
		printf("\n");
#endif
	}
	

	//================================================
	// SPPFP故障シミュレーション(イベントドリブン)開始
	//================================================
	while(event_lev != (max_level+1) ){

		//-------------------------------------------
		// レベライズスタックから信号線を取り出す
		//-------------------------------------------
		temp_net = lev_temp[event_lev].net[lev_temp[event_lev].n_net-1];	//ケツから取り出す
		
		//-------------------------------------------
		// temp_netの故障値計算
		//-------------------------------------------
		(*func[temp_net->type])(temp_net, tp_id, ed_flag);
#ifdef CAL_DEBUG
		printf("%s xf: ",temp_net->name);
		dtob(temp_net->x_fault, n_fault);
		printf("%s pf: ",temp_net->name);
		dtob(temp_net->p_fault, n_fault);
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
			if(temp_net->x_fault!=all_zero ||temp_net->p_fault!=all_one){
				if(temp_net->n_out != 0){
					SAF_3v_sppfp_repEDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
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
			if(temp_net->x_fault!=all_one ||temp_net->p_fault!=all_zero){
				SAF_3v_sppfp_repEDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
			//正常値=故障値(故障非伝搬)
			else{
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
			}
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
	printf("// DEBUG: SPPFP\n");
	printf("//-----------------------------------\n");

	for(i=0; i<max_level+1; i++){
		for(j=0; j<rep_lev[i].n_net; j++){
			//故障設置信号線
			if(rep_lev[i].net[j]->flag == ed_flag){
				printf("%s 故障\n",rep_lev[i].net[j]->name);
			}
			//プッシュされた信号線
			else if(rep_lev[i].net[j]->flag == (ed_flag-1)){
				printf("%s プッシュ\n",rep_lev[i].net[j]->name);
			}
			//故障設置信号線+プッシュ
			if(rep_lev[i].net[j]->flag == (ed_flag+1)){
				printf("%s 故障プッシュ\n",rep_lev[i].net[j]->name);
			}
		}
		printf("\n");
	}
	printf("\n");
	for(i=0; i<n_po; i++){
		printf("%s\n",po[i]->name);
		bit_nval = Get_NBit(po[i]->nval, tp_id);
		if(bit_nval == 0){
			printf(" nval: 0\n");
		}
		else if(bit_nval == 1){
			printf(" nval: 1\n");
		}
		else if(bit_nval == 3){
			printf(" nval: X\n");
		}

		printf(" x-f : ");
		dtob(po[i]->x_fault, n_fault);
		printf(" p-f : ");
		dtob(po[i]->p_fault, n_fault);
	}
	
	printf("\n");
#endif


}
//***********************************************************************************************
//----------------------------------------------
//  関数名 : SAF_3v_sppfp_repED_fault_push
//  機  能 : 【代表信号線】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//  memo   : ※PPSFPのとは少し違うので注意！
//			　【故障信号線をの時使用(最初の32回のみ)】
//----------------------------------------------
void SAF_3v_sppfp_repED_fault_push(NLIST *e_net,int ed_flag){
	
	int				i;

#ifdef ED_DEBUG2
	int				j;
#endif
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG2
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
	// 自分に故障信号線フラグを立てる
	//=========================================================================
	//自分が他故障信号線の次イベントになっていた場合
	if(e_net->flag == ed_flag-1){
		e_net->flag = ed_flag+1;	//故障設置信号線&&スタックに積んだ信号線
	}
	else{
		e_net->flag = ed_flag;		//故障設置信号線
	}

	//=========================================================================
	// イベントドリブンスタックに対象信号線の【代表信号線】出力を全てプッシュ
	//=========================================================================
	for(i=0; i<e_net->n_next_event; i++){	//出力代表信号線数分ループ
	
		//------------------------------------------------
		//出力信号線が故障設置信号線の場合
		//------------------------------------------------
		if(e_net->next_event[i]->flag == ed_flag){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->next_event[i]->level].net[lev_temp[e_net->next_event[i]->level].n_net] = e_net->next_event[i];
			
			//flagが『故障設置信号線&&スタックに積んだ信号線』
			e_net->next_event[i]->flag = ed_flag+1;

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->next_event[i]->level].n_net++;
		}
		
		//------------------------------------------------
		//出力信号線がまだプッシュされてない場合
		//------------------------------------------------
		else if(e_net->next_event[i]->flag!=ed_flag-1 && e_net->next_event[i]->flag!=ed_flag+1){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->next_event[i]->level].net[lev_temp[e_net->next_event[i]->level].n_net] = e_net->next_event[i];
			
			//故障設置信号線の場合『ed_flag+1』
			e_net->next_event[i]->flag = ed_flag-1;

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->next_event[i]->level].n_net++;
		}	

	}
	
	//=========================================================================
	// 【DEBUG】lev_temp表示
	//=========================================================================
#ifdef ED_DEBUG2
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

//----------------------------------------------
//  関数名 : SAF_3v_sppfp_repEDpush
//  機  能 : 【代表信号線】イベントドリブン用スタック(lev_temp で管理)
//  戻り値 : なし
//  引  数 : e_net(計算候補ネットリストポインタ), ed_flag(イベントドリブン用計算フラグ値)
//  memo   : ※PPSFPのとは少し違うので注意！
//			　【故障信号線以外の時使用】
//----------------------------------------------
void SAF_3v_sppfp_repEDpush(NLIST *e_net,int ed_flag){
	
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
	for(i=0; i<e_net->n_next_event; i++){	//出力代表信号線数分ループ
	
		//対象信号線の出力信号線がプッシュされてない場合(ed_flag-1, ed_flag+1でない)
		if(e_net->next_event[i]->flag!=(ed_flag-1) && e_net->next_event[i]->flag!=(ed_flag+1)){

			//lev_tempに出力信号線をプッシュ
			lev_temp[e_net->next_event[i]->level].net[lev_temp[e_net->next_event[i]->level].n_net] = e_net->next_event[i];
			
			//故障設置信号線の場合『ed_flag+1』
			if(e_net->next_event[i]->flag == ed_flag){
				e_net->next_event[i]->flag = ed_flag+1;
			}
			//ただの信号線の場合『ed_flag-1』
			else{
				e_net->next_event[i]->flag = ed_flag-1;
			}

			//lev_tempの保持信号線数をインクリメント
			lev_temp[e_net->next_event[i]->level].n_net++;
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

	//※POでFOUTの場合は『代表信号線』になるため必要
	//================================================
	// 【入力の反転フラグNO】x_faultとp_faultをそのまま引き継ぐ
	//================================================
	if(t_net->in[0]->inv_flag == NO){
		t_net->x_fault = t_net->influence_net->x_fault;
		t_net->p_fault = t_net->influence_net->p_fault;
	}

	//================================================
	// 【入力の反転フラグYES】x_faultとp_faultを入れ替えて引き継ぐ
	//================================================
	else if(t_net->in[0]->inv_flag == YES){
		t_net->x_fault = t_net->influence_net->p_fault;
		t_net->p_fault = t_net->influence_net->x_fault;
	}
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_buf
//  機  能 : 【BUF】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_buf(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	// 【入力の反転フラグNO】x_faultとp_faultをそのまま引き継ぐ
	//================================================
	if(t_net->in[0]->inv_flag == NO){
		t_net->x_fault = t_net->influence_net->x_fault;
		t_net->p_fault = t_net->influence_net->p_fault;
	}

	//================================================
	// 【入力の反転フラグYES】x_faultとp_faultを入れ替えて引き継ぐ
	//================================================
	else if(t_net->in[0]->inv_flag == YES){
		t_net->x_fault = t_net->influence_net->p_fault;
		t_net->p_fault = t_net->influence_net->x_fault;
	}
		
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_ED_inv
//  機  能 : 【INV】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
static void	SAF_3v_ED_inv(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	// 【入力の反転フラグNO】INV処理(入れ替える)
	//================================================
	if(t_net->in[0]->inv_flag == NO){
		t_net->x_fault = t_net->influence_net->x_fault;
		t_net->p_fault = t_net->influence_net->p_fault;
	}

	//================================================
	// 【入力の反転フラグYES】否定の否定⇒BUFと同じ処理(入れ替えない)
	//================================================
	else if(t_net->in[0]->inv_flag == YES){
		t_net->x_fault = t_net->influence_net->x_fault;
		t_net->p_fault = t_net->influence_net->p_fault;
	}
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
	}
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
	if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==(ed_flag-1) || t_net->in[0]->influence_net->flag==(ed_flag+1)){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			t_net->p_fault = t_net->in[0]->influence_net->p_fault;
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = t_net->in[0]->influence_net->p_fault;
			t_net->p_fault = t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag==ed_flag || t_net->in[i]->influence_net->flag==(ed_flag-1 || t_net->in[i]->influence_net->flag==(ed_flag+1))){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault &= t_net->in[i]->influence_net->x_fault;
				t_net->p_fault |= t_net->in[i]->influence_net->p_fault;
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault &= t_net->in[i]->influence_net->p_fault;
				t_net->p_fault |= t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->influence_net->nval, tp_id);
			
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault &= all_zero;
					t_net->p_fault |= all_one;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_zero;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_one;
				}
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_zero;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault &= all_zero;
					t_net->p_fault |= all_one;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_one;
				}
			}
		}
	}
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
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
	if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==(ed_flag-1) || t_net->in[0]->influence_net->flag==(ed_flag+1)){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			t_net->p_fault = t_net->in[0]->influence_net->p_fault;
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = t_net->in[0]->influence_net->p_fault;
			t_net->p_fault = t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag==ed_flag || t_net->in[i]->influence_net->flag==(ed_flag-1) || t_net->in[i]->influence_net->flag==(ed_flag+1)){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault &= t_net->in[i]->influence_net->x_fault;
				t_net->p_fault |= t_net->in[i]->influence_net->p_fault;
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault &= t_net->in[i]->influence_net->p_fault;
				t_net->p_fault |= t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->influence_net->nval, tp_id);

			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault &= all_zero;
					t_net->p_fault |= all_one;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_zero;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_one;
				}
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_zero;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault &= all_zero;
					t_net->p_fault |= all_one;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault &= all_one;
					t_net->p_fault |= all_one;
				}
			}
		}
	}
	
	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
	}
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
	if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==(ed_flag-1) || t_net->in[0]->influence_net->flag==(ed_flag+1)){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			t_net->p_fault = t_net->in[0]->influence_net->p_fault;
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = t_net->in[0]->influence_net->p_fault;
			t_net->p_fault = t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag==ed_flag || t_net->in[i]->influence_net->flag==(ed_flag-1) || t_net->in[i]->influence_net->flag==(ed_flag+1)){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault |= t_net->in[i]->influence_net->x_fault;
				t_net->p_fault &= t_net->in[i]->influence_net->p_fault;
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault |= t_net->in[i]->influence_net->p_fault;
				t_net->p_fault &= t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->influence_net->nval, tp_id);

			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault |= all_zero;
					t_net->p_fault &= all_one;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_zero;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_one;
				}
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_zero;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault |= all_zero;
					t_net->p_fault &= all_one;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_one;
				}
			}
		}
	}
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
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
	if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==(ed_flag-1) || t_net->in[0]->influence_net->flag==(ed_flag+1)){
		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			t_net->x_fault = t_net->in[0]->influence_net->x_fault;
			t_net->p_fault = t_net->in[0]->influence_net->p_fault;
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			t_net->x_fault = t_net->in[0]->influence_net->p_fault;
			t_net->p_fault = t_net->in[0]->influence_net->x_fault;
		}
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

		//入力の反転フラグNO
		if(t_net->in[0]->inv_flag == NO){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
		//入力の反転フラグYES(xとpを入れ替えて代入)
		else if(t_net->in[0]->inv_flag == YES){
			//正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み ⇒ 故障値で計算
		//-----------------------------------
		if(t_net->in[i]->influence_net->flag==ed_flag || t_net->in[i]->influence_net->flag==(ed_flag-1) || t_net->in[i]->influence_net->flag==(ed_flag+1)){
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				t_net->x_fault |= t_net->in[i]->influence_net->x_fault;
				t_net->p_fault &= t_net->in[i]->influence_net->p_fault;
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				t_net->x_fault |= t_net->in[i]->influence_net->p_fault;
				t_net->p_fault &= t_net->in[i]->influence_net->x_fault;
			}
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->influence_net->nval, tp_id);
			
			//入力の反転フラグNO
			if(t_net->in[i]->inv_flag == NO){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault |= all_zero;
					t_net->p_fault &= all_one;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_zero;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_one;
				}
			}
			//入力の反転フラグYES(xとpを入れ替えて代入)
			else if(t_net->in[i]->inv_flag == YES){
				//正常値=0
				if(bit_nval == 0){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_zero;
				}
				//正常値=1
				else if(bit_nval == 1){
					t_net->x_fault |= all_zero;
					t_net->p_fault &= all_one;
				}
				//正常値=X
				else if(bit_nval == 3){
					t_net->x_fault |= all_one;
					t_net->p_fault &= all_one;
				}
			}
		}
	}

	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
	}
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
	// 2入力とも【反転フラグNO】
	//===========================================================
	if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->p_fault) | (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->x_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->x_fault) & (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->p_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_one) | (t_net->in[0]->influence_net->p_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_zero) & (t_net->in[0]->influence_net->x_fault | all_one);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_zero) | (t_net->in[0]->influence_net->p_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_one) & (t_net->in[0]->influence_net->x_fault | all_zero);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){	
				t_net->x_fault = (all_zero & t_net->in[1]->influence_net->p_fault) | (all_one & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_one | t_net->in[1]->influence_net->x_fault) & (all_zero | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (all_one & t_net->in[1]->influence_net->p_fault) | (all_zero & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_zero | t_net->in[1]->influence_net->x_fault) & (all_one | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			
			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}

	//===========================================================
	// 0番目入力のみ【反転フラグYES】(in[0]のxとpを入れ替えて演算)
	//===========================================================
	else if(t_net->in[0]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->p_fault) | (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->x_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->x_fault) & (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->p_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_one) | (t_net->in[0]->influence_net->x_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_zero) & (t_net->in[0]->influence_net->p_fault | all_one);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_zero) | (t_net->in[0]->influence_net->x_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_one) & (t_net->in[0]->influence_net->p_fault | all_zero);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){	
				t_net->x_fault = (all_one & t_net->in[1]->influence_net->p_fault) | (all_zero & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_zero | t_net->in[1]->influence_net->x_fault) & (all_one | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (all_zero & t_net->in[1]->influence_net->p_fault) | (all_one & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_one | t_net->in[1]->influence_net->x_fault) & (all_zero | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}

	//===========================================================
	// 1番目入力のみ【反転フラグYES】(in[1]のxとpを入れ替えて演算)
	//===========================================================
	else if(t_net->in[1]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->x_fault) | (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->p_fault) & (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->x_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_zero) | (t_net->in[0]->influence_net->p_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_one) & (t_net->in[0]->influence_net->x_fault | all_zero);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_one) | (t_net->in[0]->influence_net->p_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_zero) & (t_net->in[0]->influence_net->x_fault | all_one);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){
			t_net->x_fault = (all_zero & t_net->in[1]->influence_net->x_fault) | (all_one & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (all_one | t_net->in[1]->influence_net->p_fault) & (all_zero | t_net->in[1]->influence_net->x_fault);
}
			//0番目正常値=1
			else if(bit_nval == 1){
			t_net->x_fault = (all_one & t_net->in[1]->influence_net->x_fault) | (all_zero & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (all_zero | t_net->in[1]->influence_net->p_fault) & (all_one | t_net->in[1]->influence_net->x_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{

			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}		}
	}

	//===========================================================
	// 2入力とも【反転フラグYES】(in[0],in[1]のxとpを入れ替えて演算)
	//===========================================================
	else{
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->x_fault) | (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->p_fault) & (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->x_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_zero) | (t_net->in[0]->influence_net->x_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_one) & (t_net->in[0]->influence_net->p_fault | all_zero);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_one) | (t_net->in[0]->influence_net->x_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_zero) & (t_net->in[0]->influence_net->p_fault | all_one);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
		
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (all_one & t_net->in[1]->influence_net->x_fault) | (all_zero & t_net->in[1]->influence_net->p_fault);
				t_net->p_fault = (all_zero | t_net->in[1]->influence_net->p_fault) & (all_one | t_net->in[1]->influence_net->x_fault);
			}
			//0番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (all_zero & t_net->in[1]->influence_net->x_fault) | (all_one & t_net->in[1]->influence_net->p_fault);
				t_net->p_fault = (all_one | t_net->in[1]->influence_net->p_fault) & (all_zero | t_net->in[1]->influence_net->x_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
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
	// 2入力とも【反転フラグNO】
	//===========================================================
	if(t_net->in[0]->inv_flag==NO && t_net->in[1]->inv_flag==NO){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->p_fault) | (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->x_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->x_fault) & (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->p_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_one) | (t_net->in[0]->influence_net->p_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_zero) & (t_net->in[0]->influence_net->x_fault | all_one);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_zero) | (t_net->in[0]->influence_net->p_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_one) & (t_net->in[0]->influence_net->x_fault | all_zero);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){	
				t_net->x_fault = (all_zero & t_net->in[1]->influence_net->p_fault) | (all_one & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_one | t_net->in[1]->influence_net->x_fault) & (all_zero | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (all_one & t_net->in[1]->influence_net->p_fault) | (all_zero & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_zero | t_net->in[1]->influence_net->x_fault) & (all_one | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			
			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}

	//===========================================================
	// 0番目入力のみ【反転フラグYES】(in[0]のxとpを入れ替えて演算)
	//===========================================================
	else if(t_net->in[0]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->p_fault) | (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->x_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->x_fault) & (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->p_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_one) | (t_net->in[0]->influence_net->x_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_zero) & (t_net->in[0]->influence_net->p_fault | all_one);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_zero) | (t_net->in[0]->influence_net->x_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_one) & (t_net->in[0]->influence_net->p_fault | all_zero);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){	
				t_net->x_fault = (all_one & t_net->in[1]->influence_net->p_fault) | (all_zero & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_zero | t_net->in[1]->influence_net->x_fault) & (all_one | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (all_zero & t_net->in[1]->influence_net->p_fault) | (all_one & t_net->in[1]->influence_net->x_fault);
				t_net->p_fault = (all_one | t_net->in[1]->influence_net->x_fault) & (all_zero | t_net->in[1]->influence_net->p_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}

	//===========================================================
	// 1番目入力のみ【反転フラグYES】(in[1]のxとpを入れ替えて演算)
	//===========================================================
	else if(t_net->in[1]->inv_flag == YES){
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->x_fault) | (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->p_fault) & (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->x_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_zero) | (t_net->in[0]->influence_net->p_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_one) & (t_net->in[0]->influence_net->x_fault | all_zero);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->x_fault & all_one) | (t_net->in[0]->influence_net->p_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->p_fault | all_zero) & (t_net->in[0]->influence_net->x_fault | all_one);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){
			t_net->x_fault = (all_zero & t_net->in[1]->influence_net->x_fault) | (all_one & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (all_one | t_net->in[1]->influence_net->p_fault) & (all_zero | t_net->in[1]->influence_net->x_fault);
}
			//0番目正常値=1
			else if(bit_nval == 1){
			t_net->x_fault = (all_one & t_net->in[1]->influence_net->x_fault) | (all_zero & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (all_zero | t_net->in[1]->influence_net->p_fault) & (all_one | t_net->in[1]->influence_net->x_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{

			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}		}
	}

	//===========================================================
	// 2入力とも【反転フラグYES】(in[0],in[1]のxとpを入れ替えて演算)
	//===========================================================
	else{
		//------------------------------------------------
		// 2入力とも計算済み⇒両方故障値で計算
		//------------------------------------------------
		if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1 && t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
			t_net->x_fault = (t_net->in[0]->influence_net->p_fault & t_net->in[1]->influence_net->x_fault) | (t_net->in[0]->influence_net->x_fault & t_net->in[1]->influence_net->p_fault);
			t_net->p_fault = (t_net->in[0]->influence_net->x_fault | t_net->in[1]->influence_net->p_fault) & (t_net->in[0]->influence_net->p_fault | t_net->in[1]->influence_net->x_fault);
		}
		//------------------------------------------------
		// 0番目入力のみ計算済み(0番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[0]->influence_net->flag==ed_flag || t_net->in[0]->influence_net->flag==ed_flag-1 || t_net->in[0]->influence_net->flag==ed_flag+1){
			
			//1番目正常値を取得
			bit_nval = Get_NBit(t_net->in[1]->influence_net->nval, tp_id);

			//1番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_zero) | (t_net->in[0]->influence_net->x_fault & all_one);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_one) & (t_net->in[0]->influence_net->p_fault | all_zero);
			}
			//1番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (t_net->in[0]->influence_net->p_fault & all_one) | (t_net->in[0]->influence_net->x_fault & all_zero);
				t_net->p_fault = (t_net->in[0]->influence_net->x_fault | all_zero) & (t_net->in[0]->influence_net->p_fault | all_one);
			}
			//1番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 1番目入力のみ計算済み(1番目入力のみ故障値で計算)
		//------------------------------------------------
		else if(t_net->in[1]->influence_net->flag==ed_flag || t_net->in[1]->influence_net->flag==ed_flag-1 || t_net->in[1]->influence_net->flag==ed_flag+1){
		
			//0番目正常値を取得
			bit_nval = Get_NBit(t_net->in[0]->influence_net->nval, tp_id);

			//0番目正常値=0
			if(bit_nval == 0){
				t_net->x_fault = (all_one & t_net->in[1]->influence_net->x_fault) | (all_zero & t_net->in[1]->influence_net->p_fault);
				t_net->p_fault = (all_zero | t_net->in[1]->influence_net->p_fault) & (all_one | t_net->in[1]->influence_net->x_fault);
			}
			//0番目正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = (all_zero & t_net->in[1]->influence_net->x_fault) | (all_one & t_net->in[1]->influence_net->p_fault);
				t_net->p_fault = (all_one | t_net->in[1]->influence_net->p_fault) & (all_zero | t_net->in[1]->influence_net->x_fault);
			}
			//0番目正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	
		//------------------------------------------------
		// 2入力とも正常値計算
		//------------------------------------------------
		else{
			//EXOR出力信号線の正常値を取得
			bit_nval = Get_NBit(t_net->nval, tp_id);

			//EXOR出力信号線正常値=0
			if(bit_nval == 0){
				t_net->x_fault = all_zero;
				t_net->p_fault = all_one;
			}
			//EXOR出力信号線正常値=1
			else if(bit_nval == 1){
				t_net->x_fault = all_one;
				t_net->p_fault = all_zero;
			}
			//EXOR出力信号線正常値=X
			else if(bit_nval == 3){
				t_net->x_fault = all_one;
				t_net->p_fault = all_one;
			}
		}
	}

	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	
	//================================================
	// 故障信号線の場合(32故障の内の1つ)
	//================================================
	if(t_net->flag == (ed_flag+1)){
		//故障値ビット目のみ正常値反転
		t_net->x_fault ^= MASKbit[t_net->nbit_fault];	//x_fault  EXOR  MASKbit[故障bit目]
		t_net->p_fault ^= MASKbit[t_net->nbit_fault];	//p_fault  EXOR  MASKbit[故障bit目]
	}
}

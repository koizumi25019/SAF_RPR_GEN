//------------------------------------------------------------------------
//File name : SAF_3v_XID_SPPFP
//Date : 2013/2/19
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
void	dtob					(unsigned int, int);
void	SAF_EDpush				(NLIST*,unsigned int);
int		Set_SAF_XID_fdictionary	(int, XID_LIST**, int, int);

void	SAF_3v_xid_sppfp_ED_fout	(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_buf		(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_inv		(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_and		(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_nand	(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_or		(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_nor		(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_exor	(NLIST*, int, unsigned int);
void	SAF_3v_xid_sppfp_ED_exnor	(NLIST*, int, unsigned int);
//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define TP_DEBUG
	//#define FSET_DEBUG
	//#define ED_DEBUG
	//#define ED_DEBUG2
	//#define CAL_DEBUG
	//#define FDIC_DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//
//  ed_flag-1	: 計算候補信号線(そのうち計算される信号線)
//	ed_flag		: 計算済み信号線
//	※最初に故障設置信号線=ed_flagは代入しない！ (一番最後に代入)
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
//  関数名 : FPointer_SAF_3v_SPPFP
//  機  能 : 【関数ポインタ作成】X抽出後テストパターン用の3値SPPFP関数ポインタ作成
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	FPointer_SAF_3v_XID_SPPFP(){
	
	//=========================================================
	// 関数ポインタ作成
	//=========================================================
	func_3v_sppfp[FOUT]		= SAF_3v_xid_sppfp_ED_fout;
	func_3v_sppfp[BUF]		= SAF_3v_xid_sppfp_ED_buf;
	func_3v_sppfp[INV]		= SAF_3v_xid_sppfp_ED_inv;
	func_3v_sppfp[AND]		= SAF_3v_xid_sppfp_ED_and;
	func_3v_sppfp[NAND]		= SAF_3v_xid_sppfp_ED_nand;
	func_3v_sppfp[OR]		= SAF_3v_xid_sppfp_ED_or;
	func_3v_sppfp[NOR]		= SAF_3v_xid_sppfp_ED_nor;
	func_3v_sppfp[EXOR]		= SAF_3v_xid_sppfp_ED_exor;
	func_3v_sppfp[EXNOR]	= SAF_3v_xid_sppfp_ED_exnor;
}

//----------------------------------------------
//  関数名 : SAF_3v_XID_SPPFP
//  機  能 : X抽出後テストパターン{0,1,X}に対する縮退故障故障シミュレーション(SPPFP)
//  戻り値 : n_det_fault(検出故障数)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ)
//----------------------------------------------
int	SAF_3v_XID_SPPFP(int tp_id, XID_LIST **t_fault, int n_fault, unsigned int ed_flag){
	
	int			i,j;
	int			level_flag = 0;				//event_lev更新時にbreakしないようにする
	int			event_lev = max_level;		//イベント計算するレベル(※計算しながら更新)
	NLIST		*temp_net;					//レベライズスタックから取り出した信号線を一時的に格納
	int			bit_nval;					//信号線の正常値判定
	int			n_det_fault=0;				//検出故障数(1故障1回カウント: 0〜32)

	//==========================================================================================
	// DEBUG:テストパターン表示
	//==========================================================================================
#ifdef TP_DEBUG
	printf("\nX抽出後テストパターン[%d] 対象故障数%d\n", tp_id, n_fault);
	
	for(i=0; i<n_pi; i++){
		//--------------------------------------------
		//X抽出後テストパターン{0,1,X}を確認(xid_nval)
		//--------------------------------------------
		j = Get_NBit(pi[i]->xid_nval, tp_id);

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
	printf("\n");


#endif
	
	//==========================================================================================
	// 1〜32故障値設置
	//==========================================================================================
	for(i=0; i<n_fault; i++){
		
#ifdef FSET_DEBUG
		printf("故障[%d]:%s sa%d\n", i, t_fault[i]->net->name, t_fault[i]->fault_type);
#endif
		//----------------------------------------------
		//(故障信号線の次イベントを)レベライズスタックへプッシュ
		//----------------------------------------------
		if(t_fault[i]->net->n_out > 0){		//PO以外
			SAF_EDpush(t_fault[i]->net, ed_flag);	//故障信号線の出力にflag= ed_flag-1
		}
		
		//----------------------------------------------
		// 0縮退故障の場合
		//----------------------------------------------
		if(t_fault[i]->fault_type == SAF0){

			//対象信号線の対象テストパターン番目の正常値を代入
			//正常値: 1
			t_fault[i]->net->x_fault = all_one;
			t_fault[i]->net->p_fault = all_zero;

			//ネットリストの故障情報格納
			t_fault[i]->net->nbit_fault = i;	//i番目割当て
		}
		
		//----------------------------------------------
		// 1縮退故障場合
		//----------------------------------------------
		else if(t_fault[i]->fault_type == SAF1){

			//対象信号線の対象テストパターン番目の正常値を代入
			//正常値: 0
			t_fault[i]->net->x_fault = all_zero;
			t_fault[i]->net->p_fault = all_one;

			//ネットリストの故障情報格納
			t_fault[i]->net->nbit_fault = i;	//i番目割当て
		}
		
		//----------------------------------------------
		// 対象信号線がPOの場合特殊処理
		//----------------------------------------------
		//POでflag-1が設置されない場合
		if(t_fault[i]->net->n_out==0 && t_fault[i]->net->flag != (ed_flag-1) ){
			
			//POをイベントドリブンスタックに積む
			lev_temp[t_fault[i]->net->level].net[lev_temp[t_fault[i]->net->level].n_net] = t_fault[i]->net;
			lev_temp[t_fault[i]->net->level].n_net++;

			//計算候補フラグを設置
			t_fault[i]->net->flag = ed_flag-1;

			//各入力信号線の故障値初期化
			for(j=0; j<t_fault[i]->net->n_in; j++){
				if(t_fault[i]->net->in[j]->flag != (ed_flag-1) ){

					//入力の正常値を得る
					bit_nval = Get_NBit(t_fault[i]->net->in[j]->xid_nval, tp_id);

					//入力の正常値：0
					if(bit_nval == 0){
						t_fault[i]->net->in[j]->x_fault = all_zero;
						t_fault[i]->net->in[j]->p_fault = all_one;
					}
					//入力の正常値:1
					else if(bit_nval == 1){
						t_fault[i]->net->in[j]->x_fault = all_one;
						t_fault[i]->net->in[j]->p_fault = all_zero;
					}
					//入力の正常値:X
					else if(bit_nval == 3){
						t_fault[i]->net->in[j]->x_fault = all_one;
						t_fault[i]->net->in[j]->p_fault = all_one;
					}
				}
			}
		}
		//----------------------------------------------
		// 故障値設定
		//----------------------------------------------
		//PO以外 or 対象POがスタックに積まれている場合
		else{
			//故障値ビット目(i番目)のみ故障値に変更
			t_fault[i]->net->x_fault ^= MASKbit[i];	//x_fault  EXOR  MASKbit[i]
			t_fault[i]->net->p_fault ^= MASKbit[i];	//p_fault  EXOR  MASKbit[i]
		}
	}
	
	
	//==========================================================================================
	// イベントドリブン開始レベルの決定
	//==========================================================================================
	for(i=0; i<max_level+1; i++){
		//保持信号線数≠0のレベルスタック発見
		if(lev_temp[i].n_net != 0){
			event_lev = i;	//イベント開始レベルを決定
			break;
		}
	}
	
#ifdef TP_DEBUG

	for(i=0; i<n_net; i++){
		printf("%s (ed_flag=%d)\n", nl[i].name, nl[i].flag);
		printf("%s xf: ",nl[i].name);
		dtob(nl[i].x_fault, n_fault);
		printf("%s pf: ",nl[i].name);
		dtob(nl[i].p_fault, n_fault);
		printf("\n");
	}
		printf("\n");
#endif

	//==========================================================================================
	// SPPFP故障シミュレーション(イベントドリブン)開始
	//==========================================================================================
#ifdef TP_DEBUG
		printf("\n//-------------------------------------\n");
		printf("// DEBUG: X抽出後SPPFPのイベントドリブン\n");
		printf("//-------------------------------------\n");
#endif
	while(event_lev != (max_level+1) ){

		//-------------------------------------------
		// レベライズスタックから信号線を取り出す
		//-------------------------------------------
		temp_net = lev_temp[event_lev].net[lev_temp[event_lev].n_net-1];	//ケツから取り出す
		
		//-------------------------------------------
		// 計算済みフラグを代入
		//-------------------------------------------
		temp_net->flag = ed_flag;
#ifdef TP_DEBUG
		printf("●%s (ed_flag=%d)\n", temp_net->name, temp_net->flag);
#endif
		//-------------------------------------------
		// temp_netの故障値計算
		//-------------------------------------------
		//ゲート別演算
		(*func_3v_sppfp[temp_net->type])(temp_net, tp_id, ed_flag);

		//★対象信号線が故障設置信号線の場合
		if(temp_net->nbit_fault != -1){
			temp_net->x_fault ^= MASKbit[temp_net->nbit_fault];	//故障値を再度設定
			temp_net->p_fault ^= MASKbit[temp_net->nbit_fault];	//故障値を再度設定
		}

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
		//対象信号線の正常値{0,1,X}を取得
		bit_nval = Get_NBit(temp_net->xid_nval, tp_id);

		//==========================================================================
		//■対象信号線の正常値0
		//==========================================================================
		if(bit_nval == 0){
			//故障値==0 (故障非伝搬)
			if(temp_net->x_fault==all_zero && temp_net->p_fault==all_one){
				lev_temp[event_lev].n_net--;		//現在レベルのレベライズスタックの保持信号線数をデクリメント
				temp_net->flag = ed_flag-1;		//フラグ値を『ed_flag - 1』に変更！(故障伝搬してないため)
#ifdef TP_DEBUG
				printf("フラグ修正%s (ed_flag=%d)\n", temp_net->name, temp_net->flag);
#endif
			}
			//故障値==1 || 故障値==X
			else{
				//----------------------------------------
				//対象信号線がPOの場合⇒検出PO情報更新
				//----------------------------------------
				if(temp_net->n_out == 0){
					//現在レベルのレベライズスタックの保持信号線数をデクリメント
					lev_temp[event_lev].n_net--;

					//対象POで検出可能な故障をチェック
					for(i=0; i<n_fault; i++){
						if( (temp_net->x_fault & MASKbit[i])!=0 && (temp_net->p_fault & MASKbit[i])==0){		//対象bitのXbuff=1 && Pbuff=0
							
							//X抽出後用の故障辞書内容作成
							n_det_fault += Set_SAF_XID_fdictionary(tp_id, t_fault, i, temp_net->po_id);
						}
					}
				}
				//----------------------------------------
				//PO以外⇒プッシュ
				//----------------------------------------
				else{
					SAF_EDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
					lev_temp[event_lev].n_net--;			//現在レベルのレベライズスタックの保持信号線数をデクリメント
				}
			}
		}
		
		//==========================================================================
		//■対象信号線の正常値1
		//==========================================================================
		else if(bit_nval == 1){
			//故障値==1 (故障非伝搬)
			if(temp_net->x_fault==all_one && temp_net->p_fault==all_zero){
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
				temp_net->flag = ed_flag-1;		//フラグ値を『ed_flag - 1』に変更！(故障伝搬してないため)
#ifdef TP_DEBUG
				printf("フラグ修正%s (ed_flag=%d)\n", temp_net->name, temp_net->flag);
#endif
			}
			//故障値==0 || 故障値==X
			else{
				//----------------------------------------
				//対象信号線がPOの場合⇒検出PO情報更新
				//----------------------------------------
				if(temp_net->n_out == 0){
					//現在レベルのレベライズスタックの保持信号線数をデクリメント
					lev_temp[event_lev].n_net--;

					//対象POで検出可能な故障をチェック
					for(i=0; i<n_fault; i++){
						if( (temp_net->x_fault & MASKbit[i])==0  && (temp_net->p_fault & MASKbit[i])!=0){		//対象bitのXbuff=0 && Pbuff=1
							
							//X抽出後用の故障辞書内容作成
							n_det_fault += Set_SAF_XID_fdictionary(tp_id, t_fault, i, temp_net->po_id);
						}
					}
				}
				//----------------------------------------
				//PO以外⇒プッシュ
				//----------------------------------------
				else{
					SAF_EDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
					lev_temp[event_lev].n_net--;			//現在レベルのレベライズスタックの保持信号線数をデクリメント
				}
			}
		}
		//==========================================================================
		//■対象信号線の正常値X
		//==========================================================================
		else if(bit_nval == 3){
			//故障値==X (故障非伝搬)
			if(temp_net->x_fault==all_one && temp_net->p_fault==all_one){
				lev_temp[event_lev].n_net--;	//現在レベルのレベライズスタックの保持信号線数をデクリメント
				temp_net->flag = ed_flag-1;		//フラグ値を『ed_flag - 1』に変更！(故障伝搬してないため)
#ifdef TP_DEBUG
				printf("フラグ修正%s (ed_flag=%d)\n", temp_net->name, temp_net->flag);
#endif
			}
			//故障値==0 || 故障値==X
			else{
				//----------------------------------------
				//対象信号線がPOの場合(X/0, X/1は故障検出は不可能)
				//----------------------------------------
				if(temp_net->n_out == 0){
					//現在レベルのレベライズスタックの保持信号線数をデクリメント
					lev_temp[event_lev].n_net--;					
				}
				//----------------------------------------
				//PO以外⇒プッシュ
				//----------------------------------------
				else{
					SAF_EDpush(temp_net, ed_flag);	//temp_netの出力をプッシュ
					lev_temp[event_lev].n_net--;			//現在レベルのレベライズスタックの保持信号線数をデクリメント
				}
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
	

	//==========================================================================================
	// 32故障に対してフラグ最終処理
	//==========================================================================================
	for(i=0; i<n_fault; i++){
		//------------------------------------
		//FSIM対象故障フラグのリセット
		//------------------------------------
		t_fault[i]->net->nbit_fault = -1;
		
		//------------------------------------
		//故障伝搬フラグ設置(ed_flag値を設定)
		//------------------------------------
		if(t_fault[i]->net->flag != ed_flag){
			t_fault[i]->net->flag = ed_flag;
		}
		else if(t_fault[i]->net->flag!=ed_flag && t_fault[i]->net->flag!=(ed_flag-1)){
			printf("//----------------------------------------\n");
			printf("// ERROR ? : SAF_3v_SPPFP\n");
			printf("//----------------------------------------\n");
			printf("%s 故障信号線なのに謎フラグ値だよ！\n", t_fault[i]->net->name);  //フラグ値の説明は次の行にあるため%dを削除
			printf("※要解析 (ed_falg=%d, ed_flag-1=%d)\n\n", ed_flag, ed_flag-1);
		}
	}
	
	//==========================================================================================
	// DEBUG
	//==========================================================================================
#ifdef CAL_DEBUG
	printf("\n//-----------------------------------\n");
	printf("// DEBUG: X抽出後SPPFP\n");
	printf("//-----------------------------------\n");

	for(i=0; i<max_level+1; i++){
		for(j=0; j<nml_lev[i].n_net; j++){
			//故障設置信号線
			if(nml_lev[i].net[j]->flag == ed_flag){
				printf("%s 故障\n",nml_lev[i].net[j]->name);
			}
			//プッシュされた信号線
			else if(nml_lev[i].net[j]->flag == (ed_flag-1)){
				printf("%s プッシュ\n",nml_lev[i].net[j]->name);
			}
		}
		printf("\n");
	}
	printf("\n");
	for(i=0; i<n_po; i++){
		printf("%s\n",po[i]->name);
		bit_nval = Get_NBit(po[i]->xid_nval, tp_id);
		if(bit_nval == 0){
			printf(" xid_nval: 0\n");
		}
		else if(bit_nval == 1){
			printf(" xid_nval: 1\n");
		}
		else if(bit_nval == 3){
			printf(" xid_nval: X\n");
		}

		printf(" x-f : ");
		dtob(po[i]->x_fault, n_fault);
		printf(" p-f : ");
		dtob(po[i]->p_fault, n_fault);
	}
	
	printf("\n");
#endif

	return n_det_fault;
}

//----------------------------------------------
//  関数名 : Set_SAF_XID_fdictionary
//  機  能 : X抽出後テストパターン{0,1,X}に対する故障辞書内容の作成
//  戻り値 : n_det_fault(検出故障数)
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), f_id(検出故障番号(0〜31)), po_id(故障検出POのID)
//----------------------------------------------
int	Set_SAF_XID_fdictionary(int tp_id, XID_LIST **t_fault, int f_id, int po_id){

	int	n_det_fault=0;

	//======================================================
	// 0縮退故障が検出
	//======================================================
	if(t_fault[f_id]->fault_type == SAF0){

		//0縮退故障辞書にフラグ立て
		Set_NINT_One(xid_fdic_sa0[tp_id], t_fault[f_id]->net->n);
		
		//X抽出後テスト集合での検出故障数更新
		if(t_fault[f_id]->net->xid_det_sf0 == NO){			
			t_fault[f_id]->net->xid_det_sf0 = YES;	//X抽出後テストパターンで0縮退故障を検出
			n_xid_detect++;							//X抽出後テスト集合での検出故障数更新
			n_det_fault++;							//対象故障のうち検出した故障数
		}

#ifdef FDIC_DEBUG
		printf("XTP%d: %s SAF0 検出\n", tp_id, t_fault[f_id]->net->name);
#endif

	}

	//======================================================
	// 1縮退故障が検出
	//======================================================
	else if(t_fault[f_id]->fault_type == SAF1){

		//1縮退故障辞書にフラグ立て
		Set_NINT_One(xid_fdic_sa1[tp_id], t_fault[f_id]->net->n);
				
		//X抽出後テスト集合での検出故障数更新
		if(t_fault[f_id]->net->xid_det_sf1 == NO){			
			t_fault[f_id]->net->xid_det_sf1 = YES;	//X抽出後テストパターンで1縮退故障を検出
			n_xid_detect++;							//X抽出後テスト集合での検出故障数更新
			n_det_fault++;							//対象故障のうち検出した故障数
		}
		
#ifdef FDIC_DEBUG
		printf("XTP%d: %s SAF1 検出\n", tp_id, t_fault[f_id]->net->name);
#endif

	}

	//======================================================
	// ERROR
	//======================================================
	else{
		printf("\n//---------------------------------\n");
		printf("// ERROR: Set_SAF_XID_fdictionary()\n");
		printf("//---------------------------------\n");
		printf("0縮退でも1縮退でもない故障タイプになってます！\n");
		exit(-1);
	}

	return n_det_fault;
}


//******************************************************************************************************
// 関数ポインタさんの中身 (★xid_nvalを見るため別関数を用意★)
//******************************************************************************************************
//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_fout
//  機  能 : 【FOUT】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_fout(NLIST *t_net, int tp_id, unsigned int ed_flag){

	//================================================
	//そのまま引き継ぎ
	//================================================
	t_net->x_fault = t_net->in[0]->x_fault;
	t_net->p_fault = t_net->in[0]->p_fault;

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_buf
//  機  能 : 【BUF】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_buf(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	//そのまま引き継ぎ
	//================================================
	t_net->x_fault = t_net->in[0]->x_fault;
	t_net->p_fault = t_net->in[0]->p_fault;

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_inv
//  機  能 : 【INV】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_inv(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	//================================================
	//入れ替える
	//================================================
	t_net->x_fault = t_net->in[0]->p_fault;
	t_net->p_fault = t_net->in[0]->x_fault;

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_and
//  機  能 : 【AND】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_and(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	int				bit_nval;	//tp_id番目の各信号線正常値保持
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->xid_nval, tp_id);

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
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[i]->flag==ed_flag || t_net->in[i]->nbit_fault!=-1){
			t_net->x_fault &= t_net->in[i]->x_fault;
			t_net->p_fault |= t_net->in[i]->p_fault;
			
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);
			
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
	}

}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_sppfp_ED_nand
//  機  能 : 【NAND】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_nand(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	unsigned int	not_x;	//x_faultの反転値を一時保存
	int				bit_nval;	//tp_id番目の各信号線正常値保持
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->xid_nval, tp_id);

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
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[i]->flag==ed_flag || t_net->in[i]->nbit_fault!=-1){
			t_net->x_fault &= t_net->in[i]->x_fault;
			t_net->p_fault |= t_net->in[i]->p_fault;
			
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);
			
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
	}
	
	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_or
//  機  能 : 【OR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_or(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	int				bit_nval;	//tp_id番目の各信号線正常値保持
	
	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->xid_nval, tp_id);

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
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[i]->flag==ed_flag || t_net->in[i]->nbit_fault!=-1){
			t_net->x_fault |= t_net->in[i]->x_fault;
			t_net->p_fault &= t_net->in[i]->p_fault;
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);

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
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_nor
//  機  能 : 【NOR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_nor(NLIST *t_net, int tp_id, unsigned int ed_flag){

	int				i;
	unsigned int	not_x;	//x_faultの反転値を一時保存
	int				bit_nval;	//tp_id番目の各信号線正常値保持

	//================================================
	//入力の0番目の影響信号線値代入
	//================================================
	//-----------------------------------
	//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
	//-----------------------------------
	if(t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1){
		t_net->x_fault = t_net->in[0]->x_fault;
		t_net->p_fault = t_net->in[0]->p_fault;
	}
	//-----------------------------------
	//正常値を代入
	//-----------------------------------
	else{
		//対象信号線のtp_id番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->xid_nval, tp_id);

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
	
	//================================================
	//残りの入力の影響信号線値計算
	//================================================
	for(i=1; i<t_net->n_in; i++){
		
		//-----------------------------------
		//入力が計算済み || 故障設置信号線 ⇒ 故障値を代入
		//-----------------------------------
		if(t_net->in[i]->flag==ed_flag || t_net->in[i]->nbit_fault!=-1){
			t_net->x_fault |= t_net->in[i]->x_fault;
			t_net->p_fault &= t_net->in[i]->p_fault;
		}
		
		//-----------------------------------
		//正常値で計算
		//-----------------------------------
		else{
			//対象信号線のtp_id番目正常値を取得
			bit_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);

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
	}

	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;
	
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_exor
//  機  能 : 【EXOR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_exor(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	int				bit_nval;	//tp_id番目の各信号線正常値保持

	//------------------------------------------------
	// 2入力とも計算済み || 故障設置信号線 ⇒両方故障値で計算
	//------------------------------------------------
	if( (t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1) && (t_net->in[1]->flag==ed_flag || t_net->in[1]->nbit_fault!=-1) ){
		t_net->x_fault = (t_net->in[0]->x_fault & t_net->in[1]->p_fault) | (t_net->in[0]->p_fault & t_net->in[1]->x_fault);
		t_net->p_fault = (t_net->in[0]->p_fault | t_net->in[1]->x_fault) & (t_net->in[0]->x_fault | t_net->in[1]->p_fault);
	}
	//------------------------------------------------
	// 0番目入力のみ計算済み || 故障設置信号線 (0番目入力のみ故障値で計算)
	//------------------------------------------------
	else if(t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1){
			
		//1番目正常値を取得
		bit_nval = Get_NBit(t_net->in[1]->xid_nval, tp_id);

		//1番目正常値=0
		if(bit_nval == 0){
			t_net->x_fault = (t_net->in[0]->x_fault & all_one) | (t_net->in[0]->p_fault & all_zero);
			t_net->p_fault = (t_net->in[0]->p_fault | all_zero) & (t_net->in[0]->x_fault | all_one);
		}
		//1番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (t_net->in[0]->x_fault & all_zero) | (t_net->in[0]->p_fault & all_one);
			t_net->p_fault = (t_net->in[0]->p_fault | all_one) & (t_net->in[0]->x_fault | all_zero);
		}
		//1番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = all_one;
			t_net->p_fault = all_one;
		}
	}
	
	//------------------------------------------------
	// 1番目入力のみ計算済み || 故障設置信号線 (1番目入力のみ故障値で計算)
	//------------------------------------------------
	else if(t_net->in[1]->flag==ed_flag || t_net->in[1]->nbit_fault!=-1){
			
		//0番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->xid_nval, tp_id);

		//0番目正常値=0
		if(bit_nval == 0){	
			t_net->x_fault = (all_zero & t_net->in[1]->p_fault) | (all_one & t_net->in[1]->x_fault);
			t_net->p_fault = (all_one | t_net->in[1]->x_fault) & (all_zero | t_net->in[1]->p_fault);
		}
		//0番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (all_one & t_net->in[1]->p_fault) | (all_zero & t_net->in[1]->x_fault);
			t_net->p_fault = (all_zero | t_net->in[1]->x_fault) & (all_one | t_net->in[1]->p_fault);
		}
		//0番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = all_one;
			t_net->p_fault = all_one;
		}
	}
	
	//------------------------------------------------
	// 2入力とも正常値計算 (動作的にあり得ないような…)
	//------------------------------------------------
	else{
		printf("ERROR: EXORの入力が両方とも正常値\n");
	}
}

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_xid_sppfp_ED_exnor
//  機  能 : 【EXNOR】3値縮退故障イベントドリブン(故障値計算)
//  戻り値 : なし
//  引  数 : t_net(対象信号線), tp_id(テストパターンの何番目か), ed_flag(現在のイベントフラグ値)
//※2入力のみ対応
//------------------------------------------------------------------------------------
void	SAF_3v_xid_sppfp_ED_exnor(NLIST *t_net, int tp_id, unsigned int ed_flag){
	
	unsigned int	not_x;	//x_faultの反転値を一時保存
	int				bit_nval;	//tp_id番目の各信号線正常値保持

	//------------------------------------------------
	// 2入力とも計算済み || 故障設置信号線 ⇒両方故障値で計算
	//------------------------------------------------
	if( (t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1) && (t_net->in[1]->flag==ed_flag || t_net->in[1]->nbit_fault!=-1) ){
		t_net->x_fault = (t_net->in[0]->x_fault & t_net->in[1]->p_fault) | (t_net->in[0]->p_fault & t_net->in[1]->x_fault);
		t_net->p_fault = (t_net->in[0]->p_fault | t_net->in[1]->x_fault) & (t_net->in[0]->x_fault | t_net->in[1]->p_fault);
	}
	//------------------------------------------------
	// 0番目入力のみ計算済み || 故障設置信号線 (0番目入力のみ故障値で計算)
	//------------------------------------------------
	else if(t_net->in[0]->flag==ed_flag || t_net->in[0]->nbit_fault!=-1){
			
		//1番目正常値を取得
		bit_nval = Get_NBit(t_net->in[1]->xid_nval, tp_id);

		//1番目正常値=0
		if(bit_nval == 0){
			t_net->x_fault = (t_net->in[0]->x_fault & all_one) | (t_net->in[0]->p_fault & all_zero);
			t_net->p_fault = (t_net->in[0]->p_fault | all_zero) & (t_net->in[0]->x_fault | all_one);
		}
		//1番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (t_net->in[0]->x_fault & all_zero) | (t_net->in[0]->p_fault & all_one);
			t_net->p_fault = (t_net->in[0]->p_fault | all_one) & (t_net->in[0]->x_fault | all_zero);
		}
		//1番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = all_one;
			t_net->p_fault = all_one;
		}
	}
	
	//------------------------------------------------
	// 1番目入力のみ計算済み || 故障設置信号線 (1番目入力のみ故障値で計算)
	//------------------------------------------------
	else if(t_net->in[1]->flag==ed_flag || t_net->in[1]->nbit_fault!=-1){
			
		//0番目正常値を取得
		bit_nval = Get_NBit(t_net->in[0]->xid_nval, tp_id);

		//0番目正常値=0
		if(bit_nval == 0){	
			t_net->x_fault = (all_zero & t_net->in[1]->p_fault) | (all_one & t_net->in[1]->x_fault);
			t_net->p_fault = (all_one | t_net->in[1]->x_fault) & (all_zero | t_net->in[1]->p_fault);
		}
		//0番目正常値=1
		else if(bit_nval == 1){
			t_net->x_fault = (all_one & t_net->in[1]->p_fault) | (all_zero & t_net->in[1]->x_fault);
			t_net->p_fault = (all_zero | t_net->in[1]->x_fault) & (all_one | t_net->in[1]->p_fault);
		}
		//0番目正常値=X
		else if(bit_nval == 3){
			t_net->x_fault = all_one;
			t_net->p_fault = all_one;
		}
	}
	
	//------------------------------------------------
	// 2入力とも正常値計算 (動作的にあり得ないような…)
	//------------------------------------------------
	else{
		printf("ERROR: EXORの入力が両方とも正常値\n");
	}

	//================================================
	//出力結果を反転(x_fault と p_faultを入れ替える)
	//================================================
	not_x = t_net->x_fault;				//x_faultを一時保存
	t_net->x_fault = t_net->p_fault;
	t_net->p_fault = not_x;

}
//------------------------------------------------------------------------
//File name : DEBUG_TDF_SPPFP.c
//Date : 2013/10/7
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	<string.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	TDF_2v_SPPFP						(int, SXID_LIST**, int, unsigned int);
void	TDF_3v_SPPFP						(int, SXID_LIST**, int, unsigned int);
void	DEBUG_TDF_SPPFP_Fault_Dictionay		(int, SXID_LIST**, int, unsigned int);
void	FPointer_TDF_2v_SPPFP				();
void	FPointer_TDF_3v_SPPFP				();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG
	//#define FLT_DEBUG
	
//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : DEBUG_TDF_SPPFP
//  機  能 : 【DEBUG関数】SPPFP故障シミュレーター
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	DEBUG_TDF_SPPFP(){


	int				i,j,k;
	unsigned int	ed_flag=2;		//イベントドリブンフラグ
	int				count=0;		//t_fault内に何個必須故障を入れたかのカウンタ
	SXID_LIST		**t_fault;		//故障リスト配列(MAX32故障)
	int				nval_t1;		//1時刻目正常値
	int				nval_t2;		//2時刻目正常値
	
#ifdef BUG
	FILE			*fp;
	fp = fopen("BUG_3v_sppfp.txt","w");
#endif

#ifdef DEBUG
	printf("\n//======================================\n");
	printf("// 【DEBUG関数】SPPFP\n");
	printf("//======================================\n");
#endif	
	//====================================================================================
	// 初期化とメモリ確保
	//====================================================================================	
	//---------------------------------------------------------
	// 故障辞書のメモリ確保
	//---------------------------------------------------------
	fdic_str = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	fdic_stf = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		fdic_str[i] = Alloc_Bit_INT(n_snet);		//ヨコ
		fdic_stf[i] = Alloc_Bit_INT(n_snet);		//ヨコ
	}
	
	//---------------------------------------------------------
	//32個分の故障リストメモリ確保
	//---------------------------------------------------------
	t_fault = (SXID_LIST**)malloc(sizeof(SXID_LIST*) * 32);
	for(i=0; i<32; i++)t_fault[i] = (SXID_LIST*)malloc(sizeof(SXID_LIST));

	//---------------------------------------------------------
	//SPPFPで各PPOで検出した故障した故障情報を保持する構造体のメモリ確保
	//---------------------------------------------------------
	propa_po = (XID_PROPA_PO*)malloc(sizeof(XID_PROPA_PO) * n_ppo);	//PPO数分メモリ確保

	//疑似外部出力いろいろ初期化
	for(i=0; i<n_ppo; i++){
		propa_po[i].po_id = i;						//PPOのID値を代入
		propa_po[i].n_det = 0;						//初期化
		s_nl[ppo[i]->n].ppo_id = i;					//po[i]のi番目情報を保持
		propa_po[i].det_flag = Alloc_Bit_INT(32);	//メモリ確保
	}

	//ドントケア抽出後のテストパターンの正常値計算用メモリ確保
	for(i=0; i<n_snet; i++){
		s_nl[i].xid_nval_t1 = Alloc_Bit_TP(n_tp);
		s_nl[i].xid_nval_t2 = Alloc_Bit_TP(n_tp);
	}
		
	//---------------------------------------------------------
	// フラグ初期化
	//---------------------------------------------------------
	for(i=0; i<n_snet; i++)s_nl[i].flag = 0;

	
	//---------------------------------------------------------
	// 関数ポインタセット
	//---------------------------------------------------------
	if(n_before_x == 0){
		FPointer_TDF_2v_SPPFP();			//2値-SPPFP
	}
	else{
		FPointer_TDF_3v_SPPFP();			//3値-SPPFP
	}

	
	//====================================================================================
	// 全故障に対してSPPFP
	//====================================================================================
	fprintf(stderr,"\n\n 【DEBUG mode】 SPPFP\n");

	for(i=0; i<n_tp; i++){

		printf("%d回目/%d\r", i+1, n_tp);

#ifdef DEBUG
		printf("\n//---------------------------------------\n");
		printf("//%d番目のテストパターン\n", i);
		printf("//---------------------------------------\n");
		printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);

		if(n_before_x != 0){
			for(j=0; j<n_pi; j++){
				nval_t1 = Get_NBit(pi[j]->nval, i);
				if(nval_t1 == 0){
					printf("%s: 0\n", pi[j]->name);
				}
				else if(nval_t1 == 1){
					printf("%s: 1\n", pi[j]->name);
				}
				else if(nval_t1 == 3){
					printf("%s: X\n", pi[j]->name);
				}
			}
		}
#endif

		for(j=0; j<n_snet; j++){
			
			//---------------------------------------------------------------------
			//対象信号線のiパターン目の2時刻目正常値取得
			//---------------------------------------------------------------------
			//2値の場合
			if(n_before_x == 0){
				nval_t1 = Get_NBit_Xbuf(s_nl[j].nval_t1, i);
				nval_t2 = Get_NBit_Xbuf(s_nl[j].nval_t2, i);
			}
			//3値の場合
			else{
				nval_t1 = Get_NBit(s_nl[j].nval_t1, i);
				nval_t2 = Get_NBit(s_nl[j].nval_t2, i);
			}

			//---------------------------------------------------------------------
			// ■立上り遷移故障(0→1/0)
			//---------------------------------------------------------------------
			if(s_nl[j].test_str==YES && s_nl[j].testable_net==YES && nval_t2==1 && nval_t1==0){
								
				//--------------------------------
				//故障シミュレーション(SPPFP)
				//--------------------------------
				if(count == 31){
						
					//32個目の故障を故障リストへ追加
					t_fault[count]->net			= &s_nl[j];		//信号線名
					t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障(0→1/0))
#ifdef FLT_DEBUG
					printf("flt[%d]: %s\n", count, t_fault[count]->net->name);
#endif
						
					//故障リスト内の故障数更新
					count++;

					//【2値FSIM】
					if(n_before_x == 0){
						TDF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
					}
					//【3値FSIM】
					else{
						TDF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
					}

					//★故障辞書作成
					DEBUG_TDF_SPPFP_Fault_Dictionay(i, t_fault, count, ed_flag);

					//故障設置フラグ初期化
					for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

					//故障リスト再初期化
					count = 0;

					//EDフラグ更新
					ed_flag+=2;
				}
				//--------------------------------
				//故障リストの更新
				//--------------------------------
				else{

					//故障リストへ追加
					t_fault[count]->net			= &s_nl[j];		//信号線名
					t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障(0→1/0))
#ifdef FLT_DEBUG
					printf("flt[%d]: %s\n", count, t_fault[count]->net->name);
#endif

					//故障リスト内の故障数更新
					count++;
				}
			}
						
			//---------------------------------------------------------------------
			// ■立下り遷移故障(1→0/1)
			//---------------------------------------------------------------------
			if(s_nl[j].test_stf==YES && s_nl[j].testable_net==YES && nval_t2==0 && nval_t1==1){
					
				//--------------------------------
				//故障シミュレーション(SPPFP)
				//--------------------------------
				if(count == 31){
						
					//32個目の故障を故障リストへ追加
					t_fault[count]->net			= &s_nl[j];		//信号線名
					t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障(1→0/1))
#ifdef FLT_DEBUG
					printf("flt[%d]: %s\n", count, t_fault[count]->net->name);
#endif
						
					//故障リスト内の故障数更新
					count++;

					//【2値FSIM】
					if(n_before_x == 0){
						TDF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
					}
					//【3値FSIM】
					else{
						TDF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
					}

					//★故障辞書作成
					DEBUG_TDF_SPPFP_Fault_Dictionay(i, t_fault, count, ed_flag);

					//故障設置フラグ初期化
					for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;	//故障設置フラグ初期化

					//故障リスト再初期化
					count = 0;

					//EDフラグ更新
					ed_flag+=2;
				}
				//--------------------------------
				//故障リストの更新
				//--------------------------------
				else{

					//故障リストへ追加
					t_fault[count]->net			= &s_nl[j];	//信号線名
					t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障(1→0/1))
#ifdef FLT_DEBUG
					printf("flt[%d]: %s\n", count, t_fault[count]->net->name);
#endif

					//故障リスト内の故障数更新
					count++;
				}
			}
			
		}// for文(n_snet)
		
		//---------------------------------------------------------------------
		//■故障リスト内に故障が余っていた場合
		//---------------------------------------------------------------------
		if(count > 0){
			
			//【2値FSIM】
			if(n_before_x == 0){
				TDF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
			}
			//【3値FSIM】
			else{
				TDF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
			}
			
			//★故障辞書作成
			DEBUG_TDF_SPPFP_Fault_Dictionay(i, t_fault, count, ed_flag);

			//故障設置フラグ初期化
			for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

			//故障リスト再初期化
			count = 0;
			
			//EDフラグ更新
			ed_flag+=2;
		}

	}// for文(n_tp)
	
#ifdef BUG
	fclose(fp);
#endif

}//END

//----------------------------------------------
//  関数名 : DEBUG_TDF_SPPFP_Fault_Dictionay
//  機  能 : SPPFP関数で故障辞書作成
//  戻り値 : なし
//  引  数 : tp_id(テストパターンID, t_fault(最大32個の故障リスト), n_fault(対象故障数), ed_flag(イベントドリブン用フラグ)
//----------------------------------------------
void	DEBUG_TDF_SPPFP_Fault_Dictionay(int tp_id, SXID_LIST **t_fault, int n_fault, unsigned int ed_flag){

	int		i,j;

	//=====================================================
	// 故障辞書作成
	//=====================================================
	//最大32故障に対してループ
	for(i=0; i<n_fault; i++){

		//各PPOに対して対象故障を検出してるか確認
		for(j=0; j<n_ppo; j++){
			
			//直前のSPPFPでj番目PPOで検出した故障数が1以上存在
			if(propa_po[j].n_det > 0){

				//指定ビット目(i番目故障)に『1』が立っている(i番目故障が検出可能)
				if(Get_NBit_INT(propa_po[j].det_flag, i) == 1){

					//-----------------------------------------------
					// 立上り遷移故障(0→1/0)検出
					//-----------------------------------------------
					if(t_fault[i]->fault_type == TDF_STR){

						//故障辞書にフラグ立て
						Set_NINT_One(fdic_str[tp_id], t_fault[i]->net->n);

						//検出故障回数更新
						t_fault[i]->net->det_str++;
#ifdef DEBUG
						printf("STR検出: %s\n", t_fault[i]->net->name);
#endif
						break;
					}
					
					//-----------------------------------------------
					// 立下り遷移故障(1→0/1)検出
					//-----------------------------------------------
					else if(t_fault[i]->fault_type == TDF_STF){
						
						//故障辞書にフラグ立て
						Set_NINT_One(fdic_stf[tp_id], t_fault[i]->net->n);
						
						//検出故障回数更新
						t_fault[i]->net->det_stf++;
#ifdef DEBUG
						printf("STF検出: %s\n", t_fault[i]->net->name);
#endif
						break;
					}
				}
			}
		}
	}
	
	//=====================================================
	//故障伝搬POフラグの初期化(X抽出用のSPPFPの結果情報)
	//=====================================================
	for(i=0; i<n_ppo; i++){
		if(propa_po[i].n_det > 0){
			propa_po[i].n_det = 0;					//対象PPOでの故障検出数初期化
			All_INT_Zero(propa_po[i].det_flag);		//対象PPOでの故障検出フラグ初期化(どの故障を検出したか)
		}
	}

}
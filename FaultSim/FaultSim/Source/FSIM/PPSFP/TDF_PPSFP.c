//------------------------------------------------------------------------
//File name : TDF_PPSFP.c
//Date : 2011/7/11
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	<string.h>
#include	"../../Netlist/s_netlist.h"
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
void	TDF_2v_ED_PPSFP			(int, S_NLIST*, unsigned int, int);
void	TDF_3v_ED_PPSFP			(int, S_NLIST*, unsigned int, int);
void	Make_FP_TDF_2v_ED_PPSFP	();
void	Make_FP_TDF_3v_ED_PPSFP	();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_2v_PPSFP
//  機  能 : 2値遷移故障故障シミュレーション(PPSFP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	TDF_2v_PPSFP(){

	int				i;
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ
	int				nval_t1;	//1時刻目正常値
	int				nval_t2;	//2時刻目正常値
#ifdef DEBUG
	int				k;
#endif

	//====================================================
	// 故障辞書のメモリ確保
	//====================================================
	fdic_str = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	fdic_stf = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		fdic_str[i] = Alloc_Bit_INT(n_snet);		//ヨコ
		fdic_stf[i] = Alloc_Bit_INT(n_snet);		//ヨコ
	}
	
	//====================================================
	// フラグ初期化
	//====================================================
	for(i=0; i<n_snet; i++)s_nl[i].flag = 0;
	

	//====================================================
	// 関数ポインタ作成
	//====================================================
	Make_FP_TDF_2v_ED_PPSFP	();			//イベントドリブン用 


	//====================================================
	// 故障シミュレーション
	//====================================================
	fprintf(stderr,"\n FSIM_2v_sequential(PPSFP_MAXdrop)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	//--------------------------------------------------
	// 2時刻目PPOに対してクリティカルパストレーシング
	//--------------------------------------------------
	//※FFRはPO，PPO，STEMの順番
	for(i=n_spo; i<(n_spo+n_ppo); i++){

		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

#ifdef DEBUG
		printf("\nPPO[%d]: %s\n", i, sffr[i].FoutStem->name);
#endif

		for(j=0; (int)j<n_tp; j++){

#ifdef DEBUG
			//printf("tp[%d]: ", j);
			//for(k=0; k<n_spi; k++)printf("%d", Get_NBit_Xbuf(spi[k]->nval_t1, j));
			//for(k=0; k<n_ppi; k++)printf("%d", Get_NBit_Xbuf(ppi[k]->nval_t1, j));
			//printf("\n");
#endif
			//------------------------------------
			//正常値取得
			//------------------------------------
			nval_t1 = Get_NBit_Xbuf(sffr[i].FoutStem->nval_t1, j);
			nval_t2 = Get_NBit_Xbuf(sffr[i].FoutStem->nval_t2, j);
#ifdef DEBUG
			printf("%s t1正常値:%d t2正常値:%d\n",sffr[i].FoutStem->name, nval_t1, nval_t2);
#endif

			//------------------------------------
			// 2時刻目正常値 = 0
			//------------------------------------
			if(nval_t2 == 0){

				//1時刻目正常値=1(PPOで立下り遷移故障(1→0/1)検出)
				if(nval_t1 == 1){
					sffr[i].n_detect++;									//FFR内の検出故障数更新
					sffr[i].FoutStem->det_stf++;							//対象信号線の立下り遷移故障(1→0/1)の検出回数更新
					Set_NINT_One(fdic_stf[j], sffr[i].FoutStem->n);		//故障辞書にフラグ立て
#ifdef DEBUG
					printf("tp[%d] STR(1→0/1) %s\n", j, sffr[i].FoutStem->name);
#endif
				}
				
				//2時刻目正常値=0からのCPT開始
				TDF_2v_CPT0(i, j, sffr[i].FoutStem);
			}

			//------------------------------------
			// 2時刻目正常値 = 1
			//------------------------------------
			else if(nval_t2 == 1){

				//1時刻目正常値=0(PPOで立上り遷移故障(0→1/0)検出)
				if(nval_t1 == 0){
					sffr[i].n_detect++;									//FFR内の検出故障数更新
					sffr[i].FoutStem->det_str++;							//対象信号線の立上り遷移故障の検出回数更新
					Set_NINT_One(fdic_str[j], sffr[i].FoutStem->n);		//故障辞書にフラグ立て					
#ifdef DEBUG
					printf("tp[%d] STF(0→1/0) %s\n", j, sffr[i].FoutStem->name);
#endif
				}

				//2時刻目正常値=1からのCPT開始
				TDF_2v_CPT1(i, j, sffr[i].FoutStem);
			}
		}
	}
	//--------------------------------------------------
	// FOUT-STEMに対してクリティカルパストレーシング
	//--------------------------------------------------
	for( ; i<n_ffr; i++){ //初期化式にiは使われないため，iを空欄に変更
		
		//----------------------------------------------------------------
		// Progress Bar
		//----------------------------------------------------------------
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");
		
#ifdef DEBUG
		printf("\nFFR[%d]: %s\n", i, sffr[i].FoutStem->name);
#endif
		//----------------------------------------------------------------
		// PPSFPノーマルイベントドリブン
		//----------------------------------------------------------------
		//構造的テスト可能FFRのみイベントドリブン処理
		if(sffr[i].FoutStem->testable_net == YES){

			for(j=0; j<n_tp_int; j++){	//確保したunsigned int数分ループ			

				//対象ステムに故障値挿入
				sffr[i].FoutStem->x_fault = ~sffr[i].FoutStem->nval_t2->x_buf[j];	//x_fault に 2時刻目x_buffの反転値を代入
			
				//イベントドリブン用フラグ設定
				ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
				sffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
				//PPSFPイベントドリブン開始
				TDF_2v_ED_PPSFP(i, sffr[i].FoutStem, ed_flag, j);	//2値PPSFPイベントドリブン
				
			}
		}
	}

}//END


//***********************************************************************************************
//----------------------------------------------
//  関数名 : TDF_3v_PPSFP
//  機  能 : 3値遷移故障故障シミュレーション(PPSFP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	TDF_3v_PPSFP(){

	int				i;
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ
	int				nval_t1;	//1時刻目正常値
	int				nval_t2;	//2時刻目正常値
#ifdef DEBUG
	int				k;
#endif

	//====================================================
	// 故障辞書のメモリ確保
	//====================================================
	fdic_str = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	fdic_stf = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		fdic_str[i] = Alloc_Bit_INT(n_snet);		//ヨコ
		fdic_stf[i] = Alloc_Bit_INT(n_snet);		//ヨコ
	}
	
	//====================================================
	// フラグ初期化
	//====================================================
	for(i=0; i<n_snet; i++)s_nl[i].flag = 0;
	

	//====================================================
	// 関数ポインタ作成
	//====================================================
	Make_FP_TDF_3v_ED_PPSFP	();			//イベントドリブン用 


	//====================================================
	// 故障シミュレーション
	//====================================================
	fprintf(stderr,"\n FSIM_3v_sequential(PPSFP)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	//--------------------------------------------------
	// 2時刻目PPOに対してクリティカルパストレーシング
	//--------------------------------------------------
	//※FFRはPO，PPO，STEMの順番
	for(i=n_spo; i<(n_spo+n_ppo); i++){

		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

#ifdef DEBUG
		printf("\nPPO[%d]: %s\n", i, sffr[i].FoutStem->name);
#endif

		for(j=0; (int)j<n_tp; j++){

#ifdef DEBUG
			printf("tp[%d]: ", j);
			for(k=0; k<n_spi; k++)printf("%d", Get_NBit_Xbuf(spi[k]->nval_t1, j));
			for(k=0; k<n_ppi; k++)printf("%d", Get_NBit_Xbuf(ppi[k]->nval_t1, j));
			printf("\n");
#endif
			//------------------------------------
			//正常値取得
			//------------------------------------
			nval_t1 = Get_NBit(sffr[i].FoutStem->nval_t1, j);
			nval_t2 = Get_NBit(sffr[i].FoutStem->nval_t2, j);
#ifdef DEBUG
			printf("%s t1正常値:",sffr[i].FoutStem->name);
			if(nval_t1 == 3){
				printf("X");
			}
			else{
				printf("%d", nval_t1);
			}
			printf(" t2正常値:");
			if(nval_t2 == 3){
				printf("X");
			}
			else{
				printf("%d\n", nval_t2);
			}
#endif

			//------------------------------------
			// 2時刻目正常値 = 0
			//------------------------------------
			if(nval_t2 == 0){

				//1時刻目正常値=1(PPOで立下り遷移故障(1→0/1)検出)
				if(nval_t1 == 1){
					sffr[i].n_detect++;									//FFR内の検出故障数更新
					sffr[i].FoutStem->det_stf++;							//対象信号線の立下り遷移故障(1→0/1)の検出回数更新
					Set_NINT_One(fdic_stf[j], sffr[i].FoutStem->n);		//故障辞書にフラグ立て
#ifdef DEBUG
					printf("tp[%d] STR(1→0/1) %s\n", j, sffr[i].FoutStem->name);
#endif
				}
				
				//2時刻目正常値=0からのCPT開始
				TDF_3v_CPT0(i, j, sffr[i].FoutStem);
			}

			//------------------------------------
			// 2時刻目正常値 = 1
			//------------------------------------
			else if(nval_t2 == 1){

				//1時刻目正常値=0(PPOで立上り遷移故障(0→1/0)検出)
				if(nval_t1 == 0){
					sffr[i].n_detect++;									//FFR内の検出故障数更新
					sffr[i].FoutStem->det_str++;							//対象信号線の立上り遷移故障の検出回数更新
					Set_NINT_One(fdic_str[j], sffr[i].FoutStem->n);		//故障辞書にフラグ立て					
#ifdef DEBUG
					printf("tp[%d] STF(0→1/0) %s\n", j, sffr[i].FoutStem->name);
#endif
				}

				//2時刻目正常値=1からのCPT開始
				TDF_3v_CPT1(i, j, sffr[i].FoutStem);
			}
		}
	}
	//--------------------------------------------------
	// FOUT-STEMに対してクリティカルパストレーシング
	//--------------------------------------------------
	for( ; i<n_ffr; i++){ //初期化式にiは使われないため，iを空欄に変更
		
		//----------------------------------------------------------------
		// Progress Bar
		//----------------------------------------------------------------
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");
		
#ifdef DEBUG
		printf("\nFFR[%d]: %s\n", i, sffr[i].FoutStem->name);
#endif
		//----------------------------------------------------------------
		// PPSFPノーマルイベントドリブン
		//----------------------------------------------------------------
		//構造的テスト可能FFRのみイベントドリブン処理
		if(sffr[i].FoutStem->testable_net == YES){

			for(j=0; j<n_tp_int; j++){	//確保したunsigned int数分ループ			

				//対象ステムに故障値挿入
				sffr[i].FoutStem->x_fault = ~sffr[i].FoutStem->nval_t2->x_buf[j];	//x_fault に 2時刻目x_buffの反転値を代入
				sffr[i].FoutStem->p_fault = ~sffr[i].FoutStem->nval_t2->p_buf[j];	//x_fault に 2時刻目p_buffの反転値を代入
			
				//イベントドリブン用フラグ設定
				ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
				sffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
				//PPSFPイベントドリブン開始
				TDF_3v_ED_PPSFP(i, sffr[i].FoutStem, ed_flag, j);	//2値PPSFPイベントドリブン
				
			}
		}
	}

}//END
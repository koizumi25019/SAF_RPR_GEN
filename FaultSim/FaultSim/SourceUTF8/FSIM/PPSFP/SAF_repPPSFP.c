//------------------------------------------------------------------------
//File name : SAF_repPPSFP.c
//Date : 2011/7/11
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
void	SAF_2v_CPT0				(int, unsigned int, NLIST*);
void	SAF_2v_CPT1				(int, unsigned int, NLIST*);
void	SAF_3v_CPT0				(int, unsigned int, NLIST*);
void	SAF_3v_CPT1				(int, unsigned int, NLIST*);
void	SAF_2v_repED_PPSFP		(int, NLIST*, unsigned int, int);
void	SAF_3v_repED_PPSFP		(int, NLIST*, unsigned int, int);
void	SAF_2v_EXrepED_PPSFP	(int, NLIST*, unsigned int, int);
void	SAF_3v_EXrepED_PPSFP	(int, NLIST*, unsigned int, int);

void	SAF_2v_ED_PPSFP			(int, NLIST*, unsigned int, int);
void	SAF_3v_ED_PPSFP			(int, NLIST*, unsigned int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_2v_repPPSFP
//  機  能 : 【代表信号線】2値縮退故障故障シミュレーション(PPSFP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	SAF_2v_repPPSFP(){
	
	int				i;			//FFR-ID
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	int				k;			//FOUTブランチ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ

	//====================================================
	// 故障辞書のメモリ確保
	//====================================================
	fdic_sa0 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	fdic_sa1 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		fdic_sa0[i] = Alloc_Bit_INT(n_net);		//ヨコ
		fdic_sa1[i] = Alloc_Bit_INT(n_net);		//ヨコ
	}


	//====================================================
	// フラグ初期化
	//====================================================
	for(i=0; i<n_net; i++)nl[i].flag = 0;


	//====================================================
	// 故障シミュレーション
	//====================================================
	fprintf(stderr,"\n\n FSIM_2v_combinational(PPSFP)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	//====================================================
	// 外部出力に対してクリティカルパストレーシング
	//====================================================
	for(i=0; i<n_po; i++){
		
		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		for(j=0; (int)j<n_tp; j++){
			//------------------------------------
			// 正常値=0 (1縮退故障検出)
			//------------------------------------
			if(Get_NBit_Xbuf(ffr[i].FoutStem->nval, j) == 0){
				ffr[i].n_detect++;							//FFR内の検出故障数更新
				ffr[i].FoutStem->det_sf1++;						//対象信号線の1縮退故障の検出回数更新
				Set_NINT_One(fdic_sa1[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				SAF_2v_CPT0(i, j, ffr[i].FoutStem);				//CPT開始
			}

			//------------------------------------
			// 正常値=1 (0縮退故障検出)
			//------------------------------------
			else if(Get_NBit_Xbuf(ffr[i].FoutStem->nval, j) == 1){
				ffr[i].n_detect++;							//FFR内の検出故障数更新
				ffr[i].FoutStem->det_sf0++;						//対象信号線の0縮退故障の検出回数更新
				Set_NINT_One(fdic_sa0[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				SAF_2v_CPT1(i, j, ffr[i].FoutStem);				//CPT開始
			}
		}
	}
	
	//====================================================
	// FOUT-STEMに対してクリティカルパストレーシング
	//====================================================
	for(i; i<n_ffr; i++){
		
		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		//■再収斂してない
		if(ffr[i].Reconv == NO){
			//----------------------------------------------------------------
			// 出力のFOUTブランチの故障検出状態を確認(イベントドリブン不要)
			//----------------------------------------------------------------
			for(j=0; (int)j<n_tp; j++){
				for(k=0; k<ffr[i].FoutStem->n_out; k++){	//ブランチの本数分ループ
				
					//--------------------------------------------------------
					//ブランチk番目が0縮退故障検出してるか確認(ステム正常値=1)
					//--------------------------------------------------------
					if(Get_NBit_INT(fdic_sa0[j],ffr[i].FoutStem->out[k]->n) == 1){
						//ステムも0縮退故障検出可能
						ffr[i].n_detect++;							//FFR内の検出故障数更新
						ffr[i].FoutStem->det_sf0++;						//対象信号線の0縮退故障の検出回数更新
						Set_NINT_One(fdic_sa0[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
						SAF_2v_CPT1(i, j, ffr[i].FoutStem);				//内部信号線のCPT開始
						break;
					}
				
					//--------------------------------------------------------
					//ブランチj番目が1縮退故障検出してるか確認(ステム正常値=0)
					//--------------------------------------------------------
					else if(Get_NBit_INT(fdic_sa1[j],ffr[i].FoutStem->out[k]->n) == 1){
						//ステムも1縮退故障検出可能
						ffr[i].n_detect++;							//FFR内の検出故障数更新
						ffr[i].FoutStem->det_sf1++;						//対象信号線の1縮退故障の検出回数更新
						Set_NINT_One(fdic_sa1[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
						SAF_2v_CPT0(i, j, ffr[i].FoutStem);				//内部信号線のCPT開始
						break;
					}
				}
			}
		}

		//■再収斂してる
		else{
			//----------------------------------------------------------------
			// PPSFPイベントドリブン
			//----------------------------------------------------------------
			for(j=0; j<n_tp_int; j++){	//確保したunsigned int数分ループ
			
				//対象ステムに故障値挿入
				ffr[i].FoutStem->x_fault = ~ffr[i].FoutStem->nval->x_buf[j];	//x_fault に x_buffの反転値を代入
			
				//イベントドリブン用フラグ設定
				ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
				ffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			

				//PPSFPイベントドリブン開始
				if(ffr[i].FoutStem->rep_net == NO){	//代表信号線じゃない場合
					//SAF_2v_ED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//2値PPSFPイベントドリブン
					SAF_2v_EXrepED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//【特殊処理】2値PPSFP代表信号線イベントドリブン
				}
				else if(ffr[i].FoutStem->rep_net == YES){	//代表信号線の場合
					SAF_2v_repED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//2値PPSFP代表信号線イベントドリブン
				}

			}
		}
	}


}//END

//***********************************************************************************************
//***********************************************************************************************
//***********************************************************************************************
//----------------------------------------------
//  関数名 : SAF_3v_repPPSFP
//  機  能 : 【代表信号線】3値縮退故障故障シミュレーション(PPSFP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	SAF_3v_repPPSFP(){
	
	int				i;			//FFR-ID
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	int				k;			//FOUTブランチ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ

	//====================================================
	// 故障辞書のメモリ確保
	//====================================================
	fdic_sa0 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ
	fdic_sa1 = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);	//タテ

	for(i=0;i<n_tp;i++){
		fdic_sa0[i] = Alloc_Bit_INT(n_net);		//ヨコ
		fdic_sa1[i] = Alloc_Bit_INT(n_net);		//ヨコ
	}
	

	//====================================================
	// フラグ初期化
	//====================================================
	for(i=0; i<n_net; i++)nl[i].flag = 0;


	//====================================================
	// 故障シミュレーション
	//====================================================
	fprintf(stderr,"\n\n FSIM_3v_combinational(PPSFP)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	//====================================================
	// 外部出力に対してクリティカルパストレーシング
	//====================================================
	for(i=0; i<n_po; i++){
		
		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		for(j=0; (int)j<n_tp; j++){
			//------------------------------------
			// 正常値=0 (1縮退故障検出)
			//------------------------------------
			if(Get_NBit(ffr[i].FoutStem->nval, j) == 0){
				ffr[i].n_detect++;							//FFR内の検出故障数更新
				ffr[i].FoutStem->det_sf1++;						//対象信号線の1縮退故障の検出回数更新
				Set_NINT_One(fdic_sa1[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				SAF_3v_CPT0(i, j, ffr[i].FoutStem);				//CPT開始
			}

			//------------------------------------
			// 正常値=1 (0縮退故障検出)
			//------------------------------------
			else if(Get_NBit(ffr[i].FoutStem->nval, j) == 1){
				ffr[i].n_detect++;							//FFR内の検出故障数更新
				ffr[i].FoutStem->det_sf0++;						//対象信号線の0縮退故障の検出回数更新
				Set_NINT_One(fdic_sa0[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				SAF_3v_CPT1(i, j, ffr[i].FoutStem);				//CPT開始
			}
		}
	}
	
	//====================================================
	// FOUT-STEMに対してクリティカルパストレーシング
	//====================================================
	for(i; i<n_ffr; i++){
		
		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		//■再収斂してない
		if(ffr[i].Reconv == NO){
			//----------------------------------------------------------------
			// 出力のFOUTブランチの故障検出状態を確認(イベントドリブン不要)
			//----------------------------------------------------------------
			for(j=0; (int)j<n_tp; j++){
				for(k=0; k<ffr[i].FoutStem->n_out; k++){	//ステムの本数分ループ
				
					//--------------------------------------------------------
					//ブランチk番目が0縮退故障検出してるか確認(ステム正常値=1)
					//--------------------------------------------------------
					if(Get_NBit_INT(fdic_sa0[j],ffr[i].FoutStem->out[k]->n) == 1){
						//ステムも0縮退故障検出可能
						ffr[i].n_detect++;							//FFR内の検出故障数更新
						ffr[i].FoutStem->det_sf0++;						//対象信号線の0縮退故障の検出回数更新
						Set_NINT_One(fdic_sa0[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
						SAF_3v_CPT1(i, j, ffr[i].FoutStem);				//内部信号線のCPT開始
						break;
					}
				
					//--------------------------------------------------------
					//ブランチj番目が1縮退故障検出してるか確認(ステム正常値=0)
					//--------------------------------------------------------
					else if(Get_NBit_INT(fdic_sa1[j],ffr[i].FoutStem->out[k]->n) == 1){
						//ステムも1縮退故障検出可能
						ffr[i].n_detect++;							//FFR内の検出故障数更新
						ffr[i].FoutStem->det_sf1++;						//対象信号線の1縮退故障の検出回数更新
						Set_NINT_One(fdic_sa1[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
						SAF_3v_CPT0(i, j, ffr[i].FoutStem);				//内部信号線のCPT開始
						break;
					}
				}
			}
		}

		//■再収斂してる
		else{
			//----------------------------------------------------------------
			// PPSFPイベントドリブン
			//----------------------------------------------------------------
			for(j=0; j<n_tp_int; j++){	//確保したunsigned int数分ループ
			
				//対象ステムに故障値挿入
				ffr[i].FoutStem->x_fault = ffr[i].FoutStem->nval->p_buf[j];	//x_fault に p_buffを代入(反転値を代入)
				ffr[i].FoutStem->p_fault = ffr[i].FoutStem->nval->x_buf[j];	//p_fault に x_buffを代入(反転値を代入)
			
				//イベントドリブン用フラグ設定
				ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
				ffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
				//PPSFPイベントドリブン開始
				if(ffr[i].FoutStem->rep_net == NO){	//代表信号線じゃない場合
					SAF_3v_ED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//3値PPSFPイベントドリブン
					//SAF_3v_EXrepED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//【特殊処理】3値PPSFP代表信号線イベントドリブン
				}
				else if(ffr[i].FoutStem->rep_net == YES){	//代表信号線の場合
					SAF_3v_repED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//3値PPSFP代表信号線イベントドリブン
				}

			}
		}
	}

}//END
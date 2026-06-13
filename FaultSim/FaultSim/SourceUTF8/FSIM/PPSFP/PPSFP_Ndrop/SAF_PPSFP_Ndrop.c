//------------------------------------------------------------------------
//File name : SAF_PPSFP_Ndrop.c
//Date : 2013/8/7
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../../Netlist/netlist.h"
#include	"../../../Lib/bit_int.h"
#include	"../../../StandardHead.h"
#include	"../../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	SAF_2v_CPT0_Ndrop				(int, unsigned int, NLIST*);
void	SAF_2v_CPT1_Ndrop				(int, unsigned int, NLIST*);
void	SAF_3v_CPT0_Ndrop				(int, unsigned int, NLIST*);
void	SAF_3v_CPT1_Ndrop				(int, unsigned int, NLIST*);
void	SAF_2v_ED_PPSFP_Ndrop			(int, NLIST*, unsigned int, int);
void	SAF_3v_ED_PPSFP_Ndrop			(int, NLIST*, unsigned int, int);
void	Make_FP_SAF_2v_ED_PPSFP	();
void	Make_FP_SAF_3v_ED_PPSFP	();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define BREAK

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_2v_PPSFP_Ndrop
//  機  能 : 2値縮退故障故障シミュレーション(PPSFP)
//  戻り値 : なし
//  引  数 : なし
//  memo   : 再収斂情報，代表信号線情報と使用しないでFSIM(ZONOXと同じ)
//----------------------------------------------
void	SAF_2v_PPSFP_Ndrop(){
	
	int				i;
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ
	int				nval;
#ifdef BREAK
	int				n_break=0;	//break回数
#endif

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
	// 関数ポインタ作成
	//====================================================
	Make_FP_SAF_2v_ED_PPSFP	();			//イベントドリブン用 


	//====================================================
	// 故障シミュレーション
	//====================================================
	fprintf(stderr,"\n\n FSIM_2v_combinational(PPSFP %ddrop)\n", opt.n_drop);
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
	//--------------------------------------------------
	// 外部出力に対してクリティカルパストレーシング
	//--------------------------------------------------
	for(i=0; i<n_po; i++){

		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		for(j=0; (int)j<n_tp; j++){
			//------------------------------------
			//正常値取得
			//------------------------------------
			nval = Get_NBit_Xbuf(ffr[i].FoutStem->nval, j);

			//------------------------------------
			// 正常値=0 (1縮退故障検出)
			//------------------------------------
			if(nval == 0){
				
				//検出回数がopt.n_drop回以下なら検出回数等を保存
				if(ffr[i].FoutStem->det_sf1<opt.n_drop && ffr[i].FoutStem->test_sf1==YES){
					ffr[i].n_detect++;								//FFR内の検出故障数更新
					ffr[i].FoutStem->det_sf1++;						//対象信号線の1縮退故障の検出回数更新
					Set_NINT_One(fdic_sa1[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				}

				SAF_2v_CPT0_Ndrop(i, j, ffr[i].FoutStem);				//CPT開始
			}

			//------------------------------------
			// 正常値=1 (0縮退故障検出)
			//------------------------------------
			else if(nval == 1){

				//検出回数がopt.n_drop回以下なら検出回数等を保存
				if(ffr[i].FoutStem->det_sf0<opt.n_drop && ffr[i].FoutStem->test_sf0==YES){
					ffr[i].n_detect++;								//FFR内の検出故障数更新
					ffr[i].FoutStem->det_sf0++;						//対象信号線の0縮退故障の検出回数更新
					Set_NINT_One(fdic_sa0[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				}
				
				SAF_2v_CPT1_Ndrop(i, j, ffr[i].FoutStem);				//CPT開始
			}
			
			//------------------------------------
			//★対象FFR内の全故障をopt.n_drop回数以上検出したらbreak!!
			//------------------------------------
			if(ffr[i].n_detect == ffr[i].n_fault *opt.n_drop){
#ifdef BREAK
				n_break++;
#endif
				break;
			}
		}
	}
	//--------------------------------------------------
	// FOUT-STEMに対してクリティカルパストレーシング
	//--------------------------------------------------
	for(i; i<n_ffr; i++){
		
		//----------------------------------------------------------------
		// Progress Bar
		//----------------------------------------------------------------
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		//----------------------------------------------------------------
		// PPSFPノーマルイベントドリブン
		//----------------------------------------------------------------
		for(j=0; j<n_tp_int; j++){	//確保したunsigned int数分ループ
			
			//対象ステムに故障値挿入
			ffr[i].FoutStem->x_fault = ~ffr[i].FoutStem->nval->x_buf[j];	//x_fault に x_buffの反転値を代入
			
			//イベントドリブン用フラグ設定
			ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
			ffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
			//PPSFPイベントドリブン開始
			SAF_2v_ED_PPSFP_Ndrop(i, ffr[i].FoutStem, ed_flag, j);	//2値PPSFPイベントドリブン
			
			//★対象FFR内の全故障をopt.n_drop回数以上検出したらbreak!!
			if(ffr[i].n_detect == ffr[i].n_fault*opt.n_drop){
#ifdef BREAK
				n_break++;
#endif
				break;
			}
		}
	}

#ifdef BREAK
	printf("\nFSIM-break回数: %d\n", n_break);
#endif

}//END


//***********************************************************************************************
//----------------------------------------------
//  関数名 : SAF_3v_PPSFP_Ndrop
//  機  能 : 3値縮退故障故障シミュレーション(PPSFP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	SAF_3v_PPSFP_Ndrop(){
	
	int				i;
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ
	int				nval;

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
	// イベントドリブン用 関数ポインタ作成
	//====================================================
	Make_FP_SAF_3v_ED_PPSFP	();


	//====================================================
	// 故障シミュレーション
	//====================================================
	fprintf(stderr,"\n\n FSIM_3v_combinational(PPSFP %ddrop)\n", opt.n_drop);
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
	//--------------------------------------------------
	// 外部出力に対してクリティカルパストレーシング
	//--------------------------------------------------
	for(i=0; i<n_po; i++){
		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		for(j=0; (int)j<n_tp; j++){

			//------------------------------------
			//正常値取得
			//------------------------------------
			nval = Get_NBit(ffr[i].FoutStem->nval, j);

			//------------------------------------
			// 正常値=0 (1縮退故障検出)
			//------------------------------------
			if(nval == 0){
				//検出回数がopt.n_drop回以下なら検出回数等を保存
				if(ffr[i].FoutStem->det_sf1<opt.n_drop && ffr[i].FoutStem->test_sf1==YES){
					ffr[i].n_detect++;								//FFR内の検出故障数更新
					ffr[i].FoutStem->det_sf1++;						//対象信号線の1縮退故障の検出回数更新
					Set_NINT_One(fdic_sa1[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				}
				SAF_3v_CPT0_Ndrop(i, j, ffr[i].FoutStem);				//CPT開始
			}

			//------------------------------------
			// 正常値=1 (0縮退故障検出)
			//------------------------------------
			else if(nval == 1){
				//検出回数がopt.n_drop回以下なら検出回数等を保存
				if(ffr[i].FoutStem->det_sf0<opt.n_drop && ffr[i].FoutStem->test_sf0==YES){
					ffr[i].n_detect++;								//FFR内の検出故障数更新
					ffr[i].FoutStem->det_sf0++;						//対象信号線の0縮退故障の検出回数更新
					Set_NINT_One(fdic_sa0[j], ffr[i].FoutStem->n);	//故障辞書にフラグ立て
				}
				SAF_3v_CPT1_Ndrop(i, j, ffr[i].FoutStem);				//CPT開始
			}
			//------------------------------------
			//★対象FFR内の全故障をopt.n_drop回数以上検出したらbreak!!
			//------------------------------------
			if(ffr[i].n_detect == ffr[i].n_fault*opt.n_drop){
				break;
			}
		}
	}

	//--------------------------------------------------
	// FOUT-STEMに対してクリティカルパストレーシング
	//--------------------------------------------------
	for(i; i<n_ffr; i++){
		
		//----------------------------------------------------------------
		// Progress Bar
		//----------------------------------------------------------------
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");

		//----------------------------------------------------------------
		// PPSFPノーマルイベントドリブン
		//----------------------------------------------------------------
		for(j=0; j<n_tp_int; j++){	//確保したunsigned int数分ループ
			
			//対象ステムに故障値挿入(正常値反転)
			ffr[i].FoutStem->x_fault = ffr[i].FoutStem->nval->p_buf[j];	//x_fault に p_buffを代入
			ffr[i].FoutStem->p_fault = ffr[i].FoutStem->nval->x_buf[j];	//p_fault に x_buffを代入
			
			//イベントドリブン用フラグ設定
			ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
			ffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
			//PPSFPイベントドリブン開始
			SAF_3v_ED_PPSFP_Ndrop(i, ffr[i].FoutStem, ed_flag, j);	//3値PPSFPイベントドリブン


			//★対象FFR内の全故障をopt.n_drop回数以上検出したらbreak!!
			if(ffr[i].n_detect == ffr[i].n_fault*opt.n_drop){
				break;
			}
		}
	}

}//END
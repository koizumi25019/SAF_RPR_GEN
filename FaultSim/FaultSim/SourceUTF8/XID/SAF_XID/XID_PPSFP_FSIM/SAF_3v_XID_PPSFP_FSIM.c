//------------------------------------------------------------------------
//File name : SAF_3v_XID_PPSFP_FSIM.c
//Date : 2012/8/18
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
void	SAF_3v_XID_PPSFP_LSIM			();
void	SAF_3v_XID_PPSFP_CPT0			(int, unsigned int, NLIST*);
void	SAF_3v_XID_PPSFP_CPT1			(int, unsigned int, NLIST*);
void	SAF_3v_XID_ED_PPSFP				(int, NLIST*, unsigned int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//#define DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_3v_XID_PPSFP_FSIM
//  機  能 : X抽出後テストパターンで3値故障シミュレーション(X抽出専用: ※1故障1ドロップ)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	SAF_3v_XID_PPSFP_FSIM(){

	int				i;
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ
	int				nval;		//信号線の正常値
#ifdef DEBUG
	int				xval;
	int				pval;
	int				k;
#endif
	
#ifdef DEBUG
		printf("\n//======================================\n");
		printf("// X抽出後の3値PPSFP\n");
		printf("//======================================\n");
#endif
	//===========================================================
	// フラグ初期化
	//===========================================================
	for(i=0; i<n_net; i++)nl[i].flag = 0;

	
	//===========================================================
	// 論理シミュレーション (X-filling関数内部で含意操作してるのでいらない？)
	//===========================================================
	//SAF_3v_XID_PPSFP_LSIM();		//テストパターン印加もここで
#ifdef DEBUG
	printf("\nX抽出後の内部信号線値\n");
	//PI値
	for(i=0; i<n_pi; i++){
		printf("%s\t", pi[i]->name);
		for(j=0; j<(unsigned int)n_tp; j++){
			nval = Get_NBit(pi[i]->xid_nval, j);
			if(nval == 0){
				printf("0");
			}
			else if(nval == 1){
				printf("1");
			}
			else if(nval == 3){
				printf("X");
			}
		}
		printf("\n");
	}
	//その他信号線値
	for(i=0; i<n_net; i++){
		if(nl[i].type != IN){
			printf("%s\t", nl[i].name);
			for(j=0; j<(unsigned int)n_tp; j++){
				nval = Get_NBit(nl[i].xid_nval, j);
				if(nval == 0){
					printf("0");
				}
				else if(nval == 1){
					printf("1");
				}
				else if(nval == 3){
					printf("X");
				}
			}
			printf("\n");
		}
	}
#endif

	
	//===========================================================
	// 故障シミュレーション (PPSFP)※未検出故障存在FFRのみ対象
	//===========================================================
	fprintf(stderr,"\n\n SAF_3v_XID_PPSFP_FSIM\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
	
	//--------------------------------------------------
	// ■外部出力に対してクリティカルパストレーシング
	//--------------------------------------------------
	for(i=0; i<n_po; i++){
		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");
		
#ifdef DEBUG
		printf("\n//------------------------\n");
		printf("//開始PO： %s\n", ffr[i].FoutStem->name);
		printf("//------------------------\n");
#endif
		//対象FFR内に未検出故障が存在
		if(ffr[i].n_xid_detect < ffr[i].n_det_fault){

			//テストパターンでループ
			for(j=0; (int)j<n_tp; j++){

				//----------------------------------------------------------------
				//対象FFR内の全故障を検出
				//----------------------------------------------------------------
				if(ffr[i].n_xid_detect == ffr[i].n_det_fault){
#ifdef DEBUG
					printf("break: 対象FFR内の全故障を検出\n");
#endif
					break;
				}
				
				//----------------------------------------------------------------
				//対象FFR内に未検出故障が存在
				//----------------------------------------------------------------
				else{
#ifdef DEBUG
					printf("【%dパターン目】\n", j);
#endif
					//------------------------------------
					//X抽出後の正常値取得
					//------------------------------------
					nval = Get_NBit(ffr[i].FoutStem->xid_nval, j);
					
					//------------------------------------
					// 正常値=0 (1縮退故障検出)
					//------------------------------------
					if(nval == 0){
						if(ffr[i].FoutStem->xid_det_sf1 == NO){
							ffr[i].FoutStem->xid_det_sf1 = YES;			//対象信号線の1縮退故障を検出

							if(ffr[i].FoutStem->test_sf1 == YES){
								ffr[i].n_xid_detect++;					//対象FFRの検出故障数更新
								n_xid_detect++;							//X抽出後テスト集合の検出故障数更新
							}
#ifdef DEBUG
							printf("SF1 %s\n", ffr[i].FoutStem->name);
#endif
						}

						SAF_3v_XID_PPSFP_CPT0(i, j, ffr[i].FoutStem);	//CPT開始
					}

					//------------------------------------
					// 正常値=1 (0縮退故障検出)
					//------------------------------------
					else if(nval == 1){
						if(ffr[i].FoutStem->xid_det_sf0 == NO){
							ffr[i].FoutStem->xid_det_sf0 = YES;			//対象信号線の0縮退故障を検出
							
							if(ffr[i].FoutStem->test_sf0 == YES){
								ffr[i].n_xid_detect++;					//対象FFRの検出故障数更新
								n_xid_detect++;							//X抽出後テスト集合の検出故障数更新
							}
#ifdef DEBUG
							printf("SF0 %s\n", ffr[i].FoutStem->name);
#endif
						}
						SAF_3v_XID_PPSFP_CPT1(i, j, ffr[i].FoutStem);	//CPT開始
					}
				}
				
#ifdef DEBUG
				printf("\n");
#endif
			}
		}
	}

	//--------------------------------------------------
	// ■FOUT-STEMに対してクリティカルパストレーシング
	//--------------------------------------------------
	for(i; i<n_ffr; i++){
		
		//----------------------------------------------------------------
		// Progress Bar
		//----------------------------------------------------------------
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");
		
#ifdef DEBUG
		printf("\n//------------------------\n");
		printf("//開始STEM： %s\n", ffr[i].FoutStem->name);
		printf("//------------------------\n");
#endif
		//----------------------------------------------------------------
		// PPSFPノーマルイベントドリブン
		//----------------------------------------------------------------
		//対象FFR内に未検出故障が存在
		if(ffr[i].n_xid_detect < ffr[i].n_det_fault){

			//------------------------------------
			//確保したunsigned int数分ループ
			//------------------------------------
			for(j=0; j<n_tp_int; j++){
				
				//----------------------------------------------------------------
				//対象FFR内の全故障を検出
				//----------------------------------------------------------------
				if(ffr[i].n_xid_detect == ffr[i].n_det_fault){
#ifdef DEBUG
					printf("break: 対象FFR内の全故障を検出\n");
#endif
					break;
				}
				
				//----------------------------------------------------------------
				//対象FFR内に未検出故障が存在
				//----------------------------------------------------------------
				else{
#ifdef DEBUG
					printf("【%d unsigned int目】\n", j);
#endif
					//対象ステムに故障値挿入(正常値反転)
					ffr[i].FoutStem->x_fault = ffr[i].FoutStem->xid_nval->p_buf[j];	//x_fault に p_buffを代入
					ffr[i].FoutStem->p_fault = ffr[i].FoutStem->xid_nval->x_buf[j];	//p_fault に x_buffを代入
#ifdef DEBUG
					//--DEBUG-----------------------------------------
					printf("故障値確認\n");
					for(k=0; k<n_tp; k++){
						xval = ffr[i].FoutStem->x_fault & MASK[k];
						pval = ffr[i].FoutStem->p_fault & MASK[k];
						//0の場合
						if(xval==0 && pval!=0){
							printf("0");
						}
						//1の場合
						else if(xval!=0 && pval==0){
							printf("1");
						}
						//Xの場合
						else if(xval!=0 && pval!=0){
							printf("X");
						}
					}
					printf("\n");
					//------------------------------------------------
#endif
			
					//イベントドリブン用フラグ設定
					ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
					ffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
					//PPSFPイベントドリブン開始
					SAF_3v_XID_ED_PPSFP(i, ffr[i].FoutStem, ed_flag, j);	//3値PPSFPイベントドリブン
				}
				
#ifdef DEBUG
				printf("\n");
#endif
			}
		}
	}

#ifdef DEBUG
	printf("\n//--------------------------\n");
	printf("// XID対象の未検出故障\n");
	printf("//--------------------------\n");
	for(i=0; i<n_net; i++){
		if(nl[i].test_sf0==YES && nl[i].det_sf0>0 && nl[i].xid_det_sf0==NO){
			printf("SF0 %s\n", nl[i].name);
		}
		if(nl[i].test_sf1==YES && nl[i].det_sf1>0 && nl[i].xid_det_sf1==NO){
			printf("SF1 %s\n", nl[i].name);
		}
	}
#endif

}//END
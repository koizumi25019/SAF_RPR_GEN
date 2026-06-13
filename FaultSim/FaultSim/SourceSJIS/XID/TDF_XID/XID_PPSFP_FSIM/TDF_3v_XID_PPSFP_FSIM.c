//------------------------------------------------------------------------
//File name : TDF_3v_XID_PPSFP_FSIM.c
//Date : 2013/10/14
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../../Netlist/s_netlist.h"
#include	"../../../Lib/bit_int.h"
#include	"../../../StandardHead.h"
#include	"../../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void	TDF_3v_XID_PPSFP_LSIM			();
void	TDF_3v_XID_PPSFP_CPT0			(int, unsigned int, S_NLIST*);
void	TDF_3v_XID_PPSFP_CPT1			(int, unsigned int, S_NLIST*);
void	TDF_3v_XID_ED_PPSFP				(int, S_NLIST*, unsigned int, int);

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
//  関数名 : TDF_3v_XID_PPSFP_FSIM
//  機  能 : X抽出後テストパターンで3値故障シミュレーション(X抽出専用: ※1故障1ドロップ)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	TDF_3v_XID_PPSFP_FSIM(){

	int				i;
	unsigned int	j;			//テストパターン番号, 確保したunsigned int数分ループ
	unsigned int	ed_flag=0;	//イベントドリブン計算済みフラグ
	int				nval_t2;	//信号線の2時刻目正常値
#ifdef DEBUG
	int				xval;
	int				pval;
	int				k;
#endif
	
#ifdef DEBUG

		printf("\n//======================================\n");
		printf("// X抽出後の3値PPSFP\n");
		printf("//======================================\n");
		
		printf("\n//--------------------------------------\n");
		printf("// 初期テスト集合の論理値確認\n");
		printf("//--------------------------------------\n");
		for(i=0; i<n_tp; i++){
			printf("-------------------------\n");
			printf("  %dパターン目\n", i);
			//PI
			for(j=0; j<n_spi; j++){
				printf("name:%s  type:PI  nval:(", spi[j]->name);
				//1時刻目
				if(Get_NBit_Xbuf(spi[j]->nval_t1, i) == 0){
						printf("0, ");
				}
				else if(Get_NBit_Xbuf(spi[j]->nval_t1, i) == 1){
						printf("1, ");
				}
				//2時刻目
				if(Get_NBit_Xbuf(spi[j]->nval_t2, i) == 0){
						printf("0)\n");
				}
				else if(Get_NBit_Xbuf(spi[j]->nval_t2, i) == 1){
						printf("1)\n");
				}
			}

			//PPI
			for(j=0; j<n_ppi; j++){
				printf("name:%s  type:PPI  nval:(", ppi[j]->name);
				//1時刻目
				if(Get_NBit_Xbuf(ppi[j]->nval_t1, i) == 0){
						printf("0, ");
				}
				else if(Get_NBit_Xbuf(ppi[j]->nval_t1, i) == 1){
						printf("1, ");
				}
				//2時刻目
				if(Get_NBit_Xbuf(ppi[j]->nval_t2, i) == 0){
						printf("0)\n");
				}
				else if(Get_NBit_Xbuf(ppi[j]->nval_t2, i) == 1){
						printf("1)\n");
				}
			}

			printf("\n");

			//PI,PPI以外
			for(j=0; j<max_level+1; j++){
				for(k=0; k<nml_slev[j].n_net; k++){
					if(nml_slev[j].net[k]->type!=IN && nml_slev[j].net[k]->type!=DFF){
						printf("name:%s  type:%d  nval:", nml_slev[j].net[k]->name, nml_slev[j].net[k]->type);
						//1時刻目
						if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t1, i) == 0){
							printf("0, ");
						}
						else if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t1, i) == 1){
							printf("1, ");
						}
						//2時刻目
						if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t2, i) == 0){
							printf("0)\n");
						}
						else if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t2, i) == 1){
							printf("1)\n");
						}
					}
				}
				printf("\n");
			}
			printf("\n");
		}
		printf("\n//--------------------------------------\n");
		printf("// X抽出後テスト集合の論理値確認\n");
		printf("//--------------------------------------\n");
		for(i=0; i<n_tp; i++){
			printf("-------------------------\n");
			printf(" XID %dパターン目\n", i);
			//PI
			for(j=0; j<n_spi; j++){
				printf("name:%s  type:PI  nval:(", spi[j]->name);
				//1時刻目
				if(Get_NBit(spi[j]->xid_nval_t1, i) == 0){
						printf("0, ");
				}
				else if(Get_NBit(spi[j]->xid_nval_t1, i) == 1){
						printf("1, ");
				}
				else if(Get_NBit(spi[j]->xid_nval_t1, i) == 3){
						printf("X, ");
				}
				//2時刻目
				if(Get_NBit(spi[j]->xid_nval_t2, i) == 0){
						printf("0)\n");
				}
				else if(Get_NBit(spi[j]->xid_nval_t2, i) == 1){
						printf("1)\n");
				}
				else if(Get_NBit(spi[j]->xid_nval_t2, i) == 3){
						printf("X)\n");
				}
			}

			//PPI
			for(j=0; j<n_ppi; j++){
				printf("name:%s  type:PPI  nval:(", ppi[j]->name);
				//1時刻目
				if(Get_NBit(ppi[j]->xid_nval_t1, i) == 0){
						printf("0, ");
				}
				else if(Get_NBit(ppi[j]->xid_nval_t1, i) == 1){
						printf("1, ");
				}
				else if(Get_NBit(ppi[j]->xid_nval_t1, i) == 3){
						printf("X, ");
				}
				//2時刻目
				if(Get_NBit(ppi[j]->xid_nval_t2, i) == 0){
						printf("0)\n");
				}
				else if(Get_NBit(ppi[j]->xid_nval_t2, i) == 1){
						printf("1)\n");
				}
				else if(Get_NBit(ppi[j]->xid_nval_t2, i) == 3){
						printf("X)\n");
				}
			}

			printf("\n");
			
			//PI,PPI以外
			for(j=0; j<max_level+1; j++){
				for(k=0; k<nml_slev[j].n_net; k++){
					if(nml_slev[j].net[k]->type!=IN && nml_slev[j].net[k]->type!=DFF){
						printf("name:%s  type:%d  nval:", nml_slev[j].net[k]->name, nml_slev[j].net[k]->type);
						//1時刻目
						if(Get_NBit(nml_slev[j].net[k]->xid_nval_t1, i) == 0){
							printf("0, ");
						}
						else if(Get_NBit(nml_slev[j].net[k]->xid_nval_t1, i) == 1){
							printf("1, ");
						}
						else if(Get_NBit(nml_slev[j].net[k]->xid_nval_t1, i) == 3){
							printf("X, ");
						}
						//2時刻目
						if(Get_NBit(nml_slev[j].net[k]->xid_nval_t2, i) == 0){
							printf("0)\n");
						}
						else if(Get_NBit(nml_slev[j].net[k]->xid_nval_t2, i) == 1){
							printf("1)\n");
						}
						else if(Get_NBit(nml_slev[j].net[k]->xid_nval_t2, i) == 3){
							printf("X)\n");
						}
					}
				}
				printf("\n");
			}
			printf("\n");
		}
		printf("\n");	
#endif
	//===========================================================
	// フラグ初期化
	//===========================================================
	for(i=0; i<n_snet; i++)s_nl[i].flag = 0;

	
	//===========================================================
	// 論理シミュレーション (X-filling関数内部で含意操作してるのでいらない？)
	//===========================================================
	//TDF_3v_XID_PPSFP_LSIM();		//テストパターン印加もここで

	
	//===========================================================
	// 故障シミュレーション (PPSFP)※未検出故障存在FFRのみ対象
	//===========================================================
	fprintf(stderr,"\n\n TDF_3v_XID_PPSFP_FSIM\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
		
	//--------------------------------------------------
	// ■2時刻目PPOに対してクリティカルパストレーシング
	//--------------------------------------------------
	//※FFRはPO，PPO，STEMの順番
	for(i=n_spo; i<(n_spo+n_ppo); i++){

		// Progress Bar
		if(((int)i % (int)((n_ffr*2/100)+1)) == 0)fprintf(stderr,"*");
		
#ifdef DEBUG
		printf("\n//------------------------\n");
		printf("// PPO[%d]: %s\n", i, sffr[i].FoutStem->name);
		printf("//------------------------\n");
#endif		
		//対象FFR内に未検出故障が存在
		if(sffr[i].n_xid_detect < sffr[i].n_det_fault){

			//テストパターンでループ
			for(j=0; (int)j<n_tp; j++){

				//----------------------------------------------------------------
				//対象FFR内の全故障を検出
				//----------------------------------------------------------------
				if(sffr[i].n_xid_detect == sffr[i].n_det_fault){
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
					//X抽出後の2時刻目正常値取得
					//------------------------------------
					nval_t2 = Get_NBit(sffr[i].FoutStem->xid_nval_t2, j);
					
					//------------------------------------
					// 2時刻目正常値=0
					//------------------------------------
					if(nval_t2 == 0){

						//1時刻目正常値=1 && 対象故障が未検出
						if(Get_NBit(sffr[i].FoutStem->xid_nval_t1, j)==1 && sffr[i].FoutStem->xid_det_stf==NO){

							//対象信号線の立下り遷移故障(1→0/1)を検出
							sffr[i].FoutStem->xid_det_stf = YES;		

							//対象故障がテスト対象の場合
							if(sffr[i].FoutStem->test_stf == YES){
								sffr[i].n_xid_detect++;					//対象FFRの検出故障数更新
								n_xid_detect++;							//X抽出後テスト集合の検出故障数更新
							}
#ifdef DEBUG
							printf("STF %s\n", sffr[i].FoutStem->name);
#endif
						}

						//CPT開始
						TDF_3v_XID_PPSFP_CPT0(i, j, sffr[i].FoutStem);
					}

					//------------------------------------
					// 2時刻目正常値=1
					//------------------------------------
					else if(nval_t2 == 1){

						//1時刻目正常値=0 && 対象故障が未検出
						if(Get_NBit(sffr[i].FoutStem->xid_nval_t1, j)==0 && sffr[i].FoutStem->xid_det_str==NO){

							//対象信号線の立上り遷移故障(0→1/0)を検出
							sffr[i].FoutStem->xid_det_str = YES;
							
							//対象故障がテスト対象の場合
							if(sffr[i].FoutStem->test_str == YES){
								sffr[i].n_xid_detect++;					//対象FFRの検出故障数更新
								n_xid_detect++;							//X抽出後テスト集合の検出故障数更新
							}
#ifdef DEBUG
							printf("STR %s\n", sffr[i].FoutStem->name);
#endif
						}
						
						//CPT開始
						TDF_3v_XID_PPSFP_CPT1(i, j, sffr[i].FoutStem);
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
		printf("//開始STEM： %s\n", sffr[i].FoutStem->name);
		printf("//------------------------\n");
#endif
		
		//----------------------------------------------------------------
		// PPSFPノーマルイベントドリブン
		//----------------------------------------------------------------
		//対象FFR内に未検出故障が存在
		if(sffr[i].n_xid_detect < sffr[i].n_det_fault){

			//------------------------------------
			//確保したunsigned int数分ループ
			//------------------------------------
			for(j=0; j<n_tp_int; j++){
				
				//----------------------------------------------------------------
				//対象FFR内の全故障を検出
				//----------------------------------------------------------------
				if(sffr[i].n_xid_detect == sffr[i].n_det_fault){
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
					sffr[i].FoutStem->x_fault = sffr[i].FoutStem->xid_nval_t2->p_buf[j];	//x_fault に p_buffを代入
					sffr[i].FoutStem->p_fault = sffr[i].FoutStem->xid_nval_t2->x_buf[j];	//p_fault に x_buffを代入
#ifdef DEBUG
					//--DEBUG-----------------------------------------
					printf("故障値確認\n");
					for(k=0; k<n_tp; k++){
						xval = sffr[i].FoutStem->x_fault & MASK[k];
						pval = sffr[i].FoutStem->p_fault & MASK[k];
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
					sffr[i].FoutStem->flag = ed_flag;				//対象ステム信号線にflag値を代入
			
					//PPSFPイベントドリブン開始
					TDF_3v_XID_ED_PPSFP(i, sffr[i].FoutStem, ed_flag, j);	//3値PPSFPイベントドリブン
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
		if(s_nl[i].test_str==YES && s_nl[i].det_str>0 && s_nl[i].xid_det_str==NO){
			printf("STR %s\n", s_nl[i].name);
		}
		if(s_nl[i].test_stf==YES && s_nl[i].det_stf>0 && s_nl[i].xid_det_stf==NO){
			printf("STF %s\n", s_nl[i].name);
		}
	}
#endif

}//END
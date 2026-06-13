//------------------------------------------------------------------------
//File name : TDF_m2004_XID.c
//Date : 2013/10/17
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			TDF_2v_SPPFP			(int, SXID_LIST**, int, unsigned int);
void			TDF_3v_SPPFP			(int, SXID_LIST**, int, unsigned int);
unsigned int	TDF_2v_xfilling			(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*);
unsigned int	TDF_3v_xfilling			(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*);
int				TDF_3v_XID_SPPFP_FSIM	(int, unsigned int*, SXID_LIST**);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_m2004_XID
//  機  能 : 【遷移故障】Miyase2004ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱)
//----------------------------------------------
void	TDF_m2004_XID(SXID_LIST	**t_fault){

	int				i,j,k;
	unsigned int	ed_flag=2;								//イベントドリブンフラグ
	unsigned int	xid_flag=2;								//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;								//ドントケア抽出用フラグ(限定正当化に使用)
	int				count=0;								//t_fault内に何個対象故障を入れたかのカウンタ
	int				n_t_fault = n_detect - n_xid_detect;	//X抽出対象故障数(未検出故障数)

		
#ifdef DEBUG
		printf("\n//======================================\n");
		printf("// Miyase2004 X抽出\n");
		printf("//======================================\n");
		printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);
		printf("xid_flag: %d〜\n", xid_flag);
#endif

	//===================================================================
	// フラグ初期化
	//===================================================================
	for(i=0; i<n_snet; i++){
		s_nl[i].flag = 0;
		s_nl[i].xid_flag = 0;
		s_nl[i].jus_flag = 0;
	}
			
	//===================================================================
	// Miyase2004 X抽出
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Undetected Faults (Miyase2004)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	for(i=0; i<n_tp; i++){
		//-------------------------------------------------------------------------------
		// Progress Bar
		//-------------------------------------------------------------------------------
		//if(((int)i % (int)((n_tp*2)+1)) == 0)fprintf(stderr,"*");
		printf("%d回目/%d\r", i+1, n_tp);

#ifdef DEBUG
		printf("\n【%dパターン目】\n", i);
			
		//初期テストパターン
		//PI
		for(j=0; j<n_spi; j++){
			k = Get_NBit(spi[j]->nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
		}
		//PPI
		for(j=0; j<n_ppi; j++){
			k = Get_NBit(ppi[j]->nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
		}
		
		printf(" -> ");

		//X抽出後テストパターン
		//PI
		for(j=0; j<n_spi; j++){
			k = Get_NBit(spi[j]->xid_nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}
		//PPI
		for(j=0; j<n_ppi; j++){
			k = Get_NBit(ppi[j]->xid_nval_t1, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
			}
			else if(k==3){
				printf("X");
			}
		}
		printf("\n");
#endif
		//-------------------------------------------
		// 全未検出故障に対して処理を行ったら終了
		//-------------------------------------------
		if(n_t_fault == 0){
			break;
		}

		for(j=0; j<n_snet; j++){

			//====================================================================================
			// ■立上り遷移故障(0→1/0)の対象故障判定
			//====================================================================================
			if(s_nl[j].test_str==YES && s_nl[j].det_str>1 && s_nl[j].xid_det_str==NO && s_nl[j].n_str_xid<opt.n_xid){	//対象故障かつ1回以上検出かつ未検出？

				if( Get_NBit_INT(fdic_str[i], j)==YES ){	//対象TPで検出するか？
#ifdef DEBUG
					printf("STR %s\n", s_nl[j].name);
#endif
					//テストパターンごとのXID故障数更新
					tp_info[i].n_xid_fault++;

					//対象信号線立上り遷移故障のX抽出実行回数更新
					s_nl[j].n_str_xid++;

					//==================================================
					//故障シミュレーション(SPPFP)⇒X抽出
					//==================================================
					if(count == 31){
						
						//32個目の対象故障を対象故障リストへ追加
						t_fault[count]->net			= &s_nl[j];	//信号線名
						t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)
						
						//対象故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							TDF_2v_SPPFP(i, t_fault, count, ed_flag);									//SPPFP故障SIM
							xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);	//SPPFPからの2値XID
						}
						//【3値FSIM】&【X抽出】
						else{
							TDF_3v_SPPFP(i, t_fault, count, ed_flag);									//SPPFP故障SIM
							xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);	//SPPFPからの3値XID
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

						//対象故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
						jus_flag+=3;

						//----------------------------------------------
						//■X抽出後テストパターンでFSIM(偶発的故障検出)
						//----------------------------------------------
						n_t_fault -= TDF_3v_XID_SPPFP_FSIM(i, &ed_flag, t_fault);		//X抽出対象故障を更新

					}
					//==================================================
					//対象故障リストの更新
					//==================================================
					else{

						//対象故障リストへ追加
						t_fault[count]->net			= &s_nl[j];		//信号線名
						t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)

						//対象故障リスト内の故障数更新
						count++;
					}

				}
			}

			//====================================================================================
			// ■立下り遷移故障(1→0/1)の対象故障判定
			//====================================================================================
			if(s_nl[j].test_stf==YES && s_nl[j].det_stf>1 && s_nl[j].xid_det_stf==NO && s_nl[j].n_stf_xid<opt.n_xid){	//対象故障かつ1回以上検出かつ未検出？

				if( Get_NBit_INT(fdic_stf[i], j)==YES ){	//対象TPで検出するか？
#ifdef DEBUG
					printf("STF %s\n", s_nl[j].name);
#endif
					//テストパターンごとのXID故障数更新
					tp_info[i].n_xid_fault++;

					//対象信号線立下り遷移故障のX抽出実行回数更新
					s_nl[j].n_stf_xid++;

					//==================================================
					//故障シミュレーション(SPPFP)⇒X抽出
					//==================================================
					if(count == 31){
						
						//32個目の対象故障を対象故障リストへ追加
						t_fault[count]->net			= &s_nl[j];	//信号線名
						t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)
						
						//対象故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							TDF_2v_SPPFP(i, t_fault, count, ed_flag);									//SPPFP故障SIM
							xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);	//SPPFPからの2値XID
						}
						//【3値FSIM】&【X抽出】
						else{
							TDF_3v_SPPFP(i, t_fault, count, ed_flag);									//SPPFP故障SIM
							xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);	//SPPFPからの3値XID
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;	//故障設置フラグ初期化

						//対象故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
						jus_flag+=3;
						
						//----------------------------------------------
						//■X抽出後テストパターンでFSIM(偶発的故障検出)
						//----------------------------------------------
						n_t_fault -= TDF_3v_XID_SPPFP_FSIM(i, &ed_flag, t_fault);		//X抽出対象故障を更新

					}
					//==================================================
					//対象故障リストの更新
					//==================================================
					else{

						//対象故障リストへ追加
						t_fault[count]->net			= &s_nl[j];	//信号線名
						t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)

						//対象故障リスト内の故障数更新
						count++;
					}
				}
			}

		}// for文(n_snet)

		//====================================================================================
		//■対象故障リスト内に故障が余っていた場合
		//====================================================================================
		if(count > 0){

			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				TDF_2v_SPPFP(i, t_fault, count, ed_flag);									//SPPFP故障SIM
				xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);	//SPPFPからの2値XID
			}
			//【3値FSIM】&【X抽出】
			else{
				TDF_3v_SPPFP(i, t_fault, count, ed_flag);									//SPPFP故障SIM
				xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);	//SPPFPからの3値XID
			}
			
			//故障設置フラグ初期化
			for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

			//対象故障リスト再初期化
			count = 0;
			
			//EDフラグ更新
			ed_flag+=2;
			jus_flag+=3;

			//----------------------------------------------
			//■X抽出後テストパターンでFSIM(偶発的故障検出)
			//----------------------------------------------
			n_t_fault -= TDF_3v_XID_SPPFP_FSIM(i, &ed_flag, t_fault);		//X抽出対象故障を更新

		}

	}// for文(n_tp)
	
}
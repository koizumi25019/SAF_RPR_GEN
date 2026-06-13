//------------------------------------------------------------------------
//File name : TDF_priority_Essential_XID.c
//Date : 2013/11/22
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"
#include	"../Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			TDF_2v_SPPFP				(int, SXID_LIST**, int, unsigned int);
void			TDF_3v_SPPFP				(int, SXID_LIST**, int, unsigned int);
unsigned int	TDF_priority_2v_xfilling	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*, unsigned int);
unsigned int	TDF_priority_3v_xfilling	(int, SXID_LIST**, int, unsigned int, unsigned int, unsigned int*, unsigned int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_priority_Essential_XID
//  機  能 : ★優先パスモード【必須故障：限定含意・限定正当化】遷移故障ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱), priroty(優先パスフラグ)
//----------------------------------------------
void	TDF_priority_Essential_XID(SXID_LIST **t_fault, unsigned int priority){


	int				i,j,k;
	unsigned int	ed_flag=2;		//イベントドリブンフラグ
	unsigned int	xid_flag=2;		//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;		//ドントケア抽出用フラグ(限定正当化に使用)
	int				count=0;		//t_fault内に何個必須故障を入れたかのカウンタ
	
	
#ifdef DEBUG
	printf("\n//======================================\n");
	printf("// 必須故障X抽出\n");
	printf("//======================================\n");
#endif

#ifdef DEBUG		
	printf("//--------------------------------------\n");
	printf("必須故障リスト\n");
	for(i=0; i<n_tp; i++){
		printf("tp[%d]\n", i);
		for(j=0; j<n_snet; j++){
			// 立上り遷移故障(0→1/0)
			if(flt_stbl[j]->test_str==YES && flt_stbl[j]->det_str==1 && flt_stbl[j]->xid_det_str==NO){	//必須故障かつ未検出？
				if( Get_NBit_INT(fdic_str[i], j)==YES ){	//対象TPで検出するか？
					printf("STR %s\n", flt_stbl[j]->name);
				}
			}						
			// 立下り遷移故障(1→0/1)
			if(flt_stbl[j]->test_stf==YES && flt_stbl[j]->det_stf==1 && flt_stbl[j]->xid_det_stf==NO){	//必須故障かつ未検出？
				if( Get_NBit_INT(fdic_stf[i], j)==YES ){	//対象TPで検出するか？
					printf("STF %s\n", flt_stbl[j]->name);
				}
			}
		}
		printf("\n");
	}
	printf("//--------------------------------------\n");
#endif

	//===============================================
	// フラグ初期化
	//===============================================
	for(i=0; i<n_snet; i++){
		s_nl[i].flag = 0;
		s_nl[i].xid_flag = 0;
		s_nl[i].jus_flag = 0;
	}	
	
	//===============================================
	// 必須故障に対してX抽出
	//===============================================
	fprintf(stderr,"\n\n X-Identification for Essential Faults\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");
	for(i=0; i<n_tp; i++){

		//-------------------------------------------------------------------------------
		// Progress Bar
		//-------------------------------------------------------------------------------
		//if(((int)i % (int)((n_tp*2)+1)) == 0)fprintf(stderr,"*");
		printf("%d回目/%d\r", i+1, n_tp);

#ifdef DEBUG
		printf("\n//---------------------------------------\n");
		printf("//%d番目のテストパターン\n", i);
		printf("//---------------------------------------\n");
		printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);
		printf("xid_flag: %d〜\n", xid_flag);
#endif

		for(j=0; j<n_snet; j++){

			//====================================================================================
			// ■立上り遷移故障(0→1/0)の必須故障判定
			//====================================================================================
			if(flt_stbl[j]->test_str==YES && flt_stbl[j]->det_str==1 && flt_stbl[j]->xid_det_str==NO){	//必須故障かつ未検出？
				if( Get_NBit_INT(fdic_str[i], flt_stbl[j]->n)==YES ){	//対象TPで検出するか？

					//テストパターンごとの必須故障数更新
					tp_info[i].n_essential_fault++;
					tp_info[i].n_xid_fault++;

					//総必須故障数更新
					n_essential_fault++;

					//対象信号線立上り遷移故障(0→1/0)のX抽出実行回数更新
					flt_stbl[j]->n_str_xid++;

					//--------------------------------
					//故障シミュレーション(SPPFP)⇒X抽出
					//--------------------------------
					if(count == 31){
						
						//32個目の必須故障を必須故障リストへ追加
						t_fault[count]->net			= flt_stbl[j];	//信号線名
						t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障(0→1/0))
						
						//必須故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							//SPPFP故障SIM
							TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
							//優先パス2値XID
							xid_flag = TDF_priority_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag, priority);
														
						}
						//【3値FSIM】&【X抽出】
						else{
							//SPPFP故障SIM
							TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
							//優先パス3値XID
							xid_flag = TDF_priority_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag, priority);
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

						//必須故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
						jus_flag+=3;
					}
					//--------------------------------
					//必須故障リストの更新
					//--------------------------------
					else{

						//必須故障リストへ追加
						t_fault[count]->net			= flt_stbl[j];		//信号線名
						t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障(0→1/0))

						//必須故障リスト内の故障数更新
						count++;
					}
				}
			}
						
			//====================================================================================
			// ■立下り遷移故障(1→0/1)の必須故障判定
			//====================================================================================
			if(flt_stbl[j]->test_stf==YES && flt_stbl[j]->det_stf==1 && flt_stbl[j]->xid_det_stf==NO){	//必須故障かつ未検出？
				if( Get_NBit_INT(fdic_stf[i], flt_stbl[j]->n)==YES ){	//対象TPで検出するか？
					
					//テストパターンごとの必須故障数更新
					tp_info[i].n_essential_fault++;
					tp_info[i].n_xid_fault++;

					//総必須故障数更新
					n_essential_fault++;

					//対象信号線立下り遷移故障(1→0/1)のX抽出実行回数更新
					flt_stbl[j]->n_stf_xid++;

					//--------------------------------
					//故障シミュレーション(SPPFP)⇒X抽出
					//--------------------------------
					if(count == 31){
						
						//32個目の必須故障を必須故障リストへ追加
						t_fault[count]->net			= flt_stbl[j];	//信号線名
						t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障(1→0/1))
						
						//必須故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							//SPPFP故障SIM
							TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
							//優先パス2値XID
							xid_flag = TDF_priority_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag, priority);
							
						}
						//【3値FSIM】&【X抽出】
						else{
							//SPPFP故障SIM
							TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
							//優先パス3値XID
							xid_flag = TDF_priority_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag, priority);
						
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;	//故障設置フラグ初期化

						//必須故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
						jus_flag+=3;
					}
					//--------------------------------
					//必須故障リストの更新
					//--------------------------------
					else{

						//必須故障リストへ追加
						t_fault[count]->net			= flt_stbl[j];		//信号線名
						t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障(1→0/1))

						//必須故障リスト内の故障数更新
						count++;
					}
				}
			}
			
		}// for文(n_snet)
		
		//====================================================================================
		//■必須故障リスト内に故障が余っていた場合
		//====================================================================================
		if(count > 0){

			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				//SPPFP故障SIM
				TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
				//優先パス2値XID
				xid_flag = TDF_priority_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag, priority);
				
			}
			//【3値FSIM】&【X抽出】
			else{
				//SPPFP故障SIM
				TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
				//優先パス3値XID
				xid_flag = TDF_priority_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag, priority);
				
			}
			
			//故障設置フラグ初期化
			for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

			//必須故障リスト再初期化
			count = 0;
			
			//EDフラグ更新
			ed_flag+=2;
			jus_flag+=3;
		}

	}// for文(n_tp)

	
	//===============================================
	// DEBUG：必須故障X抽出後のテストパターン確認
	//===============================================
#ifdef DEBUG
	printf("\n//======================================\n");
	printf("// 必須故障X抽出後のテストパターン確認\n");
	printf("//======================================\n");
	for(i=0; i<n_tp; i++){
		printf("tp[%d]\t", i);
		
		//初期テストパターンPI
		for(j=0; j<n_spi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(spi[j]->nval_t1, i);
			}
			//3値
			else{
				k = Get_NBit(spi[j]->nval_t1, i);
			}

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
		
		//初期テストパターンPPI
		for(j=0; j<n_ppi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(ppi[j]->nval_t1, i);
			}
			//3値
			else{
				k = Get_NBit(ppi[j]->nval_t1, i);
			}

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

		printf(" -> ");

		//X抽出後テストパターンPI
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

		//X抽出後テストパターンPPI
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
	}

	//各パターンのX数
	printf("\n//-------------------\n");
	printf("// 各パターンのケアビット数\n");
	printf("//-------------------\n");
	for(i=0; i<n_tp; i++){
		printf("tp[%d]: C=%d, 0=%d, 1=%d\n", i, tp_bit[i].n_cbit, tp_bit[i].n_0bit, tp_bit[i].n_1bit);
	}

	//各PIのX数
	printf("\n//-------------------\n");
	printf("// 各PIのケアビット数\n");
	printf("//-------------------\n");
	for(i=0; i<n_spi; i++){
		printf("pi[%d]: C=%d, 0=%d, 1=%d\n", i, pi_bit[i].n_cbit, pi_bit[i].n_0bit, pi_bit[i].n_1bit);
	}

	//各PPIのX数
	printf("\n//-------------------\n");
	printf("// 各PPIのケアビット数\n");
	printf("//-------------------\n");
	for(i=0; i<n_spi; i++){
		printf("ppi[%d]: C=%d, 0=%d, 1=%d\n", i, ppi_bit[i].n_cbit, ppi_bit[i].n_0bit, ppi_bit[i].n_1bit);
	}

#endif


}//END
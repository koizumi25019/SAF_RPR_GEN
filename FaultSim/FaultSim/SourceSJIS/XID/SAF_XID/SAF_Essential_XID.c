//------------------------------------------------------------------------
//File name : SAF_Essential_XID.c
//Date : 2012/2/18
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../StandardHead.h"
#include	"../../option.h"
#include	"../Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			SAF_2v_SPPFP		(int, XID_LIST**, int, unsigned int);
void			SAF_3v_SPPFP		(int, XID_LIST**, int, unsigned int);
unsigned int	SAF_2v_xfilling		(int, XID_LIST**, int, unsigned int, unsigned int);
unsigned int	SAF_3v_xfilling		(int, XID_LIST**, int, unsigned int, unsigned int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_Essential_XID
//  機  能 : 【必須故障：限定含意・限定正当化】縮退故障ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱)
//----------------------------------------------
void	SAF_Essential_XID(XID_LIST	**t_fault){


	int				i,j,k;
	unsigned int	ed_flag=2;		//イベントドリブンフラグ
	unsigned int	xid_flag=2;		//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
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
			for(j=0; j<n_net; j++){
				// 0縮退故障
				if(nl[j].test_sf0==YES && nl[j].det_sf0==1 && nl[j].xid_det_sf0==NO){	//必須故障かつ未検出？
					if( Get_NBit_INT(fdic_sa0[i], j)==YES ){	//対象TPで検出するか？
						printf("SF0 %s\n", nl[j].name);
					}
				}						
				// 1縮退故障
				if(nl[j].test_sf1==YES && nl[j].det_sf1==1 && nl[j].xid_det_sf1==NO){	//必須故障かつ未検出？
					if( Get_NBit_INT(fdic_sa1[i], j)==YES ){	//対象TPで検出するか？
						printf("SF1 %s\n", nl[j].name);
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
	for(i=0; i<n_net; i++){
		nl[i].flag = 0;
		nl[i].xid_flag = 0;
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
		printf("xid_flag: %d～\n", xid_flag);
#endif

		for(j=0; j<n_net; j++){

			//====================================================================================
			// ■0縮退故障の必須故障判定
			//====================================================================================
			if(nl[j].test_sf0==YES && nl[j].det_sf0==1 && nl[j].xid_det_sf0==NO){	//必須故障かつ未検出？
				if( Get_NBit_INT(fdic_sa0[i], j)==YES ){	//対象TPで検出するか？

					//テストパターンごとの必須故障数更新
					tp_info[i].n_essential_fault++;
					tp_info[i].n_xid_fault++;

					//総必須故障数更新
					n_essential_fault++;

					//対象信号線0縮退故障のX抽出実行回数更新
					nl[j].n_sa0_xid++;

					//--------------------------------
					//故障シミュレーション(SPPFP)⇒X抽出
					//--------------------------------
					if(count == 31){
						
						//32個目の必須故障を必須故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF0;		//故障タイプ(0縮退故障)
						
						//必須故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							SAF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							xid_flag = SAF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag);	//SPPFPからの2値XID
						}
						//【3値FSIM】&【X抽出】
						else{
							SAF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							//xid_flag = SAF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag);	//SPPFPからの3値XID
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

						//必須故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
					}
					//--------------------------------
					//必須故障リストの更新
					//--------------------------------
					else{

						//必須故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF0;		//故障タイプ(0縮退故障)

						//必須故障リスト内の故障数更新
						count++;
					}
				}
			}
						
			//====================================================================================
			// ■1縮退故障の必須故障判定
			//====================================================================================
			if(nl[j].test_sf1==YES && nl[j].det_sf1==1 && nl[j].xid_det_sf1==NO){	//必須故障かつ未検出？
				if( Get_NBit_INT(fdic_sa1[i], j)==YES ){	//対象TPで検出するか？
					
					//テストパターンごとの必須故障数更新
					tp_info[i].n_essential_fault++;
					tp_info[i].n_xid_fault++;

					//総必須故障数更新
					n_essential_fault++;

					//対象信号線1縮退故障のX抽出実行回数更新
					nl[j].n_sa1_xid++;

					//--------------------------------
					//故障シミュレーション(SPPFP)⇒X抽出
					//--------------------------------
					if(count == 31){
						
						//32個目の必須故障を必須故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF1;		//故障タイプ(1縮退故障)
						
						//必須故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							SAF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							xid_flag = SAF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag);	//SPPFPからの2値XID
						}
						//【3値FSIM】&【X抽出】
						else{
							SAF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							//xid_flag = SAF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag);	//SPPFPからの3値XID
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;	//故障設置フラグ初期化

						//必須故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
					}
					//--------------------------------
					//必須故障リストの更新
					//--------------------------------
					else{

						//必須故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF1;		//故障タイプ(1縮退故障)

						//必須故障リスト内の故障数更新
						count++;
					}
				}
			}
			
		}// for文(n_net)
		
		//====================================================================================
		//■必須故障リスト内に故障が余っていた場合
		//====================================================================================
		if(count > 0){

			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				SAF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
				xid_flag = SAF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag);	//SPPFPからの2値XID
			}
			//【3値FSIM】&【X抽出】
			else{
				SAF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
				//xid_flag = SAF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag);	//SPPFPからの3値XID
			}
			
			//故障設置フラグ初期化
			for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

			//必須故障リスト再初期化
			count = 0;
			
			//EDフラグ更新
			ed_flag+=2;
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
		
		//初期テストパターン
		for(j=0; j<n_pi; j++){

			//2値
			if(n_before_x == 0){
				k = Get_NBit_Xbuf(pi[j]->nval, i);
			}
			//3値
			else{
				k = Get_NBit(pi[j]->nval, i);
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

		//X抽出後テストパターン
		for(j=0; j<n_pi; j++){
			k = Get_NBit(pi[j]->xid_nval, i);

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
	for(i=0; i<n_pi; i++){
		printf("pi[%d]: C=%d, 0=%d, 1=%d\n", i, pi_bit[i].n_cbit, pi_bit[i].n_0bit, pi_bit[i].n_1bit);
	}

#endif


}//END
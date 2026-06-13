//------------------------------------------------------------------------
//File name : SAF_EX_XID.c
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

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void			SAF_2v_SPPFP			(int, XID_LIST**, int, unsigned int);
void			SAF_3v_SPPFP			(int, XID_LIST**, int, unsigned int);
unsigned int	SAF_2v_EX_xfilling		(int, XID_LIST**, int, unsigned int, unsigned int);
unsigned int	SAF_3v_EX_xfilling		(int, XID_LIST**, int, unsigned int, unsigned int);
int				SAF_3v_XID_SPPFP_FSIM	(int, unsigned int*, XID_LIST**);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define DEBUG
	//#define EXF_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : SAF_EX_XID
//  機  能 : 【見逃し故障：拡張含意】縮退故障ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出する必須故障リスト)
//----------------------------------------------
void	SAF_EX_XID(XID_LIST	**t_fault){

	int				i,j,k;
	unsigned int	ed_flag=2;								//イベントドリブンフラグ
	unsigned int	xid_flag=2;								//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	int				count=0;								//t_fault内に何個対象故障を入れたかのカウンタ
	int				n_t_fault = n_missed_fault;				//X抽出対象故障数(見逃し故障数)


#ifdef DEBUG
	printf("\n//======================================\n");
	printf("// 見逃し故障X抽出 (見逃し故障数:%d)\n", n_missed_fault);
	printf("//======================================\n");
	printf("ed_flag: %d と %d\n", ed_flag-1, ed_flag);
	printf("xid_flag: %d〜\n", xid_flag);
#endif
	
	//===================================================================
	// フラグ初期化
	//===================================================================
	for(i=0; i<n_net; i++){
		nl[i].flag = 0;
		nl[i].xid_flag = 0;
	}
			
	//===================================================================
	// 見逃し故障 X抽出
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Missed Fault (EX-XID)\n");
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
		for(j=0; j<n_pi; j++){
			k = Get_NBit(pi[j]->nval, i);

			if(k==0){
				printf("0");
			}
			else if(k==1){
				printf("1");
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
#endif
		//-------------------------------------------
		// 全未検出故障に対して処理を行ったら終了
		//-------------------------------------------
		if(n_t_fault == 0){
			break;
		}

		for(j=0; j<n_net; j++){

			//====================================================================================
			// ■0縮退故障の対象故障判定
			//====================================================================================
			if(nl[j].test_sf0==YES && nl[j].det_sf0>=1 && nl[j].xid_det_sf0==NO){	//対象故障かつ1回以上検出かつ未検出？

				if( Get_NBit_INT(fdic_sa0[i], j)==YES ){	//対象TPで検出するか？
#ifdef EXF_DEBUG
					printf("SF0 %s\n", nl[j].name);
#endif
					//==================================================
					//故障シミュレーション(SPPFP)⇒X抽出
					//==================================================
					if(count == 31){
						
						//32個目の対象故障を対象故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF0;		//故障タイプ(0縮退故障)
						
						//対象故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							SAF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							xid_flag = SAF_2v_EX_xfilling(i, t_fault, count, ed_flag, xid_flag);//SPPFPからの2値XID
						}
						//【3値FSIM】&【X抽出】
						else{
							SAF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							xid_flag = SAF_3v_EX_xfilling(i, t_fault, count, ed_flag, xid_flag);//SPPFPからの3値XID
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

						//対象故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;

						//----------------------------------------------
						//■X抽出後テストパターンでFSIM(偶発的故障検出)
						//----------------------------------------------
						n_t_fault -= SAF_3v_XID_SPPFP_FSIM(i, &ed_flag, t_fault);		//X抽出対象故障を更新

					}
					//==================================================
					//対象故障リストの更新
					//==================================================
					else{

						//対象故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF0;		//故障タイプ(0縮退故障)

						//対象故障リスト内の故障数更新
						count++;
					}

				}
			}

			//====================================================================================
			// ■1縮退故障の対象故障判定
			//====================================================================================
			if(nl[j].test_sf1==YES && nl[j].det_sf1>=1 && nl[j].xid_det_sf1==NO){	//対象故障かつ1回以上検出かつ未検出？

				if( Get_NBit_INT(fdic_sa1[i], j)==YES ){	//対象TPで検出するか？
#ifdef EXF_DEBUG
					printf("SF1 %s\n", nl[j].name);
#endif					
					//==================================================
					//故障シミュレーション(SPPFP)⇒X抽出
					//==================================================
					if(count == 31){
						
						//32個目の対象故障を対象故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF1;		//故障タイプ(1縮退故障)
						
						//対象故障リスト内の故障数更新
						count++;

						//【2値FSIM】&【X抽出】
						if(n_before_x == 0){
							SAF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							xid_flag = SAF_2v_EX_xfilling(i, t_fault, count, ed_flag, xid_flag);//SPPFPからの2値XID
						}
						//【3値FSIM】&【X抽出】
						else{
							SAF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
							xid_flag = SAF_3v_EX_xfilling(i, t_fault, count, ed_flag, xid_flag);//SPPFPからの3値XID
						}

						//故障設置フラグ初期化
						for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;	//故障設置フラグ初期化

						//対象故障リスト再初期化
						count = 0;

						//EDフラグ更新
						ed_flag+=2;
						
						//----------------------------------------------
						//■X抽出後テストパターンでFSIM(偶発的故障検出)
						//----------------------------------------------
						n_t_fault -= SAF_3v_XID_SPPFP_FSIM(i, &ed_flag, t_fault);		//X抽出対象故障を更新

					}
					//==================================================
					//対象故障リストの更新
					//==================================================
					else{

						//対象故障リストへ追加
						t_fault[count]->net			= &nl[j];	//信号線名
						t_fault[count]->fault_type	= SAF1;		//故障タイプ(1縮退故障)

						//対象故障リスト内の故障数更新
						count++;
					}
				}
			}

		}// for文(n_net)

		//====================================================================================
		//■対象故障リスト内に故障が余っていた場合
		//====================================================================================
		if(count > 0){

			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				SAF_2v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
				xid_flag = SAF_2v_EX_xfilling(i, t_fault, count, ed_flag, xid_flag);//SPPFPからの2値XID
			}
			//【3値FSIM】&【X抽出】
			else{
				SAF_3v_SPPFP(i, t_fault, count, ed_flag);							//SPPFP故障SIM
				xid_flag = SAF_3v_EX_xfilling(i, t_fault, count, ed_flag, xid_flag);//SPPFPからの3値XID
			}
			
			//故障設置フラグ初期化
			for(k=0; k<count; k++)t_fault[k]->net->nbit_fault = -1;

			//対象故障リスト再初期化
			count = 0;
			
			//EDフラグ更新
			ed_flag+=2;

			//----------------------------------------------
			//■X抽出後テストパターンでFSIM(偶発的故障検出)
			//----------------------------------------------
			n_t_fault -= SAF_3v_XID_SPPFP_FSIM(i, &ed_flag, t_fault);		//X抽出対象故障を更新

		}

	}// for文(n_tp)
	
}
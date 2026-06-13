//------------------------------------------------------------------------
//File name : TDF_distribution_XID.c
//Date : 2013/12/31
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
void			TDF_2v_distribution_LSIM		(int*, int);
void			TDF_3v_distribution_LSIM		(int*, int);
void			TDF_2v_distribution_ppsfp		(int*, int, SXID_LIST*, unsigned int);	
void			TDF_2v_SPPFP					(int, SXID_LIST**, int, unsigned int);
void			TDF_3v_SPPFP					(int, SXID_LIST**, int, unsigned int);
unsigned int	TDF_2v_distribution_xfilling	(int*, int, SXID_LIST*, unsigned int, unsigned int, unsigned int*);

SXID_LIST		*pop_sxid_list					();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	#define DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : TDF_distribution_XID
//  機  能 : 【遷移故障】分散制御ドントケア抽出
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	TDF_distribution_XID(){

	int				i; //j,k; //j, kは未使用変数のため削除
	unsigned int	ed_flag=2;								//イベントドリブンフラグ
	unsigned int	xid_flag=2;								//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;								//ドントケア抽出用フラグ(限定正当化に使用)
//	int				n_t_fault = n_detect - n_xid_detect;		//X抽出対象故障数(未検出故障数) n_t_fautは未使用変数のため削除
	SXID_LIST		*fnet;									//故障信号線の一時格納
//	unsigned int	min_cost;								//最小コスト //min_costは未使用変数のため削除
//	int				min_tp;									//最小コストのテストパターン番号 //min_tpは未使用変数のため削除
	int				count=0;								//何個テストパターンをを入れたかのカウンタ
	int				tp_id[32];								//PPSFP_XIDする32個のテストパターンID
	int				xid_count=0;							//1故障を何個のテストパターンでXIDしたかのカウンタ(XIDのbreak用)

#ifdef DEBUG
		printf("\n//======================================\n");
		printf("// 分散制御ドントケア抽出\n");
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
	// 分散制御ドントケア抽出
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Undetected Faults ");
	
	if(opt.pi_vari){
		fprintf(stderr,"(PI_variance)\n");
	}
	else if(opt.pi_x){
		fprintf(stderr,"(PI+Xnum)\n");
	}
	else if(opt.tp_vari){
		fprintf(stderr,"(TP_variance)\n");
	}
	else if(opt.tp_x){
		fprintf(stderr,"(TP+Xnum)\n");
	}
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");


	//故障リスト内の故障1つずつXID
	while((fnet = pop_sxid_list())!=NULL){

		//ポップした故障が未検出の場合XID実行
		if( (fnet->fault_type==TDF_STR && fnet->net->xid_det_str==NO && fnet->net->n_str_xid<opt.n_xid) || (fnet->fault_type==TDF_STF && fnet->net->xid_det_stf==NO && fnet->net->n_stf_xid<opt.n_xid)){
			
#ifdef DEBUG
			if(fnet->fault_type == TDF_STR){
				printf("STR: ");
			}
			else if(fnet->fault_type == TDF_STF){
				printf("STF: ");
			}
			printf("%s\n", fnet->net->name);
#endif
			//-------------------------------------------------------
			// 初期化
			//-------------------------------------------------------
			//最小コストを初期化
//			min_cost = 0xffffffff;	//unsigned intの最大値代入 //min_costの結果は未使用のため削除

			//テストパターン格納数の初期化
			count = 0;
				
			//1故障のXID回数カウンタの初期化
			xid_count = 0;

			//-------------------------------------------------------
			// 32テストパターンずつ分散制御ドントケア抽出
			//-------------------------------------------------------
			//ポップした故障を検出可能なテストパターンを全てXID⇒最初コストテストパターンを採用
			for(i=0; i<n_tp; i++){
				
				//================================================================
				//■初期テスト集合での検出回数と同じ回数XIDしたらbreak
				//================================================================
				if(fnet->n_detect == xid_count){
					break;
				}
				
				//================================================================
				//■立上り遷移故障：ポップした故障を検出するテストパターンか判定
				//================================================================
				if(fnet->fault_type == TDF_STR){
					if( Get_NBit_INT(fdic_str[i], fnet->net->n)==YES ){
						
						//-----------------------------------------------------
						//32個目のテストパターンを格納⇒XID
						//-----------------------------------------------------
						if(count == 31){

							tp_id[count] = i;		//テストパターンID格納
							count++;				//テストパターン格納数更新
							
							//【2値FSIM】&【X抽出】
							if(n_before_x == 0){
								TDF_2v_distribution_LSIM(tp_id, count);											//分散制御XID用32パターンLSIM
								TDF_2v_distribution_ppsfp(tp_id, count, fnet, ed_flag);								//分散制御XID用PPSFP
								//TDF_2v_distribution_xfilling(tp_id, count, fnet, ed_flag, xid_flag, &jus_flag);	//分散制御XID
							}
							//【3値FSIM】&【X抽出】
							else{
								TDF_3v_distribution_LSIM(tp_id, count);											//分散制御XID用32パターンLSIM
								//TDF_3v_distribution_ppsfp(tp_id, count, fnet, ed_flag);								//分散制御XID用PPSFP
								//TDF_3v_distribution_xfilling(tp_id, count, fnet, ed_flag, xid_flag, &jus_flag);	//分散制御XID
							}

							//EDフラグ更新
							ed_flag+=2;
							jus_flag+=3;


							//テストパターン格納数カウンタの初期化
							count = 0;
						}
						
						//-----------------------------------------------------
						//テストパターンリストに新しいテストパターンを格納
						//-----------------------------------------------------
						else{
							tp_id[count] = i;		//テストパターンID格納
							count++;				//テストパターン格納数更新
						}
					}
				}
				
				//================================================================
				//■立下り遷移故障：ポップした故障を検出するテストパターンか判定
				//================================================================
				else if(fnet->fault_type == TDF_STF){
					if( Get_NBit_INT(fdic_stf[i], fnet->net->n)==YES ){
						
						//-----------------------------------------------------
						//32個目のテストパターンを格納⇒XID
						//-----------------------------------------------------
						if(count == 31){

							tp_id[count] = i;		//テストパターンID格納
							count++;				//テストパターン格納数更新
							
							//【2値FSIM】&【X抽出】
							if(n_before_x == 0){
								TDF_2v_distribution_LSIM(tp_id, count);											//分散制御XID用32パターンLSIM
								//TDF_2v_distribution_ppsfp(tp_id, count, fnet, ed_flag);								//分散制御XID用PPSFP
								//TDF_2v_distribution_xfilling(tp_id, count, fnet, ed_flag, xid_flag, &jus_flag);	//分散制御XID
							}
							//【3値FSIM】&【X抽出】
							else{
								TDF_3v_distribution_LSIM(tp_id, count);											//分散制御XID用32パターンLSIM
								//TDF_3v_distribution_ppsfp(tp_id, count, fnet, ed_flag);								//分散制御XID用PPSFP
								//TDF_3v_distribution_xfilling(tp_id, count, fnet, ed_flag, xid_flag, &jus_flag);	//分散制御XID
							}

							//EDフラグ更新
							ed_flag+=2;
							jus_flag+=3;


							//テストパターン格納数カウンタの初期化
							count = 0;
						}
						
						//-----------------------------------------------------
						//テストパターンリストに新しいテストパターンを格納
						//-----------------------------------------------------
						else{
							tp_id[count] = i;		//テストパターンID格納
							count++;				//テストパターン格納数更新
						}
					}
				}

			}
			
			//-------------------------------------------------------
			// テストパターンが余っていた場合⇒分散制御ドントケア抽出
			//-------------------------------------------------------
			if(count > 0){

				//【2値FSIM】&【分散制御X抽出】
				if(n_before_x == 0){
					TDF_2v_distribution_LSIM(tp_id, count);											//分散制御XID用32パターンLSIM
					TDF_2v_distribution_ppsfp(tp_id, count, fnet, ed_flag);								//分散制御XID用PPSFP
					TDF_2v_distribution_xfilling(tp_id, count, fnet, ed_flag, xid_flag, &jus_flag);	//分散制御XID
				}
				//【3値FSIM】&【分散制御X抽出】
				else{
					TDF_3v_distribution_LSIM(tp_id, count);											//分散制御XID用32パターンLSIM
					//TDF_3v_distribution_ppsfp(tp_id, count, fnet, ed_flag);								//分散制御XID用PPSFP
					//TDF_3v_distribution_xfilling(tp_id, count, fnet, ed_flag, xid_flag, &jus_flag);	//分散制御XID
				}
				
				//EDフラグ更新
				ed_flag+=2;
				jus_flag+=3;

				//テストパターン格納数カウンタの初期化
				count = 0;
			}
			
			//-------------------------------------------------------
			// ポップ故障を検出するテストパターンを最終決定
			//-------------------------------------------------------
			//テストパターンの最終決定

			//X抽出後テストパターンでFSIM(偶発的故障検出)
		}

	}

}
//------------------------------------------------------------------------
//File name : TDF_m2008_XID.c
//Date : 2013/12/23
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
//  関数名 : TDF_m2008_XID
//  機  能 : 【遷移故障】Miyase2008ドントケア抽出
//  戻り値 : なし
//  引  数 : t_fault(X抽出対象故障リストの空の箱)
//----------------------------------------------
void	TDF_m2008_XID(SXID_LIST	**t_fault){

	int				i,j,k;
	unsigned int	ed_flag=2;								//イベントドリブンフラグ
	unsigned int	xid_flag=2;								//ドントケア抽出用フラグ(伝搬パス，正当化，前方含意に使用)
	unsigned int	jus_flag=3;								//ドントケア抽出用フラグ(限定正当化に使用)
	int				count=0;								//t_fault内に何個対象故障を入れたかのカウンタ
	int				n_t_fault = n_detect - n_xid_detect;	//X抽出対象故障数(未検出故障数)
	int				max_xid = 0;							//全テストパターン中でXIDした故障数の最大値(全パターンでの最大必須故障数)
	SXID_LIST		*temp;									//Miyase2008用の故障信号線の一時格納
		
#ifdef DEBUG
		printf("\n//======================================\n");
		printf("// Miyase2008 X抽出\n");
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
	// 最大XID故障数の探索
	//===================================================================
	for(i=0; i<n_tp; i++){

		//最大XID値の更新
		if(max_xid < tp_info[i].n_xid_fault){
			max_xid = tp_info[i].n_xid_fault;
		}
	}

	//最大XID数の半分の値で検出故障数の均一化を図る
	max_xid = max_xid/2+1;

	//===================================================================
	// Miyase2008 X抽出 1回目 (全テストパターンのXID故障数をmax_xidに揃える)
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Undetected Faults (Miyase2008 : 1回目)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	for(i=0; i<n_tp; i++){
		//-------------------------------------------------------------------------------
		// Progress Bar
		//-------------------------------------------------------------------------------
		//if(((int)i % (int)((n_tp*2)+1)) == 0)fprintf(stderr,"*");
		printf("%d回目/%d\r", i+1, n_tp);
		
		//-------------------------------------------
		// 全未検出故障に対して処理を行ったら終了
		//-------------------------------------------
		if(n_t_fault == 0){
			break;
		}
		
		//-------------------------------------------
		// XIDするテストパターンか判定
		//-------------------------------------------
		if(tp_info[i].n_xid_fault < max_xid){
			
#ifdef DEBUG
			printf("\n\n%dパターン目\n", i);
#endif
			//カウンタ初期化
			count = 0;

			//故障リストの一番最初を代入
			temp = sxid_head.xid_list_head;

			//バグ回避(2014/05/16 by山崎)
			if(sxid_head.n_fault == 1){
				j=0;
			}
			else{
				j=1;
			}

			//故障リスト内の故障をXID
			for(; j<sxid_head.n_fault; j++){

				//====================================================
				// ■立上り遷移故障(0→1/0)の対象故障判定
				//====================================================
				if(temp->fault_type == TDF_STR){

					//未検出か判定
					if(temp->net->xid_det_str==NO && temp->net->n_str_xid<opt.n_xid){

						//現在のテストパターンで検出可能か判定
						if( Get_NBit_INT(fdic_str[i], temp->net->n)==YES ){	//対象TPで検出するか？
#ifdef DEBUG
							printf("STR %s\n", temp->net->name);
#endif
							//テストパターンごとのXID故障数更新
							tp_info[i].n_xid_fault++;

							//対象信号線立上り遷移故障のX抽出実行回数更新
							temp->net->n_str_xid++;

							//==================================================
							//故障シミュレーション(SPPFP)⇒X抽出
							//==================================================
							if(count==31 || tp_info[i].n_xid_fault==max_xid){
						
								//32個目の対象故障を対象故障リストへ追加
								t_fault[count]->net			= temp->net;	//信号線名
								t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)
						
								//対象故障リスト内の故障数更新
								count++;

								//【2値FSIM】&【X抽出】
								if(n_before_x == 0){
									//SPPFP故障SIM
									TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
									//2値XID
									xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
								}
								//【3値FSIM】&【X抽出】
								else{
									//SPPFP故障SIM
									TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
									//3値XID
									xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
							
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
							
								//----------------------------------------------
								// XID故障数が均一になったら終了
								//----------------------------------------------
								if(tp_info[i].n_xid_fault  == max_xid){
									break;
								}
							}
							//==================================================
							//対象故障リストの更新
							//==================================================
							else{

								//対象故障リストへ追加
								t_fault[count]->net			= temp->net;	//信号線名
								t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)

								//対象故障リスト内の故障数更新
								count++;
							}
						}
					}
				}
				
				//====================================================
				// ■立下り遷移故障(1→0/1)の対象故障判定
				//====================================================
				else if(temp->fault_type == TDF_STF){

					//未検出か判定
					if(temp->net->xid_det_stf==NO && temp->net->n_stf_xid<opt.n_xid){

						//現在のテストパターンで検出可能か判定//現在のテストパターンで検出可能か判定
						if( Get_NBit_INT(fdic_stf[i], temp->net->n)==YES ){	//対象TPで検出するか？
#ifdef DEBUG
							printf("STF %s\n", temp->net->name);
#endif
							//テストパターンごとのXID故障数更新
							tp_info[i].n_xid_fault++;

							//対象信号線立下り遷移故障のX抽出実行回数更新
							temp->net->n_stf_xid++;

							//==================================================
							//故障シミュレーション(SPPFP)⇒X抽出
							//==================================================
							if(count==31 || tp_info[i].n_xid_fault==max_xid){
						
								//32個目の対象故障を対象故障リストへ追加
								t_fault[count]->net			= temp->net;	//信号線名
								t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)
						
								//対象故障リスト内の故障数更新
								count++;

								//【2値FSIM】&【X抽出】
								if(n_before_x == 0){
									//SPPFP故障SIM
									TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
									//2値XID
									xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
								}
								//【3値FSIM】&【X抽出】
								else{
									//SPPFP故障SIM
									TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
									//3値XID
									xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
							
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
							
								//----------------------------------------------
								// XID故障数が均一になったら終了
								//----------------------------------------------
								if(tp_info[i].n_xid_fault  == max_xid){
									break;
								}
							}
							//==================================================
							//対象故障リストの更新
							//==================================================
							else{

								//対象故障リストへ追加
								t_fault[count]->net			= temp->net;	//信号線名
								t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)

								//対象故障リスト内の故障数更新
								count++;
							}
						}
					}
				}

				//====================================================
				// 対象故障の更新
				//====================================================
				temp = temp->next;
			}

			//====================================================================================
			//■対象故障リスト内に故障が余っていた場合
			//====================================================================================
			if(count > 0){

				//【2値FSIM】&【X抽出】
				if(n_before_x == 0){
					//SPPFP故障SIM
					TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
					//2値XID
					xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
				
				}
				//【3値FSIM】&【X抽出】
				else{
					//SPPFP故障SIM
					TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
					//3値XID
					xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
				
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
		}
	}

	//printf("\n検出故障数:%d\n", n_detect);for(i=0; i<n_tp; i++)printf("tp[%d]: 必須:%d 検出:%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);

			
	//===================================================================
	// Miyase2008 X抽出 2回目 (全未検出故障を均等にXIDしていく)
	//===================================================================
	fprintf(stderr,"\n\n X-Identification for Undetected Faults (Miyase2008 : 2回目)\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	for(i=0; i<n_tp; i++){
		//-------------------------------------------------------------------------------
		// Progress Bar
		//-------------------------------------------------------------------------------
		//if(((int)i % (int)((n_tp*2)+1)) == 0)fprintf(stderr,"*");
		printf("%d回目/%d\r", i+1, n_tp);
		
		//-------------------------------------------
		// 全未検出故障に対して処理を行ったら終了
		//-------------------------------------------
		if(n_t_fault == 0){
			break;
		}
		
		//-------------------------------------------
		// XIDするテストパターンか判定
		//-------------------------------------------
#ifdef DEBUG
		printf("\n\n%dパターン目\n", i);
#endif
		//カウンタ初期化
		count = 0;

		//故障リストの一番最初を代入
		temp = sxid_head.xid_list_head;
		
		//バグ回避(2014/05/16 by山崎)
		if(sxid_head.n_fault == 1){
			j=0;
		}
		else{
			j=1;
		}

		//故障リスト内の故障をXID
		for(; j<sxid_head.n_fault; j++){

			//====================================================
			// ■立上り遷移故障(0→1/0)の対象故障判定
			//====================================================
			if(temp->fault_type == TDF_STR){

				//未検出か判定
				if(temp->net->xid_det_str==NO && temp->net->n_str_xid<opt.n_xid){

					//現在のテストパターンで検出可能か判定
					if( Get_NBit_INT(fdic_str[i], temp->net->n)==YES ){	//対象TPで検出するか？
#ifdef DEBUG
						printf("STR %s\n", temp->net->name);
#endif
						//テストパターンごとのXID故障数更新
						tp_info[i].n_xid_fault++;

						//対象信号線立上り遷移故障のX抽出実行回数更新
						temp->net->n_str_xid++;

						//==================================================
						//故障シミュレーション(SPPFP)⇒X抽出
						//==================================================
						if(count==31 || tp_info[i].n_xid_fault==max_xid){
						
							//32個目の対象故障を対象故障リストへ追加
							t_fault[count]->net			= temp->net;	//信号線名
							t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)
						
							//対象故障リスト内の故障数更新
							count++;

							//【2値FSIM】&【X抽出】
							if(n_before_x == 0){
								//SPPFP故障SIM
								TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
								//2値XID
								xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
							}
							//【3値FSIM】&【X抽出】
							else{
								//SPPFP故障SIM
								TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
								//3値XID
								xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
							
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
							t_fault[count]->net			= temp->net;	//信号線名
							t_fault[count]->fault_type	= TDF_STR;		//故障タイプ(立上り遷移故障)

							//対象故障リスト内の故障数更新
							count++;
						}
					}
				}
			}
				
			//====================================================
			// ■立下り遷移故障(1→0/1)の対象故障判定
			//====================================================
			else if(temp->fault_type == TDF_STF){

				//未検出か判定
				if(temp->net->xid_det_stf==NO && temp->net->n_stf_xid<opt.n_xid){

					//現在のテストパターンで検出可能か判定//現在のテストパターンで検出可能か判定
					if( Get_NBit_INT(fdic_stf[i], temp->net->n)==YES ){	//対象TPで検出するか？
#ifdef DEBUG
						printf("STF %s\n", temp->net->name);
#endif
						//テストパターンごとのXID故障数更新
						tp_info[i].n_xid_fault++;

						//対象信号線立下り遷移故障のX抽出実行回数更新
						temp->net->n_stf_xid++;

						//==================================================
						//故障シミュレーション(SPPFP)⇒X抽出
						//==================================================
						if(count==31 || tp_info[i].n_xid_fault==max_xid){
						
							//32個目の対象故障を対象故障リストへ追加
							t_fault[count]->net			= temp->net;	//信号線名
							t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)
						
							//対象故障リスト内の故障数更新
							count++;

							//【2値FSIM】&【X抽出】
							if(n_before_x == 0){
								//SPPFP故障SIM
								TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
								//2値XID
								xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
							}
							//【3値FSIM】&【X抽出】
							else{
								//SPPFP故障SIM
								TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
								//3値XID
								xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
							
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
							t_fault[count]->net			= temp->net;	//信号線名
							t_fault[count]->fault_type	= TDF_STF;		//故障タイプ(立下り遷移故障)

							//対象故障リスト内の故障数更新
							count++;
						}
					}
				}
			}

			//====================================================
			// 対象故障の更新
			//====================================================
			temp = temp->next;
		}

		//====================================================================================
		//■対象故障リスト内に故障が余っていた場合
		//====================================================================================
		if(count > 0){

			//【2値FSIM】&【X抽出】
			if(n_before_x == 0){
				//SPPFP故障SIM
				TDF_2v_SPPFP(i, t_fault, count, ed_flag);
							
				//2値XID
				xid_flag = TDF_2v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
				
			}
			//【3値FSIM】&【X抽出】
			else{
				//SPPFP故障SIM
				TDF_3v_SPPFP(i, t_fault, count, ed_flag);
							
				//3値XID
				xid_flag = TDF_3v_xfilling(i, t_fault, count, ed_flag, xid_flag, &jus_flag);
				
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
	}
}
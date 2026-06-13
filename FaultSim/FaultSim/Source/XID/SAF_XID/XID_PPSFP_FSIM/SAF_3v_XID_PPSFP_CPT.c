//------------------------------------------------------------------------
//File name : SAF_3v_XID_CPT.c
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
void SAF_3v_XID_PPSFP_CPT0	(int , unsigned int , NLIST *);
void SAF_3v_XID_PPSFP_CPT1	(int , unsigned int , NLIST *);
void dtob					(unsigned int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

	//#define DEBUG
	//#define TP_DEBUG

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

//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_XID_PPSFP_CPT0
//  機  能 : 【X抽出時用】3値縮退故障クリティカルパストレーシング
//  戻り値 : なし
//  引  数 : ffr_id(FFR番号), tp_id(テストパターンの何番目か), t_net(CPTを始める信号線[=FFRの先頭])
//------------------------------------------------------------------------------------
void	SAF_3v_XID_PPSFP_CPT0(int ffr_id, unsigned int tp_id, NLIST *t_net){

	int i;
	int	n_contorolling=0;	//入力の制御値の本数
	int	n_xbit=0;			//入力のXbitの本数
	int	in;					//t_netのi番目(制御値の場所を格納)
	int	xid_nval;			//X抽出後テストパターンの正常値を一時保存

	switch(t_net->type){
		//********************************************************************
		case BUF:
			//-----------------------------------------------------------
			// 出力=0(入力の1縮退故障が検出可能)
			//-----------------------------------------------------------
			//        |＼
			//        |  ＼
			//   0 ---|BUF >--- 0
			//        |  ／
			//        |／
			
			//対象信号線の1縮退故障の検出状態更新
			if(t_net->in[0]->xid_det_sf1 == NO){

				t_net->in[0]->xid_det_sf1 = YES;		//1縮退を検出
				
				if(t_net->in[0]->test_sf1 == YES){
					ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
					n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
				}
#ifdef DEBUG
				printf("SF1 %s\n", t_net->in[0]->name);
#endif
			}
			
			//さらに入力信号線をCPT
			SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[0]);

		break;
		//********************************************************************
		case INV:
			//-----------------------------------------------------------
			// 出力=0(入力の0縮退故障が検出可能)
			//-----------------------------------------------------------
			//        |＼
			//        |  ＼
			//   1 ---|INV >○-- 0
			//        |  ／
			//        |／

			//対象信号線の0縮退故障の検出状態更新
			if(t_net->in[0]->xid_det_sf0 == NO){
				t_net->in[0]->xid_det_sf0 = YES;		//0縮退を検出

				if(t_net->in[0]->test_sf0 == YES){
					ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
					n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
				}
#ifdef DEBUG
				printf("SF0 %s\n", t_net->in[0]->name);
#endif
			}

			//さらに入力信号線をCPT
			SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[0]);

		break;
		//********************************************************************
		case AND:
			//-----------------------------------------------------------
			// 出力=0(入力の『制御値0の本数』と『Xbitの存在』を確認)
			//-----------------------------------------------------------
			//       ＿＿
			//   0 --|   ＼
			//   X --| AND│--- 0
			//   1 --|   ／
			//       ‾‾

			for(i=0; i<t_net->n_in; i++){
		
				//入力信号線の正常値取得
				xid_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);

				//入力信号線が制御値:0
				if(xid_nval == 0){
					n_contorolling++;
					in = i;
			
					//入力信号線に制御値が2個以上存在
					if(n_contorolling >= 2){
						break;	//故障検出不可能
					}
				}
				//入力信号線が制御値:X
				else if(xid_nval == 3){
					n_xbit++;
					break;	//故障検出不可能
				}
			}

			//-----------------------------------------------------------
			// 入力信号線の『制御値が1本』かつ『Xbitが存在しない』なら故障検出可能
			//-----------------------------------------------------------
			//       ＿＿
			//   0 --|   ＼
			//   1 --| AND│--- 0
			//   1 --|   ／
			//       ‾‾
			if(n_contorolling==1 && n_xbit==0){
					
				//対象信号線の1縮退故障の検出状態更新
				if(t_net->in[in]->xid_det_sf1 == NO){
					t_net->in[in]->xid_det_sf1 = YES;		//1縮退を検出

					if(t_net->in[in]->test_sf1 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF1 %s\n", t_net->in[in]->name);
#endif
				}
				
				//入力信号線のさらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[in]);
			}

		break;
		//********************************************************************
		case NAND:
			//-----------------------------------------------------------
			// 出力=0(入力信号線は全て非制御値⇒入力の0縮退故障が検出可能)
			//-----------------------------------------------------------
			//       ＿＿
			//   1 --|   ＼
			//   1 --|NAND│○-- 0
			//   1 --|   ／
			//       ‾‾      ※全入力信号線=1 (非制御値)

			for(i=0; i<t_net->n_in; i++){

				//対象信号線の0縮退故障の検出状態更新
				if(t_net->in[i]->xid_det_sf0 == NO){
					t_net->in[i]->xid_det_sf0 = YES;		//0縮退を検出

					if(t_net->in[i]->test_sf0 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF0 %s\n", t_net->in[i]->name);
#endif
				}

				//さらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[i]);
			}

		break;
		//********************************************************************
		case OR:
			//-----------------------------------------------------------
			// 出力=0(入力信号線は全て非制御値⇒入力の1縮退故障が検出可能)
			//-----------------------------------------------------------
			//       ＿＿
			//   0 --＼   ＼
			//   0 ----) OR >--- 0
			//   0 --／   ／
			//       ‾‾      ※全入力信号線=0 (非制御値)

			for(i=0; i<t_net->n_in; i++){

				//対象信号線の1縮退故障の検出状態更新
				if(t_net->in[i]->xid_det_sf1 == NO){
					t_net->in[i]->xid_det_sf1 = YES;	//1縮退を検出

					if(t_net->in[i]->test_sf1 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF1 %s\n", t_net->in[i]->name);
#endif
				}

				//さらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[i]);
			}

		break;
		//********************************************************************
		case NOR:
			//-----------------------------------------------------------
			// 出力=0(入力の『制御値0の本数』と『Xbitの存在』を確認)
			//-----------------------------------------------------------
			//       ＿＿
			//   1 --＼   ＼
			//   0 ----)NOR >○-- 0
			//   X --／   ／
			//       ‾‾

			for(i=0; i<t_net->n_in; i++){
		
				//入力信号線の正常値取得
				xid_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);

				//入力信号線が制御値:1
				if(xid_nval == 1){
					n_contorolling++;
					in = i;
			
					//入力信号線に制御値が2個以上存在
					if(n_contorolling >= 2){
						break;	//故障検出不可能
					}
				}
				//入力信号線が制御値:X
				else if(xid_nval == 3){
					n_xbit++;
					break;	//故障検出不可能
				}
			}

			//-----------------------------------------------------------
			// 入力信号線の『制御値が1本』かつ『Xbitが存在しない』なら故障検出可能
			//-----------------------------------------------------------
			//       ＿＿
			//   1 --＼   ＼
			//   0 ----)NOR >○-- 0
			//   0 --／   ／
			//       ‾‾
			if(n_contorolling==1 && n_xbit==0){
				
				//対象信号線の0縮退故障の検出状態更新
				if(t_net->in[in]->xid_det_sf0 == NO){
					t_net->in[in]->xid_det_sf0 = YES;	//0縮退を検出

					if(t_net->in[in]->test_sf0 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF0 %s\n", t_net->in[in]->name);
#endif
				}

				//入力信号線のさらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[in]);
			}

		break;
		//********************************************************************
		case EXOR:
			//-----------------------------------------------------------
			// 出力=0 (※2入力のみなので，入力信号線は全て検出可能)
			//-----------------------------------------------------------
			//       ＿＿＿                     ＿＿＿
			//   0 --＼    ＼               1 --＼    ＼
			//         ))EXOR>--- 0               ))EXOR>--- 0
			//   0 --／    ／               1 --／    ／
			//       ‾‾‾                     ‾‾‾  
	
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------------
				// 入力信号線=0 (1縮退故障が検出可能)
				//----------------------------------------
				if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==0 ){

					//対象信号線の1縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf1 == NO){
						t_net->in[i]->xid_det_sf1 = YES;			//1縮退を検出

						if(t_net->in[i]->test_sf1 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF1 %s\n", t_net->in[i]->name);
#endif
					}
	
					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[i]);
				}

				//----------------------------------------
				// 入力信号線=1 (0縮退故障が検出可能)
				//----------------------------------------
				else if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==1 ){

					//対象信号線の0縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf0 == NO){
						t_net->in[i]->xid_det_sf0 = YES;			//0縮退を検出

						if(t_net->in[i]->test_sf0 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF0 %s\n", t_net->in[i]->name);
#endif
					}
	
					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[i]);
				}
			}

		break;
		//********************************************************************
		case EXNOR:
			//-----------------------------------------------------------
			// 出力=0 (※2入力のみなので，入力信号線は全て検出可能)
			//-----------------------------------------------------------
			//       ＿＿＿                     ＿＿＿
			//   1 --＼    ＼               0 --＼    ＼
			//        ))EXNOR>○-- 0             ))EXNOR>○-- 0
			//   0 --／    ／               1 --／    ／
			//       ‾‾‾                     ‾‾‾  
	
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------------
				// 入力信号線=0 (1縮退故障が検出可能)
				//----------------------------------------
				if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==0 ){

					//対象信号線の1縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf1 == NO){
						t_net->in[i]->xid_det_sf1 = YES;			//1縮退を検出
						
						if(t_net->in[i]->test_sf1 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF1 %s\n", t_net->in[i]->name);
#endif
					}
	
					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[i]);
				}

				//----------------------------------------
				// 入力信号線=1 (0縮退故障が検出可能)
				//----------------------------------------
				else if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==1 ){

					//対象信号線の0縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf0 == NO){
						t_net->in[i]->xid_det_sf0 = YES;			//0縮退を検出

						if(t_net->in[i]->test_sf0 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF0 %s\n", t_net->in[i]->name);
#endif
					}

					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[i]);
				}
			}

		break;
		//********************************************************************
	}

}//END


//------------------------------------------------------------------------------------
//  関数名 : SAF_3v_XID_PPSFP_CPT1
//  機  能 : 【X抽出時用】3値縮退故障クリティカルパストレーシング
//  戻り値 : なし
//  引  数 : ffr_id(FFR番号), tp_id(テストパターンの何番目か), t_net(CPTを始める信号線[=FFRの先頭])
//------------------------------------------------------------------------------------
void	SAF_3v_XID_PPSFP_CPT1(int ffr_id, unsigned int tp_id, NLIST *t_net){
	
	int i;
	int	n_contorolling=0;	//入力の制御値の本数
	int	n_xbit=0;			//入力のXbitの本数
	int	in;					//t_netのi番目(制御値の場所を格納)
	int	xid_nval;			//X抽出後テストパターンの正常値を一時保存

	switch(t_net->type){
		//********************************************************************
		case BUF:
			//-----------------------------------------------------------
			// 出力=1(入力の0縮退故障が検出可能)
			//-----------------------------------------------------------
			//        |＼
			//        |  ＼
			//   1 ---|BUF >--- 1
			//        |  ／
			//        |／

			//対象信号線の0縮退故障の検出状態更新
			if(t_net->in[0]->xid_det_sf0 == NO){
				t_net->in[0]->xid_det_sf0 = YES;	//0縮退故障を検出

				if(t_net->in[0]->test_sf0 == YES){
					ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
					n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
				}
#ifdef DEBUG
				printf("SF0 %s\n", t_net->in[0]->name);
#endif
			}

			//さらに入力信号線をCPT
			SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[0]);

		break;
		//********************************************************************
		case INV:
			//-----------------------------------------------------------
			// 出力=0(入力の0縮退故障が検出可能)
			//-----------------------------------------------------------
			//        |＼
			//        |  ＼
			//   0 ---|INV >○-- 1
			//        |  ／
			//        |／

			//対象信号線の1縮退故障の検出状態更新
			if(t_net->in[0]->xid_det_sf1 == NO){
				t_net->in[0]->xid_det_sf1 = YES;	//1縮退故障を検出

				if(t_net->in[0]->test_sf1 == YES){
					ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
					n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
				}
#ifdef DEBUG
				printf("SF1 %s\n", t_net->in[0]->name);
#endif
			}

			//さらに入力信号線をCPT
			SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[0]);

		break;
		//********************************************************************
		case AND:
			//-----------------------------------------------------------
			// 出力=1(入力信号線は全て非制御値⇒入力の0縮退故障が検出可能)
			//-----------------------------------------------------------
			//       ＿＿
			//   1 --|   ＼
			//   1 --| AND│--- 1
			//   1 --|   ／
			//       ‾‾      ※全入力信号線=1 (非制御値)

			for(i=0; i<t_net->n_in; i++){

				//対象信号線の0縮退故障の検出状態更新
				if(t_net->in[i]->xid_det_sf0 == NO){
					t_net->in[i]->xid_det_sf0 = YES;

					if(t_net->in[i]->test_sf0 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF0 %s\n", t_net->in[i]->name);
#endif
				}

				//さらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[i]);
			}

		break;
		//********************************************************************
		case NAND:
			//-----------------------------------------------------------
			// 出力=1(入力の『制御値0の本数』と『Xbitの存在』を確認)
			//-----------------------------------------------------------
			//       ＿＿
			//   0 --|   ＼
			//   X --|NAND│○-- 1
			//   1 --|   ／
			//       ‾‾

			for(i=0; i<t_net->n_in; i++){
		
				//入力信号線の正常値取得
				xid_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);

				//入力信号線が制御値:0
				if(xid_nval == 0){
					n_contorolling++;
					in = i;
			
					//入力信号線に制御値が2個以上存在
					if(n_contorolling >= 2){
						break;	//故障検出不可能
					}
				}
				//入力信号線が制御値:X
				else if(xid_nval == 3){
					n_xbit++;
					break;	//故障検出不可能
				}
			}

			//-----------------------------------------------------------
			// 入力信号線の『制御値が1本』かつ『Xbitが存在しない』なら故障検出可能
			//-----------------------------------------------------------
			//       ＿＿
			//   0 --|   ＼
			//   1 --|NAND│○-- 1
			//   1 --|   ／
			//       ‾‾
			if(n_contorolling==1 && n_xbit==0){

				//対象信号線の1縮退故障の検出状態更新
				if(t_net->in[in]->xid_det_sf1 == NO){
					t_net->in[in]->xid_det_sf1 = YES;	//1縮退故障を検出

					if(t_net->in[in]->test_sf1 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF1 %s\n", t_net->in[in]->name);
#endif
				}

				//入力信号線のさらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[in]);
			}

		break;
		//********************************************************************
		case OR:
			//-----------------------------------------------------------
			// 出力=0(入力の『制御値0の本数』と『Xbitの存在』を確認)
			//-----------------------------------------------------------
			//       ＿＿
			//   1 --＼   ＼
			//   0 ----) OR >--- 1
			//   X --／   ／
			//       ‾‾

			for(i=0; i<t_net->n_in; i++){
		
				//入力信号線の正常値取得
				xid_nval = Get_NBit(t_net->in[i]->xid_nval, tp_id);

				//入力信号線が制御値:1
				if(xid_nval == 1){
					n_contorolling++;
					in = i;
			
					//入力信号線に制御値が2個以上存在
					if(n_contorolling >= 2){
						break;	//故障検出不可能
					}
				}
				//入力信号線が制御値:X
				else if(xid_nval == 3){
					n_xbit++;
					break;	//故障検出不可能
				}
			}

			//-----------------------------------------------------------
			// 入力信号線の『制御値が1本』かつ『Xbitが存在しない』なら故障検出可能
			//-----------------------------------------------------------
			//       ＿＿
			//   1 --＼   ＼
			//   0 ----) OR >--- 1
			//   0 --／   ／
			//       ‾‾
			if(n_contorolling==1 && n_xbit==0){

				//対象信号線の0縮退故障の検出状態更新
				if(t_net->in[in]->xid_det_sf0 == NO){
					t_net->in[in]->xid_det_sf0 = YES;	//0縮退故障を検出

					if(t_net->in[in]->test_sf0 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF0 %s\n", t_net->in[in]->name);
#endif
				}

				//入力信号線のさらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[in]);
			}

		break;
		//********************************************************************
		case NOR:
			//-----------------------------------------------------------
			// 出力=0(入力信号線は全て非制御値⇒入力の1縮退故障が検出可能)
			//-----------------------------------------------------------
			//       ＿＿
			//   0 --＼   ＼
			//   0 ----)NOR >○-- 1
			//   0 --／   ／
			//       ‾‾      ※全入力信号線=0 (非制御値)

			for(i=0; i<t_net->n_in; i++){

				//対象信号線の1縮退故障の検出状態更新
				if(t_net->in[i]->xid_det_sf1 == NO){
					t_net->in[i]->xid_det_sf1 = YES;	//1縮退故障を検出

					if(t_net->in[i]->test_sf1 == YES){
						ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
						n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
					}
#ifdef DEBUG
					printf("SF1 %s\n", t_net->in[i]->name);
#endif
				}

				//さらに入力信号線をCPT
				SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[i]);
			}

		break;
		//********************************************************************
		case EXOR:
			//-----------------------------------------------------------
			// 出力=1 (※2入力のみなので，入力信号線は全て検出可能)
			//-----------------------------------------------------------
			//       ＿＿＿                     ＿＿＿
			//   1 --＼    ＼               0 --＼    ＼
			//         ))EXOR>--- 1               ))EXOR>--- 1
			//   0 --／    ／               1 --／    ／
			//       ‾‾‾                     ‾‾‾  
	
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------------
				// 入力信号線=0 (1縮退故障が検出可能)
				//----------------------------------------
				if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==0 ){

					//対象信号線の1縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf1 == NO){
						t_net->in[i]->xid_det_sf1 = YES;	//1縮退故障を検出

						if(t_net->in[i]->test_sf1 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF1 %s\n", t_net->in[i]->name);
#endif
					}

					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[i]);
				}

				//----------------------------------------
				// 入力信号線=1 (0縮退故障が検出可能)
				//----------------------------------------
				else if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==1 ){

					//対象信号線の0縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf0 == NO){
						t_net->in[i]->xid_det_sf0 = YES;	//0縮退故障を検出

						if(t_net->in[i]->test_sf0 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF0 %s\n", t_net->in[i]->name);
#endif
					}

					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[i]);
				}
			}

		break;
		//********************************************************************
		case EXNOR:
			//-----------------------------------------------------------
			// 出力=1 (※2入力のみなので，入力信号線は全て検出可能)
			//-----------------------------------------------------------
			//       ＿＿＿                     ＿＿＿
			//   0 --＼    ＼               1 --＼    ＼
			//        ))EXNOR>○-- 1             ))EXNOR>○-- 1
			//   0 --／    ／               1 --／    ／
			//       ‾‾‾                     ‾‾‾  
	
			for(i=0; i<t_net->n_in; i++){
				//----------------------------------------
				// 入力信号線=0 (1縮退故障が検出可能)
				//----------------------------------------
				if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==0 ){

					//対象信号線の1縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf1 == NO){
						t_net->in[i]->xid_det_sf1 = YES;	//1縮退故障を検出

						if(t_net->in[i]->test_sf1 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF1 %s\n", t_net->in[i]->name);
#endif
					}

					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, t_net->in[i]);
				}

				//----------------------------------------
				// 入力信号線=1 (0縮退故障が検出可能)
				//----------------------------------------
				else if( Get_NBit(t_net->in[i]->xid_nval, tp_id)==1 ){

					//対象信号線の0縮退故障の検出状態更新
					if(t_net->in[i]->xid_det_sf0 == NO){
						t_net->in[i]->xid_det_sf0 = YES;	//0縮退故障を検出

						if(t_net->in[i]->test_sf0 == YES){
							ffr[ffr_id].n_xid_detect++;			//対象FFR内の検出故障数更新
							n_xid_detect++;						//X抽出後テスト集合の検出故障数更新
						}
#ifdef DEBUG
						printf("SF0 %s\n", t_net->in[i]->name);
#endif
					}

					//入力信号線のさらに入力信号線をCPT
					SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, t_net->in[i]);
				}
			}

		break;
		//********************************************************************
	}

}//END


//----------------------------------------------
//  関数名 : SAF_FOUT_3v_XID_CPT 
//  機  能 : 【X抽出時用】POまで故障伝搬したTPが何番目か判定し，CPT関数実行
//  戻り値 : なし
//  引  数 : ffr_id(ステムのFFR番号), stem_net(ステム信号線), ui_num(unsigned int番目), det_tp_list(POまで故障伝搬したTPリスト【0:未伝搬 1:伝搬(=故障検出)】), t_po(故障伝搬したPO)
//----------------------------------------------
void	SAF_FOUT_3v_XID_CPT	(int ffr_id, NLIST* stem_net, int ui_num, unsigned int* det_tp_list, NLIST* t_po){

	int				i;
	int				tp_id;		//テストパターンの何番目か
	unsigned int	x_buff;		//正常値x_buff一時保存
	unsigned int	p_buff;		//正常値p_buff一時保存
	unsigned int	fault_tp;	//故障伝搬したTP
#ifdef DEBUG
	int				j,k;
#endif
	
	//===========================================================
	// POの故障伝搬テストパターン判定(故障伝搬パターンに1が立つ)
	//===========================================================
	fault_tp = (t_po->xid_nval->x_buf[ui_num] ^ t_po->x_fault) & (t_po->xid_nval->p_buf[ui_num] ^ t_po->p_fault);	//(X-buff同士のEXOR)AND(P-buff同士のEXOR)
#ifdef TP_DEBUG
	printf("\n正常値X-buf: \n");
	dtob(t_po->xid_nval->x_buf[ui_num], 6);
	printf("\n正常値P-buf: \n");
	dtob(t_po->xid_nval->p_buf[ui_num], 6);
	printf("\n故障値X-buf: \n");
	dtob(t_po->x_fault, 6);
	printf("\n故障値P-buf: \n");
	dtob(t_po->p_fault, 6);

	printf("\nPO(%s)の故障伝搬テストパターン判定\n", t_po->name);
	dtob(fault_tp, 6);
#endif


	//===========================================================
	// テストパターン数が32未満(確保したunsinged int が1個)
	//===========================================================
	if(n_tp_int == 1){

		for(i=0; i<n_tp; i++){
			
			//---------------------------------------------------------------------------
			//対象FFR内の全故障を検出
			//---------------------------------------------------------------------------
			if(ffr[ffr_id].n_xid_detect == ffr[ffr_id].n_det_fault){
#ifdef DEBUG
			printf("break: 対象FFR内の全故障を検出\n");
#endif
				break;
			}
			
			//---------------------------------------------------------------------------
			//対象FFR内に未検出故障が存在
			//---------------------------------------------------------------------------
			else{
				//------------------------------------------------
				//det_tp_listのi番目=0(テストパターンi番目が未検出)
				//------------------------------------------------
				if((*det_tp_list & MASKbit[i]) == 0){
					
					//------------------------------------------------
					//fault_tpのiビット目に『1』が立っているか？(故障検出パターンか)
					//------------------------------------------------
					if( (fault_tp & MASKbit[i]) != 0){
#ifdef DEBUG
						printf("【%d パターン目】", i);
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
						//------------------------------------------------
						//ステム信号線の正常値取得
						//------------------------------------------------
						x_buff = stem_net->xid_nval->x_buf[ui_num] & MASKbit[i];	//ステム信号線のXbufのj番目以外全て0の状態にする
						p_buff = stem_net->xid_nval->p_buf[ui_num] & MASKbit[i];	//ステム信号線のPbufのj番目以外全て0の状態にする
						
#ifdef TP_DEBUG
						printf("ステム(%s)正常値:", stem_net->name);
						if(x_buff==0 && p_buff!=0){
							printf("0\n");
						}
						else if(x_buff!=0 && p_buff==0){
							printf("1\n");
						}
						else if(x_buff!=0 && p_buff!=0){
							printf("X\n");
						}
#endif
						//ステム信号線の正常値がXbitじゃない場合
						if((x_buff&p_buff)==0){

							//==========================================
							// 0縮退故障検出可能(正常値==1)
							//==========================================
							if(x_buff != 0){	//i番目に1が立ってる
																				
								*det_tp_list |= MASKbit[i];		//det_tp_listのi番目に『1』を立てる
								tp_id = i;						//テストパターン番号を求める		

								//対象信号線の0縮退故障の検出状態更新
								if(ffr[ffr_id].FoutStem->xid_det_sf0 == NO){
									ffr[ffr_id].FoutStem->xid_det_sf0 = YES;		//0縮退故障を検出

									if(ffr[ffr_id].FoutStem->test_sf0 == YES){
										ffr[ffr_id].n_xid_detect++;					//対象FFR内の検出故障数更新
										n_xid_detect++;								//X抽出後テスト集合の検出故障数更新
									}
#ifdef DEBUG
									printf("SF0 %s\n", ffr[ffr_id].FoutStem->name);
#endif
								}

								//正常値1のCPT開始
								SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, stem_net);
							}
								
							//==========================================
							// 1縮退故障検出可能(正常値==0)
							//==========================================
							else{	//全部0になった

								*det_tp_list |= MASKbit[i];		//det_tp_listのi番目に『1』を立てる
								tp_id = i;						//テストパターン番号を求める

								//対象信号線の1縮退故障の検出状態更新
								if(ffr[ffr_id].FoutStem->xid_det_sf1 == NO){
									ffr[ffr_id].FoutStem->xid_det_sf1 = YES;	//1縮退故障を検出

									if(ffr[ffr_id].FoutStem->test_sf1 == YES){
										ffr[ffr_id].n_xid_detect++;					//対象FFR内の検出故障数更新
										n_xid_detect++;								//X抽出後テスト集合の検出故障数更新
									}
#ifdef DEBUG
									printf("SF1 %s\n", ffr[ffr_id].FoutStem->name);
#endif
								}

								//正常値0のCPT開始
								SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, stem_net);
							}
						}
					}
				}
			}
		}

	}
	//===========================================================
	// テストパターン数が32以上
	//===========================================================
	else{
		for(i=0; i<32; i++){
			
			//---------------------------------------------------------------------------
			//対象FFR内の全故障を検出
			//---------------------------------------------------------------------------
			if(ffr[ffr_id].n_xid_detect == ffr[ffr_id].n_det_fault){
#ifdef DEBUG
				printf("break: 対象FFR内の全故障を検出\n");
#endif
				break;
			}
			
			//---------------------------------------------------------------------------
			//対象FFR内に未検出故障が存在
			//---------------------------------------------------------------------------
			else{
				//------------------------------------------------
				//det_tp_listのi番目=0(テストパターンi番目が未検出)
				//------------------------------------------------
				if((*det_tp_list & MASKbit[i]) == 0){

					//------------------------------------------------
					//fault_tpのiビット目に『1』が立っているか？(故障検出パターンか)
					//------------------------------------------------
					if( (fault_tp & MASKbit[i]) != 0){
#ifdef DEBUG
						printf("【%d パターン目】\n", i+(ui_num*32));
#endif
						//------------------------------------------------
						//ステム信号線の正常値取得
						//------------------------------------------------
						x_buff = stem_net->xid_nval->x_buf[ui_num] & MASKbit[i];	//ステム信号線のXbufのj番目以外全て0の状態にする
						p_buff = stem_net->xid_nval->p_buf[ui_num] & MASKbit[i];	//ステム信号線のPbufのj番目以外全て0の状態にする
						
						//ステム信号線の正常値がXbitじゃない場合
						if((x_buff&p_buff)==0){

							//==========================================
							// 0縮退故障検出可能(正常値==1)
							//==========================================
							if(x_buff != 0){	//i番目に1が立ってる
																				
								*det_tp_list |= MASKbit[i];		//det_tp_listのi番目に『1』を立てる
								tp_id = (ui_num*32)+i;			//テストパターン番号を求める

								//対象信号線の0縮退故障の検出状態更新
								if(ffr[ffr_id].FoutStem->xid_det_sf0 == NO){
									ffr[ffr_id].FoutStem->xid_det_sf0 = YES;	//0縮退故障を検出

									if(ffr[ffr_id].FoutStem->test_sf0 == YES){
										ffr[ffr_id].n_xid_detect++;					//対象FFR内の検出故障数更新
										n_xid_detect++;								//X抽出後テスト集合の検出故障数更新
									}
#ifdef DEBUG
									printf("SF0 %s\n", ffr[ffr_id].FoutStem->name);
#endif
								}

								//正常値1のCPT開始
								SAF_3v_XID_PPSFP_CPT1(ffr_id, tp_id, stem_net);
							}
								
							//==========================================
							// 1縮退故障検出可能(正常値==0)
							//==========================================
							else{	//全部0になった
										
								*det_tp_list |= MASKbit[i];		//det_tp_listのi番目に『1』を立てる
								tp_id = (ui_num*32)+i;			//テストパターン番号を求める						

								//対象信号線の1縮退故障の検出状態更新
								if(ffr[ffr_id].FoutStem->xid_det_sf1 == NO){
									ffr[ffr_id].FoutStem->xid_det_sf1 = YES;	//1縮退故障を検出

									if(ffr[ffr_id].FoutStem->test_sf1 == YES){
										ffr[ffr_id].n_xid_detect++;					//対象FFR内の検出故障数更新
										n_xid_detect++;								//X抽出後テスト集合の検出故障数更新
									}
#ifdef DEBUG
									printf("SF1 %s\n", ffr[ffr_id].FoutStem->name);
#endif
								}

								//正常値0のCPT開始
								SAF_3v_XID_PPSFP_CPT0(ffr_id, tp_id, stem_net);
							}
						}
					}
				}
			}
		}
	}
}

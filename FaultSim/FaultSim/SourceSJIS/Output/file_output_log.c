//------------------------------------------------------------------------
//File name : file_outpuf_log.c
//Date : 2013/7/7
//Designer : H.Yamazaki
//Ver : 0.01
//
//memo:
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<time.h>

#include	"../StandardHead.h"
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../option.h"
#include	"../XID/Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

#define	DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//--------------------------------------------------------------------
//関数名：file_output_log
//機能　：ログファイル出力
//引数　：fp(出力ファイルポインタ)
//戻り値：なし
//--------------------------------------------------------------------
void file_output_log(FILE* fp, char** file_names, double cput){

	int		i;
	int		count=0;

	//======================================================
	// 全体レポート
	//======================================================
	fprintf(fp, "//-------------------------------------------------------\n");
	fprintf(fp, "// Report\n");
	fprintf(fp, "//-------------------------------------------------------\n");
	if(opt.fsim == YES){
		fprintf(fp, "Mode                            : Fault Simulation\n");
	}
	else if(opt.xid == YES){
		fprintf(fp, "Mode                            : X-Identification\n");
	}
	else if(opt.cpi_xid == YES){
		fprintf(fp, "Mode                            : CPI X-Identification\n");
	}
	
	if(opt.fmodel == SAF){
		fprintf(fp, "Target Fault Model              : SAF\n");
	}
	else if(opt.fmodel == TDF){
		fprintf(fp, "Target Fault Model              : TDF\n");
	}
	fprintf(fp, "Target Circuit                  : %s\n",file_names[F_NET]);

	if(opt.tp_stil == YES){
		fprintf(fp, "Target Test Set                 : %s\n",file_names[F_TPST]);
	}
	else if(opt.tp_txt == YES){
		fprintf(fp, "Target Test Set                 : %s\n",file_names[F_TPTX]);
	}

	fprintf(fp, "#of Test Pattern                : %d\n",n_tp);
	
	if(opt.fmodel == SAF){
		fprintf(fp, "#of Primary Input               : %d\n",n_pi);
		fprintf(fp, "#of Pseudo Primary Input        : %d\n",n_dff);
		fprintf(fp, "#of Initial X-bit Ratio         : %.2lf%%\n",((double)n_before_x/(double)(n_tp*n_pi))*100 );
		if(opt.fault_coverage_rep == YES){
			fprintf(fp, "#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_rep)*100 );
		}
		else{
			fprintf(fp, "#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_net)*100 );
		}
	}
	else if(opt.fmodel == TDF){
		fprintf(fp, "#of Primary Input               : %d\n",n_spi);
		fprintf(fp, "#of Pseudo Primary Input        : %d\n",n_ppi);
		fprintf(fp, "#of Initial X-bit Ratio         : %.2lf%%\n",((double)n_before_x/(double)(n_tp*(n_spi+n_ppi)))*100 );
		if(opt.fault_coverage_rep == YES){
			fprintf(fp, "#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_rep)*100 );
		}
		else{
			fprintf(fp, "#of Fault Coverage              : %.2lf%%\n",((double)n_detect/(double)n_snet)*100 );
		}
	}

	fprintf(fp, "#of Target Fault                : %d\n",n_rep );
	fprintf(fp, "#of Detected Fault              : %d\n",n_detect );
	fprintf(fp, "#of Undetected Fault            : %d\n",n_rep-n_detect );

	if(opt.xid==YES || opt.cpi_xid==YES){
		fprintf(fp, "#of Essential Fault             : %d\n",n_essential_fault );
		fprintf(fp, "#of Hard_to_detect Fault        : %d\n",n_hd_fault );
		fprintf(fp, "#of Diff Parh Fault             : %d\n",n_dpxid_fault );
		fprintf(fp, "#of Missed Fault                : %d\n",n_missed_fault );
		fprintf(fp, "#of Increase Care bit by EX-XID : %d\n",n_missed_cbit );
		fprintf(fp, "#of Number of Run XID           : %d\n",opt.n_xid );
		fprintf(fp, "#of Number of Run Diff-Path XID : %d\n",opt.diff_path_n_xid );
		if(opt.diff_path_n_xid>0){
			fprintf(fp, "#of Diff-Path XID mode          : ");
			if(opt.diff_path_mode == RAND_FIX){
			fprintf(fp, "RAND_FIX\n");
			}
			else if(opt.diff_path_mode == RAND){
			fprintf(fp, "RAND\n");
			}
			else if(opt.diff_path_mode == DIFF){
			fprintf(fp, "DIFF\n");
			}
		}
		
		if(opt.cpi_xid == YES){
			fprintf(fp, "#of After X-bit Ratio (All_PIN) : %.2lf%%\n",((double)n_after_x/(double)(n_tp*(n_spi+n_ppi)))*100 );
			fprintf(fp, "  X-bit Ratio of PI+PPI         : %.2lf%%\n", ((double)n_ori_x/(double)(n_tp*(n_ori_pi+n_ori_dff)))*100 );
			fprintf(fp, "  X-bit Ratio of PI+CP          : %.2lf%%\n", ((double)n_pi_cp_x/(double)(n_tp*(n_ori_pi+n_cpi)))*100 );
			fprintf(fp, "  X-bit Ratio of PI+PPI+CP      : %.2lf%%\n", ((double)n_cpi_x/(double)(n_tp*(n_ori_pi+n_ori_dff+n_cpi)))*100 );
			fprintf(fp, "#of Minimum Test Pattern        : %d\n", n_min_comtp);
		}
		else if(opt.fmodel == SAF){
			fprintf(fp, "#of After X-bit Ratio           : %.2lf%%\n",((double)n_after_x/(double)(n_tp*n_pi))*100 );
		}
		else if(opt.fmodel == TDF){
			fprintf(fp, "#of After X-bit Ratio           : %.2lf%%\n",((double)n_after_x/(double)(n_tp*(n_spi+n_ppi)))*100 );
		}
	}

	fprintf(fp, "Total CPU time                  : %.2lf (sec)\n", cput);
	fprintf(fp, "//-------------------------------------------------------\n\n");
	
	//---------------------------------
	// WARNING系
	//---------------------------------
	if(n_not_found_fault != 0){
		fprintf(fp, "WARNING: %d個の故障が故障リスト読込み時に故障設置できませんでした\n", n_not_found_fault);
	}
	if(n_not_found_ffc != 0){
		fprintf(fp, "WARNING: %d個のFFがCPIリスト読込み時にFFC=1にできませんでした\n", n_not_found_ffc);
	}
	if(n_not_found_link_CP_FF != 0){
		fprintf(fp, "WARNING: %d個のCPとFFがポインタ接続できませんでした\n", n_not_found_link_CP_FF);
	}
	if(n_not_found_link_FFC_FF != 0){
		fprintf(fp, "WARNING: %d個のFFCとFFがポインタ接続できませんでした\n", n_not_found_link_FFC_FF);
	}


	
	//======================================================
	// X抽出レポート
	//======================================================
	if(opt.xid==YES || opt.cpi_xid==YES){
		
		for(i=0; i<n_tp; i++){
			fprintf(fp, "tp	%d	essential_xid	%d	xid	%d\n", i, tp_info[i].n_essential_fault, tp_info[i].n_xid_fault);
			count += tp_info[i].n_xid_fault;
		}
		fprintf(fp, "XID故障数	%d\n", count);
		/*
		//---------------------------------------------------
		// テストパターンごとの情報
		//---------------------------------------------------
		fprintf(fp, "\n\n//===================================================\n");
		fprintf(fp, "// テストパターンごとの情報 (テストパターン数:%d)\n", n_tp);
		fprintf(fp, "//===================================================\n");
		
		fprintf(fp, "//---------------------------------------------------\n");
		fprintf(fp, "// テストパターンごとのXビット数\n");
		fprintf(fp, "//---------------------------------------------------\n");
		for(i=0; i<n_tp; i++){
			fprintf(fp, "tp[%d]:	%d\n", i, n_pi-tp_bit[i].n_cbit);
		}

		fprintf(fp, "\n//---------------------------------------------------\n");
		fprintf(fp, "// テストパターンごとのケアビット数\n");
		fprintf(fp, "//---------------------------------------------------\n");
		for(i=0; i<n_tp; i++){
			fprintf(fp, "tp[%d]:	%d\n", i, tp_bit[i].n_cbit);
		}

		fprintf(fp, "\n//---------------------------------------------------\n");
		fprintf(fp, "// テストパターンごとの論理値0の数\n");
		fprintf(fp, "//---------------------------------------------------\n");
		for(i=0; i<n_tp; i++){
			fprintf(fp, "tp[%d]:	%d\n", i, tp_bit[i].n_0bit);
		}

		fprintf(fp, "\n//---------------------------------------------------\n");
		fprintf(fp, "// テストパターンごとの論理値1の数\n");
		fprintf(fp, "//---------------------------------------------------\n");
		for(i=0; i<n_tp; i++){
			fprintf(fp, "tp[%d]:	%d\n", i, tp_bit[i].n_1bit);
		}
		
		//---------------------------------------------------
		// 外部入力ごとの情報
		//---------------------------------------------------
		if(opt.fmodel == SAF){
			fprintf(fp, "\n\n//===================================================\n");
			fprintf(fp, "// 外部入力ごとの情報 (外部入力数:%d)\n", n_pi);
			fprintf(fp, "//===================================================\n");
		
			fprintf(fp, "//---------------------------------------------------\n");
			fprintf(fp, "// 外部入力ごとのXビット数\n");
			fprintf(fp, "//---------------------------------------------------\n");
			for(i=0; i<n_pi; i++){
				fprintf(fp, "pi[%d]:	%d\n", i, n_tp-pi_bit[i].n_cbit);
			}

			fprintf(fp, "\n//---------------------------------------------------\n");
			fprintf(fp, "// 外部入力ごとのケアビット数\n");
			fprintf(fp, "//---------------------------------------------------\n");
			for(i=0; i<n_pi; i++){
				fprintf(fp, "pi[%d]:	%d\n", i, pi_bit[i].n_cbit);
			}

			fprintf(fp, "\n//---------------------------------------------------\n");
			fprintf(fp, "// 外部入力ごとの論理値0の数\n");
			fprintf(fp, "//---------------------------------------------------\n");
			for(i=0; i<n_pi; i++){
				fprintf(fp, "pi[%d]:	%d\n", i, pi_bit[i].n_0bit);
			}

			fprintf(fp, "\n//---------------------------------------------------\n");
			fprintf(fp, "// 外部入力ごとの論理値1の数\n");
			fprintf(fp, "//---------------------------------------------------\n");
			for(i=0; i<n_pi; i++){
				fprintf(fp, "pi[%d]:	%d\n", i, pi_bit[i].n_1bit);
			}
		}
		*/
		
	}
	
}

//--------------------------------------------------------------------
//関数名：file_output_add_log
//機能　：追記ログファイル出力
//引数　：fp(出力ファイルポインタ)
//戻り値：なし
//--------------------------------------------------------------------
void file_output_add_log(FILE* fp, char** f_net, double cput){
		
	fprintf(fp, "回路名\t%s\t",f_net);
	fprintf(fp, "テストパターン数\t%d\t",n_tp);
	fprintf(fp, "（P)PI数\t%d\t",n_pi);
	fprintf(fp, "故障検出率\t%.2lf%%\t",((double)n_detect/(double)n_rep)*100 );
	fprintf(fp, "対象故障数\t%d\t",n_rep );
	fprintf(fp, "検出故障数\t%d\t",n_detect );
	fprintf(fp, "未検出故障数\t%d\t",n_rep-n_detect );

	if(opt.xid == YES){
		fprintf(fp, "必須故障数\t%d\t",n_essential_fault );
		fprintf(fp, "別パスXID検出故障数\t%d\t",n_dpxid_fault );
		fprintf(fp, "見逃し故障数\t%d\t",n_missed_fault );
		fprintf(fp, "見逃し故障XIDでの増加ケアビット数\t%d\t",n_missed_cbit );
		if(opt.diff_path_n_xid > 0){
			if(opt.diff_path_mode == RAND_FIX){
			fprintf(fp, "別パスXIDモード\tRAND_FIX\t");
			}
			else if(opt.diff_path_mode == RAND){
			fprintf(fp, "別パスXIDモード\tRAND\t");
			}
			else if(opt.diff_path_mode == DIFF){
			fprintf(fp, "別パスXIDモード\tDIFF\t");
			}
		}
		fprintf(fp, "X率(%%)\t%.2lf\t",((double)n_after_x/(double)(n_tp*n_pi))*100 );
	}

	fprintf(fp, "実行時間\t%.2lf\n", cput);
	
}
//------------------------------------------------------------------------
//File name : Initialize.c
//Date : 2011/6/14
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../StandardHead.h"
#include	"../Lib/bit_tp.h"
#include	"../Lib/alloc.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : Initialize
//  機  能 : 外部変数等の初期化
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void  Initialize (){

	int			i;

	//============================================
	// 外部変数初期化
	//============================================
	n_tp					= 0;		
	n_tp_int				= 0;
	n_detect				= 0;
	n_xid_detect			= 0;
	n_fstem					= 0;
	n_before_x				= 0;
	n_after_x				= 0;
	n_ffr					= 0;
	n_essential_fault		= 0;
	n_missed_fault			= 0;
	n_hd_fault				= 0;
	n_missed_cbit			= 0;
	n_dpxid_fault			= 0;
	n_not_found_fault		= 0;
	n_not_found_ffc			= 0;	
	n_not_found_link_CP_FF	= 0;
	n_not_found_link_FFC_FF	= 0;
	n_cpi_corn_fault		= 0;
	n_testable_net			= 0;

	//============================================
	// 縮退故障の場合
	//============================================
	if(opt.fmodel == SAF){
		
		//--------------------------------
		// キューのメモリ確保
		//--------------------------------
		nl_queue = init_queue(n_net*2);

		if(opt.xid == YES){
			fimpl_queue = init_queue(n_net*2);		//前方含意キュー
			bimpl_queue = init_queue(n_net*2);		//後方含意キュー
		}

		//--------------------------------
		// Netlist初期化
		//--------------------------------
		for(i=0; i<n_net; i++){
			nl[i].n_corn_pi = 0;
			nl[i].det_sf0 = 0;
			nl[i].det_sf1 = 0;
			nl[i].f_stem = NO;
			nl[i].ffr_id = UK;
			nl[i].n_carebit = 0;
			nl[i].n_sa0_carebit = 0;
			nl[i].n_sa1_carebit = 0;
			nl[i].rep_net = UK;
			nl[i].inv_flag = UK;
			nl[i].nbit_fault = -1;
			nl[i].xid_det_sf0 = NO;
			nl[i].xid_det_sf1 = NO;
			nl[i].n_sa0_xid = 0;
			nl[i].n_sa1_xid = 0;
			nl[i].n_sa0_diff_xid = 0;
			nl[i].n_sa1_diff_xid = 0;
		}

		//--------------------------------
		// 回路チェック
		//--------------------------------
		if(n_dff != 0){
			printf("\n//-------------------------\n");
			printf("// 読み込み回路エラー\n");
			printf("//-------------------------\n");
			printf("これ順序回路じゃね？ (´・ω・｀)FFあるお\n");
			exit(-1);
		}
	}

	//============================================
	// 遷移故障の場合
	//============================================
	else if(opt.fmodel == TDF){
		
		//--------------------------------
		// キューのメモリ確保
		//--------------------------------
		nl_queue = init_queue(n_snet*3);

		if(opt.xid==YES || opt.cpi_xid==YES){
			fimpl_queue = init_queue(n_snet*3);		//前方含意キュー
			bimpl_queue = init_queue(n_snet*3);		//後方含意キュー
		}

		//--------------------------------
		// Netlist初期化
		//--------------------------------
		for(i=0; i<n_snet; i++){
			s_nl[i].n_corn_pi = 0;
			s_nl[i].det_str = 0;
			s_nl[i].det_stf = 0;
			s_nl[i].f_stem = NO;
			s_nl[i].ffr_id = UK;
			s_nl[i].n_carebit = 0;
			s_nl[i].n_str_carebit = 0;
			s_nl[i].n_stf_carebit = 0;
			s_nl[i].rep_net = UK;
			s_nl[i].inv_flag = UK;
			s_nl[i].nbit_fault = -1;
			s_nl[i].xid_det_str = NO;
			s_nl[i].xid_det_stf = NO;
			s_nl[i].n_str_xid = 0;
			s_nl[i].n_stf_xid = 0;
			s_nl[i].n_str_diff_xid = 0;
			s_nl[i].n_stf_diff_xid = 0;
			s_nl[i].testable_net = NO;
			s_nl[i].ppo_flag = NO;
			s_nl[i].pi_id = UK;
			s_nl[i].ppi_id = UK;
			s_nl[i].ppo_id = UK;
			s_nl[i].link_cpi = NULL;
			s_nl[i].pri_path = 0;
		}

		//--------------------------------
		// PPOフラグの設置
		//--------------------------------
		for(i=0; i<n_ppo; i++){
			ppo[i]->ppo_flag = YES;
		}

		//--------------------------------
		// 回路チェック
		//--------------------------------
		if(n_sdff == 0){
			printf("\n//-------------------------\n");
			printf("// 読み込み回路エラー\n");
			printf("//-------------------------\n");
			printf("これ組合せ回路じゃね？ (´・ω・｀)FFないお\n");
			exit(-1);
		}
	}

	//-------------------------------------------
	// MASKビット作成
	//-------------------------------------------
	MASKbit=(unsigned int*)calloc(32,sizeof(unsigned int));
	MASKbit[0]=1;
	for(i=1;i<32;i++){
		MASKbit[i] = MASKbit[i-1]<<1;
	}

}//END

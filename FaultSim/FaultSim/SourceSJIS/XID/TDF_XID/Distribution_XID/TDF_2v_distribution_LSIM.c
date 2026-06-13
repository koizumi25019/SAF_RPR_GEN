//------------------------------------------------------------------------
//File name : TDF_2v_distribution_LSIM.c
//Date : 2014/1/2
//Designer : H.Yamazaki
//Ver : 0.01
//memo : 【チューニング可能関数：未チューニング】
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
void dis_lsim_2v_seq_t1_single_gate	(S_NLIST*);
void dis_lsim_2v_seq_t1_inv			(S_NLIST*);
void dis_lsim_2v_seq_t1_and			(S_NLIST*);
void dis_lsim_2v_seq_t1_nand		(S_NLIST*);
void dis_lsim_2v_seq_t1_or			(S_NLIST*);
void dis_lsim_2v_seq_t1_nor			(S_NLIST*);
void dis_lsim_2v_seq_t1_exor		(S_NLIST*);
void dis_lsim_2v_seq_t1_exnor		(S_NLIST*);

void dis_lsim_2v_seq_t2_single_gate	(S_NLIST*);
void dis_lsim_2v_seq_t2_inv			(S_NLIST*);
void dis_lsim_2v_seq_t2_and			(S_NLIST*);
void dis_lsim_2v_seq_t2_nand		(S_NLIST*);
void dis_lsim_2v_seq_t2_or			(S_NLIST*);
void dis_lsim_2v_seq_t2_nor			(S_NLIST*);
void dis_lsim_2v_seq_t2_exor		(S_NLIST*);
void dis_lsim_2v_seq_t2_exnor		(S_NLIST*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		LSIM_DEBUG
	//#define		TEST_SO_DEBUG

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
//  関数名 : Make_FP_Dis_LSIM_2v_sequential
//  機  能 : 【分散制御XID用】2値論理シミュレーション用関数ポインタ作成
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void Make_FP_Dis_LSIM_2v_sequential(){

	//---------------------------------------------
	// 1時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t1_dis_lsim[FOUT]		= dis_lsim_2v_seq_t1_single_gate;
	sfunc_t1_dis_lsim[BUF]		= dis_lsim_2v_seq_t1_single_gate;
	sfunc_t1_dis_lsim[INV]		= dis_lsim_2v_seq_t1_inv;
	sfunc_t1_dis_lsim[AND]		= dis_lsim_2v_seq_t1_and;
	sfunc_t1_dis_lsim[NAND]		= dis_lsim_2v_seq_t1_nand;
	sfunc_t1_dis_lsim[OR]		= dis_lsim_2v_seq_t1_or;
	sfunc_t1_dis_lsim[NOR]		= dis_lsim_2v_seq_t1_nor;
	sfunc_t1_dis_lsim[EXOR]		= dis_lsim_2v_seq_t1_exor;
	sfunc_t1_dis_lsim[EXNOR]	= dis_lsim_2v_seq_t1_exnor;
		
	//---------------------------------------------
	// 2時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t2_dis_lsim[FOUT]		= dis_lsim_2v_seq_t2_single_gate;
	sfunc_t2_dis_lsim[BUF]		= dis_lsim_2v_seq_t2_single_gate;
	sfunc_t2_dis_lsim[INV]		= dis_lsim_2v_seq_t2_inv;
	sfunc_t2_dis_lsim[AND]		= dis_lsim_2v_seq_t2_and;
	sfunc_t2_dis_lsim[NAND]		= dis_lsim_2v_seq_t2_nand;
	sfunc_t2_dis_lsim[OR]		= dis_lsim_2v_seq_t2_or;
	sfunc_t2_dis_lsim[NOR]		= dis_lsim_2v_seq_t2_nor;
	sfunc_t2_dis_lsim[EXOR]		= dis_lsim_2v_seq_t2_exor;
	sfunc_t2_dis_lsim[EXNOR]	= dis_lsim_2v_seq_t2_exnor;
	
}

//----------------------------------------------
//  関数名 : TDF_2v_distribution_LSIM
//  機  能 : 【遷移故障】分散制御ドントケア抽出用の2値論理シミュレーション【分散制御XID用】
//  戻り値 : なし
//  引  数 : tp_id(テストパターンID), count(テストパターン数)
//----------------------------------------------
void	TDF_2v_distribution_LSIM(int *tp_id, int count){

	int			i,j;
	int			nval_temp;			//正常値の一時保存


	//=======================================================
	// テストパターンの設置
	//=======================================================
	//--------------------------------------------------
	// PI信号線のテストパターン設置
	//--------------------------------------------------
	for(i=0; i<n_spi; i++){
		for(j=0; j<count; j++){

			//1時刻目の正常値取得
			nval_temp = Get_NBit_Xbuf(spi[i]->dis_nval_t1, tp_id[j]);

			//1時刻目と2時刻目を代入(※ブロードサイドなので)
			if(nval_temp == 0){
				Set_NBit_Zero(spi[i]->dis_nval_t1, j);		//1時刻目PI=0
				Set_NBit_Zero(spi[i]->dis_nval_t2, j);		//2時刻目PI=0
			}
			else if(nval_temp == 1){
				Set_NBit_One(spi[i]->dis_nval_t1, j);		//1時刻目PI=1
				Set_NBit_One(spi[i]->dis_nval_t2, j);		//2時刻目PI=1
			}			
		}
	}
		
	//--------------------------------------------------
	// PPI信号線のテストパターン設置
	//--------------------------------------------------
	for(i=0; i<n_ppi; i++){
		for(j=0; j<count; j++){

			//1時刻目の正常値取得
			nval_temp = Get_NBit_Xbuf(ppi[i]->dis_nval_t1, tp_id[j]);

			//1時刻目を代入
			if(nval_temp == 0){
				Set_NBit_Zero(ppi[i]->dis_nval_t1, j);		//1時刻目PPI=0
			}
			else if(nval_temp == 1){
				Set_NBit_One(ppi[i]->dis_nval_t1, j);		//1時刻目PPI=1
			}
		}
	}


	//=========================================================
	// 1時刻目論理シミュレーション
	//=========================================================
#ifdef LSIM_DEBUG
	printf("\n//------------------------------------\n");
	printf("// 1時刻目論理シミュレーション\n");
	printf("//------------------------------------\n");
#endif

	//---------------------------------------------
	// 論理シミュレーション
	//---------------------------------------------
	for(i=1; i<max_level+1; i++){

		//1時刻目論理SIM
		for(j=0; j<nml_slev[i].n_net; j++){
#ifdef LSIM_DEBUG
			printf("%s (type:%d)\n", nml_slev[i].net[j]->name, nml_slev[i].net[j]->type);
#endif
			if(nml_slev[i].net[j]->type != DFF){
				(*sfunc_t1_dis_lsim[nml_slev[i].net[j]->type])(nml_slev[i].net[j]);
			}
		}
	}	

	//=========================================================
	// 2時刻目論理シミュレーション
	//=========================================================
#ifdef LSIM_DEBUG
	printf("\n//------------------------------------\n");
	printf("// 2時刻目論理シミュレーション\n");
	printf("//------------------------------------\n");
#endif

	//---------------------------------------------
	// 1次刻目PPOの値を2時刻目PPIに印加(Xbufのみ引き継ぎ)
	//---------------------------------------------
	for(i=0; i<n_ppi; i++){
#ifdef LSIM_DEBUG
		printf("%s\n", ppi[i]->name);
#endif
		for(j=0; j<(int)ppi[i]->dis_nval_t1->int_num; j++){
			ppi[i]->dis_nval_t2->x_buf[j] = ppi[i]->in[0]->dis_nval_t1->x_buf[j];
		}
	}

	//---------------------------------------------
	// 論理シミュレーション
	//---------------------------------------------
	for(i=1; i<max_level+1; i++){

		//2時刻目論理SIM
		for(j=0; j<nml_slev[i].n_net; j++){			
#ifdef LSIM_DEBUG
			printf("%s (type:%d)\n", nml_slev[i].net[j]->name, nml_slev[i].net[j]->type);
#endif
			if(nml_slev[i].net[j]->type != DFF){
				(*sfunc_t2_dis_lsim[nml_slev[i].net[j]->type])(nml_slev[i].net[j]);
			}
		}
	}
}

//*************************************************************************************
//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_single_gate
//  機  能 : 【分散制御XID用】BUF・FOUT・DFF論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->dis_nval_t1->int_num; i++){
		net->dis_nval_t1->x_buf[i] = net->in[0]->dis_nval_t1->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_single_gate
//  機  能 : 【分散制御XID用】BUF・FOUT・DFF論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->dis_nval_t2->int_num; i++){
		net->dis_nval_t2->x_buf[i] = net->in[0]->dis_nval_t2->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_inv
//  機  能 : 【分散制御XID用】INV論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->dis_nval_t1->int_num; i++){
		net->dis_nval_t1->x_buf[i] = ~(net->in[0]->dis_nval_t1->x_buf[i]);
	}
}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_inv
//  機  能 : 【分散制御XID用】INV論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->dis_nval_t2->int_num; i++){
		net->dis_nval_t2->x_buf[i] = ~(net->in[0]->dis_nval_t2->x_buf[i]);
	}
}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_and
//  機  能 : 【分散制御XID用】AND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->dis_nval_t1->int_num; i++){
		net->dis_nval_t1->x_buf[i] = net->in[0]->dis_nval_t1->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->dis_nval_t1->int_num; j++){
			net->dis_nval_t1->x_buf[j] &= net->in[i]->dis_nval_t1->x_buf[j];	//x_buf同士をAND演算
		}
	}

}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_and
//  機  能 : 【分散制御XID用】AND論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->dis_nval_t2->int_num; i++){
		net->dis_nval_t2->x_buf[i] = net->in[0]->dis_nval_t2->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->dis_nval_t2->int_num; j++){
			net->dis_nval_t2->x_buf[j] &= net->in[i]->dis_nval_t2->x_buf[j];	//x_buf同士をAND演算
		}
	}

}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_nand
//  機  能 : 【分散制御XID用】NAND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->dis_nval_t1->int_num; i++){
		net->dis_nval_t1->x_buf[i] = net->in[0]->dis_nval_t1->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->dis_nval_t1->int_num; j++){
			net->dis_nval_t1->x_buf[j] &= net->in[i]->dis_nval_t1->x_buf[j];	//x_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理
	//-------------------------------------
	for(i=0; i<net->dis_nval_t1->int_num; i++){
		net->dis_nval_t1->x_buf[i] = ~net->dis_nval_t1->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_nand
//  機  能 : 【分散制御XID用】NAND論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->dis_nval_t2->int_num; i++){
		net->dis_nval_t2->x_buf[i] = net->in[0]->dis_nval_t2->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->dis_nval_t2->int_num; j++){
			net->dis_nval_t2->x_buf[j] &= net->in[i]->dis_nval_t2->x_buf[j];	//x_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理
	//-------------------------------------
	for(i=0; i<net->dis_nval_t2->int_num; i++){
		net->dis_nval_t2->x_buf[i] = ~net->dis_nval_t2->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_or
//  機  能 : 【分散制御XID用】OR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t1->int_num; j++){
		net->dis_nval_t1->x_buf[j] = net->in[0]->dis_nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t1->int_num; j++){
			net->dis_nval_t1->x_buf[j] |= net->in[i]->dis_nval_t1->x_buf[j];	//x_buf同士をOR演算
		}
	}
}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_or
//  機  能 : 【分散制御XID用】OR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t2->int_num; j++){
		net->dis_nval_t2->x_buf[j] = net->in[0]->dis_nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t2->int_num; j++){
			net->dis_nval_t2->x_buf[j] |= net->in[i]->dis_nval_t2->x_buf[j];	//x_buf同士をOR演算
		}
	}
}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_nor
//  機  能 : 【分散制御XID用】NOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_nor(S_NLIST *net){

	int					i;
	unsigned int		j;

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t1->int_num; j++){
		net->dis_nval_t1->x_buf[j] = net->in[0]->dis_nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t1->int_num; j++){
			net->dis_nval_t1->x_buf[j] |= net->in[i]->dis_nval_t1->x_buf[j];	//x_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->dis_nval_t1->int_num; j++){
		net->dis_nval_t1->x_buf[j] = ~net->dis_nval_t1->x_buf[j];
	}
}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_nor
//  機  能 : 【分散制御XID用】NOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_nor(S_NLIST *net){

	int					i;
	unsigned int		j;

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t2->int_num; j++){
		net->dis_nval_t2->x_buf[j] = net->in[0]->dis_nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t2->int_num; j++){
			net->dis_nval_t2->x_buf[j] |= net->in[i]->dis_nval_t2->x_buf[j];	//x_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->dis_nval_t2->int_num; j++){
		net->dis_nval_t2->x_buf[j] = ~net->dis_nval_t2->x_buf[j];
	}
}

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_exor
//  機  能 : 【分散制御XID用】EXOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_exor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t1->int_num; j++){
		net->dis_nval_t1->x_buf[j] = net->in[0]->dis_nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t1->int_num; j++){
			net->dis_nval_t1->x_buf[j] ^= net->in[i]->dis_nval_t1->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

}//END

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_exor
//  機  能 : 【分散制御XID用】EXOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_exor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t2->int_num; j++){
		net->dis_nval_t2->x_buf[j] = net->in[0]->dis_nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t2->int_num; j++){
			net->dis_nval_t2->x_buf[j] ^= net->in[i]->dis_nval_t2->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

}//END

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t1_exnor
//  機  能 : 【分散制御XID用】EXNOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t1_exnor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//-------------------------------------
	// EXORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t1->int_num; j++){
		net->dis_nval_t1->x_buf[j] = net->in[0]->dis_nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t1->int_num; j++){
			net->dis_nval_t1->x_buf[j] ^= net->in[i]->dis_nval_t1->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->dis_nval_t1->int_num; j++){
		net->dis_nval_t1->x_buf[j] = ~net->dis_nval_t1->x_buf[j];
	}

}//END

//----------------------------------------------
//  関数名 : dis_lsim_2v_seq_t2_exnor
//  機  能 : 【分散制御XID用】EXNOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void dis_lsim_2v_seq_t2_exnor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//-------------------------------------
	// EXORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->dis_nval_t2->int_num; j++){
		net->dis_nval_t2->x_buf[j] = net->in[0]->dis_nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->dis_nval_t2->int_num; j++){
			net->dis_nval_t2->x_buf[j] ^= net->in[i]->dis_nval_t2->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->dis_nval_t2->int_num; j++){
		net->dis_nval_t2->x_buf[j] = ~net->dis_nval_t2->x_buf[j];
	}

}//END

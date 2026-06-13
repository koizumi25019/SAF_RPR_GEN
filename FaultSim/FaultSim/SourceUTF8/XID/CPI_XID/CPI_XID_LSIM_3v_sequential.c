//------------------------------------------------------------------------
//File name : CPI_XID_LSIM_3v_sequential.c
//Date : 2013/11/29
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/s_netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../Lib/bit_tp.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void Make_FP_XID_LSIM_3v_sequential	();

void xid_lsim3v_seq_t1_single_gate	(S_NLIST*);
void xid_lsim3v_seq_t1_inv			(S_NLIST*);
void xid_lsim3v_seq_t1_and			(S_NLIST*);
void xid_lsim3v_seq_t1_nand			(S_NLIST*);
void xid_lsim3v_seq_t1_or			(S_NLIST*);
void xid_lsim3v_seq_t1_nor			(S_NLIST*);
void xid_lsim3v_seq_t1_exor			(S_NLIST*);
void xid_lsim3v_seq_t1_exnor		(S_NLIST*);

void xid_lsim3v_seq_t2_single_gate	(S_NLIST*);
void xid_lsim3v_seq_t2_inv			(S_NLIST*);
void xid_lsim3v_seq_t2_and			(S_NLIST*);
void xid_lsim3v_seq_t2_nand			(S_NLIST*);
void xid_lsim3v_seq_t2_or			(S_NLIST*);
void xid_lsim3v_seq_t2_nor			(S_NLIST*);
void xid_lsim3v_seq_t2_exor			(S_NLIST*);
void xid_lsim3v_seq_t2_exnor		(S_NLIST*);

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
//  関数名 : Make_FP_XID_LSIM_3v_sequential
//  機  能 : 3値論理シミュレーション用関数ポインタ作成
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void Make_FP_XID_LSIM_3v_sequential(){

	//---------------------------------------------
	// 1時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t1_xid_lsim[FOUT]		= xid_lsim3v_seq_t1_single_gate;
	sfunc_t1_xid_lsim[BUF]		= xid_lsim3v_seq_t1_single_gate;
	sfunc_t1_xid_lsim[INV]		= xid_lsim3v_seq_t1_inv;
	sfunc_t1_xid_lsim[AND]		= xid_lsim3v_seq_t1_and;
	sfunc_t1_xid_lsim[NAND]		= xid_lsim3v_seq_t1_nand;
	sfunc_t1_xid_lsim[OR]		= xid_lsim3v_seq_t1_or;
	sfunc_t1_xid_lsim[NOR]		= xid_lsim3v_seq_t1_nor;
	sfunc_t1_xid_lsim[EXOR]		= xid_lsim3v_seq_t1_exor;
	sfunc_t1_xid_lsim[EXNOR]	= xid_lsim3v_seq_t1_exnor;
		
	//---------------------------------------------
	// 2時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t2_xid_lsim[FOUT]		= xid_lsim3v_seq_t2_single_gate;
	sfunc_t2_xid_lsim[BUF]		= xid_lsim3v_seq_t2_single_gate;
	sfunc_t2_xid_lsim[INV]		= xid_lsim3v_seq_t2_inv;
	sfunc_t2_xid_lsim[AND]		= xid_lsim3v_seq_t2_and;
	sfunc_t2_xid_lsim[NAND]		= xid_lsim3v_seq_t2_nand;
	sfunc_t2_xid_lsim[OR]		= xid_lsim3v_seq_t2_or;
	sfunc_t2_xid_lsim[NOR]		= xid_lsim3v_seq_t2_nor;
	sfunc_t2_xid_lsim[EXOR]		= xid_lsim3v_seq_t2_exor;
	sfunc_t2_xid_lsim[EXNOR]	= xid_lsim3v_seq_t2_exnor;
	
}

//----------------------------------------------
//  関数名 : XID_LSIM_3v_sequential
//  機  能 : 【X抽出後値】3値論理シミュレーション(順序回路：遷移故障)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	XID_LSIM_3v_sequential(){
	
	int i, j;

	fprintf(stderr,"\n XID_LSIM_3v_sequential\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

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
		// Progress Bar
		if(((int)i % (int)(((max_level-10)*2/50)+1)) == 0)fprintf(stderr,"*");

		//1時刻目論理SIM
		for(j=0; j<nml_slev[i].n_net; j++){
#ifdef LSIM_DEBUG
			printf("%s (type:%d)\n", nml_slev[i].net[j]->name, nml_slev[i].net[j]->type);
#endif
			if(nml_slev[i].net[j]->type != DFF){
				(*sfunc_t1_xid_lsim[nml_slev[i].net[j]->type])(nml_slev[i].net[j]);
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
	// 1次刻目PPOの値を2時刻目PPIに印加
	//---------------------------------------------
	for(i=0; i<n_ppi; i++){
		for(j=0; j<(int)ppi[i]->nval_t1->int_num; j++){
			ppi[i]->xid_nval_t2->x_buf[j] = ppi[i]->in[0]->xid_nval_t1->x_buf[j];
			ppi[i]->xid_nval_t2->p_buf[j] = ppi[i]->in[0]->xid_nval_t1->p_buf[j];
		}
	}

	//---------------------------------------------
	// 論理シミュレーション
	//---------------------------------------------
	for(i=1; i<max_level+1; i++){
		// Progress Bar
		if(((int)i % (int)(((max_level-10)*2/50)+1)) == 0)fprintf(stderr,"*");

		//2時刻目論理SIM
		for(j=0; j<nml_slev[i].n_net; j++){			
#ifdef LSIM_DEBUG
			printf("%s (type:%d)\n", nml_slev[i].net[j]->name, nml_slev[i].net[j]->type);
#endif
			if(nml_slev[i].net[j]->type != DFF){
				(*sfunc_t2_xid_lsim[nml_slev[i].net[j]->type])(nml_slev[i].net[j]);
			}
		}
	}

}//END

//*************************************************************************************************

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_single_gate
//  機  能 : BUF・FOUT・DFF論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t1_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->xid_nval_t1->int_num; i++){
		net->xid_nval_t1->x_buf[i] = net->in[0]->xid_nval_t1->x_buf[i];
		net->xid_nval_t1->p_buf[i] = net->in[0]->xid_nval_t1->p_buf[i];
	}

}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_single_gate
//  機  能 : BUF・FOUT・DFF論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t2_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->xid_nval_t2->int_num; i++){
		net->xid_nval_t2->x_buf[i] = net->in[0]->xid_nval_t2->x_buf[i];
		net->xid_nval_t2->p_buf[i] = net->in[0]->xid_nval_t2->p_buf[i];
	}

}
//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_inv
//  機  能 : INV論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t1_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->xid_nval_t1->int_num; i++){
		net->xid_nval_t1->x_buf[i] = net->in[0]->xid_nval_t1->p_buf[i];
		net->xid_nval_t1->p_buf[i] = net->in[0]->xid_nval_t1->x_buf[i];
	}
}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_inv
//  機  能 : INV論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t2_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->xid_nval_t2->int_num; i++){
		net->xid_nval_t2->x_buf[i] = net->in[0]->xid_nval_t2->p_buf[i];
		net->xid_nval_t2->p_buf[i] = net->in[0]->xid_nval_t2->x_buf[i];
	}
}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_and
//  機  能 : AND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t1_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->xid_nval_t1->int_num; i++){
		net->xid_nval_t1->x_buf[i] = net->in[0]->xid_nval_t1->x_buf[i];
		net->xid_nval_t1->p_buf[i] = net->in[0]->xid_nval_t1->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->xid_nval_t1->int_num; j++){
			net->xid_nval_t1->x_buf[j] &= net->in[i]->xid_nval_t1->x_buf[j];	//x_buf同士をAND演算
			net->xid_nval_t1->p_buf[j] |= net->in[i]->xid_nval_t1->p_buf[j];	//p_buf同士をOR演算
		}
	}

}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_and
//  機  能 : AND論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t2_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->xid_nval_t2->int_num; i++){
		net->xid_nval_t2->x_buf[i] = net->in[0]->xid_nval_t2->x_buf[i];
		net->xid_nval_t2->p_buf[i] = net->in[0]->xid_nval_t2->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->xid_nval_t2->int_num; j++){
			net->xid_nval_t2->x_buf[j] &= net->in[i]->xid_nval_t2->x_buf[j];	//x_buf同士をAND演算
			net->xid_nval_t2->p_buf[j] |= net->in[i]->xid_nval_t2->p_buf[j];	//p_buf同士をOR演算
		}
	}

}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_nand
//  機  能 : NAND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t1_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->xid_nval_t1->int_num; i++){
		net->xid_nval_t1->x_buf[i] = net->in[0]->xid_nval_t1->x_buf[i];
		net->xid_nval_t1->p_buf[i] = net->in[0]->xid_nval_t1->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->xid_nval_t1->int_num; j++){
			net->xid_nval_t1->x_buf[j] &= net->in[i]->xid_nval_t1->x_buf[j];	//x_buf同士をAND演算
			net->xid_nval_t1->p_buf[j] |= net->in[i]->xid_nval_t1->p_buf[j];	//p_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->xid_nval_t1->int_num; j++){
		temp = net->xid_nval_t1->x_buf[j];
		net->xid_nval_t1->x_buf[j] = net->xid_nval_t1->p_buf[j];	//x_bufにp_bufを代入
		net->xid_nval_t1->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}

}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_nand
//  機  能 : NAND論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t2_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->xid_nval_t2->int_num; i++){
		net->xid_nval_t2->x_buf[i] = net->in[0]->xid_nval_t2->x_buf[i];
		net->xid_nval_t2->p_buf[i] = net->in[0]->xid_nval_t2->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->xid_nval_t2->int_num; j++){
			net->xid_nval_t2->x_buf[j] &= net->in[i]->xid_nval_t2->x_buf[j];	//x_buf同士をAND演算
			net->xid_nval_t2->p_buf[j] |= net->in[i]->xid_nval_t2->p_buf[j];	//p_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->xid_nval_t2->int_num; j++){
		temp = net->xid_nval_t2->x_buf[j];
		net->xid_nval_t2->x_buf[j] = net->xid_nval_t2->p_buf[j];	//x_bufにp_bufを代入
		net->xid_nval_t2->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}

}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_or
//  機  能 : OR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t1_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->xid_nval_t1->int_num; j++){
		net->xid_nval_t1->x_buf[j] = net->in[0]->xid_nval_t1->x_buf[j];
		net->xid_nval_t1->p_buf[j] = net->in[0]->xid_nval_t1->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->xid_nval_t1->int_num; j++){
			net->xid_nval_t1->x_buf[j] |= net->in[i]->xid_nval_t1->x_buf[j];	//x_buf同士をOR演算
			net->xid_nval_t1->p_buf[j] &= net->in[i]->xid_nval_t1->p_buf[j];	//p_buf同士をAND演算
		}
	}
}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_or
//  機  能 : OR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t2_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->xid_nval_t2->int_num; j++){
		net->xid_nval_t2->x_buf[j] = net->in[0]->xid_nval_t2->x_buf[j];
		net->xid_nval_t2->p_buf[j] = net->in[0]->xid_nval_t2->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->xid_nval_t2->int_num; j++){
			net->xid_nval_t2->x_buf[j] |= net->in[i]->xid_nval_t2->x_buf[j];	//x_buf同士をOR演算
			net->xid_nval_t2->p_buf[j] &= net->in[i]->xid_nval_t2->p_buf[j];	//p_buf同士をAND演算
		}
	}
}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_nor
//  機  能 : NOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t1_nor(S_NLIST *net){

	int					i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->xid_nval_t1->int_num; j++){
		net->xid_nval_t1->x_buf[j] = net->in[0]->xid_nval_t1->x_buf[j];
		net->xid_nval_t1->p_buf[j] = net->in[0]->xid_nval_t1->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->xid_nval_t1->int_num; j++){
			net->xid_nval_t1->x_buf[j] |= net->in[i]->xid_nval_t1->x_buf[j];	//x_buf同士をOR演算
			net->xid_nval_t1->p_buf[j] &= net->in[i]->xid_nval_t1->p_buf[j];	//p_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->xid_nval_t1->int_num; j++){
		temp = net->xid_nval_t1->x_buf[j];
		net->xid_nval_t1->x_buf[j] = net->xid_nval_t1->p_buf[j];	//x_bufにp_bufを代入
		net->xid_nval_t1->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}
}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_nor
//  機  能 : NOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void xid_lsim3v_seq_t2_nor(S_NLIST *net){

	int					i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->xid_nval_t2->int_num; j++){
		net->xid_nval_t2->x_buf[j] = net->in[0]->xid_nval_t2->x_buf[j];
		net->xid_nval_t2->p_buf[j] = net->in[0]->xid_nval_t2->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->xid_nval_t2->int_num; j++){
			net->xid_nval_t2->x_buf[j] |= net->in[i]->xid_nval_t2->x_buf[j];	//x_buf同士をOR演算
			net->xid_nval_t2->p_buf[j] &= net->in[i]->xid_nval_t2->p_buf[j];	//p_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->xid_nval_t2->int_num; j++){
		temp = net->xid_nval_t2->x_buf[j];
		net->xid_nval_t2->x_buf[j] = net->xid_nval_t2->p_buf[j];	//x_bufにp_bufを代入
		net->xid_nval_t2->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}
}

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_exor
//  機  能 : EXOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXORのみ対応！
//----------------------------------------------
void xid_lsim3v_seq_t1_exor(S_NLIST *net){
	
	unsigned int		i;
	
	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->xid_nval_t1->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->xid_nval_t1->x_buf[i] = (net->in[0]->xid_nval_t1->x_buf[i]&net->in[1]->xid_nval_t1->p_buf[i]) | (net->in[0]->xid_nval_t1->p_buf[i]&net->in[1]->xid_nval_t1->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->xid_nval_t1->p_buf[i] = (net->in[0]->xid_nval_t1->p_buf[i]|net->in[1]->xid_nval_t1->x_buf[i]) & (net->in[0]->xid_nval_t1->x_buf[i]|net->in[1]->xid_nval_t1->p_buf[i]);
	}


}//END

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_exor
//  機  能 : EXOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXORのみ対応！
//----------------------------------------------
void xid_lsim3v_seq_t2_exor(S_NLIST *net){
	
	unsigned int		i;
	
	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->xid_nval_t2->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->xid_nval_t2->x_buf[i] = (net->in[0]->xid_nval_t2->x_buf[i]&net->in[1]->xid_nval_t2->p_buf[i]) | (net->in[0]->xid_nval_t2->p_buf[i]&net->in[1]->xid_nval_t2->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->xid_nval_t2->p_buf[i] = (net->in[0]->xid_nval_t2->p_buf[i]|net->in[1]->xid_nval_t2->x_buf[i]) & (net->in[0]->xid_nval_t2->x_buf[i]|net->in[1]->xid_nval_t2->p_buf[i]);
	}


}//END

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t1_exnor
//  機  能 : EXNOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXNORのみ対応！
//----------------------------------------------
void xid_lsim3v_seq_t1_exnor(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用
	

	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->xid_nval_t1->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->xid_nval_t1->x_buf[i] = (net->in[0]->xid_nval_t1->x_buf[i]&net->in[1]->xid_nval_t1->p_buf[i]) | (net->in[0]->xid_nval_t1->p_buf[i]&net->in[1]->xid_nval_t1->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->xid_nval_t1->p_buf[i] = (net->in[0]->xid_nval_t1->p_buf[i]|net->in[1]->xid_nval_t1->x_buf[i]) & (net->in[0]->xid_nval_t1->x_buf[i]|net->in[1]->xid_nval_t1->p_buf[i]);


		//-------------------------------------
		// NOT演算処理(x_bufとp_bufを入れ替える)
		//-------------------------------------
		temp = net->xid_nval_t1->x_buf[i];
		net->xid_nval_t1->x_buf[i] = net->xid_nval_t1->p_buf[i];	//x_bufにp_bufを代入
		net->xid_nval_t1->p_buf[i] = temp;					//p_bufにx_buf(temp)を代入
	}

}//END

//----------------------------------------------
//  関数名 : xid_lsim3v_seq_t2_exnor
//  機  能 : EXNOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXNORのみ対応！
//----------------------------------------------
void xid_lsim3v_seq_t2_exnor(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用
	

	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->xid_nval_t2->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->xid_nval_t2->x_buf[i] = (net->in[0]->xid_nval_t2->x_buf[i]&net->in[1]->xid_nval_t2->p_buf[i]) | (net->in[0]->xid_nval_t2->p_buf[i]&net->in[1]->xid_nval_t2->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->xid_nval_t2->p_buf[i] = (net->in[0]->xid_nval_t2->p_buf[i]|net->in[1]->xid_nval_t2->x_buf[i]) & (net->in[0]->xid_nval_t2->x_buf[i]|net->in[1]->xid_nval_t2->p_buf[i]);


		//-------------------------------------
		// NOT演算処理(x_bufとp_bufを入れ替える)
		//-------------------------------------
		temp = net->xid_nval_t2->x_buf[i];
		net->xid_nval_t2->x_buf[i] = net->xid_nval_t2->p_buf[i];	//x_bufにp_bufを代入
		net->xid_nval_t2->p_buf[i] = temp;					//p_bufにx_buf(temp)を代入
	}

}//END
//------------------------------------------------------------------------
//File name : LSIM_2v_sequential.c
//Date : 2012/1/9
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
void Make_FP_LSIM_2v_sequential	();

void lsim_2v_seq_t1_single_gate	(S_NLIST*);
void lsim_2v_seq_t1_inv			(S_NLIST*);
void lsim_2v_seq_t1_and			(S_NLIST*);
void lsim_2v_seq_t1_nand		(S_NLIST*);
void lsim_2v_seq_t1_or			(S_NLIST*);
void lsim_2v_seq_t1_nor			(S_NLIST*);
void lsim_2v_seq_t1_exor		(S_NLIST*);
void lsim_2v_seq_t1_exnor		(S_NLIST*);

void lsim_2v_seq_t2_single_gate	(S_NLIST*);
void lsim_2v_seq_t2_inv			(S_NLIST*);
void lsim_2v_seq_t2_and			(S_NLIST*);
void lsim_2v_seq_t2_nand		(S_NLIST*);
void lsim_2v_seq_t2_or			(S_NLIST*);
void lsim_2v_seq_t2_nor			(S_NLIST*);
void lsim_2v_seq_t2_exor		(S_NLIST*);
void lsim_2v_seq_t2_exnor		(S_NLIST*);

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
//  関数名 : Make_FP_LSIM_2v_sequential
//  機  能 : 2値論理シミュレーション用関数ポインタ作成
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void Make_FP_LSIM_2v_sequential(){

	//---------------------------------------------
	// 1時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t1_lsim[FOUT]		= lsim_2v_seq_t1_single_gate;
	sfunc_t1_lsim[BUF]		= lsim_2v_seq_t1_single_gate;
	sfunc_t1_lsim[INV]		= lsim_2v_seq_t1_inv;
	sfunc_t1_lsim[AND]		= lsim_2v_seq_t1_and;
	sfunc_t1_lsim[NAND]		= lsim_2v_seq_t1_nand;
	sfunc_t1_lsim[OR]		= lsim_2v_seq_t1_or;
	sfunc_t1_lsim[NOR]		= lsim_2v_seq_t1_nor;
	sfunc_t1_lsim[EXOR]		= lsim_2v_seq_t1_exor;
	sfunc_t1_lsim[EXNOR]	= lsim_2v_seq_t1_exnor;
		
	//---------------------------------------------
	// 2時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t2_lsim[FOUT]		= lsim_2v_seq_t2_single_gate;
	sfunc_t2_lsim[BUF]		= lsim_2v_seq_t2_single_gate;
	sfunc_t2_lsim[INV]		= lsim_2v_seq_t2_inv;
	sfunc_t2_lsim[AND]		= lsim_2v_seq_t2_and;
	sfunc_t2_lsim[NAND]		= lsim_2v_seq_t2_nand;
	sfunc_t2_lsim[OR]		= lsim_2v_seq_t2_or;
	sfunc_t2_lsim[NOR]		= lsim_2v_seq_t2_nor;
	sfunc_t2_lsim[EXOR]		= lsim_2v_seq_t2_exor;
	sfunc_t2_lsim[EXNOR]	= lsim_2v_seq_t2_exnor;
	
}


//----------------------------------------------
//  関数名 : LSIM_2v_sequential
//  機  能 : 2値論理シミュレーション(順序回路：遷移故障)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	LSIM_2v_sequential(){

	int i,j;

#ifdef LSIM_DEBUG
	int	k;
#endif
		
	//=========================================================
	// 関数ポインタセット
	//=========================================================
	Make_FP_LSIM_2v_sequential();


	fprintf(stderr,"\n LSIM_2v_sequential\n");
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
				(*sfunc_t1_lsim[nml_slev[i].net[j]->type])(nml_slev[i].net[j]);
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
		for(j=0; j<(int)ppi[i]->nval_t1->int_num; j++){
			ppi[i]->nval_t2->x_buf[j] = ppi[i]->in[0]->nval_t1->x_buf[j];
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
				(*sfunc_t2_lsim[nml_slev[i].net[j]->type])(nml_slev[i].net[j]);
			}
		}
	}

	
	//=========================================================
	// 全信号線値DEBUG
	//=========================================================
#ifdef LSIM_DEBUG
	printf("\n//-------------------------------\n");
	printf("//DEBUG:2値論理シミュレーション確認\n");
	printf("//-------------------------------\n");

	for(i=0; i<n_tp; i++){
		printf("-------------------------\n");
		printf("  %dパターン目\n", i);
		//------------------------------------------------
		// 1次刻目
		//------------------------------------------------
		printf("1次刻目\n");
		//PI
		for(j=0; j<n_spi; j++){
			printf("name:%s  type:PI  nval:", spi[j]->name);
			if(Get_NBit_Xbuf(spi[j]->nval_t1, i) == 0){
					printf("0\n");
			}
			else if(Get_NBit_Xbuf(spi[j]->nval_t1, i) == 1){
					printf("1\n");
			}
		}

		//PPI
		for(j=0; j<n_ppi; j++){
			printf("name:%s  type:PPI  nval:", ppi[j]->name);
			if(Get_NBit_Xbuf(ppi[j]->nval_t1, i) == 0){
					printf("0\n");
			}
			else if(Get_NBit_Xbuf(ppi[j]->nval_t1, i) == 1){
					printf("1\n");
			}
		}

		printf("\n");

		//PI,PPI以外
		for(j=1; j<max_level+1; j++){
			for(k=0; k<nml_slev[j].n_net; k++){
				printf("name:%s  type:%d  nval:", nml_slev[j].net[k]->name, nml_slev[j].net[k]->type);
				if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t1, i) == 0){
					printf("0\n");
				}
				else if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t1, i) == 1){
					printf("1\n");
				}
			}
			printf("\n");
		}

		//------------------------------------------------
		// 2次刻目
		//------------------------------------------------
		printf("\n2次刻目\n");
		//PI
		for(j=0; j<n_spi; j++){
			printf("name:%s  type:PI  nval:", spi[j]->name);
			if(Get_NBit_Xbuf(spi[j]->nval_t2, i) == 0){
					printf("0\n");
			}
			else if(Get_NBit_Xbuf(spi[j]->nval_t2, i) == 1){
					printf("1\n");
			}
		}

		//PPI
		for(j=0; j<n_ppi; j++){
			printf("name:%s  type:PPI  nval:", ppi[j]->name);
			if(Get_NBit_Xbuf(ppi[j]->nval_t2, i) == 0){
					printf("0\n");
			}
			else if(Get_NBit_Xbuf(ppi[j]->nval_t2, i) == 1){
					printf("1\n");
			}
		}

		printf("\n");

		//PI,PPI以外
		for(j=1; j<max_level+1; j++){
			for(k=0; k<nml_slev[j].n_net; k++){
				printf("name:%s  type:%d  nval:", nml_slev[j].net[k]->name, nml_slev[j].net[k]->type);
				if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t2, i) == 0){
					printf("0\n");
				}
				else if(Get_NBit_Xbuf(nml_slev[j].net[k]->nval_t2, i) == 1){
					printf("1\n");
				}
			}
			printf("\n");
		}
		printf("\n\n");
	}

	printf("\n");	
#endif

	
	//=========================================================
	// 出力期待値のみDEBUG
	//=========================================================
#ifdef TEST_SO_DEBUG
	printf("\n//-------------------------------\n");
	printf("//DEBUG:2値論理シミュレーション 出力期待値確認\n");
	printf("//-------------------------------\n");

	for(i=0; i<n_tp; i++){
		printf("-------------------------\n");
		printf("  %dパターン目\n", i);
		//PPI
		for(j=n_ppo-1; j>=0; j--){
			printf("%s : ", ppo[j]->out[0]->name_ins);
			if(Get_NBit_Xbuf(ppo[j]->nval_t2, i) == 0){
					printf("L\n");
			}
			else if(Get_NBit_Xbuf(ppo[j]->nval_t2, i) == 1){
					printf("H\n");
			}
		}
	}
	printf("\n");
#endif


}//END
//*************************************************************************************
//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_single_gate
//  機  能 : BUF・FOUT・DFF論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_single_gate
//  機  能 : BUF・FOUT・DFF論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->nval_t2->int_num; i++){
		net->nval_t2->x_buf[i] = net->in[0]->nval_t2->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_inv
//  機  能 : INV論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = ~(net->in[0]->nval_t1->x_buf[i]);
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_inv
//  機  能 : INV論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->nval_t2->int_num; i++){
		net->nval_t2->x_buf[i] = ~(net->in[0]->nval_t2->x_buf[i]);
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_and
//  機  能 : AND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] &= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をAND演算
		}
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_and
//  機  能 : AND論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval_t2->int_num; i++){
		net->nval_t2->x_buf[i] = net->in[0]->nval_t2->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval_t2->int_num; j++){
			net->nval_t2->x_buf[j] &= net->in[i]->nval_t2->x_buf[j];	//x_buf同士をAND演算
		}
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_nand
//  機  能 : NAND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] &= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理
	//-------------------------------------
	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = ~net->nval_t1->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_nand
//  機  能 : NAND論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval_t2->int_num; i++){
		net->nval_t2->x_buf[i] = net->in[0]->nval_t2->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval_t2->int_num; j++){
			net->nval_t2->x_buf[j] &= net->in[i]->nval_t2->x_buf[j];	//x_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理
	//-------------------------------------
	for(i=0; i<net->nval_t2->int_num; i++){
		net->nval_t2->x_buf[i] = ~net->nval_t2->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_or
//  機  能 : OR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = net->in[0]->nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] |= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をOR演算
		}
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_or
//  機  能 : OR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t2->int_num; j++){
		net->nval_t2->x_buf[j] = net->in[0]->nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t2->int_num; j++){
			net->nval_t2->x_buf[j] |= net->in[i]->nval_t2->x_buf[j];	//x_buf同士をOR演算
		}
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_nor
//  機  能 : NOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_nor(S_NLIST *net){

	int					i;
	unsigned int		j;

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = net->in[0]->nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] |= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = ~net->nval_t1->x_buf[j];
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_nor
//  機  能 : NOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_nor(S_NLIST *net){

	int					i;
	unsigned int		j;

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t2->int_num; j++){
		net->nval_t2->x_buf[j] = net->in[0]->nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t2->int_num; j++){
			net->nval_t2->x_buf[j] |= net->in[i]->nval_t2->x_buf[j];	//x_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval_t2->int_num; j++){
		net->nval_t2->x_buf[j] = ~net->nval_t2->x_buf[j];
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_exor
//  機  能 : EXOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_exor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = net->in[0]->nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] ^= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

}//END

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_exor
//  機  能 : EXOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_exor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t2->int_num; j++){
		net->nval_t2->x_buf[j] = net->in[0]->nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t2->int_num; j++){
			net->nval_t2->x_buf[j] ^= net->in[i]->nval_t2->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

}//END

//----------------------------------------------
//  関数名 : lsim_2v_seq_t1_exnor
//  機  能 : EXNOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t1_exnor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//-------------------------------------
	// EXORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = net->in[0]->nval_t1->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] ^= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = ~net->nval_t1->x_buf[j];
	}

}//END

//----------------------------------------------
//  関数名 : lsim_2v_seq_t2_exnor
//  機  能 : EXNOR論理演算(2時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_2v_seq_t2_exnor(S_NLIST *net){
	
	int					i;
	unsigned int		j;

	//-------------------------------------
	// EXORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t2->int_num; j++){
		net->nval_t2->x_buf[j] = net->in[0]->nval_t2->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t2->int_num; j++){
			net->nval_t2->x_buf[j] ^= net->in[i]->nval_t2->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval_t2->int_num; j++){
		net->nval_t2->x_buf[j] = ~net->nval_t2->x_buf[j];
	}

}//END
//------------------------------------------------------------------------
//File name : LSIM_2v_combinational.c
//Date : 2012/1/9
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../../Netlist/netlist.h"
#include	"../../Lib/bit_int.h"
#include	"../../Lib/bit_tp.h"
#include	"../../StandardHead.h"
#include	"../../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
static void lsim_2v_comb_fout	(NLIST*);
static void lsim_2v_comb_buf	(NLIST*);
static void lsim_2v_comb_inv	(NLIST*);
static void lsim_2v_comb_and	(NLIST*);
static void lsim_2v_comb_nand	(NLIST*);
static void lsim_2v_comb_or		(NLIST*);
static void lsim_2v_comb_nor	(NLIST*);
static void lsim_2v_comb_exor	(NLIST*);
static void lsim_2v_comb_exnor	(NLIST*);


//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		LSIM_DEBUG

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
//  関数名 : LSIM_2v_combinational
//  機  能 : 2値論理シミュレーション(組合せ回路:縮退故障)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	LSIM_2v_combinational(){

	int i,j;

#ifdef LSIM_DEBUG
	int	k;
#endif

	static void(*func[20])(NLIST*);
	func[FOUT]	= lsim_2v_comb_fout;
	func[BUF]	= lsim_2v_comb_buf;
	func[INV]	= lsim_2v_comb_inv;
	func[AND]	= lsim_2v_comb_and;
	func[NAND]	= lsim_2v_comb_nand;
	func[OR]	= lsim_2v_comb_or;
	func[NOR]	= lsim_2v_comb_nor;
	func[EXOR]	= lsim_2v_comb_exor;
	func[EXNOR]	= lsim_2v_comb_exnor;
	
	
	//------------------------------
	// 論理シミュレーション(Xバッファのみ計算)
	//------------------------------
	fprintf(stderr,"\n LSIM_2v_combinational\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	for(i=1; i<max_level+1; i++){
		// Progress Bar
		if(((int)i % (int)(((max_level-10)*2/100)+1)) == 0)fprintf(stderr,"*");

		//論理SIM
		for(j=0; j<nml_lev[i].n_net; j++){
			(*func[nml_lev[i].net[j]->type])(nml_lev[i].net[j]);
		}
	}

	
	//------------------------------
	// DEBUG
	//------------------------------
#ifdef LSIM_DEBUG
	printf("\n//-------------------------------\n");
	printf("//DEBUG:2値論理シミュレーション確認\n");
	printf("//-------------------------------\n");

	for(i=0; i<n_tp; i++){
		printf("-------------------------\n");
		printf("  %dパターン目\n", i);
		//PI
		for(j=0; j<n_pi; j++){
			printf("name:%s  type:PI  nval:", pi[j]->name);
			if(Get_NBit_Xbuf(pi[j]->nval, i) == 0){
					printf("0\n");
			}
			else if(Get_NBit_Xbuf(pi[j]->nval, i) == 1){
					printf("1\n");
			}
		}
		
		printf("\n");

		//PI以外
		for(j=1; j<max_level+1; j++){
			for(k=0; k<nml_lev[j].n_net; k++){
				printf("name:%s  type:%d  nval:", nml_lev[j].net[k]->name, nml_lev[j].net[k]->type);
				if(Get_NBit_Xbuf(nml_lev[j].net[k]->nval, i) == 0){
					printf("0\n");
				}
				else if(Get_NBit_Xbuf(nml_lev[j].net[k]->nval, i) == 1){
					printf("1\n");
				}
			}
			printf("\n");
		}
		printf("\n\n");
	}

	printf("\n");
#endif

}//END
//*************************************************************************************************

//----------------------------------------------
//  関数名 : lsim_2v_comb_fout
//  機  能 : FOUT論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_fout(NLIST *net){

	unsigned int	i;

	for(i=0; i<net->nval->int_num; i++){
		net->nval->x_buf[i] = net->in[0]->nval->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_comb_buf
//  機  能 : BUF論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_buf(NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->nval->int_num; i++){
		net->nval->x_buf[i] = net->in[0]->nval->x_buf[i];
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_comb_inv
//  機  能 : INV論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_inv(NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->nval->int_num; i++){
		net->nval->x_buf[i] = ~(net->in[0]->nval->x_buf[i]);
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_comb_and
//  機  能 : AND論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_and(NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval->int_num; i++){
		net->nval->x_buf[i] = net->in[0]->nval->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval->int_num; j++){
			net->nval->x_buf[j] &= net->in[i]->nval->x_buf[j];	//x_buf同士をAND演算
		}
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_comb_nand
//  機  能 : NAND論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_nand(NLIST *net){

	unsigned int		i;
	unsigned int		j;

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval->int_num; i++){
		net->nval->x_buf[i] = net->in[0]->nval->x_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval->int_num; j++){
			net->nval->x_buf[j] &= net->in[i]->nval->x_buf[j];	//x_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理
	//-------------------------------------
	for(i=0; i<net->nval->int_num; i++){
		net->nval->x_buf[i] = ~net->nval->x_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_2v_comb_or
//  機  能 : OR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_or(NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval->int_num; j++){
		net->nval->x_buf[j] = net->in[0]->nval->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval->int_num; j++){
			net->nval->x_buf[j] |= net->in[i]->nval->x_buf[j];	//x_buf同士をOR演算
		}
	}
}

//----------------------------------------------
//  関数名 : lsim_2v_comb_nor
//  機  能 : NOR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_nor(NLIST *net){

	int					i;
	unsigned int		j;

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval->int_num; j++){
		net->nval->x_buf[j] = net->in[0]->nval->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval->int_num; j++){
			net->nval->x_buf[j] |= net->in[i]->nval->x_buf[j];	//x_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval->int_num; j++){
		net->nval->x_buf[j] = ~net->nval->x_buf[j];
	}
}


//----------------------------------------------
//  関数名 : lsim_2v_comb_exor
//  機  能 : EXOR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_exor(NLIST *net){
	
	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval->int_num; j++){
		net->nval->x_buf[j] = net->in[0]->nval->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval->int_num; j++){
			net->nval->x_buf[j] ^= net->in[i]->nval->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

}//END


//----------------------------------------------
//  関数名 : lsim_2v_comb_exnor
//  機  能 : EXNOR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_2v_comb_exnor(NLIST *net){
	
	int					i;
	unsigned int		j;

	//-------------------------------------
	// EXORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval->int_num; j++){
		net->nval->x_buf[j] = net->in[0]->nval->x_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval->int_num; j++){
			net->nval->x_buf[j] ^= net->in[i]->nval->x_buf[j];	//x_buf同士をEXOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval->int_num; j++){
		net->nval->x_buf[j] = ~net->nval->x_buf[j];
	}

}//END
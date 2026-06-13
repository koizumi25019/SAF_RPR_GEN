//------------------------------------------------------------------------
//File name : SAF_3v_XID_PPSFP_LSIM.c
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
#include	"../../Xidentification.h" //"../../XIdentification.h" を "../../Xidentification.h"に変更

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
static void lsim_3v_xid_fout	(NLIST*);
static void lsim_3v_xid_buf		(NLIST*);
static void lsim_3v_xid_inv		(NLIST*);
static void lsim_3v_xid_and		(NLIST*);
static void lsim_3v_xid_nand	(NLIST*);
static void lsim_3v_xid_or		(NLIST*);
static void lsim_3v_xid_nor		(NLIST*);
static void lsim_3v_xid_exor	(NLIST*);
static void lsim_3v_xid_exnor	(NLIST*);

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
//  関数名 : SAF_3v_XID_PPSFP_LSIM
//  機  能 : X抽出後テストパターンで3値論理シミュレーション(X抽出専用)
//  戻り値 : なし
//  引  数 : なし
//【未使用関数】
//----------------------------------------------
void	SAF_3v_XID_PPSFP_LSIM(){

	int i,j;
	int	nval;	//正常値を一時保存

#ifdef LSIM_DEBUG
	int	k;
#endif

	//================================================
	// 関数ポインタ設定
	//================================================
	static void(*func[20])(NLIST*);
	func[FOUT]	= lsim_3v_xid_fout;
	func[BUF]	= lsim_3v_xid_buf;
	func[INV]	= lsim_3v_xid_inv;
	func[AND]	= lsim_3v_xid_and;
	func[NAND]	= lsim_3v_xid_nand;
	func[OR]	= lsim_3v_xid_or;
	func[NOR]	= lsim_3v_xid_nor;
	func[EXOR]	= lsim_3v_xid_exor;
	func[EXNOR]	= lsim_3v_xid_exnor;
	
	
	//================================================
	// X抽出後のテストパターンを印加
	//================================================
	for(i=0; i<n_tp; i++){
		//X抽出後TPのケアビット数>0
		if(tp_bit[i].n_cbit > 0){
			for(j=0; j<n_pi; j++){
				//対象TPの対象PIがケアビットの場合
				if(Get_NBit(pi[j]->xid_nval, i) != 3){

					//対象PIの正常値を保存
					nval = Get_NBit(pi[j]->nval, i);

					//nval値をxid_nvalに代入
					if(nval == 0){
						Set_NBit_Zero(pi[j]->xid_nval, i);
					}
					else if(nval == 1){
						Set_NBit_One(pi[j]->xid_nval, i);
					}
					else if(nval == 3){
						printf("\n//--------------------\n");
						printf("// ERROR: X抽出後のテストパターンを印加\n");
						printf("//--------------------\n");
						printf("nval:ドントケアをケアビット固定してるかも\n");
					}
					else{
						printf("\n//--------------------\n");
						printf("// ERROR: X抽出後のテストパターンを印加\n");
						printf("//--------------------\n");
						printf("0,1,X以外の謎値が出てきた!!\n");
					}
				}
			}
		}
	}


	//================================================
	// 論理シミュレーション
	//================================================
	fprintf(stderr,"\n SAF_3v_XID_PPSFP_LSIM\n");
	fprintf(stderr,"|----+----|----+----|----+----|----+----|----+----|\n");

	for(i=1; i<max_level+1; i++){
		// Progress Bar
		if(((int)i % (int)(((max_level)*2/100)+1)) == 0)fprintf(stderr,"*");

		//論理SIM
		for(j=0; j<nml_lev[i].n_net; j++){
			(*func[nml_lev[i].net[j]->type])(nml_lev[i].net[j]);
		}
	}

	
	//================================================
	// DEBUG
	//================================================
#ifdef LSIM_DEBUG
	printf("\n//-------------------------------\n");
	printf("//DEBUG:3値論理シミュレーション確認\n");
	printf("//-------------------------------\n");

	for(i=0; i<n_tp; i++){
		printf("-------------------------\n");
		printf("  %dパターン目\n", i);
		//PI
		for(j=0; j<n_pi; j++){
			printf("name:%s  type:PI  xid_nval:", pi[j]->name);
			if(Get_NBit(pi[j]->xid_nval, i) == 0){
					printf("0\n");
			}
			else if(Get_NBit(pi[j]->xid_nval, i) == 1){
					printf("1\n");
			}
			else if(Get_NBit(pi[j]->xid_nval, i) == 3){
					printf("X\n");
			}
		}
		
		printf("\n");

		//PI以外
		for(j=1; j<max_level+1; j++){
			for(k=0; k<nml_lev[j].n_net; k++){
				printf("name:%s  type:%d  xid_nval:", nml_lev[j].net[k]->name, nml_lev[j].net[k]->type);
				if(Get_NBit(nml_lev[j].net[k]->xid_nval, i) == 0){
					printf("0\n");
				}
				else if(Get_NBit(nml_lev[j].net[k]->xid_nval, i) == 1){
					printf("1\n");
				}
				else if(Get_NBit(nml_lev[j].net[k]->xid_nval, i) == 3){
					printf("X\n");
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
//  関数名 : lsim_3v_xid_fout
//  機  能 : FOUT論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_fout(NLIST *net){

	unsigned int	i;

	for(i=0; i<net->xid_nval->int_num; i++){
		net->xid_nval->x_buf[i] = net->in[0]->xid_nval->x_buf[i];
		net->xid_nval->p_buf[i] = net->in[0]->xid_nval->p_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_3v_xid_buf
//  機  能 : BUF論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_buf(NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->xid_nval->int_num; i++){
		net->xid_nval->x_buf[i] = net->in[0]->xid_nval->x_buf[i];
		net->xid_nval->p_buf[i] = net->in[0]->xid_nval->p_buf[i];
	}
}

//----------------------------------------------
//  関数名 : lsim_3v_xid_inv
//  機  能 : INV論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_inv(NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->xid_nval->int_num; i++){
		net->xid_nval->x_buf[i] = net->in[0]->xid_nval->p_buf[i];
		net->xid_nval->p_buf[i] = net->in[0]->xid_nval->x_buf[i];
	}
}

//----------------------------------------------
//  関数名 : lsim_3v_xid_and
//  機  能 : AND論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_and(NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->xid_nval->int_num; i++){
		net->xid_nval->x_buf[i] = net->in[0]->xid_nval->x_buf[i];
		net->xid_nval->p_buf[i] = net->in[0]->xid_nval->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->xid_nval->int_num; j++){
			net->xid_nval->x_buf[j] &= net->in[i]->xid_nval->x_buf[j];	//x_buf同士をAND演算
			net->xid_nval->p_buf[j] |= net->in[i]->xid_nval->p_buf[j];	//p_buf同士をOR演算
		}
	}

}

//----------------------------------------------
//  関数名 : lsim_3v_xid_nand
//  機  能 : NAND論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_nand(NLIST *net){

	unsigned int		i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->xid_nval->int_num; i++){
		net->xid_nval->x_buf[i] = net->in[0]->xid_nval->x_buf[i];
		net->xid_nval->p_buf[i] = net->in[0]->xid_nval->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->xid_nval->int_num; j++){
			net->xid_nval->x_buf[j] &= net->in[i]->xid_nval->x_buf[j];	//x_buf同士をAND演算
			net->xid_nval->p_buf[j] |= net->in[i]->xid_nval->p_buf[j];	//p_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->xid_nval->int_num; j++){
		temp = net->xid_nval->x_buf[j];
		net->xid_nval->x_buf[j] = net->xid_nval->p_buf[j];	//x_bufにp_bufを代入
		net->xid_nval->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}

}

//----------------------------------------------
//  関数名 : lsim_3v_xid_or
//  機  能 : OR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_or(NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->xid_nval->int_num; j++){
		net->xid_nval->x_buf[j] = net->in[0]->xid_nval->x_buf[j];
		net->xid_nval->p_buf[j] = net->in[0]->xid_nval->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->xid_nval->int_num; j++){
			net->xid_nval->x_buf[j] |= net->in[i]->xid_nval->x_buf[j];	//x_buf同士をOR演算
			net->xid_nval->p_buf[j] &= net->in[i]->xid_nval->p_buf[j];	//p_buf同士をAND演算
		}
	}
}

//----------------------------------------------
//  関数名 : lsim_3v_xid_nor
//  機  能 : NOR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
static void lsim_3v_xid_nor(NLIST *net){

	int					i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->xid_nval->int_num; j++){
		net->xid_nval->x_buf[j] = net->in[0]->xid_nval->x_buf[j];
		net->xid_nval->p_buf[j] = net->in[0]->xid_nval->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->xid_nval->int_num; j++){
			net->xid_nval->x_buf[j] |= net->in[i]->xid_nval->x_buf[j];	//x_buf同士をOR演算
			net->xid_nval->p_buf[j] &= net->in[i]->xid_nval->p_buf[j];	//p_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->xid_nval->int_num; j++){
		temp = net->xid_nval->x_buf[j];
		net->xid_nval->x_buf[j] = net->xid_nval->p_buf[j];	//x_bufにp_bufを代入
		net->xid_nval->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}
}


//----------------------------------------------
//  関数名 : lsim_3v_xid_exor
//  機  能 : EXOR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXORのみ対応！
//----------------------------------------------
static void lsim_3v_xid_exor(NLIST *net){
	
	unsigned int		i;
	
	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->xid_nval->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->xid_nval->x_buf[i] = (net->in[0]->xid_nval->x_buf[i]&net->in[1]->xid_nval->p_buf[i]) | (net->in[0]->xid_nval->p_buf[i]&net->in[1]->xid_nval->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->xid_nval->p_buf[i] = (net->in[0]->xid_nval->p_buf[i]|net->in[1]->xid_nval->x_buf[i]) & (net->in[0]->xid_nval->x_buf[i]|net->in[1]->xid_nval->p_buf[i]);
	}


}//END


//----------------------------------------------
//  関数名 : lsim_3v_xid_exnor
//  機  能 : EXNOR論理演算(組合せ回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXNORのみ対応！
//----------------------------------------------
static void lsim_3v_xid_exnor(NLIST *net){
	
	unsigned int		i;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用
	

	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->xid_nval->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->xid_nval->x_buf[i] = (net->in[0]->xid_nval->x_buf[i]&net->in[1]->xid_nval->p_buf[i]) | (net->in[0]->xid_nval->p_buf[i]&net->in[1]->xid_nval->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->xid_nval->p_buf[i] = (net->in[0]->xid_nval->p_buf[i]|net->in[1]->xid_nval->x_buf[i]) & (net->in[0]->xid_nval->x_buf[i]|net->in[1]->xid_nval->p_buf[i]);


		//-------------------------------------
		// NOT演算処理(x_bufとp_bufを入れ替える)
		//-------------------------------------
		temp = net->xid_nval->x_buf[i];
		net->xid_nval->x_buf[i] = net->xid_nval->p_buf[i];	//x_bufにp_bufを代入
		net->xid_nval->p_buf[i] = temp;					//p_bufにx_buf(temp)を代入
	}

}//END
//------------------------------------------------------------------------
//File name : cal_com_controllability.c
//Date : 2013/12/04
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../StandardHead.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
unsigned int	cal_com_mincost		(NLIST*, int);
unsigned int	cal_com_sumcost		(NLIST*, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define		UNSIGNED_MUGEN		0x80000000	//コスト値無限大

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : cal_com_mincost
//  機  能 : 入力コストの最小値計算
//  戻り値 : 最小コスト
//  引  数 : net(コスト計算対象信号線), cv(制御値)
//----------------------------------------------
unsigned int  cal_com_mincost(NLIST *net, int cv){

	register int	i;
	unsigned int	min;

	//in[0]の最小コスト代入
	min = net->in[0]->cost[cv];

	//最小コスト探索
	for (i=1; i<net->n_in; i++) {
		if ( net->in[i]->cost[cv] < min ){
			min = net->in[i]->cost[cv];
		}
	}
	return( min );
}

//----------------------------------------------
//  関数名 : cal_com_sumcost
//  機  能 : 入力コストの総和を計算
//  戻り値 : 入力コストの総和
//  引  数 : net(コスト計算対象信号線), cv(制御値)
//----------------------------------------------
unsigned int cal_com_sumcost(NLIST  *net , int cv){

	register int	i;
	unsigned int	sum = 0;


	for (i=0; i<net->n_in; i++) {
		
		//入力のコスト値が無限大の場合
		if(net->in[i]->cost[cv] == UNSIGNED_MUGEN){
			sum = UNSIGNED_MUGEN;
			break;
		}
		
		//その他
		else{
			sum += net->in[i]->cost[cv];
		}
	}

	return sum;

}
//----------------------------------------------
//  関数名 : initial_com_controllability
//  機  能 : PI可制御性の初期化(SCOAP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void  initial_com_controllability(){
	
	int			i;

	//PIコスト初期化
	for(i=0; i<n_pi; i++){
		pi[i]->cost[C_0] = 1;	//0可制御性
		pi[i]->cost[C_1] = 1;	//1可制御性
	}
}

//----------------------------------------------
//  関数名 : cal_com_controllability
//  機  能 : 可制御性の計算(SCOAP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void  cal_com_controllability(){

	int				i,j;
	unsigned int	and0_c0, and0_c1, and1_c0, and1_c1;		//EX(N)ORゲートで使用

	for(i=1; i<max_level+1; i++){
		for(j=0; j<nml_lev[i].n_net; j++){
#ifdef DEBUG
			printf("可制御性計算:%s (%d)\n", nml_lev[i].net[j]->name, nml_lev[i].net[j]->type);
#endif
			switch(nml_lev[i].net[j]->type){
				//-----------------------------------------------
				case FOUT:
					//入力信号線の可制御性を引継ぎ
					nml_lev[i].net[j]->cost[C_0] = nml_lev[i].net[j]->in[0]->cost[C_0];
					nml_lev[i].net[j]->cost[C_1] = nml_lev[i].net[j]->in[0]->cost[C_1];
				break;
				//-----------------------------------------------
				case BUF:
					//入力信号線の可制御性+1
					nml_lev[i].net[j]->cost[C_0] = nml_lev[i].net[j]->in[0]->cost[C_0] + 1;
					nml_lev[i].net[j]->cost[C_1] = nml_lev[i].net[j]->in[0]->cost[C_1] + 1;
				break;
				//-----------------------------------------------
				case INV:
					//入力信号線の可制御性+1
					nml_lev[i].net[j]->cost[C_0] = nml_lev[i].net[j]->in[0]->cost[C_1] + 1;	//入れ替え
					nml_lev[i].net[j]->cost[C_1] = nml_lev[i].net[j]->in[0]->cost[C_0] + 1;	//入れ替え
				break;
				//-----------------------------------------------
				case AND:
					//0可制御性:入力の最小コスト+1
					nml_lev[i].net[j]->cost[C_0] = cal_com_mincost(nml_lev[i].net[j], C_0) + 1;

					//1可制御性:入力コストの総和+1
					nml_lev[i].net[j]->cost[C_1] = cal_com_sumcost(nml_lev[i].net[j], C_1) + 1;

				break;
				//-----------------------------------------------
				case NAND:
					//1可制御性:入力の最小コスト+1
					nml_lev[i].net[j]->cost[C_1] = cal_com_mincost(nml_lev[i].net[j], C_0) + 1;

					//0可制御性:入力コストの総和+1
					nml_lev[i].net[j]->cost[C_0] = cal_com_sumcost(nml_lev[i].net[j], C_1) + 1;

				break;
				//-----------------------------------------------
				case OR:
					//1可制御性:入力の最小コスト+1
					nml_lev[i].net[j]->cost[C_1] = cal_com_mincost(nml_lev[i].net[j], C_1) + 1;

					//0可制御性:入力コストの総和+1
					nml_lev[i].net[j]->cost[C_0] = cal_com_sumcost(nml_lev[i].net[j], C_0) + 1;

				break;
				//-----------------------------------------------
				case NOR:
					//0可制御性:入力の最小コスト+1
					nml_lev[i].net[j]->cost[C_0] = cal_com_mincost(nml_lev[i].net[j], C_1) + 1;

					//1可制御性:入力コストの総和+1
					nml_lev[i].net[j]->cost[C_1] = cal_com_sumcost(nml_lev[i].net[j], C_0) + 1;

				break;
				//-----------------------------------------------
				case EXOR:
					
					//0番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_lev[i].net[j]->in[0]->cost[C_0]+1 > nml_lev[i].net[j]->in[1]->cost[C_1]+2){
						and0_c0 = nml_lev[i].net[j]->in[1]->cost[C_1]+2;		//INVを1個通過してるため+2
					}
					else{
						and0_c0 = nml_lev[i].net[j]->in[0]->cost[C_0]+1;
					}

					//0番目ANDゲートの1可制御性計算(入力の総和)
					and0_c1 = nml_lev[i].net[j]->in[0]->cost[C_1] + nml_lev[i].net[j]->in[1]->cost[C_0] + 2;


					//1番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_lev[i].net[j]->in[0]->cost[C_1]+1 > nml_lev[i].net[j]->in[1]->cost[C_0]+2){
						and1_c0 = nml_lev[i].net[j]->in[1]->cost[C_0]+2;		//INVを1個通過してるため+2
					}
					else{
						and1_c0 = nml_lev[i].net[j]->in[0]->cost[C_1]+1;
					}

					//1番目ANDゲートの1可制御性計算(入力の総和)
					and1_c1 = nml_lev[i].net[j]->in[0]->cost[C_0] + nml_lev[i].net[j]->in[1]->cost[C_1] + 2;


					//0可制御性:EXOR展開したもので計算(入力の総和)
					nml_lev[i].net[j]->cost[C_0] = and0_c0 + and1_c0 + 1;

					//1可制御性:EXOR展開したもので計算(入力の最小値)
					if(and0_c1 > and1_c1){
						nml_lev[i].net[j]->cost[C_1] = and1_c1 + 1;
					}
					else{
						nml_lev[i].net[j]->cost[C_1] = and0_c1 + 1;
					}

				break;
				//-----------------------------------------------
				case EXNOR:
					
					//0番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_lev[i].net[j]->in[0]->cost[C_0]+1 > nml_lev[i].net[j]->in[1]->cost[C_0]+1){
						and0_c0 = nml_lev[i].net[j]->in[1]->cost[C_0]+1;
					}
					else{
						and0_c0 = nml_lev[i].net[j]->in[0]->cost[C_0]+1;
					}

					//0番目ANDゲートの1可制御性計算(入力の総和)
					and0_c1 = nml_lev[i].net[j]->in[0]->cost[C_1] + nml_lev[i].net[j]->in[1]->cost[C_1] + 1;


					//1番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_lev[i].net[j]->in[0]->cost[C_1]+2 > nml_lev[i].net[j]->in[1]->cost[C_1]+2){
						and1_c0 = nml_lev[i].net[j]->in[1]->cost[C_1]+2;		//INVを1個通過してるため+2
					}
					else{
						and1_c0 = nml_lev[i].net[j]->in[0]->cost[C_1]+2;		//INVを1個通過してるため+2
					}

					//1番目ANDゲートの1可制御性計算(入力の総和)
					and1_c1 = nml_lev[i].net[j]->in[0]->cost[C_0] + nml_lev[i].net[j]->in[1]->cost[C_0] + 3; //両入力ともINV通過のため+3


					//0可制御性:EXOR展開したもので計算(入力の総和)
					nml_lev[i].net[j]->cost[C_0] = and0_c0 + and1_c0 + 1;

					//1可制御性:EXOR展開したもので計算(入力の最小値)
					if(and0_c1 > and1_c1){
						nml_lev[i].net[j]->cost[C_1] = and1_c1 + 1;
					}
					else{
						nml_lev[i].net[j]->cost[C_1] = and0_c1 + 1;
					}

				break;
				//-----------------------------------------------
				default:
				break;
				//-----------------------------------------------
			}
		}
	}
}
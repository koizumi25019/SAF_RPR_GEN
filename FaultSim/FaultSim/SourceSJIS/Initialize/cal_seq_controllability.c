//------------------------------------------------------------------------
//File name : cal_seq_controllability.c
//Date : 2013/12/04
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/s_netlist.h"
#include	"../StandardHead.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
unsigned int	cal_seq_mincost		(S_NLIST*, int, int);
unsigned int	cal_seq_sumcost		(S_NLIST*, int, int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define		UNSIGNED_MUGEN		0x80000000	//コスト値無限大

	//#define		DEBUG
//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : cal_seq_mincost
//  機  能 : 入力コストの最小値計算
//  戻り値 : 最小コスト
//  引  数 : net(コスト計算対象信号線), cv(制御値), time(時間)
//----------------------------------------------
unsigned int  cal_seq_mincost(S_NLIST *net, int cv, int time){

	register int	i;
	unsigned int	min;

	//in[0]の最小コスト代入
	min = net->in[0]->cost[cv][time];

	//最小コスト探索
	for (i=1; i<net->n_in; i++) {
		if ( net->in[i]->cost[cv][time] < min ){
			min = net->in[i]->cost[cv][time];
		}
	}
	return( min );
}

//----------------------------------------------
//  関数名 : cal_seq_sumcost
//  機  能 : 入力コストの総和を計算
//  戻り値 : 入力コストの総和
//  引  数 : net(コスト計算対象信号線), cv(制御値), time(時間)
//----------------------------------------------
unsigned int cal_seq_sumcost(S_NLIST  *net , int cv , int time){

	register int	i;
	unsigned int	sum = 0;


	for (i=0; i<net->n_in; i++) {
		
		//入力のコスト値が無限大の場合
		if(net->in[i]->cost[cv][time] == UNSIGNED_MUGEN){
			sum = UNSIGNED_MUGEN;
			break;
		}
		
		//その他
		else{
			sum += net->in[i]->cost[cv][time];
		}
	}

	return sum;

}
//----------------------------------------------
//  関数名 : initial_seq_controllability
//  機  能 : PI,PPIの可制御性の初期化(SCOAP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void  initial_seq_controllability(){
	
	int			i;

	//=========================================================
	// PIと1時刻目PPIのコスト初期化
	//=========================================================
	//PIコスト初期化
	for(i=0; i<n_spi; i++){
		spi[i]->cost[C_0][0] = 1;	//1時刻目0可制御性
		spi[i]->cost[C_0][1] = 1;	//2時刻目0可制御性
		spi[i]->cost[C_1][0] = 1;	//1時刻目1可制御性
		spi[i]->cost[C_1][1] = 1;	//2時刻目1可制御性
	}
	
	//1時刻目PPIコスト初期化
	for(i=0; i<n_ppi; i++){
		ppi[i]->cost[C_0][0] = 1;	//1時刻目0可制御性
		ppi[i]->cost[C_1][0] = 1;	//1時刻目1可制御性
	}	
}

//----------------------------------------------
//  関数名 : cal_seq_controllability
//  機  能 : 可制御性の計算(SCOAP)
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void  cal_seq_controllability(){

	int				i,j;
	unsigned int	and0_c0, and0_c1, and1_c0, and1_c1;		//EX(N)ORゲートで使用

	//=========================================================
	// 1時刻目の可制御性計算
	//=========================================================
	for(i=1; i<max_level+1; i++){
		for(j=0; j<nml_slev[i].n_net; j++){
#ifdef DEBUG
			printf("可制御性計算:%s (%d)\n", nml_slev[i].net[j]->name, nml_slev[i].net[j]->type);
#endif
			switch(nml_slev[i].net[j]->type){
				//-----------------------------------------------
				case FOUT:
					//入力信号線の可制御性を引継ぎ
					nml_slev[i].net[j]->cost[C_0][0] = nml_slev[i].net[j]->in[0]->cost[C_0][0];
					nml_slev[i].net[j]->cost[C_1][0] = nml_slev[i].net[j]->in[0]->cost[C_1][0];
				break;
				//-----------------------------------------------
				case BUF:
					//入力信号線の可制御性+1
					nml_slev[i].net[j]->cost[C_0][0] = nml_slev[i].net[j]->in[0]->cost[C_0][0] + 1;
					nml_slev[i].net[j]->cost[C_1][0] = nml_slev[i].net[j]->in[0]->cost[C_1][0] + 1;
				break;
				//-----------------------------------------------
				case INV:
					//入力信号線の可制御性+1
					nml_slev[i].net[j]->cost[C_0][0] = nml_slev[i].net[j]->in[0]->cost[C_1][0] + 1;	//入れ替え
					nml_slev[i].net[j]->cost[C_1][0] = nml_slev[i].net[j]->in[0]->cost[C_0][0] + 1;	//入れ替え
				break;
				//-----------------------------------------------
				case AND:
					//0可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_0][0] = cal_seq_mincost(nml_slev[i].net[j], C_0, 0) + 1;

					//1可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_1][0] = cal_seq_sumcost(nml_slev[i].net[j], C_1, 0) + 1;

				break;
				//-----------------------------------------------
				case NAND:
					//1可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_1][0] = cal_seq_mincost(nml_slev[i].net[j], C_0, 0) + 1;

					//0可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_0][0] = cal_seq_sumcost(nml_slev[i].net[j], C_1, 0) + 1;

				break;
				//-----------------------------------------------
				case OR:
					//1可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_1][0] = cal_seq_mincost(nml_slev[i].net[j], C_1, 0) + 1;

					//0可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_0][0] = cal_seq_sumcost(nml_slev[i].net[j], C_0, 0) + 1;

				break;
				//-----------------------------------------------
				case NOR:
					//0可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_0][0] = cal_seq_mincost(nml_slev[i].net[j], C_1, 0) + 1;

					//1可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_1][0] = cal_seq_sumcost(nml_slev[i].net[j], C_0, 0) + 1;

				break;
				//-----------------------------------------------
				case EXOR:
					
					//0番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_0][0]+1 > nml_slev[i].net[j]->in[1]->cost[C_1][0]+2){
						and0_c0 = nml_slev[i].net[j]->in[1]->cost[C_1][0]+2;		//INVを1個通過してるため+2
					}
					else{
						and0_c0 = nml_slev[i].net[j]->in[0]->cost[C_0][0]+1;
					}

					//0番目ANDゲートの1可制御性計算(入力の総和)
					and0_c1 = nml_slev[i].net[j]->in[0]->cost[C_1][0] + nml_slev[i].net[j]->in[1]->cost[C_0][0] + 2;


					//1番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_1][0]+1 > nml_slev[i].net[j]->in[1]->cost[C_0][0]+2){
						and1_c0 = nml_slev[i].net[j]->in[1]->cost[C_0][0]+2;		//INVを1個通過してるため+2
					}
					else{
						and1_c0 = nml_slev[i].net[j]->in[0]->cost[C_1][0]+1;
					}

					//1番目ANDゲートの1可制御性計算(入力の総和)
					and1_c1 = nml_slev[i].net[j]->in[0]->cost[C_0][0] + nml_slev[i].net[j]->in[1]->cost[C_1][0] + 2;


					//0可制御性:EXOR展開したもので計算(入力の総和)
					nml_slev[i].net[j]->cost[C_0][0] = and0_c0 + and1_c0 + 1;

					//1可制御性:EXOR展開したもので計算(入力の最小値)
					if(and0_c1 > and1_c1){
						nml_slev[i].net[j]->cost[C_1][0] = and1_c1 + 1;
					}
					else{
						nml_slev[i].net[j]->cost[C_1][0] = and0_c1 + 1;
					}

				break;
				//-----------------------------------------------
				case EXNOR:
					
					//0番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_0][0]+1 > nml_slev[i].net[j]->in[1]->cost[C_0][0]+1){
						and0_c0 = nml_slev[i].net[j]->in[1]->cost[C_0][0]+1;
					}
					else{
						and0_c0 = nml_slev[i].net[j]->in[0]->cost[C_0][0]+1;
					}

					//0番目ANDゲートの1可制御性計算(入力の総和)
					and0_c1 = nml_slev[i].net[j]->in[0]->cost[C_1][0] + nml_slev[i].net[j]->in[1]->cost[C_1][0] + 1;


					//1番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_1][0]+2 > nml_slev[i].net[j]->in[1]->cost[C_1][0]+2){
						and1_c0 = nml_slev[i].net[j]->in[1]->cost[C_1][0]+2;		//INVを1個通過してるため+2
					}
					else{
						and1_c0 = nml_slev[i].net[j]->in[0]->cost[C_1][0]+2;		//INVを1個通過してるため+2
					}

					//1番目ANDゲートの1可制御性計算(入力の総和)
					and1_c1 = nml_slev[i].net[j]->in[0]->cost[C_0][0] + nml_slev[i].net[j]->in[1]->cost[C_0][0] + 3; //両入力ともINV通過のため+3


					//0可制御性:EXOR展開したもので計算(入力の総和)
					nml_slev[i].net[j]->cost[C_0][0] = and0_c0 + and1_c0 + 1;

					//1可制御性:EXOR展開したもので計算(入力の最小値)
					if(and0_c1 > and1_c1){
						nml_slev[i].net[j]->cost[C_1][0] = and1_c1 + 1;
					}
					else{
						nml_slev[i].net[j]->cost[C_1][0] = and0_c1 + 1;
					}

				break;
				//-----------------------------------------------
				default:
				break;
				//-----------------------------------------------
			}
		}
	}

	//=========================================================
	// 2時刻目の可制御性計算
	//=========================================================
	//------------------------------------------
	// 2時刻目PPIコストの代入
	//------------------------------------------
	for(i=0; i<n_ppi; i++){
		ppi[i]->cost[C_0][1] = ppi[i]->in[0]->cost[C_0][0] + 1;		//2時刻目PPIコスト = 1時刻目PPOコスト+1
		ppi[i]->cost[C_1][1] = ppi[i]->in[0]->cost[C_1][0] + 1;		//2時刻目PPIコスト = 1時刻目PPOコスト+1
	}

	//------------------------------------------
	// 2時刻目可制御性計算
	//------------------------------------------
	for(i=1; i<max_level+1; i++){
		for(j=0; j<nml_slev[i].n_net; j++){
#ifdef DEBUG
			printf("可制御性計算:%s (%d)\n", nml_slev[i].net[j]->name, nml_slev[i].net[j]->type);
#endif
			switch(nml_slev[i].net[j]->type){
				//-----------------------------------------------
				case FOUT:
					//入力信号線の可制御性を引継ぎ
					nml_slev[i].net[j]->cost[C_0][1] = nml_slev[i].net[j]->in[0]->cost[C_0][1];
					nml_slev[i].net[j]->cost[C_1][1] = nml_slev[i].net[j]->in[0]->cost[C_1][1];
				break;
				//-----------------------------------------------
				case BUF:
					//入力信号線の可制御性+1
					nml_slev[i].net[j]->cost[C_0][1] = nml_slev[i].net[j]->in[0]->cost[C_0][1] + 1;
					nml_slev[i].net[j]->cost[C_1][1] = nml_slev[i].net[j]->in[0]->cost[C_1][1] + 1;
				break;
				//-----------------------------------------------
				case INV:
					//入力信号線の可制御性+1
					nml_slev[i].net[j]->cost[C_0][1] = nml_slev[i].net[j]->in[0]->cost[C_1][1] + 1;	//入れ替え
					nml_slev[i].net[j]->cost[C_1][1] = nml_slev[i].net[j]->in[0]->cost[C_0][1] + 1;	//入れ替え
				break;
				//-----------------------------------------------
				case AND:
					//0可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_0][1] = cal_seq_mincost(nml_slev[i].net[j], C_0, 1) + 1;

					//1可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_1][1] = cal_seq_sumcost(nml_slev[i].net[j], C_1, 1) + 1;

				break;
				//-----------------------------------------------
				case NAND:
					//1可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_1][1] = cal_seq_mincost(nml_slev[i].net[j], C_0, 1) + 1;

					//0可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_0][1] = cal_seq_sumcost(nml_slev[i].net[j], C_1, 1) + 1;

				break;
				//-----------------------------------------------
				case OR:
					//1可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_1][1] = cal_seq_mincost(nml_slev[i].net[j], C_1, 1) + 1;

					//0可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_0][1] = cal_seq_sumcost(nml_slev[i].net[j], C_0, 1) + 1;

				break;
				//-----------------------------------------------
				case NOR:
					//0可制御性:入力の最小コスト+1
					nml_slev[i].net[j]->cost[C_0][1] = cal_seq_mincost(nml_slev[i].net[j], C_1, 1) + 1;

					//1可制御性:入力コストの総和+1
					nml_slev[i].net[j]->cost[C_1][1] = cal_seq_sumcost(nml_slev[i].net[j], C_0, 1) + 1;

				break;
				//-----------------------------------------------
				case EXOR:
					
					//0番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_0][1]+1 > nml_slev[i].net[j]->in[1]->cost[C_1][1]+2){
						and0_c0 = nml_slev[i].net[j]->in[1]->cost[C_1][1]+2;		//INVを1個通過してるため+2
					}
					else{
						and0_c0 = nml_slev[i].net[j]->in[0]->cost[C_0][1]+1;
					}

					//0番目ANDゲートの1可制御性計算(入力の総和)
					and0_c1 = nml_slev[i].net[j]->in[0]->cost[C_1][1] + nml_slev[i].net[j]->in[1]->cost[C_0][1] + 2;


					//1番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_1][1]+1 > nml_slev[i].net[j]->in[1]->cost[C_0][1]+2){
						and1_c0 = nml_slev[i].net[j]->in[1]->cost[C_0][1]+2;		//INVを1個通過してるため+2
					}
					else{
						and1_c0 = nml_slev[i].net[j]->in[0]->cost[C_1][1]+1;
					}

					//1番目ANDゲートの1可制御性計算(入力の総和)
					and1_c1 = nml_slev[i].net[j]->in[0]->cost[C_0][1] + nml_slev[i].net[j]->in[1]->cost[C_1][1] + 2;


					//0可制御性:EXOR展開したもので計算(入力の総和)
					nml_slev[i].net[j]->cost[C_0][1] = and0_c0 + and1_c0 + 1;

					//1可制御性:EXOR展開したもので計算(入力の最小値)
					if(and0_c1 > and1_c1){
						nml_slev[i].net[j]->cost[C_1][1] = and1_c1 + 1;
					}
					else{
						nml_slev[i].net[j]->cost[C_1][1] = and0_c1 + 1;
					}

				break;
				//-----------------------------------------------
				case EXNOR:
					
					//0番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_0][1]+1 > nml_slev[i].net[j]->in[1]->cost[C_0][1]+1){
						and0_c0 = nml_slev[i].net[j]->in[1]->cost[C_0][1]+1;
					}
					else{
						and0_c0 = nml_slev[i].net[j]->in[0]->cost[C_0][1]+1;
					}

					//0番目ANDゲートの1可制御性計算(入力の総和)
					and0_c1 = nml_slev[i].net[j]->in[0]->cost[C_1][1] + nml_slev[i].net[j]->in[1]->cost[C_1][1] + 1;


					//1番目ANDゲートの0可制御性計算(入力の最小値)
					if(nml_slev[i].net[j]->in[0]->cost[C_1][1]+2 > nml_slev[i].net[j]->in[1]->cost[C_1][1]+2){
						and1_c0 = nml_slev[i].net[j]->in[1]->cost[C_1][1]+2;		//INVを1個通過してるため+2
					}
					else{
						and1_c0 = nml_slev[i].net[j]->in[0]->cost[C_1][1]+2;		//INVを1個通過してるため+2
					}

					//1番目ANDゲートの1可制御性計算(入力の総和)
					and1_c1 = nml_slev[i].net[j]->in[0]->cost[C_0][1] + nml_slev[i].net[j]->in[1]->cost[C_0][1] + 3; //両入力ともINV通過のため+3


					//0可制御性:EXOR展開したもので計算(入力の総和)
					nml_slev[i].net[j]->cost[C_0][1] = and0_c0 + and1_c0 + 1;

					//1可制御性:EXOR展開したもので計算(入力の最小値)
					if(and0_c1 > and1_c1){
						nml_slev[i].net[j]->cost[C_1][1] = and1_c1 + 1;
					}
					else{
						nml_slev[i].net[j]->cost[C_1][1] = and0_c1 + 1;
					}

				break;
				//-----------------------------------------------
				default:
				break;
				//-----------------------------------------------
			}
		}
	}


#ifdef DEBUG
	printf("\n//=================================\n");
	printf("// DEBUG: SCOAP計算\n");
	printf("//=================================\n");
	for(i=0; i<n_snet; i++){
		printf("%s t1(%d %d), t2(%d %d)\n", s_nl[i].name, s_nl[i].cost[C_0][0], s_nl[i].cost[C_1][0], s_nl[i].cost[C_0][1], s_nl[i].cost[C_1][1]);
	}
#endif

}
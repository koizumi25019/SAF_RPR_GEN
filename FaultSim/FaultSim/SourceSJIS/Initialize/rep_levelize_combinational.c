//------------------------------------------------------------------------
//File name : rep_levelize_combinational.c
//Date : 2012/1/13
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../Lib/alloc.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
int		*search_rep_net_comb();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	#define		REP_LEV_DEBUG

#define		LEVELED		11		//レベル計算済み
#define		STACK		22		//レベル未計算(スタックに積んでる)

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : rep_levelize_combinational
//  機  能 : 代表信号線レベライズ[組合せ回路用]
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	rep_levelize_combinational	(){

	int		i,j;
	int		count;
	int		*n_rep_lev_net;		//各レベルごとの代表信号線数

	
	//==========================================
	// 代表信号線解析
	//==========================================
	n_rep_lev_net = search_rep_net_comb();

#ifdef REP_LEV_DEBUG
	for(i=0; i<max_level+1; i++){
		printf("レベル%dの代表信号線数:%d\n", i , n_rep_lev_net[i]);
	}
#endif

	//==========================================
	// 代表信号線レベライズ
	//==========================================
	//----------------------------------
	// メモリ確保
	//----------------------------------
	rep_lev=(struct Levelize*)calloc(max_level+1,sizeof(struct Levelize));

	//----------------------------------
	// レベライズ構造体に格納
	//----------------------------------
	//●PI(レベル0)のみ別処理
	rep_lev[0].n_net = n_rep_lev_net[0];	//代表信号線数
	rep_lev[0].net	 = PNL_alloc(n_pi);		//メモリ確保(対象レベルの代表信号線数分)
	for(i=0; i<n_pi; i++)rep_lev[0].net[i]	= pi[i];	//代表信号線ポインタ接続


	//●レベル1以降
	for(i=1; i<max_level+1; i++){

		//初期化
		rep_lev[i].net		= NULL;
		rep_lev[i].n_net	= n_rep_lev_net[i];	//代表信号線数

		//------------------------------------------------------
		//代表信号線数が0超過
		if(rep_lev[i].n_net != 0){

			//カウンタ初期化
			count = 0;

			//メモリ確保(対象レベルの代表信号線数分)
			rep_lev[i].net = PNL_alloc(n_rep_lev_net[i]);

			//代表信号線をポインタ接続
			for(j=0; j<nml_lev[i].n_net; j++){
				if(count == n_rep_lev_net[i]){
					break;
				}

				if(nml_lev[i].net[j]->rep_net == YES){
					rep_lev[i].net[count] = nml_lev[i].net[j];	//代表信号線を格納
					count++;
				}
			}
		}
		//------------------------------------------------------

	}
	
#ifdef REP_LEV_DEBUG
	printf("\n//===========================\n");
	printf("// 代表信号線レベライズ DEBGU\n");
	printf("//===========================\n");
	for(i=0; i<max_level+1; i++){
		printf("レベル:%d  %d本\n",i, rep_lev[i].n_net);
		for(j=0; j<rep_lev[i].n_net; j++){
			printf("%s\n", rep_lev[i].net[j]->name);
		}
		printf("\n");
	}
	printf("\n");
#endif

}//END



//----------------------------------------------
//  関数名 : search_rep_net_comb
//  機  能 : 代表信号線解析[組合せ回路用]
//  戻り値 : *n_rep_lev_net(各レベルごとの代表信号線数)
//  引  数 : なし
//----------------------------------------------
int *search_rep_net_comb(){
	
	int		i,j,k,m,n;
	int		count=0;			//次イベントドリブン信号線数のカウンタ
	int		temp_id[1000];		//次イベントドリブン信号線のネットリストID
	int		*n_rep_lev_net;		//各レベルごとの代表信号線数
	

	//==========================================
	// 各レベルごとの代表信号線数 初期化
	//==========================================
	n_rep_lev_net = INT_alloc(max_level);
	for(i=0; i<max_level+1; i++)n_rep_lev_net[i] = 0;	//初期化


	//==========================================
	// PI初期化
	//==========================================
	for(i=0; i<n_pi; i++){
		pi[i]->rep_net			= YES;		//代表信号線:YES
		pi[i]->inv_flag			= NO;		//反転フラグ:NO
		pi[i]->influence_net	= pi[i];	//影響信号線は自分自身
	}

	//レベル0の代表信号線数
	n_rep_lev_net[0] = n_pi;
	
	//==========================================
	// PO初期化
	//==========================================
	for(i=0; i<n_po; i++){
		po[i]->rep_net			= YES;		//代表信号線:YES
		po[i]->next_event		= NULL;		//POなので次イベントは無し
		po[i]->n_next_event		= 0;		//POなので次イベントは無し

		//各POの代表信号線数インクリメント
		n_rep_lev_net[po[i]->level]++;
	}

	//==========================================
	// 代表信号線解析
	//==========================================
	for(i=1; i<max_level+1; i++){
		for(j=0; j<nml_lev[i].n_net; j++){

			//------------------------------------
			// 代表信号線と影響信号線判定
			//------------------------------------
			//■2入力以上の信号線
			if(nml_lev[i].net[j]->n_in >= 2){
				nml_lev[i].net[j]->rep_net = YES;						//代表信号線
				nml_lev[i].net[j]->influence_net = nml_lev[i].net[j];	//影響信号線は自分自身

				//PO以外の場合レベルiの代表信号線数更新
				if(nml_lev[i].net[j]->n_out != 0){
					n_rep_lev_net[i]++;
				}
			}
			
			//■1入力信号線 (かつPO以外)
			else if(nml_lev[i].net[j]->n_in==1 && nml_lev[i].net[j]->n_out!=0){
				nml_lev[i].net[j]->rep_net = NO;											//代表信号線
				nml_lev[i].net[j]->influence_net = nml_lev[i].net[j]->in[0]->influence_net;	//影響信号線は入力と同じ
			}

			//■POで1入力信号線
			else if(nml_lev[i].net[j]->n_in==1 && nml_lev[i].net[j]->n_out==0){
				nml_lev[i].net[j]->influence_net = nml_lev[i].net[j]->in[0]->influence_net;	//影響信号線は入力と同じ
			}

			//------------------------------------
			// 反転フラグ判定{YES, NO}
			//------------------------------------
			// ◆INVの時
			if(nml_lev[i].net[j]->type==INV){

				//入力が反転してない場合
				if(nml_lev[i].net[j]->in[0]->inv_flag == NO){
				nml_lev[i].net[j]->inv_flag = YES;	//反転
				}

				//入力反転してる場合
				else if(nml_lev[i].net[j]->in[0]->inv_flag == YES){
				nml_lev[i].net[j]->inv_flag = NO;	//反転の反転
				}
			}
			// ◆BUFかFOUTの時
			else if(nml_lev[i].net[j]->type==BUF || nml_lev[i].net[j]->type==FOUT){
				nml_lev[i].net[j]->inv_flag = nml_lev[i].net[j]->in[0]->inv_flag;	//入力の反転フラグを引き継ぐ
			}
			// ◆その他のゲート(AND, NAND, OR, NOR, EXOR, EXNOR)
			else{
				nml_lev[i].net[j]->inv_flag = NO;
			}

		}
	}
	
#ifdef REP_LEV_DEBUG
	printf("\n//===========================\n");
	printf("// 次イベントドリブン解析 DEBGU\n");
	printf("//===========================\n");
#endif

	//==========================================
	// 次イベントドリブン信号線解析
	//==========================================
	for(i=max_level-1; i>=0; i--){
		for(j=0; j<nml_lev[i].n_net; j++){

			if(nml_lev[i].net[j]->n_out != 0){	//PO以外が対象

#ifdef REP_LEV_DEBUG
				printf("i=%d j=%d 対象:%s ",i, j, nml_lev[i].net[j]->name);
#endif

				//----------------------------------------
				//●出力信号線が1本
				//----------------------------------------
				if(nml_lev[i].net[j]->n_out == 1){
					//----------------------------------------------------------
					//出力信号線が『代表信号線』
					//----------------------------------------------------------
					if(nml_lev[i].net[j]->out[0]->rep_net == YES){
#ifdef REP_LEV_DEBUG
						printf("次数:1 次：%s \n", nml_lev[i].net[j]->out[0]->name);
#endif
						nml_lev[i].net[j]->n_next_event = 1;							//次イベント数=1 (=出力信号線)
						nml_lev[i].net[j]->next_event = PNL_alloc(1);					//メモリ確保
						nml_lev[i].net[j]->next_event[0] = nml_lev[i].net[j]->out[0];	//ポインタ接続

					}
					
					//----------------------------------------------------------
					//出力信号線は『非代表信号線』 ⇒ out[0]->next_event全部
					//----------------------------------------------------------
					else{
#ifdef REP_LEV_DEBUG
						printf("次数:%d 次：", nml_lev[i].net[j]->out[0]->n_next_event);
#endif
						nml_lev[i].net[j]->n_next_event = nml_lev[i].net[j]->out[0]->n_next_event;				//次イベント数=出力信号線の次イベント数
						nml_lev[i].net[j]->next_event = PNL_alloc(nml_lev[i].net[j]->out[0]->n_next_event);		//メモリ確保
						//ポインタ接続
						for(k=0; k<nml_lev[i].net[j]->out[0]->n_next_event; k++){
#ifdef REP_LEV_DEBUG
							printf("次:%s", nml_lev[i].net[j]->out[0]->next_event[k]->name);
#endif
							nml_lev[i].net[j]->next_event[k] = nml_lev[i].net[j]->out[0]->next_event[k];	//次イベント信号線ID値を一時保存(出力信号線の次イベント信号線を格納)
						}
#ifdef REP_LEV_DEBUG
						printf("\n");
#endif
					}
				}


				//----------------------------------------
				//●出力信号線が2本以上
				//----------------------------------------
				else{
					//カウンタ初期化
					count = 0;

					//----------------------------------------------------------
					//out[0]の要素をtemp_idに全て代入
					//----------------------------------------------------------
					//out[0]が『代表信号線』
					if(nml_lev[i].net[j]->out[0]->rep_net == YES){
						temp_id[count] = nml_lev[i].net[j]->out[0]->n;	//out[0]のID値保存
						count++;										//次イベント数カウンタ更新
					}
					
					//out[0]は『非代表信号線』 ⇒ out[0]->next_event全部
					else{
						for(k=0; k<nml_lev[i].net[j]->out[0]->n_next_event; k++){
							temp_id[count] = nml_lev[i].net[j]->out[0]->next_event[k]->n;	//out[0]->next_eventのID値保存
							count++;										//次イベント数カウンタ更新
						}
					}
					
					//----------------------------------------------------------
					//残りの出力信号線の要素代入 (out[1]～)
					//----------------------------------------------------------
					for(k=1; k<nml_lev[i].net[j]->n_out; k++){
						//----------------------------
						//出力信号線が『代表信号線』
						//----------------------------
						if(nml_lev[i].net[j]->out[k]->rep_net == YES){
							//temp_id内に同じ要素が無いか探索
							for(m=0; m<count; m++){
								if(temp_id[m] == nml_lev[i].net[j]->out[k]->n){
									break;	//同じ要素があったらbreak
								}
							}
							//for文を回り切った場合temp_id内に無い要素
							if(m == count){
								temp_id[count] = nml_lev[i].net[j]->out[k]->n; //要素追加
								count++;										//次イベント数カウンタ更新
							}
						}
						
						//----------------------------
						//出力信号線が『非代表信号線』
						//----------------------------
						else{
							for(n=0; n<nml_lev[i].net[j]->out[k]->n_next_event; n++){	//out[k]->next_event全てを探索
								//temp_id内に同じ要素が無いか探索
								for(m=0; m<count; m++){
									if(temp_id[m] == nml_lev[i].net[j]->out[k]->next_event[n]->n){
										break;	//同じ要素があったらbreak
									}
								}
								//for文を回り切った場合temp_id内に無い要素
								if(m == count){
									temp_id[count] = nml_lev[i].net[j]->out[k]->next_event[n]->n; //要素追加
									count++;														//次イベント数カウンタ更新
								}
							}
						}
					}
					
					//----------------------------------------
					//メモリ確保
					//----------------------------------------
#ifdef REP_LEV_DEBUG
						printf("次数:%d 次：", count);
#endif
					nml_lev[i].net[j]->n_next_event = count;				//次イベント数を代入
					nml_lev[i].net[j]->next_event = PNL_alloc(count);	//メモリ確保

					//nml_lev[i].net[j]->next_event = (NLIST **)malloc(sizeof(NLIST *) * count);


					printf("メモリ確保完了\n");
					//----------------------------------------
					//次イベントドリブン信号線情報格納
					//----------------------------------------
					for(k=0; k<count; k++){
#ifdef REP_LEV_DEBUG
						printf("%s ", nl[ temp_id[k] ].name);
#endif
						nml_lev[i].net[j]->next_event[k] = &nl[ temp_id[k] ];
					}
#ifdef REP_LEV_DEBUG
					printf("\n");
#endif

				}

			}//POが対象のif文
		}
#ifdef REP_LEV_DEBUG
		printf("\n");
#endif
	}

	
	//==========================================
	// DEBUG
	//==========================================
#ifdef REP_LEV_DEBUG
	
	printf("\n//===========================\n");
	printf("// 代表信号線解析 DEBGU\n");
	printf("//===========================\n");

	for(i=0; i<max_level+1; i++){
		for(j=0; j<nml_lev[i].n_net; j++){
			//------------------------------------
			// 代表信号線
			//------------------------------------
			printf("%s 代表:", nml_lev[i].net[j]->name);

			if(nml_lev[i].net[j]->rep_net == YES){
				printf("YES ");
			}
			else if(nml_lev[i].net[j]->rep_net == NO){
				printf("NO ");
			}
			else{
				printf("ERROR !");
				//exit(-1);
			}
			
			//------------------------------------
			// 影響信号線
			//------------------------------------
			printf("影響:%s", nml_lev[i].net[j]->influence_net->name);

			//------------------------------------
			// 反転フラグ
			//------------------------------------
			printf("反転:");

			if(nml_lev[i].net[j]->inv_flag == YES){
				printf("YES ");
			}
			else if(nml_lev[i].net[j]->inv_flag == NO){
				printf("NO ");
			}
			else{
				printf("ERROR !");
				//exit(-1);
			}

			//------------------------------------
			// 次イベント信号線
			//------------------------------------
			printf("次:");
			for(k=0; k<nml_lev[i].net[j]->n_next_event; k++){
				printf("%s ", nml_lev[i].net[j]->next_event[k]->name);
			}

			printf("次数:%d\n",nml_lev[i].net[j]->n_next_event);
		}
		printf("\n");
	}
	printf("\n");
#endif


	return n_rep_lev_net;


}//END
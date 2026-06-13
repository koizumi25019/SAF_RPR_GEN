//------------------------------------------------------------------------
//File name : find_reachable_state.c
//Date : 2014/9/18
//Designer : H.Yamazaki
//Ver : 0.01
//memo:
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	"../Lib/mt19937ar.h"
#include	"./reachable_state.h"
#include	"../StandardHead.h"
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void lsim_3v_non_scan_single_gate	(S_NLIST*);
void lsim_3v_non_scan_inv			(S_NLIST*);
void lsim_3v_non_scan_and			(S_NLIST*);
void lsim_3v_non_scan_nand			(S_NLIST*);
void lsim_3v_non_scan_or			(S_NLIST*);
void lsim_3v_non_scan_nor			(S_NLIST*);
void lsim_3v_non_scan_exor			(S_NLIST*);
void lsim_3v_non_scan_exnor			(S_NLIST*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

//#define		DEBUG


#define		RUN_CYCLE	50			//動作サイクル数
#define		RUN_PATTERN	15			//32 * RUN_PATTERN数ぶんランダムパターン生成
//#define		MAX_STATE	1000000		//保存可能状態数(MAX100万状態)
#define		MAX_STATE	100000		//保存可能状態数(MAX10万状態)
#define		END_LIMIT	10000000		//X状態から2値状態探索用の最大動作サイクル数
#define		SKIP		3			//SKIP{0～RUN_PATTERN}
/*
#define		RUN_CYCLE	10
#define		RUN_PATTERN	100
#define		MAX_STATE	100		
#define		END_LIMIT	100
*/
//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------
//--------------------------------------------------------------------
//  関数名 : set_rand_32pattern
//  機  能 : PIにランダムパターンを32パターン設定
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------------------------
void	set_rand_32pattern(){

	int		j,k;
	int		rand;

	//------------------------------------------------
	//PIにランダムパターンを設定
	//------------------------------------------------
	for(j=0; j<n_spi; j++){

#ifdef DEBUG
		printf("\nPI: %s ", spi[j]->name);
#endif
		//32パターン分生成
		for(k=0; k<32; k++){
			//乱数生成
			rand = genrand_int31()%2;

			//0設定
			if(rand == 0){
				Set_NBit_Zero(spi[j]->nval_t1, k);
#ifdef DEBUG
				printf("0");
#endif
			}
			//1設定
			else if(rand == 1){
				Set_NBit_One(spi[j]->nval_t1, k);
#ifdef DEBUG
				printf("1");
#endif
			}
			//エラー表示
			else{
				printf("乱数生成エラー!!\n");
			}
		}
	}
}

//--------------------------------------------------------------------
//  関数名 : lsim_rand_32pattern
//  機  能 : 論理シミュレーション(1時刻目変数のみ利用して，1時刻分ごとに計算)
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------------------------
void	lsim_rand_32pattern(){

	int		j,k;
#ifdef DEBUG
	int		temp;
#endif

	for(j=1; j<max_level+1; j++){
		for(k=0; k<nml_slev[j].n_net; k++){
			if(nml_slev[j].net[k]->type != DFF){
				(*sfunc_t1_lsim[nml_slev[j].net[k]->type])(nml_slev[j].net[k]);
			}
		}
	}

#ifdef DEBUG
	printf("\nDEBUG: LSIM後 PPO状態値確認\n");
	for(k=0; k<32; k++){
		printf("rand_tp[%d] = ", k);
		for(j=0; j<n_ppo; j++){
			temp = Get_NBit(ppo[j]->nval_t1, k);

			if(temp == 0){
				printf("0");
			}
			else if(temp == 1){
				printf("1");
			}
			else if(temp == 3){
				printf("X");
			}
		}
		printf("\n");
	}
#endif

}

//--------------------------------------------------------------------
//  関数名 : take_over_ppo_value
//  機  能 : LSIM後のPPO値をPPIに引き継ぐ
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------------------------
void		take_over_ppo_value(){

	int			j;
	int			i,temp;

	for(j=0; j<n_ppo; j++){
		//			PPI								PPO
		ppo[j]->out[0]->nval_t1->x_buf[0] = ppo[j]->nval_t1->x_buf[0];
		ppo[j]->out[0]->nval_t1->p_buf[0] = ppo[j]->nval_t1->p_buf[0];
	}

	/*
	//DEBUG
	for(i=0; i<32; i++){
		for(j=0; j<n_ppo; j++){
			temp = Get_NBit(ppo[j]->out[0]->nval_t1, i);
			if(temp == 0){
				printf("0");
			}
			else if(temp == 1){
				printf("1");
			}
			else if(temp == 3){
				printf("X");
			}
		}
		printf("\n");
	}
	*/
}


//--------------------------------------------------------------------
//  関数名 : chek_overlap_state
//  機  能 : 引数番号番目のPPO状態が保存済み状態と重複しているかチェック
//  戻り値 : YES(重複している⇒保存しない), NO(重複してない⇒保存する)
//  引  数 : id(チェックする状態番号(0～32))
//--------------------------------------------------------------------
int		chek_overlap_state(int		id){

	int			i, j;
#ifdef DEBUG
	int			temp;
#endif
	
	//-------------------------------------------
	//重複状態チェック
	//-------------------------------------------
	for(i=0; i<n_r_state; i++){
		for(j=0; j<n_ppo; j++){
			//状態信号線値の不一致が1つでもあったらbreak
			if( Get_NBit(r_state[i].state, j) != Get_NBit(ppo[j]->nval_t1, id) ){
				break;
			}
		}

		//重複状態を発見
		if(j == n_ppo){
#ifdef DEBUG
			printf("重複状態 ");
			for(j=0; j<n_ppo; j++){

				temp = Get_NBit(ppo[j]->nval_t1, id);

				if(temp == 0){
					printf("0");
				}
				else if(temp == 1){
					printf("1");
				}
				else if(temp == 3){
					printf("X");
				}
			}
#endif
			return YES;
		}
	}

	//-------------------------------------------
	//全部ループし終わった⇒重複状態無し！
	//-------------------------------------------
	return NO;

}

//--------------------------------------------------------------------
//  関数名 : output_2v_state
//  機  能 : 同定した到達可能2値状態をファイル出力
//  戻り値 : なし
//  引  数 : st(状態出力用ファイルポインタ), id(出力するr_stateのID)
//--------------------------------------------------------------------
void	output_2v_state(FILE	*st, int	id){
		
	int			i,temp;

#ifdef DEBUG
	printf(" ファイル出力[%d]=", id);
#endif

	//===========================================================
	// PI値を乱数生成
	//===========================================================
	//PI値も出力
	for(i=0; i<n_spi; i++){

		//乱数生成
		temp = genrand_int31()%2;

		//状態値をファイル出力
		//0出力
		if(temp == 0){
			fprintf(st, "0");
#ifdef DEBUG
			printf("0");
#endif
		}
		
		//1出力
		else if(temp == 1){
			fprintf(st, "1");
#ifdef DEBUG
			printf("1");
#endif
		}
	}
	//===========================================================

	for(i=0; i<n_ppo; i++){

		//対象状態のPPI値を取得
		temp = Get_NBit(r_state[id].state, i);

		//状態値をファイル出力
		//0出力
		if(temp == 0){
			fprintf(st, "0");
#ifdef DEBUG
			printf("0");
#endif
		}
		
		//1出力
		else if(temp == 1){
			fprintf(st, "1");
#ifdef DEBUG
			printf("1");
#endif
		}
		
		//X出力
		else if(temp == 3){
			fprintf(st, "X");
			printf("\nWARNING!! : output_2v_state \n");
			printf("出力状態[%d]にXが混ざってるよ！\n", id);
		}
	}
	fprintf(st, "\n");
}

//--------------------------------------------------------------------
//  関数名 : check_ppo_value
//  機  能 : LSIM後のPPO値の確認
//  戻り値 : 2値状態を発見したか否か{YES：2値状態発見，NO：全部3値状態}
//  引  数 : st(状態出力用ファイルポインタ)
//--------------------------------------------------------------------
int		check_ppo_value(FILE	*st){

	int			i,j;
	int			temp;
	int			return_flag = NO;
	int			n_2v_state=0;			//32パターン中の何パターンが2値状態になったかカウント(0～32)
	
#ifdef DEBUG
	printf("\nLSIM後PPO状態の保存判定");
#endif
	
	//---------------------------------------------------
	// 32パターンLSIM後PPO状態の保存判定
	//---------------------------------------------------
	for(i=0; i<32; i++){
		
#ifdef DEBUG
		printf("\nrand_tp[%d]: ", i);
#endif
		//---------------------------------------------------
		//各PPOに対してXを含むか調査
		//---------------------------------------------------
		for(j=0; j<n_ppo; j++){
			if(Get_NBit(ppo[j]->nval_t1, i) == 3){
				break;
			}
		}
		
		//---------------------------------------------------
		//i番目パターンに対してはXを含まない状態だった！
		//---------------------------------------------------
		if(j == n_ppo){

			//2値状態数を更新
			n_2v_state++;

			//i番目状態値を保存済み状態との重複チェック
			if(chek_overlap_state(i) == NO){
				
#ifdef DEBUG
				printf("r_state[%d]=", n_r_state);
#endif
				//---------------------------------------
				//保存済み状態と重複していないため保存
				//---------------------------------------
				for(j=0; j<n_ppo; j++){

					//状態値取得
					temp = Get_NBit(ppo[j]->nval_t1, i);

					//状態保存
					//0保存
					if(temp == 0){
						Set_NBit_Zero(r_state[n_r_state].state, j);
#ifdef DEBUG
						printf("0");
#endif
					}
					//1保存
					else if(temp == 1){
						Set_NBit_One(r_state[n_r_state].state, j);
#ifdef DEBUG
						printf("1");
#endif
					}
					//その他
					else{
						printf("何かがおかしい・・・\n");
					}
				}

				//保存した状態をファイル出力
				output_2v_state(st, n_r_state);

				//保存した状態数更新
				n_r_state++;

				//戻り値フラグをYESに変更
				if(return_flag == NO){
					return_flag = YES;
				}
			}
		}
	}
	
	//---------------------------------------------------
	//32パターンの全てが2値状態になった場合探索打ち切り
	//---------------------------------------------------
	if(n_2v_state == 32){
		return_flag = YES;
#ifdef DEBUG
		printf("\n");
#endif
	}
	
	//---------------------------------------------------
	//戻り値フラグの値を返却
	//---------------------------------------------------
	return return_flag;
}


//--------------------------------------------------------------------
//  関数名 : set_initial_ff_value
//  機  能 : FF状態値の設定
//  戻り値 : なし
//  引  数 : state_id(初期化する状態番号)
//--------------------------------------------------------------------
void	set_initial_ff_value(int	state_id){

	int		j;
	int		temp;

	for(j=0; j<n_ppo; j++){

		//保存済み状態値取得
		temp = Get_NBit(r_state[state_id].state, j);

		//対象PPIをオールtemp値に初期化
		if(temp == 0){
#ifdef DEBUG
			printf("\nFF初期状態:%s  オール0", ppo[j]->out[0]->name);
#endif
			All_Bit_Zero(ppo[j]->out[0]->nval_t1);
		}
		else if(temp == 1){
#ifdef DEBUG
			printf("\nFF初期状態:%s  オール1", ppo[j]->out[0]->name);
#endif
			All_Bit_One(ppo[j]->out[0]->nval_t1);
		}
		else if(temp == 3){
#ifdef DEBUG
			printf("\nFF初期状態:%s  オールX", ppo[j]->out[0]->name);
#endif
			All_Bit_X(ppo[j]->out[0]->nval_t1);
		}
		else{
			printf("ERROR");
		}
	}
}

//--------------------------------------------------------------------
//  関数名 : save_new_reachable_state
//  機  能 : 2値状態からのLSIM後の新状態を保存
//  戻り値 : なし
//  引  数 : st(状態出力用ファイルポインタ)
//--------------------------------------------------------------------
void		save_new_reachable_state(FILE	*st){

	int			i,j;
	int			temp;
	
#ifdef DEBUG
	printf("\nLSIM後PPO状態の保存判定");
#endif
	
	//---------------------------------------------------
	// 32パターンLSIM後PPO状態の保存判定
	//---------------------------------------------------
	for(i=0; i<32; i++){
		
#ifdef DEBUG
		printf("\nrand_tp[%d]: ", i);
#endif
		//i番目状態値を保存済み状態との重複チェック
		if(chek_overlap_state(i) == NO){
				
#ifdef DEBUG
			printf("r_state[%d]=", n_r_state);
#endif
			//---------------------------------------
			//保存済み状態と重複していないため保存
			//---------------------------------------
			for(j=0; j<n_ppo; j++){

				//状態値取得
				temp = Get_NBit(ppo[j]->nval_t1, i);

				//状態保存
				//0保存
				if(temp == 0){
					Set_NBit_Zero(r_state[n_r_state].state, j);
#ifdef DEBUG
					printf("0");
#endif
				}
				//1保存
				else if(temp == 1){
					Set_NBit_One(r_state[n_r_state].state, j);
#ifdef DEBUG
					printf("1");
#endif
				}
				//その他
				else{
					printf("何かがおかしい・・・\n");
				}
			}

			//保存した状態をファイル出力
			output_2v_state(st, n_r_state);

			//保存した状態数更新
			n_r_state++;

			//状態保存数の上限に達したため終了
			if(n_r_state == MAX_STATE){
				break;
			}
		}
	}
}

//--------------------------------------------------------------------
//  関数名 : debug_check_ppi_ppo
//  機  能 : 【DEBUG用関数】PPIとPPO値の確認
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------------------------
void		debug_check_ppi_ppo(){

	int			i,j;
	int			temp;
	
	//PPIとPPO値の確認
	//for(i=0; i<32; i++){
	for(i=0; i<1; i++){
		
		printf("tp[%d] = \n", i);

		//---------------------------
		// PPI
		//---------------------------
		for(j=0; j<n_ppo; j++){
			temp = Get_NBit(ppo[j]->out[0]->nval_t1, i);
			
			if(temp == 0){
				printf("0");
			}
			else if(temp == 1){
				printf("1");
			}
			else if(temp == 3){
				printf("X");
			}
		}
		printf("\n");
		//---------------------------
		// PPO
		//---------------------------
		for(j=0; j<n_ppo; j++){
			temp = Get_NBit(ppo[j]->nval_t1, i);
			
			if(temp == 0){
				printf("0");
			}
			else if(temp == 1){
				printf("1");
			}
			else if(temp == 3){
				printf("X");
			}
		}
		printf("\n\n");
	}
}


//--------------------------------------------------------------------
//  関数名 : debug_check_ppi
//  機  能 : 【DEBUG用関数】PPI値の確認
//  戻り値 : なし
//  引  数 : なし
//--------------------------------------------------------------------
void		debug_check_ppi(){

	int			i,j;
	int			temp;
	
	//PPIとPPO値の確認
	//for(i=0; i<32; i++){
	for(i=0; i<1; i++){
		
		printf("tp[%d] = \n", i);

		//---------------------------
		// PPI
		//---------------------------
		for(j=0; j<n_ppo; j++){
			temp = Get_NBit(ppo[j]->out[0]->nval_t1, i);
			
			if(temp == 0){
				printf("0");
			}
			else if(temp == 1){
				printf("1");
			}
			else if(temp == 3){
				printf("X");
			}
		}
		printf("\n");		
	}
}
//--------------------------------------------------------------------
//関数名：find_reachable_state
//機能　：ノンスキャン回路に対してLSIMを行い到達可能状態(有効状態)を判定
//引数　：st(有効状態ファイル), pin(有効状態ファイルのPINファイル)
//戻り値：なし
//--------------------------------------------------------------------
void find_reachable_state(FILE *st, FILE *pin){

	int			i,j;
	int			count = UK;				//保存状態ループカウンタ

	int			end_flag;				//終了条件フラグ
	int			x_flag = NO;			//Xを含まない状態が発見されたかどうか{YES, NO}
	int			all_2v_flag;			//2値状態が出てきたか？{YES, NO}
	int			state_id=0;				//状態選択変数

	int			n_r_state_temp;			//n_r_stateの一時保存変数

#ifdef DEBUG
	int			temp;
#endif

	//==========================================================
	// 初期化
	//==========================================================
	//---------------------------------------------
	// 1時刻目関数ポインタセット
	//---------------------------------------------
	sfunc_t1_lsim[FOUT]		= lsim_3v_non_scan_single_gate;
	sfunc_t1_lsim[BUF]		= lsim_3v_non_scan_single_gate;
	sfunc_t1_lsim[INV]		= lsim_3v_non_scan_inv;
	sfunc_t1_lsim[AND]		= lsim_3v_non_scan_and;
	sfunc_t1_lsim[NAND]		= lsim_3v_non_scan_nand;
	sfunc_t1_lsim[OR]		= lsim_3v_non_scan_or;
	sfunc_t1_lsim[NOR]		= lsim_3v_non_scan_nor;
	sfunc_t1_lsim[EXOR]		= lsim_3v_non_scan_exor;
	sfunc_t1_lsim[EXNOR]	= lsim_3v_non_scan_exnor;
		
	//---------------------------------------------
	//正常値格納変数のメモリ確保
	//---------------------------------------------
	for(i=0; i<n_snet; i++)s_nl[i].nval_t1 = Alloc_Bit_TP(32);

	//---------------------------------------------
	// 到達可能状態数
	//---------------------------------------------
	//到達可能状態数初期化
	n_r_state = 0;	

	//とりあえずMAX_STATE状態分メモリ確保
	r_state = (RSTATE*)malloc(sizeof(RSTATE) * MAX_STATE);

	//状態保存構造体の初期化
	for(i=0; i<MAX_STATE; i++){
		r_state[i].state = Alloc_Bit_TP(n_ppi);		//状態保存先をメモリ確保
		//r_state[i].n_x = 0;							//状態に含まれるX数を初期化
		//r_state[i].cycle = UK;						//状態発見サイクル数
		//r_state[i].use = NO;						//初期状態としての使用状況{YES,NO}
	}
	

	//==========================================================
	// 到達可能状態のPIN情報をファイル出力
	//==========================================================
	//PINファイル出力
	for(i=0; i<n_ppi; i++)fprintf(pin, "%s\n", ppo[i]->out[0]->name);

	//ファイルポインタクローズ
	fclose(pin);

	
	//==========================================================
	// ■順序LSIM①■ オールケアビット状態を探索
	//==========================================================
	printf("\n//---------------------------------------\n");
	printf("//■順序LSIM①■ オールケアビット状態を探索\n");
	printf("//---------------------------------------\n");
	//32*50 = 1600パターン回LSIM
	for(i=0; i<50; i++){
				
		//------------------------------------------------
		//回数表示
		//------------------------------------------------
		printf("2値状態探索：%d / 50回目 (%d状態発見)\n", i+1, n_r_state);
		//printf("2値状態探索：%d / 50回目\r", i+1);

		//--------------------------------------------------
		//ループ終了条件の初期化(2値状態が出てきたか？)
		//--------------------------------------------------
		all_2v_flag = NO;
		
		//--------------------------------------------------
		//FFの初期値をオールXに初期化
		//--------------------------------------------------
		for(j=0; j<n_ppi; j++)All_Bit_X(ppi[j]->nval_t1);
		
		//--------------------------------------------------
		//終了条件カウンタ初期化
		//--------------------------------------------------
		end_flag = 0;

		//--------------------------------------------------
		//2値状態が発見されない間は+1時刻分LSIM
		//--------------------------------------------------
		while(all_2v_flag == NO){

			//終了条件
			if(end_flag > END_LIMIT){
				printf("%dサイクル間 2値状態が出なかったためbreak!!\n", end_flag);
				break;
			}

			//PIにランダムパターン印加
			set_rand_32pattern();
			
			//--------------------------
			//DEBUG
			//debug_check_ppi();
			//--------------------------


			//32パターン1サイクル分LSIM
			lsim_rand_32pattern();

			
			//--------------------------
			//DEBUG
			//debug_check_ppi_ppo();
			//--------------------------


			//オール2値状態が出たかPPO値確認(2値状態が出た場合は保存)
			all_2v_flag = check_ppo_value(st);

			//オール2値状態が出てない場合はPPOをPPIに代入⇒次ループ時に再LSIM
			if(all_2v_flag == NO){
				//PPO値をPPIに引継ぎ
				take_over_ppo_value();
			}

			//printf("%d回目\n", end_flag);

			//終了条件カウンタ更新
			end_flag++;
		}

	}
	
	//==========================================================
	// ■順序LSIM②■ 保存済み状態から新状態探索
	//==========================================================
	printf("\n\n//---------------------------------------\n");
	printf("//■順序LSIM②■ 保存済み状態から新状態探索\n");
	printf("//---------------------------------------\n");
	//初期化
	end_flag = END_LIMIT;
	printf("\n\n");


#ifdef DEBUG
	printf("DEBUG: 保存済み状態確認\n");
	for(i=0; i<n_r_state; i++){
		printf("r_state[%d]: ", i);
		for(j=0; j<n_ppo; j++){
			temp = Get_NBit(r_state[i].state, j);

			if(temp == 0){
				printf("0");
			}
			else if(temp == 1){
				printf("1");
			}
			else if(temp == 3){
				printf("X");
			}
		}
		printf("\n");
	}
#endif


	//探索開始
	while(n_r_state != MAX_STATE){
		
		//------------------------------------------------
		//回数表示
		//------------------------------------------------
		printf("新状態探索：%d / %d回目 (%d状態発見)\n", state_id+1, n_r_state, n_r_state);
		//printf("新状態探索：%d / %d回目 (%d状態発見)\r", state_id+1, n_r_state, n_r_state);
		
		//------------------------------------------------
		//終了条件1 : 保存済み状態数を全て使い切った
		//------------------------------------------------
		if(state_id == n_r_state){
			printf("\n保存済み状態数を全て使い切ったため終了\n");
			break;
		}
		
		//------------------------------------------------
		//スキップ判定変数の初期化
		//------------------------------------------------
		n_r_state_temp = n_r_state;

		//------------------------------------------------
		//state_id番目の保存済み状態に対して新状態探索
		//------------------------------------------------
		//32*RUN_PATTERNパターン数分をRUN_CYCLE回LSIM
		for(i=0; i<RUN_PATTERN; i++){
			
			//SKIP回数新状態が出現しない場合はこの情宝の探索を打ち切り
			if(n_r_state==n_r_state_temp && i==SKIP){
				break;
			}

			//32パターン分RUN_CYCLE回LSIM
			for(j=0; j<RUN_CYCLE; j++){


				//表示
				//printf("%dパターン目の%dサイクル回目LSIM\n", i, j);


				//PIにランダムパターン印加
				set_rand_32pattern();
				
				//1ループ目：PPIをstate_id番目の保存済み状態に初期化
				if(j==0){
					set_initial_ff_value(state_id);
				}
				//2ループ目以降：PPIに前時刻状態値の設定
				else{
					take_over_ppo_value();
				}

				//32パターン1サイクル分LSIM
				lsim_rand_32pattern();

				//新状態が出たら保存&ファイル出力
				save_new_reachable_state(st);				
		
				//状態保存数の上限に達したため終了
				if(n_r_state == MAX_STATE){
					break;
				}
			}

			//状態保存数の上限に達したため終了
			if(n_r_state == MAX_STATE){
				break;
			}
		}
		
		//------------------------------------------------
		//次状態に状態カウンタを更新
		//------------------------------------------------
		state_id++;
	}


	printf("\n\n//---------------------------------------\n");
	printf("// Report\n");
	printf("//---------------------------------------\n");
	printf("FF数                    :  %d\n", n_sdff);
	printf("最大到達可能状態数      :  %d (2^%d)\n", (int)pow(2.0,(double)n_sdff), n_sdff);
	printf("発見した到達可能状態数  :  %d\n", n_r_state);
	printf("未発見到達可能状態数    :  %d\n", ((int)pow(2.0,(double)n_sdff) - n_r_state));
}


//*************************************************************************************************

//----------------------------------------------
//  関数名 : lsim_3v_non_scan_single_gate
//  機  能 : BUF・FOUT・DFF論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_3v_non_scan_single_gate(S_NLIST *net){

	unsigned int	i;

	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->x_buf[i];
		net->nval_t1->p_buf[i] = net->in[0]->nval_t1->p_buf[i];
	}

}

//----------------------------------------------
//  関数名 : lsim_3v_non_scan_inv
//  機  能 : INV論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_3v_non_scan_inv(S_NLIST *net){
	
	unsigned int	i;

	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->p_buf[i];
		net->nval_t1->p_buf[i] = net->in[0]->nval_t1->x_buf[i];
	}
}


//----------------------------------------------
//  関数名 : lsim_3v_non_scan_and
//  機  能 : AND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_3v_non_scan_and(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->x_buf[i];
		net->nval_t1->p_buf[i] = net->in[0]->nval_t1->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] &= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をAND演算
			net->nval_t1->p_buf[j] |= net->in[i]->nval_t1->p_buf[j];	//p_buf同士をOR演算
		}
	}

}


//----------------------------------------------
//  関数名 : lsim_3v_non_scan_nand
//  機  能 : NAND論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_3v_non_scan_nand(S_NLIST *net){

	unsigned int		i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ANDゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(i=0; i<net->nval_t1->int_num; i++){
		net->nval_t1->x_buf[i] = net->in[0]->nval_t1->x_buf[i];
		net->nval_t1->p_buf[i] = net->in[0]->nval_t1->p_buf[i];
	}

	//入力線数分演算
	for(i=1; i<(unsigned int)net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] &= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をAND演算
			net->nval_t1->p_buf[j] |= net->in[i]->nval_t1->p_buf[j];	//p_buf同士をOR演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval_t1->int_num; j++){
		temp = net->nval_t1->x_buf[j];
		net->nval_t1->x_buf[j] = net->nval_t1->p_buf[j];	//x_bufにp_bufを代入
		net->nval_t1->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}

}

//----------------------------------------------
//  関数名 : lsim_3v_non_scan_or
//  機  能 : OR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_3v_non_scan_or(S_NLIST *net){

	int					i;
	unsigned int		j;

	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = net->in[0]->nval_t1->x_buf[j];
		net->nval_t1->p_buf[j] = net->in[0]->nval_t1->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] |= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をOR演算
			net->nval_t1->p_buf[j] &= net->in[i]->nval_t1->p_buf[j];	//p_buf同士をAND演算
		}
	}
}


//----------------------------------------------
//  関数名 : lsim_3v_non_scan_nor
//  機  能 : NOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
//----------------------------------------------
void lsim_3v_non_scan_nor(S_NLIST *net){

	int					i;
	unsigned int		j;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用

	//-------------------------------------
	// ORゲート演算
	//-------------------------------------
	//初期値代入 (in[0]の値)
	for(j=0; j<net->nval_t1->int_num; j++){
		net->nval_t1->x_buf[j] = net->in[0]->nval_t1->x_buf[j];
		net->nval_t1->p_buf[j] = net->in[0]->nval_t1->p_buf[j];
	}

	//入力線数分演算
	for(i=1; i<net->n_in; i++){
		for(j=0; j<net->nval_t1->int_num; j++){
			net->nval_t1->x_buf[j] |= net->in[i]->nval_t1->x_buf[j];	//x_buf同士をOR演算
			net->nval_t1->p_buf[j] &= net->in[i]->nval_t1->p_buf[j];	//p_buf同士をAND演算
		}
	}

	//-------------------------------------
	// NOT演算処理(x_bufとp_bufを入れ替える)
	//-------------------------------------
	for(j=0; j<net->nval_t1->int_num; j++){
		temp = net->nval_t1->x_buf[j];
		net->nval_t1->x_buf[j] = net->nval_t1->p_buf[j];	//x_bufにp_bufを代入
		net->nval_t1->p_buf[j] = temp;					//p_bufにx_buf(temp)を代入
	}
}

//----------------------------------------------
//  関数名 : lsim_3v_non_scan_exor
//  機  能 : EXOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXORのみ対応！
//----------------------------------------------
void lsim_3v_non_scan_exor(S_NLIST *net){
	
	unsigned int		i;
	
	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->nval_t1->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->nval_t1->x_buf[i] = (net->in[0]->nval_t1->x_buf[i]&net->in[1]->nval_t1->p_buf[i]) | (net->in[0]->nval_t1->p_buf[i]&net->in[1]->nval_t1->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->nval_t1->p_buf[i] = (net->in[0]->nval_t1->p_buf[i]|net->in[1]->nval_t1->x_buf[i]) & (net->in[0]->nval_t1->x_buf[i]|net->in[1]->nval_t1->p_buf[i]);
	}


}//END


//----------------------------------------------
//  関数名 : lsim_3v_non_scan_exnor
//  機  能 : EXNOR論理演算(1時刻目順序回路)
//  戻り値 : 演算対象信号線
//  引  数 : なし
// ※2入力EXNORのみ対応！
//----------------------------------------------
void lsim_3v_non_scan_exnor(S_NLIST *net){
	
	unsigned int		i;
	unsigned int		temp;	//x_bufとp_bufを入れ替える時に使用
	

	//===============================================
	// BIT_TPのint確保分ループ
	//===============================================
	for(i=0; i<net->nval_t1->int_num; i++){

		//                        (    A(x-buf)          &            B(p-buf)    )   |      (      A(p-buf)       &             B(x-buf)    )
		net->nval_t1->x_buf[i] = (net->in[0]->nval_t1->x_buf[i]&net->in[1]->nval_t1->p_buf[i]) | (net->in[0]->nval_t1->p_buf[i]&net->in[1]->nval_t1->x_buf[i]);
		
		//                        (    A(p-buf)          |            B(x-buf)    )   &      (      A(x-buf)       |             B(p-buf)    )
		net->nval_t1->p_buf[i] = (net->in[0]->nval_t1->p_buf[i]|net->in[1]->nval_t1->x_buf[i]) & (net->in[0]->nval_t1->x_buf[i]|net->in[1]->nval_t1->p_buf[i]);


		//-------------------------------------
		// NOT演算処理(x_bufとp_bufを入れ替える)
		//-------------------------------------
		temp = net->nval_t1->x_buf[i];
		net->nval_t1->x_buf[i] = net->nval_t1->p_buf[i];	//x_bufにp_bufを代入
		net->nval_t1->p_buf[i] = temp;					//p_bufにx_buf(temp)を代入
	}

}//END
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <string.h>
#include <direct.h>
#include <gmp.h>

#include "../../include/cudd.h"
#include "./createSGmodel.h"
#include "./asg.h"
#include "./init.h"
#include "./read.h"
#include "./fsim.h"
#include "./opb/opb.h"
#include "./opb/clasp/clasp.h"
#include "../standard.h"

#include"./MakeBlockingClause.h"

//プロトタイプ宣言
void RunBDD(DdManager* gbm,int nvars, int* pattern_list, int list_size, FILE* result_fp, mpf_t* total_prob_sums);

// 定数
#define MAX_PATTERN_CASES 100

//*************************************************************************************************************
//	@name		：　AnalyzeFaultDensity
//	@function	：	analyze the fault density
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	void
)
{
	TARGET  remain;
	TARGET	target;
	FILE* fprpr = (FILE*)NULL;
	FILE* bdd_result = (FILE*)NULL;
	FILE* cube_file = (FILE*)NULL;
	int loop = 0;
	int temp_numrema;
	int count = 0;

	// 回路における故障検出確率計算用のmpf_t の配列を用意
	mpf_t total_prob_sums[100];

	//CUDDの初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

	//SHIFTアルゴリズムを有効化
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	// 初期化
	for (int i = 0; i < 100; i++) {
		mpf_init(total_prob_sums[i]);    // メモリ確保
		mpf_set_ui(total_prob_sums[i], 0); // 0で初期化
	}

	//BDD実験結果ファイルオープン
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//BDD実験ファイル記述
	fprintf(bdd_result, "name,type,cube,rel,var,den");

	// opt構造体のデータを使ってループ
	for (int i = 0; i < opt.file.input.list_size; i++) {
		fprintf(bdd_result, ",n=%d", opt.file.input.pattern_num_list[i]);
	}
	fprintf(bdd_result, "\n");
	fclose(bdd_result);


	//変数初期化
	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

	//ファイル読み込み
	if (ReadFile() != READ_OKAY) return AFD_ERROR;

	//正常回路制約式生成
	if (CreateConsGC() != TPG_MODEL_OKAY) return AFD_ERROR;
	
	while (readdata.fault.numrema != 0)
	{
		//テストキューブファイルオープン
		fileOpen(&cube_file, "./tools/bdd/bdd_cube_file.txt", "w");

		//BDD実験結果ファイルオープン
		fileOpen(&bdd_result, opt.file.output.result, "a");

		count++;
		temp_numrema = readdata.fault.numrema;

		//故障リスト読み込み
		SetTarget(&remain, &target, loop++);

		//テスト生成モデル構築
		if (WriteTPGModel(&target) != W_TPG_MODEL_OKAY) return AFD_ERROR;

		//テスト生成回数初期化
		int test_loop = 0;

		//故障名ファイル出力
		fprintf(bdd_result, "%s,", target.list[0]->name);

		//故障タイプ出力
		if (target.list[0]->type == SF0)
		{
			fprintf(bdd_result, "sa0,");
		}
		else{
			fprintf(bdd_result, "sa1,");
		}

		//UNSATになるか，一定のテスト生成回数に達するまで繰り返す
		while (1) {
			// SATソルバ実行
			// 結果がUNSAT(解なし) -> 探索終了
			if (CLASP() != CLASP_OKAY) {

				//キューブ数出力
				fprintf(bdd_result, "%d,", test_loop);

				//テストに関係する外部入力数出力
				fprintf(bdd_result, "%d,", target.list[0]->test_relation_num);

				//テストキューブファイルクローズ	
				fclose(cube_file);

				//BDDによる真理値表密度計算
				RunBDD(
					gbm,                               // CUDDマネージャポインタ
					n_pi,                              // 変数数
					opt.file.input.pattern_num_list,   // ランダムパターン数リスト
					opt.file.input.list_size,          // リストのサイズ(個数)
				    bdd_result,                        // 結果ファイルポインタ
					total_prob_sums                    // 確率和配列
				);

				//BDD実験結果ファイルクローズ	
				fclose(bdd_result);

				//未検出故障リストから削除
				DropDeteFault(&target);

				//メモリ開放
				FreeMemory(&remain, &target);

				break;

			}
			// SAT(解あり) -> テスト生成継続
			else {
				printf("Progress >> %d/%d\n", count,readdata.fault.numinit);
				printf("SAT test generation count:%d\n", test_loop);
				
				//テスト生成回数が100回になったら打ち切り
				if (test_loop == opt.file.input.limit) {

					//キューブ数出力
					fprintf(bdd_result, "%d,", test_loop);

					//テストに関係する外部入力数出力
					fprintf(bdd_result, "%d,", target.list[0]->test_relation_num);

					//テストキューブファイルクローズ
					fclose(cube_file);

					//BDDによる真理値表密度計算
					RunBDD(
						gbm,                               // CUDDマネージャポインタ
						n_pi,                              // 変数数
						opt.file.input.pattern_num_list,   // ランダムパターン数リスト
						opt.file.input.list_size,          // リストのサイズ(個数)
						bdd_result,                        // 結果ファイルポインタ
						total_prob_sums                    // 確率和配列
					);

					//BDD実験結果ファイルクローズ	
					fclose(bdd_result);

					//未検出故障リストから削除
					DropDeteFault(&target);

					//メモリ開放
					FreeMemory(&remain, &target);

					break;
				}

				//故障に対するテスト生成回数
				test_loop++;

				//XID用テストパターン出力
				OutSolution(&target);

				//ドントケア判定
				CALL_XID_SAF(opt.file.input.net, opt.file.output.pin);

				//コンソールアプリケーションの初期化
				system("cls");

				//禁止節設定
				char* x_pattern = make_blocking_clause(&target);

				//テストキューブをファイルに書き込む
				fprintf(cube_file, "%s\n", x_pattern);

				free(x_pattern);
			}
		}
	}

	// 回路全体の故障検出率出力
	fileOpen(&bdd_result, opt.file.output.result, "a");

	fprintf(bdd_result, "\n");
	fprintf(bdd_result, "circuit fault coverage,,,,");

	// 平均計算用のGMP変数準備
	mpf_t average_val, total_faults_mpf;
	mpf_init(average_val);
	mpf_init(total_faults_mpf);

	mpf_set_ui(total_faults_mpf, readdata.fault.numinit);

	// 各ランダムパターン数ごとの平均計算
	for (int i = 0; i < opt.file.input.list_size; i++) {
		// 平均 = 合計 / 全故障数
		mpf_div(average_val, total_prob_sums[i], total_faults_mpf);

		fprintf(bdd_result, ",");
		// "%.6Ff" で小数点以下6桁まで出力
		gmp_fprintf(bdd_result, "%.10Fe", average_val);
	}
	fprintf(bdd_result, "\n");

	fclose(bdd_result);

	// --- メモリ解放 (後始末) ---
	mpf_clear(average_val);
	mpf_clear(total_faults_mpf);

	for (int i = 0; i < MAX_PATTERN_CASES; i++) {
		mpf_clear(total_prob_sums[i]);
	}

	return AFD_OKAY;
}

//*************************************************************************************************************
//	@name		：　OutSolution
//	@function	：	output the solution
//	@return		：	(bool) okay, error
//*************************************************************************************************************
void OutSolution(
	TARGET* target		  /**< target fault */
)
{

	/** for xid */
	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, "./tools/fsim/test.txt", "w");
	fprintf(fileptr, "%s\n", clasp.sol[SOL_TP]);
	fclose(fileptr);

	/** for xid  */
	FILE* filexid = (FILE*)NULL;
	fileOpen(&filexid, "./tools/fsim/xid_fault.txt", "w");
	if (target->list[0]->type == SF0) {
		fprintf(filexid, "SF0 %s\n",target->list[0]->name);
	}
	else {
		fprintf(filexid, "SF1 %s\n", target->list[0]->name);
	}

	fclose(filexid);
	

	return;
}

//*************************************************************************************************************
//	@name		：　FreeMemory
//	@function	：	free the memory
//	@return		：	(void)
//*************************************************************************************************************
void FreeMemory(
	TARGET* remain,			  /**< remain fault */
	TARGET* target			  /**< target fault */
)
{
	/** free the target fault lists */
	free(remain->list);
	free(target->list);

	/** free the faulty-circuit constraints  */
	for (int i = 0; i < n_net; i++)
	{
		for (int j = 0; j < target->num; j++)
		{
			if (nl[i].consfc[j] != NULL)
			{
				free(nl[i].consfc[j]);

			}
		}

		free(nl[i].consfc);

	}
	return;
}







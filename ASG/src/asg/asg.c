//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <string.h>
#include <direct.h>

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
void bdd(int test_relation);

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


	//BDD実験結果ファイルオープン
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//BDD実験ファイル記述
	fprintf(bdd_result, "fault name,cube num,test PI,BDD Var num,density\n");
	fclose(bdd_result);


	//変数初期化
	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

	/** read the file */
	if (ReadFile() != READ_OKAY) return AFD_ERROR;

	/** create the constraint for good-circuit */
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
		int test_loop = 1;

		//故障名ファイル出力
		fprintf(bdd_result, "%s,", target.list[0]->name);

		//UNSATになるか，一定のテスト生成回数に達するまで繰り返す
		while (1) {
			if (CLASP() != CLASP_OKAY) {

				//キューブ数出力
				fprintf(bdd_result, "%d,", test_loop);

				//テストに関係する外部入力数出力
				fprintf(bdd_result, "%d,", target.list[0]->test_relation_num);

				//未検出故障リストから削除
				DropDeteFault(&target);

				//メモリ開放
				FreeMemory(&remain, &target);

				//テストキューブファイルクローズ	
				fclose(cube_file);

				//BDD実験結果ファイルクローズ	
				fclose(bdd_result);

				//BDDによる真理値表密度計算
				bdd(target.list[0]->test_relation_num);

				break;

			}
			//解がまだ存在
			else {
				printf("Progress >> %d/%d\n", count,readdata.fault.numinit);
				printf("SAT test generation count:%d\n", test_loop);
				
				//テスト生成回数が100回を超えたら打ち切り
				if (test_loop >= 100) {

					//キューブ数出力
					fprintf(bdd_result, "%d,", test_loop);

					//テストに関係する外部入力数出力
					fprintf(bdd_result, "%d,", target.list[0]->test_relation_num);

					//テストキューブファイルクローズ	
					fclose(cube_file);

					//BDD実験結果ファイルクローズ	
					fclose(bdd_result);

					//BDDによる真理値表密度計算
					bdd(target.list[0]->test_relation_num);

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
			}
		}
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

	fclose(fileptr);
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

		return;
	}
}







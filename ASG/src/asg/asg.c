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
void bdd(void);

//*************************************************************************************************************
//	@name		：　SAF RPRF Gen
//	@function	：	generate saf rpr fault set
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool ASG(
	void
)
{
	TARGET  remain;
	TARGET	target;
	SORTED  sorted;
	FILE* fprpr = (FILE*)NULL;
	int loop = 0;
	int temp_numrema;
	int count = 0;
	int rpr_num = 0;

	//BDD実験結果ファイルオープン
	FILE* bdd_result = fopen("./tools/bdd/bdd_results.csv", "w");
	if (bdd_result == NULL) {
		perror("エラー: ファイルを開けません");
		return 1;
	}
	fclose(bdd_result);


	//RPR故障リストファイルオープン
	fileOpen(&fprpr, opt.file.output.rpr, "w");

	//真理値表密度ファイル初期化
	FILE* density_file = fopen("./tools/bdd/density_file.txt", "w");
	if (density_file == NULL) {
		perror("エラー: ファイルを開けません");
		return 1;
	}
	fclose(density_file);

	//SATテスト生成回数ファイルオープン
	FILE* sat_count = fopen("./tools/clasp/sat_count.txt", "w");
	if (sat_count == NULL) {
		perror("エラー: ファイルを開けません");
		return 1;
	}

	//変数初期化
	if (InitGlobalVars() != INIT_OKAY) return ASG_ERROR;

	/** read the file */
	if (ReadFile() != READ_OKAY) return ASG_ERROR;

	/** create the constraint for good-circuit */
	if (CreateConsGC() != TPG_MODEL_OKAY) return ASG_ERROR;

	remain_log[0] = readdata.fault.numrema;

	while (readdata.fault.numrema != 0)
	{
		//テストキューブファイルオープン
		FILE* cube_file = fopen("./tools/bdd/bdd_cube_file.txt", "w");
		if (cube_file == NULL) {
			perror("エラー: ファイルを開けません");
			return 1;
		}

		count++;
		temp_numrema = readdata.fault.numrema;

		//故障リスト読み込み
		SetTarget(&remain, &target, &sorted, loop++);

		//テスト生成モデル構築
		if (CreateSGmodel(&target) != SG_MODEL_OKAY) return ASG_ERROR;

		//テスト生成回数初期化
		int test_loop = 1;

		//UNSATになるか，一定のテスト生成回数に達するまで繰り返す
		while (1) {
			if (CLASP() != CLASP_OKAY) {

                //UNSATの場合
				rpr_num++;
				
				//故障検出パターン数ファイル出力
				if (target.list[0]->type == SF0) {
					fprintf(fprpr, "%s sa0\n", target.list[0]->name);
				}
				else {
					fprintf(fprpr, "%s sa1\n", target.list[0]->name);
				}

				//真理値表密度ファイル記述　故障名
				fprintf(density_file, "%s ", target.list[0]->name);

				//テスト生成回数(テストキューブ数)
				fprintf(sat_count, "%d\n", test_loop);

				//未検出故障リストから削除
				DropDeteFault(&target);
				//メモリ開放
				FreeMemory(&remain, &target);
				remain_log[loop] = readdata.fault.numrema + readdata.fault.numred;
				detect_log[loop - 1] = temp_numrema - readdata.fault.numrema;


				fclose(cube_file);
				//BDDによる真理値表密度計算
				bdd();

				break;

			}
			//解がまだ存在
			else {
				printf("Progress:%d/%d\n", count,readdata.fault.numinit);
				printf("SAT test generation count:%d\n", test_loop);
				printf("RPR fault count:%d\n", rpr_num);
				
				//テスト生成回数が100回を超えたら打ち切り
				if (test_loop >= 100) {

					//BDDによる真理値表密度計算
					bdd();

					//未検出故障リストから削除
					DropDeteFault(&target);
					//メモリ開放
					FreeMemory(&remain, &target);
					remain_log[loop] = readdata.fault.numrema + readdata.fault.numred;
					detect_log[loop - 1] = temp_numrema - readdata.fault.numrema;

					break;
				}

				//故障に対するテスト生成回数
				test_loop++;

				/** output the solution */
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

	fclose(sat_count);
	fclose(density_file);

	return ASG_OKAY;
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

	/** for fault simulation */
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







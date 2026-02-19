//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cudd.h>
#include <gmp.h>

#include "ccadical.h"
#include "./createSGmodel.h"
#include "./asg.h"
#include "./init.h"
#include "./read.h"
#include "./opb/opb.h"
#include "./opb/clasp/cadical.h"
#include"./MakeBlockingClause.h"

//prototype declaration
void RunBDD(
	DdManager* gbm,
	int nvars, 
	int* pattern_list, 
	int list_size, 
	FILE* result_fp, 
	mpf_t* total_prob_sums
);
void CallXidSaf(
	const char* net_file, 
	const char* pin_file
);
bool DropDeteFault(
	TARGET * target	
);

#define MAX_PATTERN_CASES 100

//-----------------------------------------------------------------------------
// ソルバの解から tp.txt (XID入力用) を作成する関数
//-----------------------------------------------------------------------------
void GenerateTpAndFile(CCaDiCaL *solver, const char* filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;

for (int i = 0; i < n_pi; i++) {
        
        // 1. そのピンの変数番号を取得
        int var = pi[i]->varsgc; 
        
        // 2. SATソルバから値を取得
        int val = ccadical_val(solver, var);
        char bit_char = (val > 0) ? '1' : '0';

        // ファイル書き込み
        fprintf(fp, "%c", bit_char);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

//*************************************************************************************************************
//	@name		�F�@AnalyzeFaultDetectionProbability
//	@function	F	analyze the fault detection probability
//	@return		F	(bool) okay, error
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

	// fault detection probability calculation array
	mpf_t total_prob_sums[100];

	//CUDD initialization
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

	//shifting algorithm
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	// initialization
	for (int i = 0; i < 100; i++) {
		mpf_init(total_prob_sums[i]);    // initialize
	    mpf_set_ui(total_prob_sums[i], 0); // set to 0
	}

	//result file open
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//BDD result file header
	fprintf(bdd_result, "name,type,cube,rel,var,den");

	// opt file input pattern numbers
	for (int i = 0; i < opt.file.input.list_size; i++) {
		fprintf(bdd_result, ",n=%d", opt.file.input.pattern_num_list[i]);
	}
	fprintf(bdd_result, "\n");
	fclose(bdd_result);


	//initialize global variables
	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

	//read the fault file
	if (ReadFile() != READ_OKAY) return AFD_ERROR;

	//create Good Circuit constraints
	if (CreateConsGC() != TPG_MODEL_OKAY) return AFD_ERROR;

	
	while (readdata.fault.numrema != 0)
	{

		// ソルバの初期化 
        CCaDiCaL *solver = ccadical_init();
		
		// 変数の未宣言エラーを回避するために factor オプションを無効化します
        ccadical_set_option(solver, "factor", 0);

		//open cube file
		fileOpen(&cube_file, "./bdd_cube_file.txt", "w");

		//open BDD result file
		fileOpen(&bdd_result, opt.file.output.result, "a");

		count++;
		temp_numrema = readdata.fault.numrema;

		//fault list set
		SetTarget(&remain, &target, loop++);

		//write TPG model
		if (WriteTPGModel(solver,&target) != W_TPG_MODEL_OKAY) return AFD_ERROR;

		//test generation loop count
		int test_loop = 0;

		//fault name output
		fprintf(bdd_result, "%s,", target.list[0]->name);

		//fault type output
		if (target.list[0]->type == SF0)
		{
			fprintf(bdd_result, "sa0,");
		}
		else{
			fprintf(bdd_result, "sa1,");
		}

		//UNSAT判定時のテスト生成終了判定
		while (1) {
			// SAT判定時
			// それ以外はUNSAT(存在しない) -> テスト終了
            int res = ccadical_solve(solver); // 10:SAT, 20:UNSAT

			//if(RunCaDiCaL() != true) {
			if(res == 20) { // 20:UNSAT

				//test generation count output
				fprintf(bdd_result, "%d,", test_loop);

				//close cube file
				fclose(cube_file);

				//BDD running
				RunBDD(
					gbm,                               // CUDD
					n_pi,                              // 
					opt.file.input.pattern_num_list,   // 
					opt.file.input.list_size,          // 
				    bdd_result,                        // 
					total_prob_sums                    //
				);

				//BDD result file close
				fclose(bdd_result);

				//detected fault list deletion
				DropDeteFault(&target);

				//free memory
				FreeMemory(&remain, &target);

				break;

			}
			// SAT-> テスト生成続行
			else {
				printf("Progress >> %d/%d\n", count,readdata.fault.numinit);
				printf("SAT test generation count:%d\n", test_loop);
				
				//test generation limit reached
				if (test_loop == opt.file.input.limit) {

					//test generation count output
					fprintf(bdd_result, "%d,", test_loop);

					//close cube file
					fclose(cube_file);

					//BDD running
					RunBDD(
						gbm,                               // CUDD
						n_pi,                              // 
						opt.file.input.pattern_num_list,   // 
						opt.file.input.list_size,          // 
						bdd_result,                        // 
						total_prob_sums                    // 
					);

					//BDD result file close
					fclose(bdd_result);

					//detected fault list deletion
					DropDeteFault(&target);

					//free memory
					FreeMemory(&remain, &target);

					break;
				}

				//test generation count increment
				test_loop++;

				//generate test pattern and output to file
				GenerateTpAndFile(solver, "./tp.txt");

				//output the xid test pattern
				OutSolution(&target);

				//dont care identification
				CallXidSaf(
					opt.file.input.net, 
					opt.file.output.pin
				);

				//generate blocking clause 
				char* x_pattern = make_blocking_clause(solver,&target);

				//output the blocking clause
				fprintf(cube_file, "%s\n", x_pattern);

				free(x_pattern);
			}
		}
		ccadical_release(solver);
	}

	// result file open
	fileOpen(&bdd_result, opt.file.output.result, "a");

	fprintf(bdd_result, "\n");
	fprintf(bdd_result, "circuit fault coverage,,,,");

	// GMP variables for average calculation
	mpf_t average_val, total_faults_mpf;
	mpf_init(average_val);
	mpf_init(total_faults_mpf);

	mpf_set_ui(total_faults_mpf, readdata.fault.numinit);

	// GMP variables for average calculation
	for (int i = 0; i < opt.file.input.list_size; i++) {
		// average = sum / total
		mpf_div(average_val, total_prob_sums[i], total_faults_mpf);

		fprintf(bdd_result, ",");
		gmp_fprintf(bdd_result, "%.10Fe", average_val);
	}
	fprintf(bdd_result, "\n");

	fclose(bdd_result);

	// CUDD quit
	mpf_clear(average_val);
	mpf_clear(total_faults_mpf);

	for (int i = 0; i < MAX_PATTERN_CASES; i++) {
		mpf_clear(total_prob_sums[i]);
	}

	return AFD_OKAY;
}

//*************************************************************************************************************
//	@name		@OutSolution
//	@function	output the solution
//	@return	    (bool) okay, error
//*************************************************************************************************************
void OutSolution(
	TARGET* target		  /**< target fault */
)
{

	/** for xid  */
FILE* filexid = fopen("./xid_fault.txt", "w");
if (filexid == NULL) {
    fprintf(stderr, "【ERROR】: Cannot open ./xid_fault.txt for writing.\n");
    exit(EXIT_FAILURE); // もしくは return false; など適切なエラー処理
}
	if (target->list[0]->type == SF0) {
		fprintf(filexid, "SF0 %s\n",target->list[0]->name);
		//printf("SF0 %s\n", target->list[0]->name);
	}
	else {
		fprintf(filexid, "SF1 %s\n", target->list[0]->name);
		//printf("SF1 %s\n", target->list[0]->name);
	}

	fclose(filexid);
	

	return;
}

//*************************************************************************************************************
//	@name		@CallXidSaf
//	@function	call Xid SAF
//	@return	    (void)
//*************************************************************************************************************
void CallXidSaf(const char* net_file, const char* pin_file) {
    char cmd[2048]; 
    snprintf(cmd, sizeof(cmd), 
        "../../../../src/FaultSim/Build/Release/XID "
        "-c %s "
        "-tx ./tp.txt "
        "-pin %s "
        "-flist ./xid_fault.txt "
        "-otx ./xid_tp.txt "
        "-fm SAF -xid YES -m2004 YES", 
        net_file, pin_file);

    int ret = system(cmd);
    if (ret != 0) { 
        printf("XID Command Failed with code: %d\n", ret); 
    }
}

//*************************************************************************************************************
//	@name		@FreeMemory
//	@function	free the memory
//	@return		(void)
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
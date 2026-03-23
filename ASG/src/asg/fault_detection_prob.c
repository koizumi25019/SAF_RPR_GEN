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
#include "./fault_detection_prob.h"
#include "./init.h"
#include "./read.h"
#include "./cnf/cnf.h"
#include "../opt/opt.h"
#include "./cudd_wrapper.h"

//-----------------------------------------------------------------------------
// ソルバの解から tp.txt (XID入力用) を作成する関数
//-----------------------------------------------------------------------------
void GenerateTpAndFile(CCaDiCaL *solver, const char* filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;

for (int i = 0; i < n_pi; i++) {
        
        // ピンの変数番号を取得
        int var = pi[i]->varsgc; 
        
        // SATソルバから値を取得
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

	//CUDD initialization
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

	//shifting algorithm
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	//result file open
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//BDD result file header
	fprintf(bdd_result, "name,type,cube,rel,var,den");

	fprintf(bdd_result, "\n");

	//initialize global variables
	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

	/** read the fault */
	if (ReadFault() != READ_OKAY) return READ_ERROR;

	//create Good Circuit constraints
	if (CreateConsGC() != true) return AFD_ERROR;

	
	while (readdata.fault.numrema != 0)
	{

		// ソルバの初期化 
        CCaDiCaL *solver = ccadical_init();
		
		// 変数の未宣言エラーを回避するために factor オプションを無効化します
        ccadical_set_option(solver, "factor", 0);

		//open cube file
		fileOpen(&cube_file, "./bdd_cube_file.txt", "w");

		count++;
		temp_numrema = readdata.fault.numrema;

		//fault list set
		SetTarget(&remain, &target, loop++);

		//write TPG model
		if (WriteTPGModel(solver,&target) != true) return AFD_ERROR;

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
            int res = ccadical_solve(solver); // 10:SAT, 20:UNSAT

			//test generation count increment
			test_loop++;
				
            if (res == 20 || test_loop == opt.file.input.limit) {

				//test generation count output
				fprintf(bdd_result, "%d,", test_loop);

				//close cube file
				fclose(cube_file);

				//BDD running
				RunBDD(
					gbm, 
					n_pi,                              
				    bdd_result                               
				);

				//detected fault list deletion
				DropDeteFault(&target);

				//free memory
				FreeMemory(&remain, &target);

				break;

			}
			// SAT-> テスト生成続行
			else {
				printf("\rProgress >> %d/%d", count,readdata.fault.numinit);

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
        exit(EXIT_FAILURE); 
    }
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

		free(remain->list);
	
		free(target->list);

	return;
}
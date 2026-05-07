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
//	@name	    @AnalyzeFaultDetectionProbability
//	@function   analyze the fault detection probability
//	@return		(bool) okay, error
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	double* out_time_cadical,
    double* out_time_bdd,
    double* out_time_xid
)
{
	TARGET  remain;
	TARGET	target;
	FILE* fprpr = (FILE*)NULL;
	FILE* bdd_result = (FILE*)NULL;
	FILE* cube_file = (FILE*)NULL;
	FILE* cube_analysis_fp = (FILE*)NULL;
	int loop = 0;
	int temp_numrema;
	int count = 0;

	// ===== CPU時間計測用変数 =====
    clock_t t_start, t_end;
    double time_cadical = 0.0;  // CaDiCaL合計CPU時間
    double time_bdd     = 0.0;  // BDD合計CPU時間
    double time_xid     = 0.0;  // ドントケア判定合計CPU時間
    // ============================

	//キューブ分析用ファイルオープン
	if (opt.file.input.cube_analysis != FILE_NOSET) {
		fileOpen(&cube_analysis_fp, opt.file.input.cube_analysis, "w");
	}

	//CUDD初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	//シフトアルゴリズム使用
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	//result file open
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//result file header
	fprintf(bdd_result, "net_name,f_type,cube_cnt,fdp\n");

	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;
	if (ReadFault() != READ_OKAY) return READ_ERROR;
	if (CreateConsGC() != true) return AFD_ERROR;
	
	while (readdata.fault.numrema != 0)
	{
		// ソルバの初期化 
        CCaDiCaL *solver = ccadical_init();
		// 変数の未宣言エラーを回避するために factor オプションを無効化
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

		//fault name,type output
		fprintf(bdd_result, "%s,", target.list[0]->name);
        fprintf(bdd_result, (target.list[0]->type == SF0) ? "sa0," : "sa1,");
		if (opt.file.input.cube_analysis != FILE_NOSET) {
		fprintf(cube_analysis_fp, "%s", target.list[0]->name);
        fprintf(cube_analysis_fp, (target.list[0]->type == SF0) ? ",sa0" : ",sa1");
	    }

		//UNSAT判定時のテスト生成終了判定
		while (1) {
            // ========================================
            // CaDiCaL CPU時間計測
            // =========================================
            t_start = clock();
            int res = ccadical_solve(solver);// 10:SAT, 20:UNSAT
            t_end   = clock();
            time_cadical += ((double)(t_end - t_start)) / CLOCKS_PER_SEC;

			//test generation count increment
			test_loop++;
				
            if (res == 20 || test_loop == opt.file.input.limit) {

				//test generation count output
				fprintf(bdd_result, "%d,", test_loop);
				if (opt.file.input.cube_analysis != FILE_NOSET) {
				fprintf(cube_analysis_fp, "\n");
				}

				//close cube file
				fclose(cube_file);

                // ========BDD CPU時間計測================
                t_start = clock();
                RunBDD(gbm, n_pi, bdd_result,NULL,&target);
                t_end   = clock();
                time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;

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

                // =========================================
                // ドントケア判定 CPU時間計測
                // =========================================
                t_start = clock();
                CallXidSaf(opt.file.input.net, opt.file.output.pin);
                t_end   = clock();
                time_xid += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				//generate blocking clause 
				char* x_pattern = make_blocking_clause(solver,&target);
				//output the blocking clause
				fprintf(cube_file, "%s\n", x_pattern);
				free(x_pattern);

				//バッファをファイルに反映してからRunBDD
				fflush(cube_file);

			    // =========================================
                // キューブ分析モード: テスト生成ごとに RunBDD
                // =========================================
                if (opt.file.input.cube_analysis != FILE_NOSET) {
                    t_start = clock();
                    RunBDD(gbm, n_pi, NULL,cube_analysis_fp,&target);
                    t_end   = clock();
                    time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;
                }
			}
		}
		ccadical_release(solver);
	}

	// ===== CPU time =====
    *out_time_cadical = time_cadical;
    *out_time_bdd     = time_bdd;
    *out_time_xid     = time_xid;

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
        "/home/koizumi/FaultSim/Build/Release/XID "
        "-c %s "
        "-tx ./tp.txt "
        "-pin %s "
        "-flist ./xid_fault.txt "
        "-otx ./xid_tp.txt "
        "-fm SAF -xid YES -m2004 YES"
		" > /dev/null 2>&1", 
        net_file, pin_file);
		int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "XID execution failed: %d\n", ret);
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
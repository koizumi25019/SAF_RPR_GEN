//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/resource.h>
#include <cudd.h>
#include <gmp.h>

#include "ccadical.h"
#include "./create_TPG_model.h"
#include "./fault_detection_prob.h"
#include "./init.h"
#include "./read.h"
#include "./cnf/cnf.h"
#include "../opt/opt.h"
#include "./cudd_wrapper.h"
#include "./xid/XID.h"

//*************************************************************************************************************
//	@name	    @AnalyzeFaultDensity
//	@function   analyze the fault detection probability
//	@return		(bool) okay, error
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	double* out_time_cadical,
    double* out_time_bdd,
    double* out_time_xid
)
{
	TARGET	target;
	FILE* bdd_result = (FILE*)NULL;
	FILE* cube_analysis_fp = (FILE*)NULL;

	int count = 0;

    printf("D-chain ON\n");

	// ===== CPU時間計測用変数 =====
    clock_t t_start, t_end;
    double time_cadical = 0.0;
    double time_bdd     = 0.0;
    double time_xid     = 0.0;
    // ============================

	//キューブ分析用ファイルオープン
	if (opt.file.input.cube_analysis != FILE_NOSET) {
		fileOpen(&cube_analysis_fp, opt.file.input.cube_analysis, "w");
	}

	//CUDD初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	//result file open
	fileOpen(&bdd_result, opt.file.output.result, "w");
	fprintf(bdd_result, "net_name,f_type,cube_cnt,complete,fdp\n");

	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;
	if (ReadFault() != READ_OKAY) return READ_ERROR;
	if (CreateConsGC() != true) return AFD_ERROR;

	while (readdata.fault.numrema != 0)
	{
		// ソルバの初期化
        CCaDiCaL *solver = ccadical_init();
        ccadical_set_option(solver, "factor", 0);

		int cubes_cap = (opt.file.input.limit > 0) ? opt.file.input.limit : 30;
		int n_cubes = 0;
		char** cubes = (char**)malloc(cubes_cap * sizeof(char*));

		count++;
		SetTarget(&target);

		if (WriteTPGModel(solver,&target) != true) return AFD_ERROR;

		if (opt.file.input.cube_analysis != FILE_NOSET) {
			fprintf(cube_analysis_fp, "%s", target.list[0]->name);
            fprintf(cube_analysis_fp, (target.list[0]->type == SF0) ? ",sa0" : ",sa1");
		}

		//UNSAT判定時のテスト生成終了判定
		while (1) {
            t_start = clock();
            int res = ccadical_solve(solver);
            t_end   = clock();
            time_cadical += ((double)(t_end - t_start)) / CLOCKS_PER_SEC;

            if (res == 20 || n_cubes == opt.file.input.limit) {
                bool limit_hit = (n_cubes == opt.file.input.limit && res != 20);

				if (opt.file.input.cube_analysis != FILE_NOSET) {
					fprintf(cube_analysis_fp, "\n");
				}

                t_start = clock();
                RunBDD(gbm, n_pi, cubes, n_cubes, bdd_result, NULL, &target, n_cubes, limit_hit);
                t_end   = clock();
                time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				DropDeteFault(&target);
				FreeMemory(&target);

				for (int i = 0; i < n_cubes; i++) free(cubes[i]);
				free(cubes);

				break;
			}
			// SAT → InlineXID でドントケア判定＋ブロッキング節追加
			else {
				printf("\rProgress >> %d/%d", count, readdata.fault.numinit);

                t_start = clock();
                char* x_pattern = InlineXID(solver, target.list[0]->netptr);
                t_end   = clock();
                time_xid += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				// キューブを配列に追加
				if (n_cubes == cubes_cap) {
					cubes_cap *= 2;
					cubes = (char**)realloc(cubes, cubes_cap * sizeof(char*));
				}
				cubes[n_cubes++] = x_pattern;

                if (opt.file.input.cube_analysis != FILE_NOSET) {
                    t_start = clock();
                    RunBDD(gbm, n_pi, cubes, n_cubes, NULL, cube_analysis_fp, &target, n_cubes, false);
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
//	@name		@FreeMemory
//	@function	free the memory
//	@return		(void)
//*************************************************************************************************************
void FreeMemory(
	TARGET* target
)
{
	free(target->list);
	return;
}

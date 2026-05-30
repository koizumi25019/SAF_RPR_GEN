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
#include "./cube_set.h"
#include "./cnf/cnf.h"
#include "./cnf/faulty_miter.h"
#include "../opt/opt.h"
#include "./cudd_wrapper.h"
#include "./xid/XID.h"

//-------------------------------------------------------------------------------------------------------------
//	全回路ミター方式（実験）：限定化せず全故障回路＋per-net 注入スイッチを一度だけ構築し、
//	故障は assumption で選択する。CNF が固定なので学習節を全故障で再利用できる。
//	各故障専用の節（ブロッキング節）だけは活性化変数 e でガードし、終了後 (¬e) で無効化する。
//-------------------------------------------------------------------------------------------------------------

//*************************************************************************************************************
//	@name		AddBlockingClauseGuarded
//	@function	キューブのブロッキング節を (元の節 ∨ ¬e) として追加（その故障専用にする）
//*************************************************************************************************************
static void AddBlockingClauseGuarded(CCaDiCaL* solver, const char* cube, int e)
{
    for (int i = 0; i < n_pi; i++)
    {
        int lit = 0;
        if      (cube[i] == '0') lit =  (int)pi[i]->varsgc;
        else if (cube[i] == '1') lit = -(int)pi[i]->varsgc;
        if (lit != 0) ccadical_add(solver, lit);
    }
    ccadical_add(solver, -e);   // この故障のときだけ有効
    ccadical_add(solver, 0);
}

//*************************************************************************************************************
//	@name		AssumeFaultSelection
//	@function	故障 k(sa-v) を選ぶ：act_k=1, sval_k=v, それ以外は act_j=0 を仮定する
//*************************************************************************************************************
static void AssumeFaultSelection(CCaDiCaL* solver, int k, int v)
{
    for (int j = 0; j < n_net; j++)
        ccadical_assume(solver, (j == k) ? mt_act[j] : -mt_act[j]);
    ccadical_assume(solver, v ? mt_sval[k] : -mt_sval[k]);
}

//*************************************************************************************************************
//	@name	    @AnalyzeFaultDensity
//	@function   analyze the fault detection probability
//	@return		(bool) okay, error
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	double* out_time_cadical,
    double* out_time_bdd,
    double* out_time_xid,
    double* out_time_read
)
{
	TARGET	target;
	FILE* bdd_result = (FILE*)NULL;
	FILE* cube_analysis_fp = (FILE*)NULL;

	int count = 0;

	// ===== CPU時間計測用変数 =====
    clock_t t_start, t_end;
    double time_cadical = 0.0;
    double time_bdd     = 0.0;
    double time_xid     = 0.0;
    double time_read    = 0.0;
    // ============================

	//キューブ分析用ファイルオープン
	if (opt.file.input.cube_analysis != FILE_NOSET) {
		fileOpen(&cube_analysis_fp, opt.file.input.cube_analysis, "w");
	}

	//CUDD初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	//result file open
	fileOpen(&bdd_result, opt.file.output.fdp, "w");
	fprintf(bdd_result, "net_name,f_type,cube_cnt,complete,fdp\n");

	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

    t_start = clock();
	printf("Reading fault data...\n");
	if (ReadFault() != READ_OKAY) return READ_ERROR;
    t_end = clock();
    time_read = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	printf("ReadFault: %.3f sec\n", time_read);

	if (CreateConsGC() != true) return AFD_ERROR;

	// 正常回路＋全回路ミターを一度だけ構築し、全故障で使い回す
	CCaDiCaL* solver = ccadical_init();
	ccadical_set_option(solver, "factor", 0);
	LoadModelToSolver(solver, NULL);   // 正常回路CNF（恒久）
	AllocMiterVars();
	BuildFaultyMiter(solver);          // 故障回路＋注入スイッチ＋検出条件（恒久）

	while (readdata.fault.numrema != 0)
	{
		count++;
		SetTarget(&target);
		FNODE* f = target.list[0];
		int k = (int)(f->netptr - nl);          // 故障サイトのネット番号
		int v = (f->type == SF0) ? 0 : 1;        // 縮退値
		int e = ++opb.total.vars;                // この故障のブロッキング節の活性化変数

		// f のテストキューブを集める集合
		CubeSet cubes;
		cubeset_init(&cubes, (opt.file.input.limit > 0) ? opt.file.input.limit : 30);

		// 部分集合側の故障（subset_faults）のキューブを種＋禁止節として流用する。
		for (int kk = 0; kk < f->n_subset_faults; kk++)
		{
			FNODE* src = f->subset_faults[kk];
			for (int m = 0; m < src->cubes.n; m++)
			{
				cubeset_push(&cubes, strdup(src->cubes.data[m]));
				AddBlockingClauseGuarded(solver, src->cubes.data[m], e);
			}
			if (--src->n_pending == 0)
				cubeset_free(&src->cubes);
		}

		if (opt.file.input.cube_analysis != FILE_NOSET) {
			fprintf(cube_analysis_fp, "%s", f->name);
			fprintf(cube_analysis_fp, (f->type == SF0) ? ",sa0" : ",sa1");
		}

		// UNSAT もしくは limit 到達でテスト生成を終了する
		while (1) {
			AssumeFaultSelection(solver, k, v);   // 故障 k を選択（毎 solve 必要）
			ccadical_assume(solver, e);           // この故障のブロッキング節を有効化

            t_start = clock();
            int res = ccadical_solve(solver);
            t_end   = clock();
            time_cadical += ((double)(t_end - t_start)) / CLOCKS_PER_SEC;

            if (res == 20 || cubes.n >= opt.file.input.limit) {
                bool limit_hit = (cubes.n >= opt.file.input.limit && res != 20);

				if (opt.file.input.cube_analysis != FILE_NOSET) {
					fprintf(cube_analysis_fp, "\n");
				}

                t_start = clock();
                RunBDD(gbm, n_pi, cubes.data, cubes.n, bdd_result, NULL, &target, cubes.n, limit_hit);
                t_end   = clock();
                time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				f->cubes = cubes;
				if (f->n_pending == 0)
					cubeset_free(&f->cubes);

				DropDeteFault(&target);
				FreeMemory(&target);
				break;
			}
			// SAT → InlineXID でドントケアを埋め、キューブ追加＋禁止節
			else {
				printf("\rProgress >> %d/%d", count, readdata.fault.numinit);

                t_start = clock();
                char* x_pattern = InlineXID(solver, f->netptr);
                t_end   = clock();
                time_xid += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				AddBlockingClauseGuarded(solver, x_pattern, e);
				cubeset_push(&cubes, x_pattern);

                if (opt.file.input.cube_analysis != FILE_NOSET) {
                    t_start = clock();
                    RunBDD(gbm, n_pi, cubes.data, cubes.n, NULL, cube_analysis_fp, &target, cubes.n, false);
                    t_end   = clock();
                    time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;
                }
			}
		}

		// この故障のブロッキング節を恒久的に無効化（retire）
		ccadical_add(solver, -e);
		ccadical_add(solver, 0);
	}
	ccadical_release(solver);

	// ===== CPU time =====
    *out_time_cadical = time_cadical;
    *out_time_bdd     = time_bdd;
    *out_time_xid     = time_xid;
    *out_time_read    = time_read;

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

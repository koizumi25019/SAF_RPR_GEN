//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/resource.h>
#include <cudd.h>
#include <gmp.h>

#include <stdlib.h>
#include "ccadical.h"
#include "./create_TPG_model.h"
#include "./fault_detection_prob.h"
#include "./init.h"
#include "./read.h"
#include "./target_fault.h"
#include "./cube_set.h"
#include "./cnf/cnf.h"
#include "../opt/opt.h"
#include "./cudd_wrapper.h"
#include "./fault_result.h"
#include "./xid/XID.h"
#include "./normal_scope.h"

//*************************************************************************************************************
//	@name		AddBlockingClauseFromCube
//	@function	キューブ文字列（'0'/'1'/'X' を n_pi 文字）からブロッキング節をソルバに追加する
//*************************************************************************************************************
static void AddBlockingClauseFromCube(CCaDiCaL* solver, const char* cube)
{
    for (int i = 0; i < n_pi; i++)
    {
        int lit = 0;
        if      (cube[i] == '0') lit =  (int)pi[i]->varsgc;
        else if (cube[i] == '1') lit = -(int)pi[i]->varsgc;
        if (lit != 0) ccadical_add(solver, lit);
    }
    ccadical_add(solver, 0);
}

//*************************************************************************************************************
//	@name	    @AnalyzeFaultDensity
//	@function   故障検出確率を算出する
//	@return	F	(bool) 正常, 異常
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	double* out_time_cadical,
    double* out_time_bdd,
    double* out_time_xid,
    double* out_time_read
)
{
	FILE* result_fp = (FILE*)NULL;

	int count = 0;

	// ===== CPU時間計測用変数 =====
    clock_t t_start, t_end;
    double time_cadical = 0.0;
    double time_bdd     = 0.0;
    double time_xid     = 0.0;
    double time_read    = 0.0;
    // ============================
    // 支配流用サマリー用アキュムレータ
    long dom_total_cubes  = 0;
    long dom_seeded_cubes = 0;

	//CUDD初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	// 結果ファイルを開く
	fileOpen(&result_fp, opt.file.output.fdp, "w");
	fprintf(result_fp, "net_name,f_type,cube_cnt,complete,fdp,seeded_cnt\n");

	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

    t_start = clock();
	printf("Reading fault data...\n");
	if (ReadFault() != READ_OKAY) return READ_ERROR;
    t_end = clock();
    time_read = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	printf("ReadFault: %.3f sec\n", time_read);

	if (CreateConsGC() != true) return AFD_ERROR;
	if (!InitTargetOrder()) {
		NormalScopeRelease();
		Cudd_Quit(gbm);
		fclose(result_fp);
		return AFD_ERROR;
	}

	while (readdata.fault.numrema != 0)
	{
		FNODE* target = SetTarget();
		if (target == NULL) {
			FreeTargetOrder();
			NormalScopeRelease();
			Cudd_Quit(gbm);
			fclose(result_fp);
			return AFD_ERROR;
		}

		// ソルバの初期化
		CCaDiCaL* solver = ccadical_init();
		ccadical_set_option(solver, "factor", 0);

		count++;

		if (WriteTPGModel(solver, target) != true) {
			ccadical_release(solver);
			FreeTargetOrder();
			NormalScopeRelease();
			Cudd_Quit(gbm);
			fclose(result_fp);
			return AFD_ERROR;
		}

		// 対象故障のテストキューブを集める集合
		CubeSet cubes;
		cubeset_init(&cubes, (opt.file.input.limit > 0) ? opt.file.input.limit : 30);

		// 部分集合側の故障（subset_faults）のキューブを種＋禁止節として流用する。
		// T(subset) ⊆ T(target) なので、これらは対象故障の正当なテストであり、
		// solver は差分 T(target)\∪T(subset) だけを探索すればよい。
		// MDC_NODOM をセットすると流用を止め、ゼロから完全列挙する（支配解析の検証用）。
		bool nodom = getenv("MDC_NODOM");
		int seeded_cnt = 0;
		for (int k = 0; k < target->n_subset_faults; k++)
		{
			FNODE* src = target->subset_faults[k];

			if (!nodom)
			{
				for (int m = 0; m < src->cubes.n; m++)
				{
					cubeset_push(&cubes, strdup(src->cubes.data[m]));
					AddBlockingClauseFromCube(solver, src->cubes.data[m]);
				}
				seeded_cnt += src->cubes.n;
			}

			// この親で src のキューブを使い切る。最後の消費者ならここで解放
			if (--src->n_pending == 0)
				cubeset_free(&src->cubes);
		}

		// limit <= 0 は「上限なし（無制限）」を意味し、UNSAT まで完全列挙する
		bool unlimited = (opt.file.input.limit <= 0);

		// UNSAT もしくは limit 到達でテスト生成を終了する
		while (1) {
            t_start = clock();
            int res = ccadical_solve(solver);
            t_end   = clock();
            time_cadical += ((double)(t_end - t_start)) / CLOCKS_PER_SEC;

            // UNKNOWN ではモデル参照・禁止節追加・結果出力を行わず異常終了する。
            if (res != 10 && res != 20) {
                fprintf(stderr, "\nERROR: SAT solver returned UNKNOWN (status=%d) for %s %s; "
                        "no cube blocked or result written for this fault.\n",
                        res, target->name, (target->type == SF0) ? "sa0" : "sa1");
                cubeset_free(&cubes);
                ccadical_release(solver);
                FreeTargetOrder();
                NormalScopeRelease();
                Cudd_Quit(gbm);
                fclose(result_fp);
                return AFD_ERROR;
            }

            if (res == 20 || (!unlimited && cubes.n >= opt.file.input.limit)) {
                bool limit_hit = (!unlimited && cubes.n >= opt.file.input.limit && res != 20);

                dom_total_cubes  += cubes.n;
                dom_seeded_cubes += seeded_cnt;

                FaultResult result = {
                    .target = target,
                    .cube_cnt = cubes.n,
                    .seeded_cnt = seeded_cnt,
                    .complete = !limit_hit
                };
                mpf_init2(result.density, 8192);

                t_start = clock();
                if (!RunBDD(gbm, n_pi, &cubes, result.density)) {
                    mpf_clear(result.density);
                    cubeset_free(&cubes);
                    ccadical_release(solver);
                    FreeTargetOrder();
                    NormalScopeRelease();
                    Cudd_Quit(gbm);
                    fclose(result_fp);
                    return AFD_ERROR;
                }
                WriteFaultResult(result_fp, &result);
                mpf_clear(result.density);
                t_end   = clock();
                time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				// キューブの所有権を故障へ移す（深いコピーはしない）。
				// 流用する親が残っていなければ即解放し、メモリを生存集合だけに保つ。
				target->cubes = cubes;
				if (target->n_pending == 0)
					cubeset_free(&target->cubes);

				DropDeteFault(target);
				break;
			}
			// SAT → InlineXID でドントケアを埋め、キューブ追加＋禁止節
			else {
				printf("\rProgress >> %d/%d", count, readdata.fault.numinit);

                t_start = clock();
                char* x_pattern = InlineXID(solver, target->netptr, -1);
                t_end   = clock();
                time_xid += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				AddBlockingClauseFromCube(solver, x_pattern);
				cubeset_push(&cubes, x_pattern);
			}
		}
		ccadical_release(solver);
	}

	// ===== 支配流用サマリー =====
	{
		long sat_calls = dom_total_cubes - dom_seeded_cubes;
		double reduction = dom_total_cubes > 0
			? 100.0 * dom_seeded_cubes / dom_total_cubes : 0.0;
		printf("\n[DOM] total_cubes=%ld  seeded=%ld  sat_calls=%ld  reduction=%.1f%%\n",
			dom_total_cubes, dom_seeded_cubes, sat_calls, reduction);
	}

	FreeTargetOrder();
	fclose(result_fp);
	Cudd_Quit(gbm);

	// ===== CPU時間 =====
    *out_time_cadical = time_cadical;
    *out_time_bdd     = time_bdd;
    *out_time_xid     = time_xid;
    *out_time_read    = time_read;

    NormalScopeRelease();
	return AFD_OKAY;
}

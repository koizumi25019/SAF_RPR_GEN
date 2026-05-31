//-------------------------------------------------------------------------------------------------------------
//	include
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
#include "./cube_set.h"
#include "./cnf/cnf.h"
#include "../opt/opt.h"
#include "./cudd_wrapper.h"
#include "./xid/XID.h"

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

/* =====================================================================
 *  EXPERIMENT (env MAXDC_MEASURE): don't-care headroom measurement.
 *  Builds an "undetection oracle" CNF (good ^ faulty-cone ^ fc[site]=stuck
 *  ^ all-PO-equal i.e. z=0) and, for each XID cube, greedily drops care
 *  bits while (cube\b ^ undetection) stays UNSAT  ==> prime implicant.
 *  Reports how many MORE bits could be X'd (the headroom for 案1).
 *  Inert unless MAXDC_MEASURE is set; production behaviour unchanged.
 * ===================================================================== */
static long mdc_cubes=0, mdc_orig=0, mdc_prime=0, mdc_hr_cubes=0, mdc_sanity_fail=0;
static long mdc_hardcubes=0, mdc_hardorig=0, mdc_hardprime=0;   /* capped faults only */
static long mdc_fcubes=0, mdc_forig=0, mdc_fprime=0;            /* per-fault accumulator */
static void mdc_dump(void){
    fprintf(stderr,"\n[MAXDC] cubes=%ld  care/cube: orig=%.2f prime=%.2f  headroom=%.2f bits/cube (%.0f%% of cubes shrink)  sanity_fail=%ld\n",
        mdc_cubes, mdc_cubes?(double)mdc_orig/mdc_cubes:0, mdc_cubes?(double)mdc_prime/mdc_cubes:0,
        mdc_cubes?(double)(mdc_orig-mdc_prime)/mdc_cubes:0, mdc_cubes?100.0*mdc_hr_cubes/mdc_cubes:0, mdc_sanity_fail);
    fprintf(stderr,"[MAXDC] capped-fault cubes=%ld  orig=%.2f prime=%.2f headroom=%.2f bits/cube\n",
        mdc_hardcubes, mdc_hardcubes?(double)mdc_hardorig/mdc_hardcubes:0,
        mdc_hardcubes?(double)mdc_hardprime/mdc_hardcubes:0,
        mdc_hardcubes?(double)(mdc_hardorig-mdc_hardprime)/mdc_hardcubes:0);
}

/* build the undetection oracle for the CURRENT target (call right after
   WriteTPGModel so TFO flags / varsfc / numtranpo are set for this fault) */
static void MDC_BuildOracle(CCaDiCaL* u, TARGET* target){
    FNODE* f = target->list[0];
    LoadModelToSolver(u, target);                 /* good circuit (varsgc) */
    for (int j=0;j<n_net;j++){                    /* faulty cone gates (varsfc) */
        if (((nl[j].flag & TFO)==TFO) && ((nl[j].flag & FP)!=FP)){
            switch(nl[j].type){
                case AND:  CreateConsFC_AND (u,&nl[j]); break;
                case NAND: CreateConsFC_NAND(u,&nl[j]); break;
                case OR:   CreateConsFC_OR  (u,&nl[j]); break;
                case NOR:  CreateConsFC_NOR (u,&nl[j]); break;
                case INV:  CreateConsFC_INV (u,&nl[j]); break;
                case BUF:
                case FOUT: CreateConsFC_BUF (u,&nl[j]); break;
                case EXOR: CreateConsFC_XOR (u,&nl[j]); break;
                case EXNOR:CreateConsFC_XNOR(u,&nl[j]); break;
                default: break;
            }
        }
    }
    int fc = f->netptr->varsfc;                   /* fault site stuck value */
    if (f->type==SF0){ ccadical_add(u,-fc); ccadical_add(u,0); }
    else             { ccadical_add(u, fc); ccadical_add(u,0); }
    CreateConsDC_XOR(u);                           /* per-PO diff = gc XOR fc */
    CreateConsDC_OR(u);                            /* z = OR diffs */
    int z = opb.total.vars;
    ccadical_add(u,-z); ccadical_add(u,0);         /* z=0 : undetection (no PO differs) */
}

/* greedy prime-implicant: how many care bits of `cube` can become X */
static void MDC_Measure(CCaDiCaL* u, const char* cube){
    static int* care=NULL; static char* kept=NULL; static int cap=0;
    if (cap<n_pi){ care=realloc(care,n_pi*sizeof(int)); kept=realloc(kept,n_pi); cap=n_pi; }
    int nc=0;
    for (int i=0;i<n_pi;i++) if (cube[i]!='X'){ care[nc]=i; kept[nc]=1; nc++; }
    if (nc==0) return;
    /* sanity: full cube must imply detection (oracle UNSAT) */
    for (int k=0;k<nc;k++){ int i=care[k]; ccadical_assume(u,(cube[i]=='1')?(int)pi[i]->varsgc:-(int)pi[i]->varsgc); }
    if (ccadical_solve(u)!=20) mdc_sanity_fail++;
    /* greedy drop */
    for (int b=0;b<nc;b++){
        for (int k=0;k<nc;k++){ if (k==b||!kept[k]) continue; int i=care[k];
            ccadical_assume(u,(cube[i]=='1')?(int)pi[i]->varsgc:-(int)pi[i]->varsgc); }
        if (ccadical_solve(u)==20) kept[b]=0;     /* UNSAT -> bit unnecessary */
    }
    int prime=0; for (int k=0;k<nc;k++) prime+=kept[k];
    mdc_cubes++; mdc_orig+=nc; mdc_prime+=prime; if (prime<nc) mdc_hr_cubes++;
    mdc_fcubes++; mdc_forig+=nc; mdc_fprime+=prime;
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

	while (readdata.fault.numrema != 0)
	{
		// ソルバの初期化
		CCaDiCaL* solver = ccadical_init();
		ccadical_set_option(solver, "factor", 0);

		count++;
		SetTarget(&target);
		FNODE* f = target.list[0];

		if (WriteTPGModel(solver, &target) != true) return AFD_ERROR;

		// 伸び代測定用：非検出オラクル（env MAXDC_MEASURE 指定時のみ）
		CCaDiCaL* u_oracle = NULL;
		if (getenv("MAXDC_MEASURE")) {
			if (mdc_cubes==0 && mdc_fcubes==0) atexit(mdc_dump);
			u_oracle = ccadical_init();
			ccadical_set_option(u_oracle, "factor", 0);
			MDC_BuildOracle(u_oracle, &target);
			mdc_fcubes=0; mdc_forig=0; mdc_fprime=0;
		}

		// f のテストキューブを集める集合
		CubeSet cubes;
		cubeset_init(&cubes, (opt.file.input.limit > 0) ? opt.file.input.limit : 30);

		// 部分集合側の故障（subset_faults）のキューブを種＋禁止節として流用する。
		// T(subset) ⊆ T(f) なので、これらは f の正当なテストであり、
		// solver は差分 T(f)\∪T(subset) だけを探索すればよい。
		for (int k = 0; k < f->n_subset_faults; k++)
		{
			FNODE* src = f->subset_faults[k];

			for (int m = 0; m < src->cubes.n; m++)
			{
				cubeset_push(&cubes, strdup(src->cubes.data[m]));
				AddBlockingClauseFromCube(solver, src->cubes.data[m]);
			}

			// この親で src のキューブを使い切る。最後の消費者ならここで解放
			if (--src->n_pending == 0)
				cubeset_free(&src->cubes);
		}

		if (opt.file.input.cube_analysis != FILE_NOSET) {
			fprintf(cube_analysis_fp, "%s", f->name);
			fprintf(cube_analysis_fp, (f->type == SF0) ? ",sa0" : ",sa1");
		}

		// UNSAT もしくは limit 到達でテスト生成を終了する
		while (1) {
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

				// 伸び代測定：この故障が capped なら hard 集計に加算し、オラクルを解放
				if (u_oracle) {
					if (limit_hit) { mdc_hardcubes+=mdc_fcubes; mdc_hardorig+=mdc_forig; mdc_hardprime+=mdc_fprime; }
					ccadical_release(u_oracle); u_oracle=NULL;
				}

				// キューブの所有権を故障へ移す（深いコピーはしない）。
				// 流用する親が残っていなければ即解放し、メモリを生存集合だけに保つ。
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

				if (u_oracle) MDC_Measure(u_oracle, x_pattern);

				AddBlockingClauseFromCube(solver, x_pattern);
				cubeset_push(&cubes, x_pattern);

                if (opt.file.input.cube_analysis != FILE_NOSET) {
                    t_start = clock();
                    RunBDD(gbm, n_pi, cubes.data, cubes.n, NULL, cube_analysis_fp, &target, cubes.n, false);
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

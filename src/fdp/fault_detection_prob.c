#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cudd.h>
#include <gmp.h>
#include "ccadical.h"
#include "create_TPG_model.h"
#include "fault_detection_prob.h"
#include "init.h"
#include "read.h"
#include "target_fault.h"
#include "cube_set.h"
#include "cnf/cnf.h"
#include "../opt/opt.h"
#include "cudd_wrapper.h"
#include "fault_result.h"
#include "xid/XID.h"
#include "normal_scope.h"
#include "fault_pool.h"

/* 各プロセス専用。BDDマネージャはfork後に初期化し、担当故障間で再利用する。
 * ソルバとキューブ集合は故障ごとに作成する。XIDのstatic領域もプロセスごとに独立。
 */
typedef struct {
    DdManager* bdd_manager;
} FaultAnalysisContext;

static void AddBlockingClauseFromCube(CCaDiCaL* solver, const char* cube)
{
    for (int index = 0; index < n_pi; index++) {
        int literal = 0;
        if (cube[index] == '0') literal = (int)pi[index]->varsgc;
        else if (cube[index] == '1') literal = -(int)pi[index]->varsgc;
        if (literal != 0) ccadical_add(solver, literal);
    }
    ccadical_add(solver, 0);
}

/* 直列・並列で共通の1故障処理。SAT→XID→禁止節を繰り返し、BDD・CSVへ進む。
 * baselineのWriteTPGModel、InlineXID、RunBDD、WriteFaultResultをそのまま使う。
 */
static bool AnalyzeOneFault(FNODE* target, int fault_number, FILE* result_fp,
                            FaultStats* stats, void* opaque)
{
    FaultAnalysisContext* context = opaque;
    if (!context->bdd_manager) {
        context->bdd_manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
        if (!context->bdd_manager) return false;
        Cudd_AutodynEnable(context->bdd_manager, CUDD_REORDER_SIFT);
    }
    CCaDiCaL* solver = ccadical_init();
    if (!solver) return false;
    ccadical_set_option(solver, "factor", 0);
    if (!WriteTPGModel(solver, target)) {
        ccadical_release(solver);
        return false;
    }

    CubeSet cubes;
    cubeset_init(&cubes, opt.file.input.limit > 0 ? opt.file.input.limit : 30);
    int seeded_cnt = 0;
    bool reuse = opt.dom_reuse == YES;
    for (int index = 0; reuse && index < target->n_subset_faults; index++) {
        FNODE* source = target->subset_faults[index];
        for (int cube = 0; cube < source->cubes.n; cube++) {
            cubeset_push(&cubes, strdup(source->cubes.data[cube]));
            AddBlockingClauseFromCube(solver, source->cubes.data[cube]);
        }
        seeded_cnt += source->cubes.n;
        if (--source->n_pending == 0) cubeset_free(&source->cubes);
    }

    bool succeeded = true;
    bool unlimited = opt.file.input.limit <= 0;
    for (;;) {
        clock_t start = clock();
        int status = ccadical_solve(solver);
        stats->cadical += (double)(clock() - start) / CLOCKS_PER_SEC;

        /* UNKNOWNではモデル参照・禁止節追加・結果出力を行わない。 */
        if (status != 10 && status != 20) {
            fprintf(stderr, "ERROR: SAT solver returned UNKNOWN (status=%d) for %s %s; "
                    "no cube blocked or result written for this fault.\n",
                    status, target->name, target->type == SF0 ? "sa0" : "sa1");
            cubeset_free(&cubes);
            succeeded = false;
            break;
        }
        if (status == 20 || (!unlimited && cubes.n >= opt.file.input.limit)) {
            bool limit_hit = !unlimited && cubes.n >= opt.file.input.limit && status != 20;
            stats->total_cubes += cubes.n;
            stats->seeded_cubes += seeded_cnt;

            FaultResult result = {
                .target = target,
                .cube_cnt = cubes.n,
                .seeded_cnt = seeded_cnt,
                .complete = !limit_hit
            };
            mpf_init2(result.density, 8192);
            start = clock();
            if (!RunBDD(context->bdd_manager, n_pi, &cubes, result.density)) {
                mpf_clear(result.density);
                cubeset_free(&cubes);
                succeeded = false;
                break;
            }
            WriteFaultResult(result_fp, &result);
            mpf_clear(result.density);
            stats->bdd += (double)(clock() - start) / CLOCKS_PER_SEC;

            /* 流用なしならこの故障で解放。流用ありは従来の参照数で管理する。 */
            target->cubes = cubes;
            if (!reuse || target->n_pending == 0) cubeset_free(&target->cubes);
            DropDeteFault(target);
            break;
        }
        if (opt.jobs == 1) {
            printf("\rProgress >> %d/%d", fault_number, readdata.fault.numinit);
        }
        start = clock();
        char* pattern = InlineXID(solver, target->netptr, -1);
        stats->xid += (double)(clock() - start) / CLOCKS_PER_SEC;
        AddBlockingClauseFromCube(solver, pattern);
        cubeset_push(&cubes, pattern);
    }
    ccadical_release(solver);
    return succeeded && !ferror(result_fp);
}

static void FinishFaultAnalysis(void* opaque)
{
    FaultAnalysisContext* context = opaque;
    if (context->bdd_manager) Cudd_Quit(context->bdd_manager);
    context->bdd_manager = NULL;
    NormalScopeRelease();
}

/* 既存InitTargetOrderの整列結果を使う。計算関数と出力順は直列・並列で共通。 */
static bool AnalyzeIndependentFaults(FaultAnalysisContext* context, FILE* result_fp,
                                     FaultStats* stats)
{
    int count;
    FNODE** faults = CopyTargetOrder(&count);
    if (!faults) return false;
    bool succeeded = true;
    if (opt.jobs > 1) {
        succeeded = FaultPoolRun(faults, count, opt.jobs, AnalyzeOneFault,
                                 FinishFaultAnalysis, context, result_fp, stats);
    } else {
        for (int index = 0; index < count && succeeded; index++) {
            succeeded = AnalyzeOneFault(faults[index], index + 1, result_fp, stats, context);
        }
    }
    free(faults);
    return succeeded;
}

bool AnalyzeFaultDensity(double* out_time_cadical, double* out_time_bdd,
                         double* out_time_xid, double* out_time_read)
{
    FILE* result_fp = NULL;
    fileOpen(&result_fp, opt.file.output.fdp, "w");
    fprintf(result_fp, "net_name,f_type,cube_cnt,complete,fdp,seeded_cnt\n");
    FaultAnalysisContext context = {0};
    FaultStats stats = {0};
    bool succeeded = InitGlobalVars() == INIT_OKAY;
    double time_read = 0.0;
    if (succeeded) {
        printf("Reading fault data...\n");
        clock_t start = clock();
        succeeded = ReadFault() == READ_OKAY;
        time_read = (double)(clock() - start) / CLOCKS_PER_SEC;
        printf("ReadFault: %.3f sec\n", time_read);
    }
    if (succeeded) succeeded = CreateConsGC();
    if (succeeded) succeeded = InitTargetOrder();

    /* 準備後にforkする。キューブ流用なしでは、全故障を独立して配分できる。 */
    if (succeeded && opt.dom_reuse == NO) {
        succeeded = AnalyzeIndependentFaults(&context, result_fp, &stats);
    } else if (succeeded) {
        int fault_number = 0;
        while (readdata.fault.numrema && succeeded) {
            FNODE* target = SetTarget();
            succeeded = target && AnalyzeOneFault(target, ++fault_number, result_fp, &stats, &context);
        }
    }
    if (opt.jobs == 1) FinishFaultAnalysis(&context);
    FreeTargetOrder();
    if (fclose(result_fp) != 0) succeeded = false;
    if (!succeeded) return AFD_ERROR;

    long sat_calls = stats.total_cubes - stats.seeded_cubes;
    double reduction = stats.total_cubes > 0 ? 100.0 * stats.seeded_cubes / stats.total_cubes : 0.0;
    printf("\n[DOM] total_cubes=%ld  seeded=%ld  sat_calls=%ld  reduction=%.1f%%\n",
           stats.total_cubes, stats.seeded_cubes, sat_calls, reduction);
    *out_time_cadical = stats.cadical;
    *out_time_bdd = stats.bdd;
    *out_time_xid = stats.xid;
    *out_time_read = time_read;
    return AFD_OKAY;
}

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <cudd.h>
#include <gmp.h>
#include "ccadical.h"
#include "fdp/cnf/tpg_model.h"
#include "fdp/analysis.h"
#include "fdp/context.h"
#include "fault/fault_list.h"
#include "fault/target_fault.h"
#include "fdp/cube_set.h"
#include "fdp/cnf/cnf.h"
#include "cli/options.h"
#include "fdp/bdd/probability.h"
#include "io/fault_csv.h"
#include "io/cube_text.h"
#include "io/file.h"
#include "fdp/xid/XID.h"
#include "fdp/cnf/normal_scope.h"
#include "fdp/worker_pool.h"

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
                            FILE* cube_fp, FaultStats* stats, void* opaque)
{
    FaultAnalysisContext* context = opaque;
    (void)fault_number; /* 進捗は故障完了後に呼び出し側で表示する。 */
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
                    status, target->name, FaultTypeName(target->type));
            cubeset_free(&cubes);
            succeeded = false;
            break;
        }
        if (status == 20 || (!unlimited && cubes.n >= opt.file.input.limit)) {
            bool limit_hit = !unlimited && cubes.n >= opt.file.input.limit && status != 20;

            FaultResult result = {
                .target = target,
                .cube_cnt = cubes.n,
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
            succeeded = WriteFaultCubes(cube_fp, &result, &cubes);
            mpf_clear(result.density);
            stats->bdd += (double)(clock() - start) / CLOCKS_PER_SEC;

            cubeset_free(&cubes);
            DropDeteFault(target);
            break;
        }
        start = clock();
        char* pattern = InlineXID(solver, target->netptr, -1, target->exc_netptr);
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

/* 直列でも1故障ずつテキスト化→保存し、全故障分をメモリに溜めない。 */
static bool AnalyzeSerialFault(FNODE* fault, int number, FILE* csv, CubeOutput* output,
                              FaultStats* stats, FaultAnalysisContext* context)
{
    char* text = NULL;
    size_t size = 0;
    FILE* stream = output ? open_memstream(&text, &size) : NULL;
    if (output && !stream) return false;
    bool success = AnalyzeOneFault(fault, number, csv, stream, stats, context);
    if (stream && fclose(stream) != 0) success = false;
    if (success && output) success = WriteCubeFile(output, fault, text, size);
    free(text);
    if (success && opt.progress == YES) {
        printf("\rProgress >> %d/%d", number, readdata.fault.numinit);
        fflush(stdout);
    }
    return success;
}

/* 既存InitTargetOrderの整列結果を使う。計算関数と出力順は直列・並列で共通。 */
static bool AnalyzeIndependentFaults(FaultAnalysisContext* context, FILE* result_fp,
                                     CubeOutput* cube_output, FaultStats* stats)
{
    int count;
    FNODE** faults = CopyTargetOrder(&count);
    if (!faults) return false;
    bool succeeded = true;
    if (opt.jobs > 1) {
        succeeded = FaultPoolRun(faults, count, opt.jobs, AnalyzeOneFault,
                                 FinishFaultAnalysis, context, result_fp, cube_output, stats);
    } else {
        for (int index = 0; index < count && succeeded; index++) {
            succeeded = AnalyzeSerialFault(faults[index], index + 1, result_fp, cube_output, stats, context);
        }
    }
    free(faults);
    return succeeded;
}

bool AnalyzeFaultDensity(double* out_time_cadical, double* out_time_bdd,
                         double* out_time_xid, double* out_time_read)
{
    FILE* result_fp = NULL;
    CubeOutput cube_output = {0};
    CubeOutput* cubes = opt.file.output.cube ? &cube_output : NULL;
    fileOpen(&result_fp, opt.file.output.fdp, "w");
    fprintf(result_fp, "net_name,f_type,cube_cnt,complete,fdp\n");
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
    if (succeeded && cubes) succeeded = InitCubeOutput(cubes, opt.file.output.cube);

    /* 準備後にforkし、各故障を独立して処理する。 */
    if (succeeded) {
        succeeded = AnalyzeIndependentFaults(&context, result_fp, cubes, &stats);
    }
    if (opt.jobs == 1) FinishFaultAnalysis(&context);
    FreeTargetOrder();
    if (fclose(result_fp) != 0) succeeded = false;
    FreeCubeOutput(&cube_output);
    if (!succeeded) return AFD_ERROR;

    *out_time_cadical = stats.cadical;
    *out_time_bdd = stats.bdd;
    *out_time_xid = stats.xid;
    *out_time_read = time_read;
    return AFD_OKAY;
}

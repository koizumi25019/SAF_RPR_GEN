#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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
    int progress_width;
} FaultAnalysisContext;

typedef struct {
    FNODE* fault;
    int fault_number;
    FaultAnalysisContext* analysis;
} SerialFaultProgress;

static bool ShowSerialFaultProgress(const FaultProgressState* state, void* opaque)
{
    SerialFaultProgress* context = opaque;
    int width = printf("\rProgress >> %d/%d | fault %d: %s %s loops=%d seeded=%d [%s]",
                       readdata.fault.numinit - readdata.fault.numrema, readdata.fault.numinit,
                       context->fault_number, context->fault->name, FaultTypeName(context->fault->type),
                       state->loops, state->seeded_cubes, FaultProgressPhaseName(state->phase));
    if (context->analysis->progress_width > width)
        printf("%*s", context->analysis->progress_width - width, "");
    context->analysis->progress_width = width;
    fflush(stdout);
    return true;
}

/* 計算が短い場合も開始・BDD移行・完了では正確な値を通知する。
 * 列挙中の通知は約1秒ごと。offなら時計参照も通信も行わない。
 */
static bool ReportFaultProgress(const FaultProgress* progress, const FaultProgressState* state,
                                struct timespec* last_update, bool force)
{
    if (!progress) return true;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = now.tv_sec - last_update->tv_sec +
                     (now.tv_nsec - last_update->tv_nsec) / 1e9;
    if (!force && elapsed < 1.0) return true;
    *last_update = now;
    return progress->update(state, progress->context);
}

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
                            FILE* cube_fp, FaultStats* stats,
                            const FaultProgress* progress, void* opaque)
{
    FaultAnalysisContext* context = opaque;
    SerialFaultProgress serial_context = { target, fault_number, context };
    FaultProgress serial_progress = { ShowSerialFaultProgress, &serial_context };
    if (opt.jobs == 1 && opt.progress == YES) progress = &serial_progress;
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
    FaultProgressState progress_state = { .seeded_cubes = seeded_cnt };
    struct timespec last_update = {0};
    if (!ReportFaultProgress(progress, &progress_state, &last_update, true)) {
        cubeset_free(&cubes);
        ccadical_release(solver);
        return false;
    }
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
            progress_state.phase = FAULT_PROGRESS_BDD;
            if (!ReportFaultProgress(progress, &progress_state, &last_update, true)) {
                cubeset_free(&cubes);
                succeeded = false;
                break;
            }
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
            succeeded = WriteFaultCubes(cube_fp, &result, &cubes);
            mpf_clear(result.density);
            stats->bdd += (double)(clock() - start) / CLOCKS_PER_SEC;

            /* 流用なしならこの故障で解放。流用ありは従来の参照数で管理する。 */
            target->cubes = cubes;
            if (!reuse || target->n_pending == 0) cubeset_free(&target->cubes);
            DropDeteFault(target);
            if (succeeded) {
                progress_state.phase = FAULT_PROGRESS_DONE;
                succeeded = ReportFaultProgress(progress, &progress_state, &last_update, true);
            }
            break;
        }
        start = clock();
        char* pattern = InlineXID(solver, target->netptr, -1, target->exc_netptr);
        stats->xid += (double)(clock() - start) / CLOCKS_PER_SEC;
        AddBlockingClauseFromCube(solver, pattern);
        cubeset_push(&cubes, pattern);
        progress_state.loops = cubes.n - seeded_cnt;
        if (!ReportFaultProgress(progress, &progress_state, &last_update, false)) {
            cubeset_free(&cubes);
            succeeded = false;
            break;
        }
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
    bool success = AnalyzeOneFault(fault, number, csv, stream, stats, NULL, context);
    if (stream && fclose(stream) != 0) success = false;
    if (success && output) success = WriteCubeFile(output, fault, text, size);
    free(text);
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
    if (succeeded && cubes) succeeded = InitCubeOutput(cubes, opt.file.output.cube);

    /* 準備後にforkする。キューブ流用なしでは、全故障を独立して配分できる。 */
    if (succeeded && opt.dom_reuse == NO) {
        succeeded = AnalyzeIndependentFaults(&context, result_fp, cubes, &stats);
    } else if (succeeded) {
        int fault_number = 0;
        while (readdata.fault.numrema && succeeded) {
            FNODE* target = SetTarget();
            succeeded = target && AnalyzeSerialFault(target, ++fault_number, result_fp, cubes, &stats, &context);
        }
    }
    if (opt.jobs == 1) FinishFaultAnalysis(&context);
    FreeTargetOrder();
    if (fclose(result_fp) != 0) succeeded = false;
    FreeCubeOutput(&cube_output);
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

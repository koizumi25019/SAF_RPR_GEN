#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fault_pool.h"
#include "gt_verify.h"
#include "fault_detection_prob.h"
#include "../opt/opt.h"

/* このファイルは故障の配分とプロセス間通信を担当する。
 * CNF→SAT→XID/CORE→BDD の計算内容は task（AnalyzeOneFault）側にある。
 *
 * 親: 起動 → 故障番号を送信 → 結果を受信 → 次の故障を送信 → 終了・回収
 * 子: 故障番号を受信 → 1故障を計算 → 結果を返信 → 次の番号を待つ
 *
 * fork後の回路・CNF・XID作業領域は各プロセスに独立して存在する。
 * 親子間で渡すのは故障番号、出力文字列、集計値のみ。キューブやBDDは渡さない。
 */
enum { NO_FAULT = -1 }; /* 親側では「担当なし」、通信では「終了要求」。 */

static double child_cpu;
static int worker_count = 1;
static volatile sig_atomic_t cancelled;

double FaultPoolChildCPU(void)
{
    return child_cpu;
}

int FaultPoolWorkers(void)
{
    return worker_count;
}

static void request_cancellation(int signal_number)
{
    (void)signal_number;
    cancelled = 1; /* シグナルハンドラ内ではフラグ変更だけを行う。 */
}

void FaultStatsAdd(FaultStats* sum, const FaultStats* part)
{
    sum->cadical += part->cadical;
    sum->bdd += part->bdd;
    sum->xid += part->xid;
    sum->total_cubes += part->total_cubes;
    sum->seeded_cubes += part->seeded_cubes;
    sum->xstat_bits += part->xstat_bits;
    sum->xstat_x += part->xstat_x;
    sum->gt_checked += part->gt_checked;
    sum->gt_unsound += part->gt_unsound;
    sum->gt_inexact += part->gt_inexact;
}

/* 同じlevelの故障はハッシュ表の走査順を保ち、従来SetTargetと同じ順にする。 */
typedef struct {
    FNODE* fault;
    int traversal_order;
} OrderedFault;

static int compare_fault_order(const void* a, const void* b)
{
    const OrderedFault* first = a;
    const OrderedFault* second = b;
    int first_level = first->fault->netptr->level;
    int second_level = second->fault->netptr->level;

    if (first_level != second_level) {
        return first_level < second_level ? -1 : 1;
    }
    return (first->traversal_order > second->traversal_order) -
           (first->traversal_order < second->traversal_order);
}

FNODE** FaultPoolOrder(int* count)
{
    *count = 0;
    for (int hash = 0; hash < MAXSIZE_HASH; hash++) {
        for (FNODE* fault = readdata.fault.list[hash]; fault; fault = fault->nextptr) {
            if (fault->detect == UNDETECTED) {
                (*count)++;
            }
        }
    }

    size_t capacity = (size_t)(*count ? *count : 1);
    OrderedFault* ordered = malloc(capacity * sizeof(*ordered));
    FNODE** faults = malloc(capacity * sizeof(*faults));
    if (!ordered || !faults) {
        free(ordered);
        free(faults);
        return NULL;
    }

    int index = 0;
    for (int hash = 0; hash < MAXSIZE_HASH; hash++) {
        for (FNODE* fault = readdata.fault.list[hash]; fault; fault = fault->nextptr) {
            if (fault->detect == UNDETECTED) {
                ordered[index].fault = fault;
                ordered[index].traversal_order = index;
                index++;
            }
        }
    }
    qsort(ordered, (size_t)*count, sizeof(*ordered), compare_fault_order);
    for (index = 0; index < *count; index++) {
        faults[index] = ordered[index].fault;
    }
    free(ordered);
    return faults; /* 配列だけを呼出側がfreeする。FNODE本体は回路側が所有する。 */
}

/* 以下のWorkerStateとFaultPoolは親側の管理情報。子の作業構造体ではない。 */
typedef struct {
    pid_t pid;
    int socket_fd;
    int fault_index; /* 現在担当しているfaults[]の添字。 */
} WorkerState;

/* 子→親の返信は [このヘッダ][CSV文字列][分析文字列] の順。
 * 同じ実行ファイルからforkした親子だけで使う内部形式（外部通信形式ではない）。
 */
typedef struct {
    int fault_index;
    int succeeded;
    size_t csv_size;
    size_t analysis_size;
    FaultStats stats;
} FaultReply;

/* 先に終わった故障の結果を親が一時保存し、故障順に出力する。 */
typedef struct {
    char* csv;
    char* analysis;
    size_t csv_size;
    size_t analysis_size;
    bool ready;
} FaultResult;

typedef struct {
    FNODE** faults;
    int fault_count;
    int worker_count;
    WorkerState* workers;
    struct pollfd* sockets;
    FaultResult* results; /* fault_count個。添字はfault_indexと同じ。 */

    int next_fault;       /* 次にワーカーへ配る故障。 */
    int completed_faults; /* 完了返信を受け取った故障数。 */
    int next_output;      /* 次にファイルへ出力する故障。 */

    FILE* csv_output;      /* 実ファイルへの書き込みは親だけが行う。 */
    FILE* analysis_output;
    FaultStats* stats;
} FaultPool;

typedef struct {
    struct sigaction interrupt;
    struct sigaction terminate;
    struct sigaction broken_pipe;
} SavedSignalHandlers;

/* socketは一度で全バイトを送受信できるとは限らないので、完了まで繰り返す。
 * 通信相手の終了・I/Oエラー・中断はfalseとして上位へ通知する。
 */
static bool transfer_bytes(int socket_fd, void* data, size_t size, bool writing)
{
    char* cursor = data;
    while (size > 0) {
        ssize_t transferred = writing ? write(socket_fd, cursor, size)
                                      : read(socket_fd, cursor, size);
        if (transferred < 0 && errno == EINTR && !cancelled) {
            continue;
        }
        if (transferred <= 0 || cancelled) {
            return false;
        }
        cursor += transferred;
        size -= (size_t)transferred;
    }
    return true;
}

static bool send_bytes(int socket_fd, void* data, size_t size)
{
    return transfer_bytes(socket_fd, data, size, true);
}

static bool receive_bytes(int socket_fd, void* data, size_t size)
{
    return transfer_bytes(socket_fd, data, size, false);
}

/* 子専用: 1故障の出力をメモリ上に作り、返信を送ったら文字列を解放する。
 * open_memstreamの文字列とサイズはfclose時に確定する。
 */
static bool analyze_and_reply(int socket_fd, FNODE* fault, int fault_index,
                              FaultTask task, void* context)
{
    FaultReply reply = { .fault_index = fault_index };
    char* csv = NULL;
    char* analysis = NULL;
    FILE* csv_stream = open_memstream(&csv, &reply.csv_size);
    FILE* analysis_stream = NULL;
    if (opt.file.input.cube_analysis) {
        analysis_stream = open_memstream(&analysis, &reply.analysis_size);
    }
    if (!csv_stream || (opt.file.input.cube_analysis && !analysis_stream)) {
        _exit(EXIT_FAILURE); /* 親が通信切断を検出して残りの子も終了させる。 */
    }

    reply.succeeded = task(fault, fault_index + 1, csv_stream, analysis_stream,
                           &reply.stats, context);
    if (fclose(csv_stream) != 0) {
        reply.succeeded = false;
    }
    if (analysis_stream && fclose(analysis_stream) != 0) {
        reply.succeeded = false;
    }
    if (!reply.succeeded) {
        reply.csv_size = 0;
        reply.analysis_size = 0; /* 失敗した故障の部分結果は親へ渡さない。 */
    }

    bool sent = send_bytes(socket_fd, &reply, sizeof(reply)) &&
                send_bytes(socket_fd, csv, reply.csv_size) &&
                send_bytes(socket_fd, analysis, reply.analysis_size);
    free(csv);
    free(analysis);
    return sent && reply.succeeded;
}

/* 子専用: 同じプロセスで繰り返し故障を処理し、BDD/XID作業領域を再利用する。 */
static void worker_loop(int socket_fd, FNODE** faults, int fault_count,
                        FaultTask task, FaultFinish finish, void* context)
{
    GT_SuppressSummary(); /* 全ワーカーの検証サマリーは親が一度だけ表示する。 */
    for (;;) {
        int fault_index;
        if (!receive_bytes(socket_fd, &fault_index, sizeof(fault_index))) {
            _exit(EXIT_FAILURE);
        }
        if (fault_index == NO_FAULT) {
            break;
        }
        if (fault_index < 0 || fault_index >= fault_count) {
            _exit(EXIT_FAILURE);
        }
        if (!analyze_and_reply(socket_fd, faults[fault_index], fault_index, task, context)) {
            _exit(EXIT_FAILURE);
        }
    }
    finish(context);
    close(socket_fd);
    exit(EXIT_SUCCESS); /* 正常終了ではプロセスごとのatexit診断も実行する。 */
}

static void install_pool_signal_handlers(SavedSignalHandlers* saved)
{
    struct sigaction cancellation_handler = {0};
    cancellation_handler.sa_handler = request_cancellation;
    sigemptyset(&cancellation_handler.sa_mask);

    struct sigaction ignored = {0};
    ignored.sa_handler = SIG_IGN;
    sigemptyset(&ignored.sa_mask);

    cancelled = 0;
    sigaction(SIGINT, &cancellation_handler, &saved->interrupt);
    sigaction(SIGTERM, &cancellation_handler, &saved->terminate);
    /* 子が死んでもSIGPIPEで親を即終了させず、通信エラーとして後始末する。 */
    sigaction(SIGPIPE, &ignored, &saved->broken_pipe);
}

static void restore_signal_handlers(const SavedSignalHandlers* saved)
{
    sigaction(SIGINT, &saved->interrupt, NULL);
    sigaction(SIGTERM, &saved->terminate, NULL);
    sigaction(SIGPIPE, &saved->broken_pipe, NULL);
}

/* 親専用: ワーカー1個を起動する。fork直後の子だけworker_loopへ進む。 */
static bool start_worker(FaultPool* pool, int worker_index, FaultTask task,
                         FaultFinish finish, void* context)
{
    int socket_pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair) < 0) {
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        close(socket_pair[0]);
        close(socket_pair[1]);
        return false;
    }
    if (pid == 0) {
        close(socket_pair[0]); /* 子は自分の通信口だけを保持する。 */
        for (int previous = 0; previous < worker_index; previous++) {
            close(pool->workers[previous].socket_fd);
        }
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        fclose(pool->csv_output);
        if (pool->analysis_output) {
            fclose(pool->analysis_output);
        }
        worker_loop(socket_pair[1], pool->faults, pool->fault_count, task, finish, context);
    }

    close(socket_pair[1]); /* 親は親側の通信口だけを保持する。 */
    pool->workers[worker_index] = (WorkerState){
        .pid = pid,
        .socket_fd = socket_pair[0],
        .fault_index = NO_FAULT
    };
    pool->sockets[worker_index].fd = socket_pair[0];
    return true;
}

/* 親専用: 空いたワーカーへ次の故障を1個配る。全配分済みなら待機状態にする。 */
static bool assign_next_fault(FaultPool* pool, int worker_index)
{
    WorkerState* worker = &pool->workers[worker_index];
    if (pool->next_fault == pool->fault_count) {
        worker->fault_index = NO_FAULT;
        pool->sockets[worker_index].events = 0;
        return true;
    }
    worker->fault_index = pool->next_fault++;
    pool->sockets[worker_index].events = POLLIN;
    return send_bytes(worker->socket_fd, &worker->fault_index, sizeof(worker->fault_index));
}

/* 親専用: 返信を担当故障と照合し、親の結果バッファへ受け取る。 */
static bool receive_fault_result(FaultPool* pool, int worker_index)
{
    WorkerState* worker = &pool->workers[worker_index];
    int fault_index = worker->fault_index;
    FaultReply reply;
    if (fault_index < 0 ||
        !receive_bytes(worker->socket_fd, &reply, sizeof(reply)) ||
        !reply.succeeded || reply.fault_index != fault_index ||
        pool->results[fault_index].ready ||
        reply.csv_size == SIZE_MAX || reply.analysis_size == SIZE_MAX) {
        fprintf(stderr, "[PARALLEL] worker %ld failed at fault %d\n",
                (long)worker->pid, fault_index);
        return false;
    }

    FaultResult* result = &pool->results[fault_index];
    result->csv_size = reply.csv_size;
    result->analysis_size = reply.analysis_size;
    result->csv = malloc(reply.csv_size + 1);
    if (reply.analysis_size > 0) {
        result->analysis = malloc(reply.analysis_size + 1);
    }
    if (!result->csv || (reply.analysis_size && !result->analysis) ||
        !receive_bytes(worker->socket_fd, result->csv, reply.csv_size) ||
        !receive_bytes(worker->socket_fd, result->analysis, reply.analysis_size)) {
        return false;
    }
    result->ready = true;
    FaultStatsAdd(pool->stats, &reply.stats);

    /* 子でのDropDeteFaultは子のコピーにだけ作用するので、親側にも完了を反映。 */
    TARGET target = { .num = 1, .list = &pool->faults[fault_index] };
    DropDeteFault(&target);
    pool->completed_faults++;
    return true;
}

/* 親専用: 完了順がB→Aでも、ファイルにはA→Bと出す。出力後にバッファを解放。 */
static bool write_ready_results(FaultPool* pool)
{
    while (pool->next_output < pool->fault_count &&
           pool->results[pool->next_output].ready) {
        FaultResult* result = &pool->results[pool->next_output++];
        if (fwrite(result->csv, 1, result->csv_size, pool->csv_output) != result->csv_size) {
            return false;
        }
        if (result->analysis_size > 0 &&
            (!pool->analysis_output ||
             fwrite(result->analysis, 1, result->analysis_size, pool->analysis_output) !=
             result->analysis_size)) {
            return false;
        }
        free(result->csv);
        free(result->analysis);
        result->csv = NULL;
        result->analysis = NULL;
    }
    return true;
}

/* 親専用: 終わったワーカーを待ち、その結果を処理して次の故障を渡す。 */
static bool collect_fault_results(FaultPool* pool)
{
    while (pool->completed_faults < pool->fault_count && !cancelled) {
        int ready_sockets = poll(pool->sockets, (nfds_t)pool->worker_count, -1);
        if (ready_sockets < 0 && errno == EINTR) {
            continue;
        }
        if (ready_sockets < 0) {
            return false;
        }
        for (int index = 0; index < pool->worker_count; index++) {
            if (!pool->sockets[index].revents) {
                continue;
            }
            if (!receive_fault_result(pool, index) ||
                !write_ready_results(pool) ||
                !assign_next_fault(pool, index)) {
                return false;
            }
        }
    }
    return !cancelled && pool->completed_faults == pool->fault_count;
}

static bool send_stop_requests(FaultPool* pool)
{
    int stop = NO_FAULT;
    for (int index = 0; index < pool->worker_count; index++) {
        if (!send_bytes(pool->workers[index].socket_fd, &stop, sizeof(stop))) {
            return false;
        }
    }
    return true;
}

static void terminate_workers(FaultPool* pool)
{
    for (int index = 0; index < pool->worker_count; index++) {
        if (pool->workers[index].pid > 0) {
            kill(pool->workers[index].pid, SIGTERM);
        }
    }
}

/* 正常終了でも失敗でも、起動した子をすべてwaitpidして回収する。 */
static bool wait_for_workers(FaultPool* pool)
{
    bool succeeded = true;
    for (int index = 0; index < pool->worker_count; index++) {
        WorkerState* worker = &pool->workers[index];
        if (worker->socket_fd >= 0) {
            close(worker->socket_fd);
        }
        if (worker->pid <= 0) {
            continue; /* 起動途中の失敗で、まだ作成されていないワーカー。 */
        }
        int status = 0;
        pid_t waited;
        do {
            waited = waitpid(worker->pid, &status, 0);
            if (waited < 0 && errno == EINTR && cancelled) {
                terminate_workers(pool);
            }
        } while (waited < 0 && errno == EINTR);
        if (waited < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            succeeded = false;
        }
        worker->pid = 0; /* 回収済みのPIDへ後からシグナルを送らない。 */
    }
    return succeeded;
}

static void free_pool_buffers(FaultPool* pool)
{
    if (pool->results) {
        for (int index = 0; index < pool->fault_count; index++) {
            free(pool->results[index].csv);
            free(pool->results[index].analysis);
        }
    }
    free(pool->workers);
    free(pool->sockets);
    free(pool->results);
}

static double usage_seconds(const struct rusage* usage)
{
    return usage->ru_utime.tv_sec + usage->ru_utime.tv_usec * 1e-6 +
           usage->ru_stime.tv_sec + usage->ru_stime.tv_usec * 1e-6;
}

/* 全体の入口。詳しい通信・出力・終了処理は上の役割別関数を参照する。 */
bool FaultPoolRun(FNODE** faults, int count, int jobs, FaultTask task,
                  FaultFinish finish, void* context, FILE* csv, FILE* analysis,
                  FaultStats* stats)
{
    if (count == 0) {
        return true;
    }
    worker_count = jobs < count ? jobs : count;
    FaultPool pool = {
        .faults = faults,
        .fault_count = count,
        .worker_count = worker_count,
        .csv_output = csv,
        .analysis_output = analysis,
        .stats = stats
    };
    pool.workers = calloc((size_t)worker_count, sizeof(*pool.workers));
    pool.sockets = calloc((size_t)worker_count, sizeof(*pool.sockets));
    pool.results = calloc((size_t)count, sizeof(*pool.results));
    if (!pool.workers || !pool.sockets || !pool.results) {
        free_pool_buffers(&pool);
        fprintf(stderr, "[PARALLEL] allocation failed\n");
        return false;
    }
    for (int index = 0; index < worker_count; index++) {
        pool.workers[index].socket_fd = -1;
    }

    SavedSignalHandlers saved_handlers;
    install_pool_signal_handlers(&saved_handlers);
    struct rusage cpu_before, cpu_after;
    getrusage(RUSAGE_CHILDREN, &cpu_before);

    /* 1. 起動。fork前に出力をflushし、子による二重出力を防ぐ。 */
    bool succeeded = fflush(NULL) == 0;
    for (int index = 0; succeeded && index < worker_count; index++) {
        succeeded = start_worker(&pool, index, task, finish, context) &&
                    assign_next_fault(&pool, index);
    }
    fprintf(stderr, "[PARALLEL] workers=%d faults=%d reuse=off\n", worker_count, count);

    /* 2. 完了返信を待つ → 出力・集計 → 空いたワーカーへ次の故障を配る。 */
    if (succeeded) {
        succeeded = collect_fault_results(&pool);
    }

    /* 3. 正常時は終了要求、失敗・中断時は強制終了。その後は必ず全員を回収。 */
    if (succeeded) {
        succeeded = send_stop_requests(&pool);
    }
    if (!succeeded) {
        terminate_workers(&pool);
    }
    bool workers_succeeded = wait_for_workers(&pool);
    succeeded = succeeded && workers_succeeded && !cancelled;

    /* 4. 全ワーカーのuser+system CPU時間を合計。実経過時間とは別の指標。 */
    getrusage(RUSAGE_CHILDREN, &cpu_after);
    child_cpu = usage_seconds(&cpu_after) - usage_seconds(&cpu_before);
    restore_signal_handlers(&saved_handlers);
    free_pool_buffers(&pool);
    if (!succeeded) {
        fprintf(stderr, "[PARALLEL] failed; output is partial, not a completed run\n");
    }
    return succeeded;
}

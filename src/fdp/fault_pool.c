#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "fault_pool.h"
#include "gt_verify.h"
#include "fault_detection_prob.h"
#include "../opt/opt.h"

static double child_cpu;
static int worker_count = 1;
static volatile sig_atomic_t cancelled;

double FaultPoolChildCPU(void) { return child_cpu; }
int FaultPoolWorkers(void) { return worker_count; }
static void cancel_pool(int signal_number) { (void)signal_number; cancelled = 1; }

void FaultStatsAdd(FaultStats* sum, const FaultStats* part) {
    sum->cadical += part->cadical; sum->bdd += part->bdd; sum->xid += part->xid;
    sum->total_cubes += part->total_cubes; sum->seeded_cubes += part->seeded_cubes;
    sum->xstat_bits += part->xstat_bits; sum->xstat_x += part->xstat_x;
    sum->gt_checked += part->gt_checked; sum->gt_unsound += part->gt_unsound;
    sum->gt_inexact += part->gt_inexact;
}

typedef struct { FNODE* fault; int rank; } OrderedFault;
static int compare_faults(const void* a, const void* b) {
    const OrderedFault *x = a, *y = b;
    if (x->fault->netptr->level != y->fault->netptr->level)
        return x->fault->netptr->level < y->fault->netptr->level ? -1 : 1;
    return (x->rank > y->rank) - (x->rank < y->rank);
}

FNODE** FaultPoolOrder(int* count) {
    *count = 0;
    for (int h = 0; h < MAXSIZE_HASH; h++)
        for (FNODE* f = readdata.fault.list[h]; f; f = f->nextptr)
            if (f->detect == UNDETECTED) (*count)++;
    size_t n = (size_t)(*count ? *count : 1);
    OrderedFault* ordered = malloc(n * sizeof(*ordered));
    FNODE** faults = malloc(n * sizeof(*faults));
    if (!ordered || !faults) { free(ordered); free(faults); return NULL; }
    int i = 0;
    for (int h = 0; h < MAXSIZE_HASH; h++)
        for (FNODE* f = readdata.fault.list[h]; f; f = f->nextptr)
            if (f->detect == UNDETECTED) {
                ordered[i].fault = f; ordered[i].rank = i; i++;
            }
    qsort(ordered, (size_t)*count, sizeof(*ordered), compare_faults);
    for (i = 0; i < *count; i++) faults[i] = ordered[i].fault;
    free(ordered);
    return faults;
}

typedef struct { pid_t pid; int fd, job; } Worker;
typedef struct {
    int id, okay;
    size_t csv_size, analysis_size;
    FaultStats stats;
} Reply;
typedef struct {
    char *csv, *analysis;
    size_t csv_size, analysis_size;
    int ready;
} Completed;

/* Stream framing handles short reads/writes and EINTR. SIGPIPE is suppressed
   during the pool so a dead worker is reported as a failed run. */
static bool transfer(int fd, void* data, size_t length, bool writing) {
    char* cursor = data;
    while (length) {
        ssize_t n = writing ? write(fd, cursor, length) : read(fd, cursor, length);
        if (n < 0 && errno == EINTR && !cancelled) continue;
        if (n <= 0 || cancelled) return false;
        cursor += n; length -= (size_t)n;
    }
    return true;
}

static void worker_loop(int fd, FNODE** faults, int count, FaultTask task,
                        FaultFinish finish, void* context) {
    GT_SuppressSummary();
    for (;;) {
        int id;
        if (!transfer(fd, &id, sizeof(id), false)) _exit(EXIT_FAILURE);
        if (id == -1) break;
        if (id < 0 || id >= count) _exit(EXIT_FAILURE);
        Reply reply = { .id = id };
        char *csv = NULL, *analysis = NULL;
        FILE* out = open_memstream(&csv, &reply.csv_size);
        FILE* series = NULL;
        if (opt.file.input.cube_analysis)
            series = open_memstream(&analysis, &reply.analysis_size);
        if (!out || (opt.file.input.cube_analysis && !series)) _exit(EXIT_FAILURE);
        reply.okay = task(faults[id], id + 1, out, series, &reply.stats, context);
        if (fclose(out) != 0) reply.okay = 0;
        if (series && fclose(series) != 0) reply.okay = 0;
        if (!reply.okay) reply.csv_size = reply.analysis_size = 0;
        bool sent = transfer(fd, &reply, sizeof(reply), true) &&
                    transfer(fd, csv, reply.csv_size, true) &&
                    transfer(fd, analysis, reply.analysis_size, true);
        free(csv); free(analysis);
        if (!sent || !reply.okay) _exit(EXIT_FAILURE);
    }
    finish(context);
    close(fd);
    exit(EXIT_SUCCESS); /* Run process-local diagnostics' atexit handlers. */
}

static double usage_seconds(const struct rusage* usage) {
    return usage->ru_utime.tv_sec + usage->ru_utime.tv_usec * 1e-6 +
           usage->ru_stime.tv_sec + usage->ru_stime.tv_usec * 1e-6;
}

bool FaultPoolRun(FNODE** faults, int count, int jobs, FaultTask task,
                  FaultFinish finish, void* context, FILE* csv, FILE* analysis,
                  FaultStats* stats) {
    if (!count) return true;
    worker_count = jobs < count ? jobs : count;
    Worker* workers = calloc((size_t)worker_count, sizeof(*workers));
    struct pollfd* watch = calloc((size_t)worker_count, sizeof(*watch));
    Completed* completed = calloc((size_t)count, sizeof(*completed));
    if (!workers || !watch || !completed) {
        free(workers); free(watch); free(completed);
        fprintf(stderr, "[PARALLEL] allocation failed\n"); return false;
    }
    for (int i = 0; i < worker_count; i++) workers[i].fd = -1;
    struct sigaction handler = {0}, ignored = {0}, old_int, old_term, old_pipe;
    handler.sa_handler = cancel_pool; sigemptyset(&handler.sa_mask);
    ignored.sa_handler = SIG_IGN; sigemptyset(&ignored.sa_mask);
    /* No threads exist at this point; all process-local mutable state is COW. */
    cancelled = 0;
    sigaction(SIGINT, &handler, &old_int);
    sigaction(SIGTERM, &handler, &old_term);
    sigaction(SIGPIPE, &ignored, &old_pipe);
    struct rusage before, after;
    getrusage(RUSAGE_CHILDREN, &before);
    bool okay = true;
    int next = 0, done = 0, emitted = 0;
    /* Children must not flush a second copy of parent output. */
    if (fflush(NULL) != 0) okay = false;
    for (int i = 0; okay && i < worker_count; i++) {
        int pair[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) < 0) { okay = false; break; }
        pid_t pid = fork();
        if (pid < 0) { close(pair[0]); close(pair[1]); okay = false; break; }
        if (pid == 0) {
            close(pair[0]);
            for (int j = 0; j < i; j++) close(workers[j].fd);
            signal(SIGINT, SIG_DFL); signal(SIGTERM, SIG_DFL);
            fclose(csv); if (analysis) fclose(analysis);
            worker_loop(pair[1], faults, count, task, finish, context);
        }
        close(pair[1]);
        workers[i] = (Worker){ .pid = pid, .fd = pair[0], .job = next++ };
        watch[i].fd = pair[0]; watch[i].events = POLLIN;
        if (!transfer(pair[0], &workers[i].job, sizeof(int), true)) { okay = false; break; }
    }
    fprintf(stderr, "[PARALLEL] workers=%d faults=%d reuse=off\n", worker_count, count);
    while (okay && done < count && !cancelled) {
        int ready = poll(watch, (nfds_t)worker_count, -1);
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0) { okay = false; break; }
        for (int i = 0; i < worker_count && okay; i++) {
            if (!watch[i].revents) continue;
            Reply reply;
            int id = workers[i].job;
            if (id < 0 || !transfer(workers[i].fd, &reply, sizeof(reply), false) ||
                !reply.okay || reply.id != id || completed[id].ready ||
                reply.csv_size == SIZE_MAX || reply.analysis_size == SIZE_MAX) {
                fprintf(stderr, "[PARALLEL] worker %ld failed at fault %d\n", (long)workers[i].pid, id);
                okay = false; break;
            }
            Completed* result = completed + id;
            result->csv_size = reply.csv_size; result->analysis_size = reply.analysis_size;
            result->csv = malloc(reply.csv_size + 1);
            if (reply.analysis_size) result->analysis = malloc(reply.analysis_size + 1);
            if (!result->csv || (reply.analysis_size && !result->analysis) ||
                !transfer(workers[i].fd, result->csv, reply.csv_size, false) ||
                !transfer(workers[i].fd, result->analysis, reply.analysis_size, false)) {
                okay = false; break;
            }
            result->ready = 1;
            FaultStatsAdd(stats, &reply.stats);
            TARGET target = { .num = 1, .list = faults + id };
            DropDeteFault(&target);
            done++;
            /* Completion order does not alter CSV/analysis row order. */
            while (emitted < count && completed[emitted].ready) {
                Completed* r = completed + emitted++;
                if (fwrite(r->csv, 1, r->csv_size, csv) != r->csv_size ||
                    (r->analysis_size && (!analysis ||
                     fwrite(r->analysis, 1, r->analysis_size, analysis) != r->analysis_size))) {
                    okay = false; break;
                }
                free(r->csv); free(r->analysis); r->csv = r->analysis = NULL;
            }
            if (next < count) {
                workers[i].job = next++;
                if (!transfer(workers[i].fd, &workers[i].job, sizeof(int), true)) okay = false;
            } else {
                workers[i].job = -1; watch[i].events = 0;
            }
        }
    }
    okay = okay && !cancelled && done == count;
    if (okay) {
        int stop = -1;
        for (int i = 0; i < worker_count; i++)
            if (!transfer(workers[i].fd, &stop, sizeof(stop), true)) okay = false;
    }
    if (!okay)
        for (int i = 0; i < worker_count; i++)
            if (workers[i].pid > 0) kill(workers[i].pid, SIGTERM);
    for (int i = 0; i < worker_count; i++) {
        if (workers[i].fd >= 0) close(workers[i].fd);
        if (workers[i].pid > 0) {
            int status = 0;
            pid_t pid;
            do {
                pid = waitpid(workers[i].pid, &status, 0);
                if (pid < 0 && errno == EINTR && cancelled)
                    for (int j = i; j < worker_count; j++)
                        if (workers[j].pid > 0) kill(workers[j].pid, SIGTERM);
            } while (pid < 0 && errno == EINTR);
            if (pid < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) okay = false;
        }
    }
    getrusage(RUSAGE_CHILDREN, &after);
    child_cpu = usage_seconds(&after) - usage_seconds(&before);
    okay = okay && !cancelled;
    sigaction(SIGINT, &old_int, NULL); sigaction(SIGTERM, &old_term, NULL);
    sigaction(SIGPIPE, &old_pipe, NULL);
    for (int i = 0; i < count; i++) { free(completed[i].csv); free(completed[i].analysis); }
    free(workers); free(watch); free(completed);
    if (!okay) fprintf(stderr, "[PARALLEL] failed; output is partial, not a completed run\n");
    return okay;
}

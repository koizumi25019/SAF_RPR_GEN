/* Fixed-seed native clasp benchmark; not a CTest (runtime depends on samples).
 * The structural TFO is marked below; NormalScopeBuild itself selects affected
 * POs and their TFI, exactly as in the application. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fdp/sbxid/sbxid.h"
#include "fdp/normal_scope.h"

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

static int net_id(const char* name)
{
    for (int i = 0; i < n_net; ++i)
        if (strcmp(nl[i].name, name) == 0) return i;
    return -1;
}

static void build_scope(int fault, unsigned char* mask, int* queue)
{
    for (int i = 0; i < n_net; ++i) nl[i].flag = 0;
    int front = 0, back = 0;
    nl[fault].flag = TFO;
    queue[back++] = fault;
    while (front < back) {
        NLIST* gate = &nl[queue[front++]];
        for (int j = 0; j < gate->n_out; ++j) {
            NLIST* out = gate->out[j];
            if (out->flag & TFO) continue;
            out->flag |= TFO;
            queue[back++] = out->n;
        }
    }
    FNODE target = { .netptr = &nl[fault] };
    NormalScopeBuild(&target);
    for (int p = 0; p < n_pi; ++p)
        mask[p] = NormalScopeRequiredNet(pi[p]->n) != 0;
}

int main(int argc, char** argv)
{
    if (argc != 9 || (strcmp(argv[5], "all") && strcmp(argv[5], "scope"))) {
        fprintf(stderr, "usage: %s NET PINS FAULTS PATTERNS all|scope SAMPLES ROWS OUTPUT\n", argv[0]);
        return 2;
    }
    int scoped = strcmp(argv[5], "scope") == 0;
    int samples = atoi(argv[6]), rows = atoi(argv[7]);
    assert(samples >= 2 && samples <= rows);
    read_nl(argv[1]);
    FILE* pins = fopen(argv[2], "r"), *faults = fopen(argv[3], "r");
    FILE* patterns = fopen(argv[4], "r"), *output = fopen(argv[8], "w");
    assert(pins && faults && patterns && output);
    int* pin_order = malloc((size_t)n_pi * sizeof(*pin_order));
    int* queue = malloc((size_t)n_net * sizeof(*queue));
    unsigned char* mask = malloc((size_t)n_pi);
    char* original = malloc((size_t)n_pi + 2), *seed = malloc((size_t)n_pi + 1);
    char* cube = malloc((size_t)n_pi + 1);
    assert(pin_order && queue && mask && original && seed && cube);
    char name[512], kind[32];
    for (int p = 0; p < n_pi; ++p) {
        assert(fscanf(pins, "%511s", name) == 1);
        int id = net_id(name);
        assert(id >= 0);
        pin_order[p] = -1;
        for (int q = 0; q < n_pi; ++q) if (pi[q]->n == id) pin_order[p] = q;
        assert(pin_order[p] >= 0);
    }
    setenv("FDP_SBXID_VERIFY", "1", 1);
    fprintf(output, "index,name,stuck,n_pi,scope_pis,variables,constraints,status,timed_out,care,cube,search_seconds,total_seconds,search_limit_seconds\n");
    int used = 0;
    for (int row = 0; row < rows; ++row) {
        assert(fscanf(faults, "%511s %31s", name, kind) == 2);
        assert(fgets(original, n_pi + 2, patterns));
        assert(strcspn(original, "\r\n") == (size_t)n_pi);
        if (used >= samples || row != (int)((long long)used * (rows - 1) / (samples - 1))) continue;
        ++used;
        for (int p = 0; p < n_pi; ++p) seed[pin_order[p]] = original[p];
        seed[n_pi] = '\0';
        int id = net_id(name), scope_pis = 0;
        assert(id >= 0 && strlen(kind) == 3 && !strncmp(kind, "sa", 2));
        build_scope(id, mask, queue);
        for (int p = 0; p < n_pi; ++p) scope_pis += mask[p] != 0;
        SbxidInfo info;
        double start = now();
        assert(sbxid_generalize(seed, id, kind[2] == '1', scoped ? mask : NULL, cube, &info));
        double elapsed = now() - start;
        assert(!info.verification_failed);
        if (info.solve.status != SBXID_SOLVE_UNKNOWN) assert(info.verified);
        int care = 0;
        for (int p = 0; p < n_pi; ++p) {
            care += cube[p] != 'X';
            if (scoped && !mask[p] && info.solve.status != SBXID_SOLVE_UNKNOWN)
                assert(cube[p] == 'X');
        }
        fprintf(output, "%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s,%.9f,%.9f,%.1f\n",
                row, name, kind[2] == '1', n_pi, scope_pis, info.variables,
                info.constraints, info.solve.status, info.solve.timed_out,
                care, cube, info.solve.seconds, elapsed, SBXID_SOLVE_LIMIT_SECONDS);
        fflush(output);
    }
    assert(used == samples);
    fclose(output); fclose(pins); fclose(faults); fclose(patterns);
    free(pin_order); free(queue); free(mask); free(original); free(seed); free(cube);
    sbxid_release(); NormalScopeRelease(); free_netlist();
    return 0;
}

/* Normal CNF support for baseline SAF: retain the entire TFI of the fault
 * TFO and every normal signal fixed by EssentialAssignment. All original
 * ccadical_add calls stay direct and unchanged. The scope is built once
 * after generation constraints, not observed while clauses are inserted.
 * Omitted gates extend uniquely from a full PI assignment; FDP still counts
 * over ALL PIs. After fork, each persistent worker owns its mutable scope state.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "normal_scope.h"
#include "essential_assignment.h"

static NLIST **scope_stack;
static unsigned char *needed;
static int top;
static unsigned long long instances, full, kept, free_pi;

static void fail(const char *message) {
    fprintf(stderr, "[NORMAL_SCOPE] %s\n", message); exit(1);
}
static void mark(NLIST *gate) {
    if (!needed[gate->n]) {
        needed[gate->n] = 1;
        scope_stack[top++] = gate;
    }
}
void NormalScopeBuild(void) {
    if (!needed) {
        needed = calloc((size_t)n_net, 1);
        scope_stack = malloc((size_t)n_net * sizeof(*scope_stack));
        if (!needed || !scope_stack) fail("allocation failed");
    }
    memset(needed, 0, (size_t)n_net);
    top = 0;
    int use_ea = !getenv("MDC_NOEA");
    for (int i = 0; i < n_net; i++) {
        /* Faulty gate side inputs, D-chain, detection outputs and excitation
         * all refer to the TFO or its fanins. EA may propagate farther, so
         * include every EA_UP signal whose unit clause was actually added. */
        if ((nl[i].flag & TFO) == TFO || (use_ea && nl[i].ea_flag == EA_UP))
            mark(nl + i);
    }
    while (top) {
        NLIST *gate = scope_stack[--top];
        if (gate->type == IN || gate->type == DFF) continue;
        for (int j = 0; j < gate->n_in; j++) mark(gate->in[j]);
    }
    for (int i = 0; i < n_net; i++) if (nl[i].consgc) {
        full++; if (needed[i]) kept++;
    }
    for (int i = 0; i < n_pi; i++) if (!needed[pi[i]->n]) free_pi++;
    instances++;
}
int NormalScopeRequiredNet(int index) {
    return !needed || needed[index];
}
void NormalScopeRelease(void) {
    if (instances) fprintf(stderr,
        "[NORMAL_SCOPE] faults=%llu normal_gates=%llu/%llu outside_pi=%llu\n",
        instances, kept, full, free_pi);
    free(scope_stack); free(needed);
    scope_stack = NULL; needed = NULL;
    top = 0;
    instances = full = kept = free_pi = 0;
    NormalScopeModelRelease();
}

/* Retain every normal signal referenced by generation constraints and its
 * entire transitive fanin, including side inputs. Omitted gate definitions
 * can be extended uniquely from any complete PI assignment. Never count
 * unrestricted internal SAT assignments: FDP is projected onto ALL PIs.
 * Like the current CNF/XID pipeline, this context is single-threaded.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "normal_scope.h"
#include "cnf/cnf.h"
#include "cnf_dump.h"

static NLIST **byvar, **scope_stack;
static unsigned char *needed;
static int enabled = -1, collecting, top;
static unsigned long long instances, full, kept, free_pi;

static void fail(const char *message) {
    fprintf(stderr, "[NORMAL_SCOPE] %s\n", message); exit(1);
}
int NormalScopeEnabled(void) {
    if (enabled < 0) {
        const char *value = getenv("FDP_NORMAL_SCOPE");
        if (value && strcmp(value, "0") && strcmp(value, "1"))
            fail("FDP_NORMAL_SCOPE must be 0 or 1");
        enabled = value && !strcmp(value, "1");
    }
    return enabled;
}
static void mark(NLIST *gate) {
    if (!needed[gate->n]) {
        needed[gate->n] = 1;
        scope_stack[top++] = gate;
    }
}
void NormalScopeBegin(void) {
    if (!NormalScopeEnabled()) return;
    if (!needed) {
        needed = calloc((size_t)n_net, 1);
        scope_stack = malloc((size_t)n_net * sizeof(*scope_stack));
        byvar = calloc((size_t)cnf.constant.vars + 1, sizeof(*byvar));
        if (!needed || !scope_stack || !byvar) fail("allocation failed");
        for (int i = 0; i < n_net; i++) {
            unsigned int var = nl[i].varsgc;
            if (!var || var > cnf.constant.vars) fail("invalid normal variable");
            byvar[var] = nl + i;
        }
    }
    memset(needed, 0, (size_t)n_net);
    top = 0; collecting = 1;
    cnf_set_literal_observer(NormalScopeObserve);
}
void NormalScopeObserve(int literal) {
    if (!collecting || !literal) return;
    int var = literal < 0 ? -literal : literal;
    if (var <= cnf.constant.vars && byvar[var]) mark(byvar[var]);
}
void NormalScopeEnd(void) {
    if (!NormalScopeEnabled()) return;
    collecting = 0;
    cnf_set_literal_observer(NULL);
    while (top) {
        NLIST *gate = scope_stack[--top];
        /* DFF is a free pseudo input in the combinational model. TDF
         * expansion replaces capture-time DFFs with explicit BUF gates. */
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
    return !NormalScopeEnabled() || !needed || needed[index];
}
void NormalScopeRelease(void) {
    if (instances) fprintf(stderr,
        "[NORMAL_SCOPE] faults=%llu normal_gates=%llu/%llu outside_pi=%llu\n",
        instances, kept, full, free_pi);
    cnf_set_literal_observer(NULL);
    free(byvar); free(scope_stack); free(needed);
    byvar = scope_stack = NULL; needed = NULL;
    enabled = -1; collecting = top = 0;
    instances = full = kept = free_pi = 0;
    NormalScopeModelRelease();
}

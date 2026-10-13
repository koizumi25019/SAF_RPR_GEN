/* Eggersgluss et al., DDECS 2012, Section II-B / Fig. 2:
 * retain the TFI of observable outputs reached by the fault's TFO.
 * Build this structural scope BEFORE EA, so EA cannot enlarge it.
 * TDF excitation and power predicates additionally retain their real normal
 * dependencies. Non-detection oracles still load complete definitions.
 * Each forked worker owns these masks; FDP is projected onto ALL original PIs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "normal_scope.h"
#include "cnf/cnf.h"
#include "cnf_dump.h"
#include "fault_detection_prob.h"
#include "../opt/opt.h"

static NLIST **byvar, **scope_stack;
static unsigned char *needed, *detection_cone;
static int collecting, top;
static unsigned long long instances, full, kept, free_pi;

static void fail(const char *message) {
    fprintf(stderr, "[NORMAL_SCOPE] %s\n", message); exit(1);
}
int NormalScopeEnabled(void) {
    return 1; /* Generation is always scoped; keep the existing query API. */
}
static void mark(NLIST *gate) {
    if (!needed[gate->n]) {
        needed[gate->n] = 1;
        scope_stack[top++] = gate;
    }
}
void NormalScopeBegin(void) {
    if (!needed) {
        needed = calloc((size_t)n_net, 1);
        detection_cone = calloc((size_t)n_net, 1);
        scope_stack = malloc((size_t)n_net * sizeof(*scope_stack));
        byvar = calloc((size_t)cnf.constant.vars + 1, sizeof(*byvar));
        if (!needed || !detection_cone || !scope_stack || !byvar) fail("allocation failed");
        for (int i = 0; i < n_net; i++) {
            unsigned int var = nl[i].varsgc;
            if (!var || var > cnf.constant.vars) fail("invalid normal variable");
            byvar[var] = nl + i;
        }
    }
    memset(needed, 0, (size_t)n_net);
    memset(detection_cone, 0, (size_t)n_net);
    top = 0; collecting = 0;
    cnf_set_literal_observer(NULL);
}
static void close_fanins(void) {
    while (top) {
        NLIST *gate = scope_stack[--top];
        /* DFF is a free pseudo input in the combinational model. TDF
         * expansion replaces capture-time DFFs with explicit BUF gates. */
        if (gate->type == IN || gate->type == DFF) continue;
        for (int j = 0; j < gate->n_in; j++) mark(gate->in[j]);
    }
}
void NormalScopeSelectOutputs(void) {
    /* SearchTFO has marked the reachable signals. Select actual observation
     * points explicitly: PO for SAF, capture-time PPO for LOC/TDF. A dangling
     * fanout is not an output, and a non-observed TDF PO is not a capture. */
    for (int i = 0; i < n_net; i++) nl[i].flag &= ~TPO;
    NLIST **outputs = opt.fault_model == FM_TDF ? ppo : po;
    int output_count = opt.fault_model == FM_TDF ? n_ppo : n_po;
    numtranpo = 0;
    for (int i = 0; i < output_count; i++) {
        NLIST *output = outputs[i];
        if ((output->flag & TFO) != TFO) continue;
        output->flag |= TPO;
        numtranpo++;
        mark(output);
    }
    close_fanins();
    memcpy(detection_cone, needed, (size_t)n_net);

    /* Ordinary SAF needs exactly the above scope. Additional TDF activation
     * and global power constraints must also have defined normal signals.
     * EA is filtered through detection_cone and never expands either mask. */
    collecting = opt.fault_model == FM_TDF || opt.low_power == YES;
    cnf_set_literal_observer(collecting ? NormalScopeObserve : NULL);
}
int NormalScopeInDetectionCone(int index) {
    return !detection_cone || detection_cone[index];
}
void NormalScopeObserve(int literal) {
    if (!collecting || !literal) return;
    int var = literal < 0 ? -literal : literal;
    if (var <= cnf.constant.vars && byvar[var]) mark(byvar[var]);
}
void NormalScopeEnd(void) {
    collecting = 0;
    cnf_set_literal_observer(NULL);
    close_fanins();
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
    cnf_set_literal_observer(NULL);
    free(byvar); free(scope_stack); free(needed); free(detection_cone);
    byvar = scope_stack = NULL; needed = NULL;
    detection_cone = NULL;
    collecting = top = 0;
    instances = full = kept = free_pi = 0;
    NormalScopeModelRelease();
}

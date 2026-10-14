/* Eggersgluss et al., DDECS 2012, Section II-B / Fig. 2:
 * retain the TFI of observed outputs reached by the fault's TFO.
 * Build the scope BEFORE fault clauses and EA, so EA cannot enlarge it.
 * Baseline uses direct ccadical_add calls and always enables EA.
 * Omitted gates extend uniquely from a full PI assignment; FDP still counts
 * over ALL PIs. After fork, each persistent worker owns its mutable scope state.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "normal_scope.h"
#include "fault_detection_prob.h"
#include "../opt/opt.h"

static NLIST **scope_stack;
static unsigned char *needed, *detection, *blocked_pi;
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
static void close_inputs(void) {
    while (top) {
        NLIST *gate = scope_stack[--top];
        if (gate->type == IN || gate->type == DFF) continue;
        for (int j = 0; j < gate->n_in; j++) mark(gate->in[j]);
    }
}
void NormalScopeBuild(FNODE *target) {
    if (!needed) {
        needed = calloc((size_t)n_net, 1);
        detection = calloc((size_t)n_net, 1);
        scope_stack = malloc((size_t)n_net * sizeof(*scope_stack));
        blocked_pi = calloc((size_t)(n_pi ? n_pi : 1), 1);
        if (!needed || !detection || !scope_stack || !blocked_pi) fail("allocation failed");
    }
    memset(needed, 0, (size_t)n_net);
    memset(blocked_pi, 0, (size_t)n_pi);
    top = 0;
    /* Select SAF POs or capture-time TDF PPOs, not internally dangling ends.
     * TPO is also used by the comparator to identify observations. */
    for (int i = 0; i < n_net; i++) nl[i].flag &= ~TPO;
    numtranpo = 0;
    NLIST **outputs = opt.fault_model == FM_TDF ? ppo : po;
    int count = opt.fault_model == FM_TDF ? n_ppo : n_po;
    for (int i = 0; i < count; i++) {
        if ((outputs[i]->flag & TFO) != TFO || (outputs[i]->flag & TPO)) continue;
        outputs[i]->flag |= TPO;
        numtranpo++;
        mark(outputs[i]);
    }
    close_inputs();
    memcpy(detection, needed, (size_t)n_net);
    /* Keep the frame-1 excitation support without extending fault propagation. */
    if (target->exc_netptr) {
        mark(target->exc_netptr);
        close_inputs();
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
int NormalScopeInDetectionCone(int index) {
    return !detection || detection[index];
}
int NormalScopeRequiredPi(int index) {
    return NormalScopeRequiredNet(pi[index]->n) || (blocked_pi && blocked_pi[index]);
}
void NormalScopeRecordBlockingPi(int index) {
    if (blocked_pi) blocked_pi[index] = 1;
}
void NormalScopeRelease(void) {
    if (instances) fprintf(stderr,
        "[NORMAL_SCOPE] faults=%llu normal_gates=%llu/%llu outside_pi=%llu\n",
        instances, kept, full, free_pi);
    free(scope_stack); free(needed); free(detection); free(blocked_pi);
    scope_stack = NULL; needed = detection = blocked_pi = NULL;
    top = 0;
    instances = full = kept = free_pi = 0;
}

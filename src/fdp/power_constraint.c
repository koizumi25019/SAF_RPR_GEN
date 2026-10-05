#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "power_constraint.h"
#include "cnf/cnf.h"
#include "cnf_dump.h"
#include "../opt/opt.h"

typedef struct { int* data; size_t n, cap; int next, one; } Definition;
typedef struct { int* bit; int width; } Number;
static Definition cached;
static int predicate, signals, budget;

static void fail(void) { fprintf(stderr, "[POWER] allocation/variable limit failure\n"); exit(1); }
static int fresh(Definition* d) { if (d->next == INT_MAX) fail(); return ++d->next; }
static void emit(Definition* d, int lit)
{
    if (d->n == d->cap) {
        size_t capacity = d->cap ? 2 * d->cap : 256;
        int* memory = realloc(d->data, capacity * sizeof(int));
        if (!memory) fail();
        d->data = memory; d->cap = capacity;
    }
    d->data[d->n++] = lit;
}
static void clause(Definition* d, int a, int b, int c, int length)
{
    emit(d, a);
    if (length > 1) emit(d, b);
    if (length > 2) emit(d, c);
    emit(d, 0);
}
/* zero is a FALSE sentinel, never a clause literal. */
static int gate_and(Definition* d, int a, int b)
{
    if (!a || !b || a == -d->one || b == -d->one || a == -b) return 0;
    if (a == d->one || a == b) return b;
    if (b == d->one) return a;
    int z = fresh(d);
    clause(d, -z, a, 0, 2); clause(d, -z, b, 0, 2); clause(d, z, -a, -b, 3);
    return z;
}
static int gate_or(Definition* d, int a, int b)
{
    if (a == -d->one) a = 0;
    if (b == -d->one) b = 0;
    if (!a) return b;
    if (!b || a == b) return a;
    if (a == d->one || b == d->one || a == -b) return d->one;
    int z = fresh(d);
    clause(d, z, -a, 0, 2); clause(d, z, -b, 0, 2); clause(d, -z, a, b, 3);
    return z;
}
static int gate_xor(Definition* d, int a, int b)
{
    if (!a) return b;
    if (!b) return a;
    if (a == b) return 0;
    if (a == -b) return d->one;
    int z = fresh(d);
    clause(d, -a, -b, -z, 3); clause(d, a, b, -z, 3);
    clause(d, a, -b, z, 3); clause(d, -a, b, z, 3);
    return z;
}

/* Balanced binary addition uses O(n) gates overall. Sum/carry gates are
   equivalences, so both polarities of the final predicate are sound. */
static Number sum(Definition* d, const int* inputs, int n)
{
    if (n == 1) {
        int* bit = malloc(sizeof(int)); if (!bit) fail();
        bit[0] = inputs[0]; return (Number){bit, 1};
    }
    int half = n / 2;
    Number a = sum(d, inputs, half), b = sum(d, inputs + half, n - half);
    int width = a.width > b.width ? a.width : b.width;
    int* bit = malloc((size_t)(width + 1) * sizeof(int)); if (!bit) fail();
    int carry = 0;
    for (int i = 0; i < width; i++) {
        int ai = i < a.width ? a.bit[i] : 0, bi = i < b.width ? b.bit[i] : 0;
        int pair = gate_xor(d, ai, bi);
        bit[i] = gate_xor(d, pair, carry);
        int both = gate_and(d, ai, bi), propagated = gate_and(d, pair, carry);
        carry = gate_or(d, both, propagated);
    }
    bit[width] = carry;
    free(a.bit); free(b.bit);
    return (Number){bit, width + 1};
}
static void init(Definition* d, int base)
{
    d->next = base;
    d->one = fresh(d);
    clause(d, d->one, 0, 0, 1);
}
static int at_most(Definition* d, const int* inputs, int n, int bound)
{
    if (bound < 0) return -d->one;
    if (bound >= n) return d->one;
    Number count = sum(d, inputs, n);
    int le = d->one;
    for (int i = 0; i < count.width; i++) {
        int inverted = count.bit[i] ? -count.bit[i] : d->one;
        if (((unsigned int)bound >> i) & 1U) le = gate_or(d, inverted, le);
        else le = gate_and(d, inverted, le);
    }
    free(count.bit);
    return le ? le : -d->one;
}
static void load(CCaDiCaL* solver, Definition* d)
{
    for (size_t i = 0; i < d->n; i++) CNF_ADD(solver, d->data[i]);
}
int PowerEncodeAtMost(CCaDiCaL* solver, const int* inputs, int n, int bound, int* last_var)
{
    Definition d = {0}; init(&d, *last_var);
    int lit = at_most(&d, inputs, n, bound);
    load(solver, &d); *last_var = d.next; free(d.data); return lit;
}
void PowerInit(void)
{
    if (opt.low_power != YES) return;
    int changing = 0;
    for (int i = 0; i < n_net; i++) if (nl[i].peer_1t) {
        signals++;
        if (nl[i].peer_1t != &nl[i]) changing++;
    }
    budget = (int)((long long)signals * opt.wsa_threshold / 100);
    init(&cached, cnf.constant.vars);
    int* transitions = malloc((size_t)(changing ? changing : 1) * sizeof(int));
    if (!transitions) fail();
    int n = 0;
    if (budget < changing) {
        for (int i = 0; i < n_net; i++) {
            NLIST* first = nl[i].peer_1t;
            if (first && first != &nl[i])
                transitions[n++] = gate_xor(&cached, (int)first->varsgc, (int)nl[i].varsgc);
        }
    }
    predicate = at_most(&cached, transitions, n, budget);
    free(transitions);
    cnf.constant.vars = cached.next;
    fprintf(stderr, "[POWER] signals=%d threshold=%d%% budget=%d definition_vars=%d\n",
            signals, opt.wsa_threshold, budget, cached.next - n_net);
}
void PowerLoadDefinition(CCaDiCaL* solver) { if (predicate) load(solver, &cached); }
int PowerLiteral(void) { return predicate; }
int PowerSignalCount(void) { return signals; }
int PowerBudget(void) { return budget; }
void PowerRelease(void)
{
    free(cached.data); cached = (Definition){0}; predicate = signals = budget = 0;
}

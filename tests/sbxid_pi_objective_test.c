/* Enumerate an OPB with an unrelated, observed PI. Free PI variables may be
 * true in a model after their objective term is removed; the cube must use X. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdp/sbxid/sbxid.h"
#include "netlist/netlist.h"

enum { MAX_VARS = 8, MAX_CONS = 20 };
typedef struct { int coefficient[MAX_VARS + 1], rhs; } Linear;
static SbxidSolveStatus status = SBXID_SOLVE_OPTIMAL;
static int scoped, calls, cost;
static char* reference_constraints;

static void terms(char* p, Linear* row, int variables)
{
    for (;;) {
        while (*p == ' ') ++p;
        if (*p == ';' || *p == '>') return;
        char* end;
        int coefficient = (int)strtol(p, &end, 10);
        assert(end != p); p = end;
        while (*p == ' ') ++p;
        assert(*p++ == 'x');
        int variable = (int)strtol(p, &end, 10);
        assert(end != p && variable > 0 && variable <= variables); p = end;
        row->coefficient[variable] += coefficient;
    }
}

static int evaluate(const Linear* row, unsigned model, int variables)
{
    int sum = 0;
    for (int v = 1; v <= variables; ++v)
        sum += row->coefficient[v] * ((model >> (v - 1)) & 1);
    return sum;
}

SbxidSolveResult sbxid_solve_opb(const char* opb, size_t length, int variables,
                               int* values, double seconds)
{
    assert(strlen(opb) == length && variables <= MAX_VARS && seconds == 1.0);
    ++calls;
    SbxidSolveResult result = { .status = status,
        .timed_out = status == SBXID_SOLVE_UNKNOWN || status == SBXID_SOLVE_FEASIBLE };
    if (status == SBXID_SOLVE_UNKNOWN) return result;
    const char* body = strchr(strstr(opb, "min:"), ';') + 1;
    if (!scoped) {
        free(reference_constraints);
        reference_constraints = strdup(body);
        assert(reference_constraints);
    } else assert(strcmp(reference_constraints, body) == 0);

    Linear objective = {0}, constraints[MAX_CONS] = {0};
    int count = 0;
    char* copy = strdup(opb), *context = NULL;
    assert(copy);
    for (char* line = strtok_r(copy, "\n", &context); line;
         line = strtok_r(NULL, "\n", &context)) {
        if (strncmp(line, "min:", 4) == 0) terms(line + 4, &objective, variables);
        char* relation = strstr(line, ">=");
        if (!relation) continue;
        assert(count < MAX_CONS);
        terms(line, &constraints[count], variables);
        constraints[count++].rhs = atoi(relation + 2);
    }
    free(copy);
    /* PI order is a,c,b, while dense net IDs are y,b,c,a. */
    assert(objective.coefficient[1] == 9 && objective.coefficient[3] == 9);
    assert(objective.coefficient[2] == (scoped ? 0 : 9));
    int optimum = INT_MAX;
    unsigned best = 0;
    for (unsigned model = 0; model < (1u << variables); ++model) {
        int valid = 1;
        for (int c = 0; c < count; ++c)
            if (evaluate(&constraints[c], model, variables) < constraints[c].rhs) valid = 0;
        if (!valid) continue;
        /* For seed=111, y=a OR b detects y/sa0 for all completions iff at
         * least one of a,b is retained; c drives its own unrelated output. */
        assert(model & 5u);
        int candidate = evaluate(&objective, model, variables);
        /* Deliberately prefer c=care when it has zero cost. */
        if (candidate < optimum || (candidate == optimum && (model & 2u))) {
            optimum = candidate; best = model;
        }
    }
    assert(optimum != INT_MAX);
    for (int v = 1; v <= variables; ++v) values[v] = (best >> (v - 1)) & 1;
    assert(values[2] == scoped);
    cost = optimum;
    return result;
}

int main(void)
{
    NLIST nets[4] = {0};
    NLIST* inputs[] = {nets + 3, nets + 2, nets + 1};
    NLIST* gate_inputs[] = {nets + 3, nets + 1};
    NLIST* outputs[] = {nets, nets + 2};
    NLIST* to_y[] = {nets};
    const char* names[] = {"y", "b", "c", "a"};
    nl = nets; n_net = 4; pi = inputs; n_pi = 3; po = outputs; n_po = 2;
    for (int i = 0; i < n_net; ++i) {
        nets[i].n = i; nets[i].name = (char*)names[i]; nets[i].type = i ? IN : OR;
    }
    nets[0].in = gate_inputs; nets[0].n_in = 2;
    nets[1].out = nets[3].out = to_y; nets[1].n_out = nets[3].n_out = 1;
    const unsigned char mask[] = {1, 0, 1}, bad_mask[] = {1, 0, 0};
    char cube[4];
    SbxidInfo info;
    setenv("FDP_SBXID_VERIFY", "1", 1);
    scoped = 0;
    assert(sbxid_generalize("111", 0, 0, NULL, cube, &info));
    int reference_cost = cost, reference_variables = info.variables;
    int reference_count = info.constraints;
    for (int trial = 0; trial < 2; ++trial) {
        scoped = 1;
        status = trial ? SBXID_SOLVE_FEASIBLE : SBXID_SOLVE_OPTIMAL;
        assert(sbxid_generalize("111", 0, 0, mask, cube, &info));
        assert(cost == reference_cost && cube[1] == 'X');
        assert((cube[0] == '1') + (cube[2] == '1') == 1);
        assert(info.variables == reference_variables && info.constraints == reference_count);
        assert(info.verified && !info.verification_failed);
    }
    status = SBXID_SOLVE_UNKNOWN;
    assert(sbxid_generalize("111", 0, 0, mask, cube, &info));
    assert(strcmp(cube, "111") == 0 && !info.verified);
    int previous_calls = calls;
    assert(!sbxid_generalize("111", 0, 0, bad_mask, cube, &info));
    assert(calls == previous_calls);
    sbxid_release();
    free(reference_constraints);
    unsetenv("FDP_SBXID_VERIFY");
    return 0;
}

/* 小さなPBOを全列挙し、サイド入力の正当化と目的値を独立に確認する。 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdp/sbxid/sbxid.h"
#include "netlist/netlist.h"

enum { MAX_VARS = 12, MAX_CONS = 64 };
typedef struct { int coefficient[MAX_VARS + 1], rhs; } Constraint;
static int fault_id, stuck_value, seed_bits, solver_calls;
static int last_cost, last_variables;

/* 回路は s=a&b, y=f&s。正常側と故障側を別々に2値計算する。 */
static int detects(int bits)
{
    int f = bits & 1, a = (bits >> 1) & 1, b = (bits >> 2) & 1;
    int good = f & a & b;
    if (fault_id == 0) f = stuck_value;
    if (fault_id == 1) a = stuck_value;
    if (fault_id == 2) b = stuck_value;
    int s = fault_id == 3 ? stuck_value : a & b;
    int faulty = fault_id == 4 ? stuck_value : f & s;
    return good != faulty;
}

static int detects_all_completions(unsigned care)
{
    for (int bits = 0; bits < 8; ++bits)
        if ((((unsigned)bits ^ (unsigned)seed_bits) & care) == 0 && !detects(bits))
            return 0;
    return 1;
}

static void parse_terms(char* cursor, Constraint* out, int variables)
{
    while (*cursor) {
        while (*cursor == ' ') ++cursor;
        if (*cursor == ';' || *cursor == '>') break;
        char* end;
        long coefficient = strtol(cursor, &end, 10);
        assert(end != cursor);
        cursor = end;
        while (*cursor == ' ') ++cursor;
        assert(*cursor++ == 'x');
        long variable = strtol(cursor, &end, 10);
        assert(variable > 0 && variable <= variables && end != cursor);
        out->coefficient[variable] += (int)coefficient;
        cursor = end;
    }
}

static int evaluate(const Constraint* c, unsigned model, int variables)
{
    int total = 0;
    for (int v = 1; v <= variables; ++v)
        total += c->coefficient[v] * ((model >> (v - 1)) & 1);
    return total;
}

SbxidSolveResult sbxid_solve_opb(const char* opb, size_t length, int variables,
                               int* values, double seconds)
{
    assert(length == strlen(opb) && seconds == 0.0 && variables <= MAX_VARS);
    char* copy = strdup(opb);
    assert(copy);
    Constraint objective = {0}, constraints[MAX_CONS] = {0};
    int count = 0, declared_variables = 0, declared_constraints = 0;
    assert(sscanf(copy, "* #variable= %d #constraint= %d",
                  &declared_variables, &declared_constraints) == 2);
    assert(declared_variables == variables);
    char* context = NULL;
    for (char* line = strtok_r(copy, "\n", &context); line;
         line = strtok_r(NULL, "\n", &context)) {
        if (strncmp(line, "min:", 4) == 0) parse_terms(line + 4, &objective, variables);
        char* comparison = strstr(line, ">=");
        if (!comparison) continue;
        assert(count < MAX_CONS);
        parse_terms(line, &constraints[count], variables);
        constraints[count++].rhs = atoi(comparison + 2);
    }
    assert(count == declared_constraints);
    free(copy);

    int optimum = INT_MAX;
    unsigned best = 0;
    for (unsigned model = 0; model < (1u << variables); ++model) {
        int valid = 1;
        for (int c = 0; c < count && valid; ++c)
            valid = evaluate(&constraints[c], model, variables) >= constraints[c].rhs;
        if (!valid) continue;
        /* 全実行可能モデルのPIキューブが、全X補完で故障検出する必要がある。 */
        assert(detects_all_completions(model & 7u));
        int cost = evaluate(&objective, model, variables);
        if (cost < optimum) { optimum = cost; best = model; }
    }
    assert(optimum != INT_MAX);
    for (int v = 1; v <= variables; ++v) values[v] = (best >> (v - 1)) & 1;
    last_cost = optimum;
    last_variables = variables;
    ++solver_calls;
    return (SbxidSolveResult){ .status = SBXID_SOLVE_OPTIMAL };
}

static void check_case(int site, int stuck, int seed)
{
    fault_id = site; stuck_value = stuck; seed_bits = seed;
    if (!detects(seed)) return;
    char pattern[4], cube[4];
    for (int p = 0; p < 3; ++p) pattern[p] = '0' + ((seed >> p) & 1);
    pattern[3] = '\0';
    SbxidInfo info;
    assert(sbxid_generalize(pattern, site, stuck, NULL, 0.0, cube, &info));
    unsigned care = 0;
    int actual_care = 0, minimum_care = 4;
    for (int p = 0; p < 3; ++p) if (cube[p] != 'X') {
        assert(cube[p] == pattern[p]); care |= 1u << p; ++actual_care;
    }
    assert(detects_all_completions(care));
    for (unsigned candidate = 0; candidate < 8; ++candidate) {
        int bits = (candidate & 1) + ((candidate >> 1) & 1) + ((candidate >> 2) & 1);
        if (detects_all_completions(candidate) && bits < minimum_care) minimum_care = bits;
    }
    assert(actual_care == minimum_care);
    /* fの故障ではsをV側へまとめても、V/S両側の費用を残す。 */
    if (site == 0) assert(last_variables == 7 && last_cost == 3 * 11 - 4);
    if (site == 1) assert(last_variables == 8); /* sはこの故障のTFO内 */
    for (int i = 0; i < n_net; ++i) {
        assert(nl[i].flag == 100 + i && nl[i].varsgc == (unsigned)(20 + i));
        assert(nl[i].logic_value == -1);
    }
}

int main(void)
{
    NLIST nets[5] = {0};
    NLIST* inputs[] = {nets, nets + 1, nets + 2};
    NLIST* outputs[] = {nets + 4};
    NLIST* s_inputs[] = {nets + 1, nets + 2};
    NLIST* y_inputs[] = {nets, nets + 3};
    NLIST* to_s[] = {nets + 3};
    NLIST* to_y[] = {nets + 4};
    const char* names[] = {"f", "a", "b", "s", "y"};
    nl = nets; n_net = 5; pi = inputs; n_pi = 3; po = outputs; n_po = 1;
    for (int i = 0; i < n_net; ++i) {
        nets[i].n = i; nets[i].name = (char*)names[i];
        nets[i].type = i < 3 ? IN : AND;
        nets[i].flag = 100 + i; nets[i].varsgc = 20 + i; nets[i].logic_value = -1;
    }
    nets[0].out = to_y; nets[0].n_out = 1;
    nets[1].out = nets[2].out = to_s; nets[1].n_out = nets[2].n_out = 1;
    nets[3].in = s_inputs; nets[3].n_in = 2; nets[3].out = to_y; nets[3].n_out = 1;
    nets[4].in = y_inputs; nets[4].n_in = 2;
    unsetenv("FDP_SBXID_VERIFY");
    /* 同じ故障点でseed/縮退値が変わる場合と、故障点が変わる場合を確認する。 */
    for (int site = 0; site < 5; ++site)
        for (int stuck = 0; stuck < 2; ++stuck)
            for (int seed = 0; seed < 8; ++seed) check_case(site, stuck, seed);
    check_case(0, 1, 6);
    sbxid_release();
    check_case(0, 1, 6); /* 解放・再初期化でもTFOキャッシュを再構築する。 */
    sbxid_release();
    assert(solver_calls == 20);
    printf("Checked %d detecting seeds and all feasible care models\n", solver_calls);
    return 0;
}

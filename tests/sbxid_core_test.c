/* Exercises the real SB core with an explicitly test-only solver double.
 * This does not test clasp or claim end-to-end SB-XID correctness.
 */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdp/sbxid/sbxid.h"
#include "netlist/netlist.h"

static SbxidSolveStatus next_status;
static int calls;
static bool invalid_candidate;

/* Check that our chosen model satisfies every generated PB constraint. */
static void check_constraints(const char* opb, const int* values, int variables)
{
    char* copy = strdup(opb);
    assert(copy);
    char* context = NULL;
    int constraints = 0, declared_vars = 0, declared_constraints = 0;
    assert(sscanf(copy, "* #variable= %d #constraint= %d", &declared_vars, &declared_constraints) == 2);
    assert(declared_vars == variables);
    for (char* line = strtok_r(copy, "\n", &context); line; line = strtok_r(NULL, "\n", &context)) {
        if (!strstr(line, ">=")) continue;
        char* cursor = line;
        long sum = 0;
        while (*cursor) {
            while (*cursor == ' ') cursor++;
            if (*cursor == '>') break;
            char* end;
            long coefficient = strtol(cursor, &end, 10);
            assert(end != cursor);
            cursor = end;
            while (*cursor == ' ') cursor++;
            assert(*cursor++ == 'x');
            long index = strtol(cursor, &end, 10);
            assert(index > 0 && index <= variables);
            sum += coefficient * values[index];
            cursor = end;
        }
        assert(cursor[0] == '>' && cursor[1] == '=');
        assert(sum >= strtol(cursor + 2, NULL, 10));
        constraints++;
    }
    assert(constraints == declared_constraints && constraints > 0);
    free(copy);
}

SbxidSolveResult sbxid_solve_opb(const char* opb, size_t length, int variables,
                               int* values, double seconds)
{
    calls++;
    assert(strlen(opb) == length);
    assert(seconds == 0.0);
    if (!strstr(opb, "Observe at least one PO")) fprintf(stderr, "OPB length=%zu\n%s\n", length, opb);
    assert(strstr(opb, "Observe at least one PO")); /* includes EOF after backpatch */
    SbxidSolveResult result = {.status = next_status};
    result.timed_out = next_status == SBXID_SOLVE_UNKNOWN || next_status == SBXID_SOLVE_FEASIBLE;
    if (next_status == SBXID_SOLVE_OPTIMAL || next_status == SBXID_SOLVE_FEASIBLE) {
        for (int i = 1; i <= variables; ++i) values[i] = 1;
        values[2] = 0; /* PI order is a,b; retain a, remove b from seed=11. */
        if (invalid_candidate) values[1] = 0;
        else check_constraints(opb, values, variables);
    }
    return result;
}

int main(void)
{
    /* Dense IDs and pi[] order deliberately differ, as in the real reader. */
    NLIST nets[3] = {0};
    NLIST* inputs[] = {&nets[2], &nets[1]};
    NLIST* outputs[] = {&nets[0]};
    NLIST* primary_inputs[] = {&nets[2], &nets[1]};
    NLIST* primary_outputs[] = {&nets[0]};
    nl = nets; n_net = 3; pi = primary_inputs; n_pi = 2; po = primary_outputs; n_po = 1;
    nets[0].name = "y"; nets[0].type = OR; nets[0].in = inputs; nets[0].n_in = 2;
    nets[1].name = "b"; nets[1].type = IN; nets[1].out = outputs; nets[1].n_out = 1;
    nets[2].name = "a"; nets[2].type = IN; nets[2].out = outputs; nets[2].n_out = 1;
    for (int i = 0; i < n_net; ++i) {
        nets[i].n = i;
        nets[i].flag = 100 + i;
        nets[i].varsgc = 20 + i;
        nets[i].logic_value = -1;
    }
    char cube[3];
    SbxidInfo info;
    unsetenv("FDP_SBXID_VERIFY");
    for (int i = 0; i < 2; ++i) {
        next_status = i ? SBXID_SOLVE_FEASIBLE : SBXID_SOLVE_OPTIMAL;
        assert(sbxid_generalize("11", 0, 0, NULL, cube, &info));
        assert(strcmp(cube, "1X") == 0);
        assert(info.solve.status == next_status && !info.verified);
    }
    next_status = SBXID_SOLVE_UNKNOWN;
    assert(sbxid_generalize("11", 0, 0, NULL, cube, &info));
    assert(strcmp(cube, "11") == 0 && info.solve.timed_out);
    next_status = SBXID_SOLVE_UNSAT;
    assert(!sbxid_generalize("11", 0, 0, NULL, cube, &info));
    next_status = SBXID_SOLVE_ERROR;
    assert(!sbxid_generalize("11", 0, 0, NULL, cube, &info));
    int previous_calls = calls;
    assert(!sbxid_generalize("00", 0, 0, NULL, cube, &info)); /* no detected seed */
    assert(!sbxid_generalize("1X", 0, 0, NULL, cube, &info)); /* seed must be binary */
    assert(calls == previous_calls);

    setenv("FDP_SBXID_VERIFY", "1", 1);
    next_status = SBXID_SOLVE_OPTIMAL;
    assert(sbxid_generalize("11", 0, 0, NULL, cube, &info));
    assert(info.verified && !info.verification_failed && strcmp(cube, "1X") == 0);
    invalid_candidate = true;
    assert(sbxid_generalize("11", 0, 0, NULL, cube, &info));
    assert(info.verified && info.verification_failed && strcmp(cube, "11") == 0);
    for (int i = 0; i < n_net; ++i) {
        assert(nets[i].flag == 100 + i && nets[i].varsgc == (unsigned)(20 + i));
        assert(nets[i].logic_value == -1 && nets[i].sbxid_normal_value == 1);
    }
    unsetenv("FDP_SBXID_VERIFY");
    sbxid_release();
    sbxid_release();
    invalid_candidate = false;
    assert(sbxid_generalize("11", 0, 0, NULL, cube, &info)); /* rebuild process indexes */
    sbxid_release();
    return 0;
}

#include <assert.h>
#include <stdio.h>
#include "fdp/power_constraint.h"

static int solve(CCaDiCaL* solver, const int* variables, int n, int mask, int predicate)
{
    for (int i = 0; i < n; i++) ccadical_assume(solver, mask & (1 << i) ? variables[i] : -variables[i]);
    if (predicate) ccadical_assume(solver, predicate);
    return ccadical_solve(solver);
}
int main(void)
{
    long cases = 0;
    for (int n = 0; n <= 8; n++) for (int bound = -1; bound <= n + 1; bound++) {
        CCaDiCaL* solver = ccadical_init();
        ccadical_set_option(solver, "factor", 0);
        int variables[8], last = n;
        for (int i = 0; i < n; i++) variables[i] = i + 1;
        int predicate = PowerEncodeAtMost(solver, variables, n, bound, &last);
        assert(predicate);
        for (int assignment = 0; assignment < (1 << n); assignment++) {
            int accepted = __builtin_popcount((unsigned int)assignment) <= bound;
            assert(solve(solver, variables, n, assignment, 0) == 10);
            assert(solve(solver, variables, n, assignment, accepted ? predicate : -predicate) == 10);
            assert(solve(solver, variables, n, assignment, accepted ? -predicate : predicate) == 20);
            cases++;
        }
        ccadical_assume(solver, predicate);
        assert(ccadical_solve(solver) == (bound >= 0 ? 10 : 20));
        ccadical_assume(solver, -predicate);
        assert(ccadical_solve(solver) == (bound < n ? 10 : 20));
        ccadical_release(solver);
    }
    printf("PASS: exact reified power predicate, both polarities, %ld full assignments\n", cases);
    return 0;
}

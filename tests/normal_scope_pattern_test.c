#include <assert.h>
#include <string.h>
#include "fdp/normal_scope.h"
#include "fdp/read.h"

int main(void)
{
    NLIST nets[2] = {0};
    NLIST* inputs[] = {&nets[0], &nets[1]};
    nl = nets; n_net = n_pi = 2; pi = inputs;
    NLIST* outputs[] = {&nets[0]};
    po = outputs; n_po = 1;
    FNODE target = { .netptr = &nets[0] };
    for (int p = 0; p < n_pi; ++p) {
        nets[p].n = p; nets[p].type = IN; nets[p].varsgc = p + 1;
    }
    nets[0].flag = TFO;
    NormalScopeBuild(&target);
    assert(NormalScopeRequiredNet(0) && !NormalScopeRequiredNet(1));
    CCaDiCaL* solver = ccadical_init();
    assert(solver);
    ccadical_set_option(solver, "factor", 0);
    ccadical_add(solver, 1); ccadical_add(solver, 0); /* detection requires a=1 */
    assert(ccadical_solve(solver) == 10);
    char seed[3];
    NormalScopeFillPiPattern(solver, seed);
    assert(strcmp(seed, "10") == 0); /* omitted b is initially completed with zero */

    /* UNKNOWN fallback blocks the complete seed 10. Now b must follow SAT,
     * even though b is still outside the normal circuit scope.
     */
    NormalScopeRecordBlockingPi(0); NormalScopeRecordBlockingPi(1);
    ccadical_add(solver, -1); ccadical_add(solver, 2); ccadical_add(solver, 0);
    assert(ccadical_solve(solver) == 10);
    NormalScopeFillPiPattern(solver, seed);
    assert(strcmp(seed, "11") == 0);
    assert(!NormalScopeRequiredNet(1) && NormalScopeRequiredPi(1));
    ccadical_add(solver, -1); ccadical_add(solver, -2); ccadical_add(solver, 0);
    assert(ccadical_solve(solver) == 20);
    ccadical_release(solver);

    NormalScopeBuild(&target); /* fault-local blocking state must not leak */
    assert(!NormalScopeRequiredPi(1));
    NormalScopeRelease();
    return 0;
}

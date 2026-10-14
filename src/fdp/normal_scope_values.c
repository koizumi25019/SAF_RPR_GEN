/* Complete a full PI pattern without reading omitted internal SAT variables.
 * Outside PIs start at zero; after they occur in cube blocking clauses, they
 * must follow the SAT model so that UNKNOWN fallback still makes progress.
 * SB-XID performs the full normal/fault simulation used for generalization.
 */
#include <stdio.h>
#include <stdlib.h>
#include "normal_scope.h"
#include "../netlist/netlist.h"

void NormalScopeFillPiPattern(CCaDiCaL* solver, char* seed)
{
    for (int p = 0; p < n_pi; ++p) {
        int bit = NormalScopeRequiredPi(p)
            ? ccadical_val(solver, (int)pi[p]->varsgc) > 0 : 0;
        seed[p] = bit ? '1' : '0';
    }
    seed[n_pi] = '\0';
}

bool NormalScopeValidateModel(CCaDiCaL* solver)
{
    if (!getenv("FDP_NORMAL_SCOPE_VALIDATE")) return true;
    for (int i = 0; i < n_net; ++i) {
        if (!NormalScopeRequiredNet(i)) continue;
        int value = ccadical_val(solver, (int)nl[i].varsgc);
        if (value && (value > 0) != nl[i].sbxid_normal_value) {
            fprintf(stderr, "SAT/SB-XID seed simulation mismatch %s\n", nl[i].name);
            return false;
        }
    }
    return true;
}

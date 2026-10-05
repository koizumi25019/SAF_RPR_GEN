#include <stdio.h>
#include <stdlib.h>
#include "./paper_core.h"
#include "./create_TPG_model.h"
#include "./cnf/cnf.h"
#include "./fault_detection_prob.h"
#include "./power_constraint.h"
#include "../netlist/netlist.h"

static long cubes, input_care, core_care, prime_care, calls;

CCaDiCaL* PaperCoreBuildOracle(TARGET* target)
{
    CCaDiCaL* u = ccadical_init();
    ccadical_set_option(u, "factor", 0);
    FNODE* f = target->list[0];
    LoadModelToSolver(u, target);
    for (int j = 0; j < n_net; j++) {
        if ((nl[j].flag & TFO) != TFO || (nl[j].flag & FP) == FP) continue;
        switch (nl[j].type) {
            case AND:   CreateConsFC_AND(u, &nl[j]); break;
            case NAND:  CreateConsFC_NAND(u, &nl[j]); break;
            case OR:    CreateConsFC_OR(u, &nl[j]); break;
            case NOR:   CreateConsFC_NOR(u, &nl[j]); break;
            case INV:   CreateConsFC_INV(u, &nl[j]); break;
            case BUF:
            case FOUT:  CreateConsFC_BUF(u, &nl[j]); break;
            case EXOR:  CreateConsFC_XOR(u, &nl[j]); break;
            case EXNOR: CreateConsFC_XNOR(u, &nl[j]); break;
            default: break;
        }
    }
    int fc = (int)f->netptr->varsfc;
    ccadical_add(u, f->type == SF0 ? -fc : fc);
    ccadical_add(u, 0);
    /* No observed output: D_f=false, so NOT D_f imposes no constraint. */
    if (numtranpo == 0) return u;

    /* Comparator auxiliaries belong to u. Preserve generator numbering. */
    int saved_vars = cnf.total.vars;
    CreateConsDC_XOR(u);
    CreateConsDC_OR(u);
    int z = cnf.total.vars;
    cnf.total.vars = saved_vars;
    ccadical_add(u, -z);
    if (f->exc_netptr) {
        int exc = (int)f->exc_netptr->varsgc;
        int exc_lit = f->type == SF0 ? -exc : exc;
        ccadical_add(u, -exc_lit);
        int current = (int)f->netptr->varsgc;
        int launched = f->type == SF0 ? current : -current;
        ccadical_add(u, -launched);
    }
    int power = PowerLiteral();
    if (power) ccadical_add(u, -power); /* NOT(z AND excitation AND power) */
    ccadical_add(u, 0);
    return u;
}

static int query(CCaDiCaL* u, const int* lits, int n, int skip)
{
    for (int i = 0; i < n; i++)
        if (i != skip && lits[i]) ccadical_assume(u, lits[i]);
    calls++;
    /* Always consume assumptions with solve, even for the empty set. */
    return ccadical_solve(u);
}

bool PaperCoreGeneralize(CCaDiCaL* u, const int* vars, int n, char* cube, bool verify)
{
    int* lits = malloc((size_t)(n ? n : 1) * sizeof(int));
    if (!lits) return false;
    for (int i = 0; i < n; i++) {
        if (vars[i] <= 0 || (cube[i] != '0' && cube[i] != '1')) {
            fprintf(stderr, "[PAPER_CORE] expected a complete input model\n");
            free(lits);
            return false;
        }
        lits[i] = cube[i] == '1' ? vars[i] : -vars[i];
    }
    if (query(u, lits, n, -1) != 20) {
        fprintf(stderr, "[PAPER_CORE] input model does not entail detection (or solve unknown)\n");
        free(lits);
        return false;
    }
    /* Collect ALL failed assumptions before assume/solve changes state. */
    int nc = 0;
    for (int i = 0; i < n; i++) {
        if (ccadical_failed(u, lits[i])) nc++;
        else lits[i] = 0;
    }
    /* Recheck even when the core kept every input; never leave pending
       assumptions for the next query by conditionally skipping solve. */
    if (query(u, lits, n, -1) != 20) {
        fprintf(stderr, "[PAPER_CORE] failed-assumption core did not recheck UNSAT\n");
        free(lits);
        return false;
    }
    int np = nc;
    for (int i = 0; i < n; i++) {
        if (!lits[i]) continue;
        int res = query(u, lits, n, i);
        if (res == 20) { lits[i] = 0; np--; }
        else if (res != 10) {
            fprintf(stderr, "[PAPER_CORE] minimization solve returned unknown\n");
            free(lits);
            return false;
        }
    }
    /* Later removals only weaken assumptions, so retained literals remain
       necessary. One deletion pass proves subset-minimality. */
    if (verify) {
        if (query(u, lits, n, -1) != 20) { free(lits); return false; }
        for (int i = 0; i < n; i++)
            if (lits[i] && query(u, lits, n, i) != 10) { free(lits); return false; }
    }
    for (int i = 0; i < n; i++) if (!lits[i]) cube[i] = 'X';
    free(lits);
    cubes++;
    input_care += n;
    core_care += nc;
    prime_care += np;
    return true;
}

void PaperCoreReport(void)
{
    fprintf(stderr, "[PAPER_CORE] cubes=%ld solves=%ld care: input=%ld core=%ld prime=%ld\n",
            cubes, calls, input_care, core_care, prime_care);
}

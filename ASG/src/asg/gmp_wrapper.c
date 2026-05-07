#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#include"../netlist/netlist.h"
#include "./target_fault.h"

void calculate_prob_with_gmp(
    const char* numStr,
     int nvars,
     FILE* result_fp,
     FILE* cube_analysis_fp,
     TARGET* target
    ) 
    {
    mpf_t num, den, density;
    mpf_set_default_prec(8192);

    mpf_init(num);
    mpf_init(den);
    mpf_init(density);

    if (mpf_set_str(num, numStr, 10) != 0) {
        mpf_set_ui(num, 0);
    }

    mpf_set_ui(den, 1);
    mpf_mul_2exp(den, den, (unsigned long)nvars); // den = 1 * 2^nvars
    mpf_div(density, num, den);

    if (result_fp != NULL) {
        gmp_fprintf(result_fp, "%.10Fe\n", density);
    }
    if (cube_analysis_fp != NULL) {
        gmp_fprintf(cube_analysis_fp, ",%.10Fe", density);
    }

    // 代表故障を出力
    if (result_fp != NULL) {
        gmp_fprintf(result_fp, "%s,%s,%.10Fe\n",
            target->list[0]->name,
            (target->list[0]->type == SF0) ? "sa0" : "sa1",
            density);
    }
    

    //等価故障と故障検出確率は同様
    if (result_fp != NULL) {
        EQUIV_NODE* e = (target->list[0]->type == SF0)
            ? target->list[0]->netptr->equiv_sa0
            : target->list[0]->netptr->equiv_sa1;
        while (e != NULL) {
            gmp_fprintf(result_fp, "%s,%s,,%.10Fe\n",
                e->net->name,
                (target->list[0]->type == SF0) ? "sa0" : "sa1",
                density);
            e = e->next;
        }
    }
    printf("testtesttest\n");
    mpf_clear(num);
    mpf_clear(den);
    mpf_clear(density);

    return;
}
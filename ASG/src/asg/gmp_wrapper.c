#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>

void calculate_prob_with_gmp(const char* numStr, int nvars, int* pattern_num_list, int list_size, FILE* result_fp, mpf_t* total_prob_sums) {
    mpf_t num, den, density, term, result;
    mpf_set_default_prec(8192);

    mpf_init(num);
    mpf_init(den);
    mpf_init(density);
    mpf_init(term);
    mpf_init(result);

    if (mpf_set_str(num, numStr, 10) != 0) {
        mpf_set_ui(num, 0);
    }

    mpf_set_ui(den, 1);
    mpf_mul_2exp(den, den, (unsigned long)nvars); // den = 1 * 2^nvars

    mpf_div(density, num, den);

    if (result_fp != NULL) {
        gmp_fprintf(result_fp, "%.10Fe,", density);
    }

    // ===========================================================
    // �v�Z��: P_det = 1 - (1 - p)^n
    // ===========================================================
    for (int i = 0; i < list_size; i++) {

        int n = pattern_num_list[i];

        // term = 1 - p
        mpf_ui_sub(term, 1, density);

        // term = term ^ num_patterns
        // (1-p) 
        mpf_pow_ui(term, term, (unsigned long)n);

        // result = 1 - term
        // �܂� 1 - (1-p)^N
        mpf_ui_sub(result, 1, term);

        if (result_fp != NULL) {
            gmp_fprintf(result_fp, "%.10Fe", result);
            if(i!=list_size-1) fprintf(result_fp, ",");
        }
        if (total_prob_sums != NULL) {
            mpf_add(total_prob_sums[i], total_prob_sums[i], result);
        }
    }

    gmp_fprintf(result_fp, "\n");

    mpf_clear(num);
    mpf_clear(den);
    mpf_clear(density);
    mpf_clear(term);
    mpf_clear(result);

    return;
}
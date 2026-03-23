#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>

void calculate_prob_with_gmp(const char* numStr, int nvars,FILE* result_fp) {
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
        gmp_fprintf(result_fp, "%.10Fe", density);
    }

    gmp_fprintf(result_fp, "\n");

    mpf_clear(num);
    mpf_clear(den);
    mpf_clear(density);
    mpf_clear(term);
    mpf_clear(result);

    return;
}
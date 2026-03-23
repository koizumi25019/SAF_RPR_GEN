#pragma once

#include <gmp.h> 

void calculate_prob_with_gmp(
     const char* numStr,
     int nvars,
     FILE* result_fp
    );
#pragma once

#include <gmp.h> 
#include "./target_fault.h"

void calculate_prob_with_gmp(
     const char* numStr,
     int nvars,
     FILE* result_fp,
     FILE* cube_analysis_fp,
     TARGET* target,
     int test_loop
    );
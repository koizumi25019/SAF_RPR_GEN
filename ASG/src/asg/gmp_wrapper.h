#pragma once

#include <gmp.h> 

void calculate_prob_with_gmp(const char* numStr, int nvars, int* pattern_num_list,int list_size, FILE* result_fp, mpf_t* accumulator);
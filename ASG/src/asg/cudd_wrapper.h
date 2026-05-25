#pragma once

#include <cudd.h> 
#include <gmp.h>

void RunBDD(
	DdManager* gbm,
	int nvars,
	char** cubes,
	int n_cubes,
	FILE* result_fp,
	FILE* cube_analysis_fp,
	TARGET* target,
	int test_loop
);
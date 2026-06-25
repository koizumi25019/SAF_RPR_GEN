#pragma once

#include <stdbool.h>
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
	int cube_cnt,
	int seeded_cnt,
	bool limit_hit
);
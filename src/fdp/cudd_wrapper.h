#pragma once

#include <stdbool.h>
#include <cudd.h>
#include <gmp.h>

/* キューブ文字列（'0'/'1'/'X' を nvars 文字）を BDD にして返す（参照済み） */
DdNode* parseCube(DdManager* gbm, const char* cubeStr, int nvars);

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
#pragma once

#include <cudd.h> 
#include <gmp.h>

void RunBDD(
	DdManager* gbm,
	int nvars, 
	FILE* result_fp
);
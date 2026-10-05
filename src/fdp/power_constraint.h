#pragma once
#include "ccadical.h"
/* Exact reified CNF: returned signed literal iff sum(inputs)<=bound.
   Definitions impose no bound until that literal is asserted. */
int PowerEncodeAtMost(CCaDiCaL* solver, const int* inputs, int n, int bound, int* last_var);
void PowerInit(void);
void PowerLoadDefinition(CCaDiCaL* solver);
int PowerLiteral(void);
int PowerSignalCount(void);
int PowerBudget(void);
void PowerRelease(void);

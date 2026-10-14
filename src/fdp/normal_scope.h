#pragma once
#include <stdbool.h>
#include "ccadical.h"
/* 正常CNFの範囲限定は常時有効。全PIとFDPの分母を維持する。 */
/* Call after fault TFO and essential assignments have been constructed. */
void NormalScopeBuild(void);
int NormalScopeRequiredNet(int index);
/* A PI omitted from the circuit CNF may later be constrained by cube blocks. */
int NormalScopeRequiredPi(int index);
void NormalScopeRecordBlockingPi(int index);
void NormalScopeFillPiPattern(CCaDiCaL *solver, char* seed);
bool NormalScopeValidateModel(CCaDiCaL *solver);
void NormalScopeRelease(void);

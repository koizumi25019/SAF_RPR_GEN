#pragma once
#include <stdbool.h>
#include "ccadical.h"
#include "read.h"
/* 正常CNFの範囲限定は常時有効。全PIとFDPの分母を維持する。 */
/* Call after SearchTFO and BEFORE fault clauses / essential assignments. */
void NormalScopeBuild(FNODE *target);
int NormalScopeInDetectionCone(int index);
int NormalScopeRequiredNet(int index);
/* A PI omitted from the circuit CNF may later be constrained by cube blocks. */
int NormalScopeRequiredPi(int index);
void NormalScopeRecordBlockingPi(int index);
void NormalScopeFillPiPattern(CCaDiCaL *solver, char* seed);
bool NormalScopeValidateModel(CCaDiCaL *solver);
void NormalScopeRelease(void);

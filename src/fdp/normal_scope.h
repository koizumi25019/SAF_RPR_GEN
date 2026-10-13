#pragma once
#include "ccadical.h"
#include "xid/XID.h"
/* Always retain TFI of the observable outputs reached by the fault TFO.
 * All original PIs and the FDP denominator are retained. */
int NormalScopeEnabled(void);
void NormalScopeBegin(void);
/* Call after SearchTFO, before fault/propagation clauses and EA. */
void NormalScopeSelectOutputs(void);
int NormalScopeInDetectionCone(int index);
void NormalScopeObserve(int literal);
void NormalScopeEnd(void);
int NormalScopeRequiredNet(int index);
void NormalScopeModelValues(CCaDiCaL *solver, XID_VAR_INFO *values);
void NormalScopeModelRelease(void);
void NormalScopeRelease(void);

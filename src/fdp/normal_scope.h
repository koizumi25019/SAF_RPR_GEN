#pragma once
#include "ccadical.h"
#include "read.h"
#include "xid/XID.h"
/* 正常CNFの範囲限定は常時有効。全PIとFDPの分母を維持する。 */
/* Call after SearchTFO and BEFORE fault clauses / essential assignments. */
void NormalScopeBuild(FNODE *target);
int NormalScopeInDetectionCone(int index);
int NormalScopeRequiredNet(int index);
void NormalScopeModelValues(CCaDiCaL *solver, XID_VAR_INFO *values);
void NormalScopeModelRelease(void);
void NormalScopeRelease(void);

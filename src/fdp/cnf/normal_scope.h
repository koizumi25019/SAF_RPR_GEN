#pragma once
#include "ccadical.h"
#include "fault/fault_list.h"
#include "fdp/xid/XID.h"
/* 正常CNFの範囲限定は常時有効。全PIとFDPの分母を維持する。 */
/* Call after SearchTFO and BEFORE fault clauses / essential assignments. */
void NormalScopeBuild(FNODE *target);
int NormalScopeInDetectionCone(int index);
int NormalScopeRequiredNet(int index);
/* NormalScopeBuildの呼び出しごとに増える番号。範囲が変わったことを検出する。 */
unsigned long long NormalScopeGeneration(void);
void NormalScopeModelValues(CCaDiCaL *solver, XID_VAR_INFO *values);
void NormalScopeModelRelease(void);
void NormalScopeRelease(void);

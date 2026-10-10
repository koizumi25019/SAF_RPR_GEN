#pragma once
#include "ccadical.h"
#include "xid/XID.h"
/* 正常CNFの範囲限定は常時有効。全PIとFDPの分母を維持する。 */
/* Call after fault TFO and essential assignments have been constructed. */
void NormalScopeBuild(void);
int NormalScopeRequiredNet(int index);
void NormalScopeModelValues(CCaDiCaL *solver, XID_VAR_INFO *values);
void NormalScopeModelRelease(void);
void NormalScopeRelease(void);

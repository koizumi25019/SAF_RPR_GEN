#pragma once
#include "ccadical.h"
#include "xid/XID.h"
/* Default-on dependency-closed normal CNF scope. All original PIs are retained. */
int NormalScopeEnabled(void);
/* Call after fault TFO and essential assignments have been constructed. */
void NormalScopeBuild(void);
int NormalScopeRequiredNet(int index);
void NormalScopeModelValues(CCaDiCaL *solver, XID_VAR_INFO *values);
void NormalScopeModelRelease(void);
void NormalScopeRelease(void);

#pragma once
#include "ccadical.h"
#include "xid/XID.h"
/* Opt-in dependency-closed normal CNF scope. All original PIs are retained. */
int NormalScopeEnabled(void);
void NormalScopeBegin(void);
void NormalScopeObserve(int literal);
void NormalScopeEnd(void);
int NormalScopeRequiredNet(int index);
void NormalScopeModelValues(CCaDiCaL *solver, XID_VAR_INFO *values);
void NormalScopeModelRelease(void);
void NormalScopeRelease(void);

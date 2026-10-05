#pragma once
#include <time.h>
#include <stdio.h>
enum { PF_READNET, PF_READFAULT, PF_TARGET, PF_MODEL, PF_GOODLOAD,
 PF_SEARCH, PF_PROP, PF_DC, PF_EA, PF_INIT, PF_RELEASE, PF_XID_VALUES,
 PF_XID_FSIM, PF_XID_FILL, PF_N };
extern double profile_seconds[PF_N];
extern unsigned long profile_calls[PF_N];
#define PM(k, stmt) do { clock_t pt = clock(); stmt; profile_seconds[k] += (double)(clock()-pt)/CLOCKS_PER_SEC; profile_calls[k]++; } while(0)
void profile_report(void);
#include "ccadical.h"
void ProfileConeBegin(void);
void ProfileConeLiteral(int);
void ProfileConeLoad(CCaDiCaL*);
void ProfileConeReport(void);
int ProfileConeRequiredNet(int);
#include "netlist/netlist.h"
void SparseResetTFO(void);
void SparseRecord(int);
void SparseFinish(void);
int SparseCount(void);
int SparseNet(int);
void SparseEAReset(void);
void SparseEAMark(NLIST*);

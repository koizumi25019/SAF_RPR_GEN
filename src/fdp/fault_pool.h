#pragma once
#include "read.h"

typedef struct {
    double cadical, bdd, xid;
    long total_cubes, seeded_cubes, xstat_bits, xstat_x;
    long gt_checked, gt_unsound, gt_inexact;
} FaultStats;

/* One complete fault: CNF, enumeration, probability, and optional validation.
   Output streams belong to this task; no cubes or BDD nodes cross processes. */
typedef bool (*FaultTask)(FNODE*, int, FILE*, FILE*, FaultStats*, void*);
typedef void (*FaultFinish)(void*);

/* Stable order equivalent to repeated SetTarget(): level then hash traversal. */
FNODE** FaultPoolOrder(int* count);
void FaultStatsAdd(FaultStats* sum, const FaultStats* part);
bool FaultPoolRun(FNODE** faults, int count, int jobs, FaultTask task,
                  FaultFinish finish, void* context, FILE* csv, FILE* analysis,
                  FaultStats* stats);
double FaultPoolChildCPU(void);
int FaultPoolWorkers(void);

#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SBXID_SOLVE_ERROR = -1,
    SBXID_SOLVE_UNKNOWN = 0,
    SBXID_SOLVE_OPTIMAL,
    SBXID_SOLVE_FEASIBLE,
    SBXID_SOLVE_UNSAT
} SbxidSolveStatus;

typedef struct {
    SbxidSolveStatus status;
    int timed_out;
    double seconds; /* elapsed search time, including cooperative cancellation */
    char error[256];
} SbxidSolveResult;

/* values[1..variables] receives the last complete feasible OPB model.
 * No host netlist/global definitions cross this C/C++ boundary.
 * Parsing and preparation precede the search time limit.
 * limit_seconds == 0 disables the deadline and waits for optimization to finish.
 * Positive finite values impose an elapsed search time limit; negatives are invalid.
 */
SbxidSolveResult sbxid_solve_opb(const char* opb, size_t length, int variables,
                               int* values, double limit_seconds);
const char* sbxid_clasp_version(void);

#ifdef __cplusplus
}
#endif

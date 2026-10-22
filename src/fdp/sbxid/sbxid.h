#pragma once
#include <stdbool.h>
#include "clasp_bridge.h"

#define SBXID_X 2
#define SBXID_FLAG_EVALUATED (1 << 0)
#define SBXID_FLAG_PATH      (1 << 1)
#define SBXID_FLAG_JUSTIFY   (1 << 2)
#define SBXID_SOLVE_LIMIT_SECONDS 1.0

typedef struct {
    SbxidSolveResult solve;
    int variables;
    int constraints;
    bool verified;
    bool verification_failed;
} SbxidInfo;

/* seed and cube are distinct buffers of n_pi characters plus a terminator,
 * in host pi[] order.
 * seed is fully specified; cube uses 0/1/X. The caller owns both buffers.
 * On UNKNOWN, return seed. On UNSAT/invalid seed/solver error, return false.
 * FDP_SBXID_VERIFY=1 enables optional post-solve three-valued verification.
 * Process-local indexes and the structural fault TFO are initialized lazily
 * after fork, then reused. Internal S variables/justification are only built
 * inside that TFO; outside signals refer to V and retain both objective costs.
 */
bool sbxid_generalize(const char* seed, int fault_net_id, int stuck_value,
                     char* cube, SbxidInfo* info);
void sbxid_release(void);

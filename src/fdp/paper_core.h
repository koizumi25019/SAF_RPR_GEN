#pragma once
#include <stdbool.h>
#include "ccadical.h"
#include "./target_fault.h"

/* After WriteTPGModel: C_f AND NOT D_f, without generator constraints.
   For TDF, D_f includes excitation. Power constraints are not implemented. */
CCaDiCaL* PaperCoreBuildOracle(TARGET* target);

/* SAT 2024 Algorithm 1 CORE: complete model -> core -> deletion minimization.
   input_vars follows cube order. Failure leaves cube unchanged. verify also
   checks soundness and primality of the result. */
bool PaperCoreGeneralize(CCaDiCaL* oracle, const int* input_vars, int n_inputs,
                         char* cube, bool verify);
void PaperCoreReport(void);

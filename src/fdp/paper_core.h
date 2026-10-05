#pragma once
#include <stdbool.h>
#include "ccadical.h"
#include "./target_fault.h"

/* Definitions AND NOT(Detection AND Excitation AND Power).
   Power is omitted when disabled; no generator units/blocking clauses. */
CCaDiCaL* PaperCoreBuildOracle(TARGET* target);

/* SAT 2024 Algorithm 1 CORE: complete model -> core -> deletion minimization.
   input_vars follows cube order. Failure leaves cube unchanged. verify also
   checks soundness and primality of the result. */
bool PaperCoreGeneralize(CCaDiCaL* oracle, const int* input_vars, int n_inputs,
                         char* cube, bool verify);
void PaperCoreReport(void);

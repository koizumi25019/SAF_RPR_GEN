#pragma once

#include "ccadical.h"
#include "./read.h"

#define EA_UNKNOWN    (-1)
#define EA_DOWN       0
#define EA_UP         1
#define UNIQUE_DOWN   0
#define UNIQUE_UP     1
#define UNIQUE_MIDDLE 2

void EssentialAssignment(CCaDiCaL* solver, FNODE* target);

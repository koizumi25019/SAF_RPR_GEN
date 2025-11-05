#pragma once
//#define XID_TP_DIR "./tools/xid/"

#include <stdio.h>
#include <stdbool.h>
#include "./target.h"

bool make_blocking_clause(TARGET* target, FILE* cube_file);
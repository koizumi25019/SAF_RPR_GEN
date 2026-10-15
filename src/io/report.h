#pragma once
#include <stdbool.h>

bool OutLogfile(
    double wall_time,
	double cpu_time,
    double time_cadical,
    double time_bdd,
    double time_xid,
    double time_read
);

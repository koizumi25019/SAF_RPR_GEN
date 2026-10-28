#include <stdio.h>
#include "io/report.h"
#include "cli/options.h"
#include "fdp/analysis.h"
#include "fdp/worker_pool.h"
#include "io/file.h"

//*************************************************************************************************************
//	@name		OutLogfile
//	@function	output the log
//	@return		(bool) okay, error
//*************************************************************************************************************
static void WriteReport(
	FILE* fp,
    double wall_time,
	double cpu_time,
	double time_cadical,
	double time_bdd,
	double time_xid,
	double time_read
)
{
	fprintf(fp, "//--------------------------------------------------------------------------------\n");
	fprintf(fp, "//                AnalyzeFaultDetectionProbability Information\n");
	fprintf(fp, "//--------------------------------------------------------------------------------\n");
	fprintf(fp, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fp, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fp, "//  Fault Model                               : %s\n",
            opt.fault_model == FM_TDF ? "TDF (LOC; capture PPO only)" : "SAF");
	fprintf(fp, "//  Normal CNF Scope                          : on (affected-output TFI)\n");
	fprintf(fp, "//  Essential Assignment                      : on\n");
	fprintf(fp, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
    fprintf(fp, "//  Don't-care Method                         : XID\n");
    fprintf(fp, "//  Fault Workers                             : %d\n", FaultPoolWorkers());
    fprintf(fp, "//  CPU Accounting                            : parent + all worker processes\n");
    fprintf(fp, "//  Time                                      : %.3f sec\n", wall_time);
	fprintf(fp, "//  CPU Time                                  : %.3f sec\n", cpu_time);
	fprintf(fp, "//  CPU Time (CaDiCaL)                        : %.3f sec\n", time_cadical);
	fprintf(fp, "//  CPU Time (BDD)                            : %.3f sec\n", time_bdd);
	fprintf(fp, "//  CPU Time (Don't care)                     : %.3f sec\n", time_xid);
	fprintf(fp, "//  CPU Time (Read Fault)                     : %.3f sec\n", time_read);
	fprintf(fp, "//--------------------------------------------------------------------------------\n");
}

bool OutLogfile(
    double wall_time,
	double cpu_time,
	double time_cadical,
	double time_bdd,
	double time_xid,
	double time_read
)
{
	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.log, "w");
	WriteReport(fileptr, wall_time, cpu_time, time_cadical, time_bdd, time_xid, time_read);

    bool log_okay = !ferror(fileptr);
    if (fclose(fileptr) != 0) log_okay = false;
    if (!log_okay) return false;
	printf("\n\n");
	WriteReport(stdout, wall_time, cpu_time, time_cadical, time_bdd, time_xid, time_read);

	return !ferror(stdout);
}

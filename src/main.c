#define _POSIX_C_SOURCE 199309L
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./main.h"
#include "./opt/opt.h"
#include "./netlist/netlist.h"
#include "./fdp/fault_detection_prob.h"
#include "./lib/lib.h"


//*************************************************************************************************************
//	@name		main
//	@function	main
//	@return		(void)
//*************************************************************************************************************
int main(
	int					  argc,				 /**< number of command-arguments */
	char** argv								 /**< command-arguments */
)
{
struct timespec start, end;
    clock_t cpu_start, cpu_end; // CPU時間計測用に追加

	// CPU時間受け取り用
    double time_cadical = 0.0;
    double time_bdd     = 0.0;
    double time_xid     = 0.0;
    double time_read    = 0.0;

    // 計測開始
    clock_gettime(CLOCK_MONOTONIC, &start);// 実実行時間の計測開始
	cpu_start = clock(); // CPU時間の計測開始

	//set the option
	if (OPT(argc, argv) != OPT_OKAY) return RETCODE_ERROR;

	/** read the netlist */
	read_nl(opt.file.input.net);

	//analyze the fault detection probability
	if (AnalyzeFaultDensity(&time_cadical, &time_bdd, &time_xid, &time_read) != AFD_OKAY) return RETCODE_ERROR;

	// 計測終了
    clock_gettime(CLOCK_MONOTONIC, &end);// 実実行時間の計測終了
	cpu_end = clock(); // CPU時間の計測終了

    // 秒単位の経過時間を計算
    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	 // CPU時間を計算
	double cpu_time = (double)(cpu_end - cpu_start) / CLOCKS_PER_SEC;
						  
	OutLogfile(elapsed_time, cpu_time, time_cadical, time_bdd, time_xid, time_read);

	free_netlist();

	return 0;
}

//*************************************************************************************************************
//	@name		OutLogfile
//	@function	output the log
//	@return		(bool) okay, error
//*************************************************************************************************************
void OutLogfile(
	double time,
	double cpu_time,
    double time_cadical,
    double time_bdd,
    double time_xid,
    double time_read
)
{

	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.log, "w");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//                AnalyzeFaultDetectionProbability Information\n");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fileptr, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fileptr, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	fprintf(fileptr, "//  Time                                      : %.3f sec\n", time);
	fprintf(fileptr, "//  CPU Time                                  : %.3f sec\n", cpu_time);  // CPU時間
	fprintf(fileptr, "//  CPU Time (CaDiCaL)                        : %.3f sec\n", time_cadical);
    fprintf(fileptr, "//  CPU Time (BDD)                            : %.3f sec\n", time_bdd);
    fprintf(fileptr, "//  CPU Time (Don't care)                     : %.3f sec\n", time_xid);
    fprintf(fileptr, "//  CPU Time (Read Fault)                     : %.3f sec\n", time_read);
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");


	printf("\n\n");
	printf("//--------------------------------------------------------------------------------\n");
	printf("//                AnalyzeFaultDetectionProbability Information\n");
	printf("//--------------------------------------------------------------------------------\n");
	printf("//  Target Circuit                            : %s\n", net_name);
	printf("//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	printf("//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	printf("//  Time                                      : %.3f sec\n", time);
	printf("//  CPU Time                                  : %.3f sec\n", cpu_time);
	printf("//  CPU Time (CaDiCaL)                        : %.3f sec\n", time_cadical);
    printf("//  CPU Time (BDD)                            : %.3f sec\n", time_bdd);
    printf("//  CPU Time (Don't care)                     : %.3f sec\n", time_xid);
    printf("//  CPU Time (Read Fault)                     : %.3f sec\n", time_read);
	printf("//--------------------------------------------------------------------------------\n");

	return;
}








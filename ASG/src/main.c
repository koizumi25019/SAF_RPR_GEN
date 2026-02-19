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
#include "./asg/asg.h"
#include "./lib/lib.h"


//*************************************************************************************************************
//	@name		�F�@main
//	@function	�F	main
//	@return		�F	(void)
//*************************************************************************************************************
int main(
	int					  argc,				 /**< number of command-arguments */
	char** argv								 /**< command-arguments */
)
{
struct timespec start, end;
    
    // 計測開始
    clock_gettime(CLOCK_MONOTONIC, &start);

	char txt_cmd[50];

	//set the option
	if (OPT(argc, argv) != OPT_OKAY) return RETCODE_ERROR;


	/** read the netlist */
	read_nl(opt.file.input.net);

	/** output the pin */
	OutPIN();

	//analyze the fault detection probability
	if (AnalyzeFaultDensity() != AFD_OKAY) return RETCODE_ERROR;

	// 計測終了
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 秒単位の経過時間を計算
    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1000000000.0;
						  
	OutLogfile(elapsed_time);

	return 0;
}

//*************************************************************************************************************
//	@name		�F�@OutPIN
//	@function	�F	output the pin
//	@return		�F	(void)
//*************************************************************************************************************
void OutPIN(
	void
)
{
	FILE* fileptr = (FILE*)NULL;

	fileOpen(&fileptr, opt.file.output.pin, "w");

	for (int i = 0; i < n_pi; i++)
	{
		fprintf(fileptr, "%s\n", pi[i]->name);
	}

	fclose(fileptr);

	return;
}

//*************************************************************************************************************
//	@name		�F�@OutLogfile
//	@function	�F	output the log
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
void OutLogfile(
	double time
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
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");


	printf("\n\n");
	printf("//--------------------------------------------------------------------------------\n");
	printf("//                AnalyzeFaultDetectionProbability Information\n");
	printf("//--------------------------------------------------------------------------------\n");
	printf("//  Target Circuit                            : %s\n", net_name);
	printf("//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	printf("//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	printf("//  Time                                    : %.3f sec\n", time);
	printf("//--------------------------------------------------------------------------------\n");

	return;
}








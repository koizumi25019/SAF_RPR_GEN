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
bool main(
	int					  argc,				 /**< number of command-arguments */
	char** argv								 /**< command-arguments */
)
{
	clock_t start, end;
	start = clock();

	char txt_cmd[50];

	//set the option
	if (OPT(argc, argv) != OPT_OKAY) return RETCODE_ERROR;


	/** read the netlist */
	read_nl(opt.file.input.net);

	/** output the pin */
	OutPIN();

	//analyze the fault detection probability
	if (AnalyzeFaultDensity() != AFD_OKAY) return RETCODE_ERROR;

	end = clock();
	OutLogfile(end - start);

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
	clock_t time
)
{

	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.log, "w");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//                          AnalyzeFaultDetectionProbability Information\n");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fileptr, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fileptr, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	fprintf(fileptr, "//  Time                                      : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");


	printf("\n\n");
	printf("//--------------------------------------------------------------------------------\n");
	printf("//                          AnalyzeFaultDetectionProbability Information\n");
	printf("//--------------------------------------------------------------------------------\n");
	printf("//  Target Circuit                            : %s\n", net_name);
	printf("//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	printf("//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	printf("//  Time                                    : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	printf("//--------------------------------------------------------------------------------\n");

	return;
}








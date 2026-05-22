//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <crtdbg.h>

#include "./main.h"
#include "./standard.h"
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

	/** set the option */
	if (OPT(argc, argv) != OPT_OKAY) {
		system("chcp 65001 && YuyaMail.exe");
		return RETCODE_ERROR;
	}

	/** read the netlist */
	read_nl(opt.file.input.net);

	/** output the pin */
	OutPIN();

	//analyze the fault density
	if (AnalyzeFaultDensity() != AFD_OKAY) {
		system("chcp 65001 && YuyaMail.exe");
		return RETCODE_ERROR;
	}

	end = clock();
	OutLogfile(end - start);

	system("chcp 65001 && YuyaMail.exe");
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
	fprintf(fileptr, "//                          AnalyzeFaultDensity Information\n");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fileptr, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fileptr, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	fprintf(fileptr, "//  AnalyzeFaultDensity Time                  : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");


	PrintMessage("\n\n");
	PrintMessage("//--------------------------------------------------------------------------------\n");
	PrintMessage("//                          AnalyzeFaultDensity Information\n");
	PrintMessage("//--------------------------------------------------------------------------------\n");
	PrintMessage("//  Target Circuit                            : %s\n", net_name);
	PrintMessage("//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	PrintMessage("//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	PrintMessage("//  AnalyzeFaultDensity Time                  : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	PrintMessage("//--------------------------------------------------------------------------------\n");

	return;
}








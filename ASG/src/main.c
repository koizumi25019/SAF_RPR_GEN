#define _CRTDBG_MAP_ALLOC
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
//	@name		：　main
//	@function	：	main
//	@return		：	(void)
//*************************************************************************************************************
bool main(
	int					  argc,				 /**< number of command-arguments */
	char** argv								 /**< command-arguments */
)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	clock_t start, end;
	start = clock();
	SYSTEM_CLS;


	/** set the option */
	if (OPT(argc, argv) != OPT_OKAY) return RETCODE_ERROR;


	/** read the netlist */
	read_nl(opt.file.input.net);

	//analyze the fault density
	if (AnalyzeFaultDensity() != AFD_OKAY) return RETCODE_ERROR;

	end = clock();
	OutLogfile(end - start);
	colorDef
		return RETCODE_OKAY;
}

//*************************************************************************************************************
//	@name		：　OutLogfile
//	@function	：	output the log
//	@return		：	(bool) okay, error
//*************************************************************************************************************
void OutLogfile(
	clock_t time
)
{

	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.log, "w");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//                             SAF RPRF Generation Information\n");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fileptr, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fileptr, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	fprintf(fileptr, "//  SAF RPR Generation Time                   : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");


	PrintMessage("\n\n");
	PrintMessage("//--------------------------------------------------------------------------------\n");
	PrintMessage("//                             SAF RPR Generation Information\n");
	PrintMessage("//--------------------------------------------------------------------------------\n");
	PrintMessage("//  Target Circuit                            : %s\n", net_name);
	PrintMessage("//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	PrintMessage("//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	PrintMessage("//  SAF RPR Generation Time                   : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	PrintMessage("//--------------------------------------------------------------------------------\n");

	return;
}








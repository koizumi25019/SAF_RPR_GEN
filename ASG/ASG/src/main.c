/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	ASG																						 */
/*	file		:	./src/main.c																	  		 */
/*	deginer		:	R.miura			covered T.sone													  		 */
/*	date		:	2022.10.01		(2023.06.21)											  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

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
	clock_t start, end;
	start = clock();
	SYSTEM_CLS;
	PrintMessage("/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */\n");
	PrintMessage("                   %s     \n", PROJECT_NAME);
	PrintMessage("/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */\n\n");


	/** set the option */
	if (OPT(argc, argv) != OPT_OKAY) return RETCODE_ERROR;


	/** read the netlist */
	read_nl(opt.file.input.net);
	WARNING_NETLIST(n_dff);


	/** output the pin */
	OutPIN();


	/** automatic seed generater */
	if (ASG() != ASG_OKAY) return RETCODE_ERROR;

	end = clock();
	OutLogfile(end - start);
	colorDef
		return RETCODE_OKAY;
}

//*************************************************************************************************************
//	@name		：　OutPIN
//	@function	：	output the pin
//	@return		：	(void)
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
//	@name		：　OutLogfile
//	@function	：	output the log
//	@return		：	(bool) okay, error
//*************************************************************************************************************
void OutLogfile(
	clock_t time
)
{
	char* mode = (char*)NULL;
	char* target = (char*)NULL;

	mode = (char*)allocMemory(10, sizeof(char));
	target = (char*)allocMemory(10, sizeof(char));

	if (opt.mode.mode == SEED)				sprintf_s(mode, 10, "Seed");
	else if (opt.mode.mode == TEST)			sprintf_s(mode, 10, "Test");
	if (opt.mode.target == SINGLE)			sprintf_s(target, 10, "Single");
	else if (opt.mode.target == MULTIPLE)	sprintf_s(target, 10, "Multiple");

	/** output the test pattern */

	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.log, "w");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//                             ASG Information\n");
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");
	fprintf(fileptr, "//  Mode                                      : %s %s Generation\n", target, mode);
	fprintf(fileptr, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fileptr, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fileptr, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	fprintf(fileptr, "//  Number of Detected Faults                 : %d\n", readdata.fault.numdete);
	fprintf(fileptr, "//  Number of Unsolved Faults                 : %d\n", readdata.fault.numred);
	fprintf(fileptr, "//  Number of Compatible fault set generation : %d\n", num_clique);
	if (opt.mode.mode == SEED)	fprintf(fileptr, "//  Number of Seed generation                 : %d\n", num_seed);
	else if (opt.mode.mode == TEST)	fprintf(fileptr, "//  Number of Test Pattern                    : %d\n", num_seed);
	fprintf(fileptr, "//  Generated Compatible Fault Set Time       : %.3f sec\n", ((float)time_graph) / CLOCKS_PER_SEC);
	fprintf(fileptr, "//  Automatic Seed Generation Time            : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	fprintf(fileptr, "//--------------------------------------------------------------------------------\n");


	FILE* fileptr2 = (FILE*)NULL;
	char* filename = (char*)NULL;
	filename = (char*)allocMemory(MAXSIZE_CHAR, sizeof(char));
	sprintf_s(filename, MAXSIZE_CHAR, "./output/log/%s_remainlog.txt",net_name);
	fileOpen(&fileptr2, filename, "w");

	for (int i = 0;remain_log[i] != -1;i++)
	{
		fprintf(fileptr2, "%d\n", remain_log[i]);
	}
	fclose(fileptr2);

	FILE* fileptr3 = (FILE*)NULL;
	char* filename2 = (char*)NULL;
	filename2 = (char*)allocMemory(MAXSIZE_CHAR, sizeof(char));
	sprintf_s(filename2, MAXSIZE_CHAR, "./output/log/%s_detectlog.txt", net_name);
	fileOpen(&fileptr3, filename2, "w");

	for (int i = 0;detect_log[i] != -1;i++)
	{
		fprintf(fileptr3, "%d\n", detect_log[i]);
	}
	fclose(fileptr3);

	PrintMessage("\n\n");
	PrintMessage("//--------------------------------------------------------------------------------\n");
	PrintMessage("//                             ASG Information\n");
	PrintMessage("//--------------------------------------------------------------------------------\n");
	PrintMessage("//  Mode                                      : %s %s Generation\n", target, mode);
	PrintMessage("//  Target Circuit                            : %s\n", net_name);
	PrintMessage("//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	PrintMessage("//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	PrintMessage("//  Number of Detected Faults                 : %d\n", readdata.fault.numdete);
	PrintMessage("//  Number of Unsolved Faults                 : %d\n", readdata.fault.numred);
	PrintMessage("//  Number of Compatible fault set generation : %d\n", num_clique);
	if (opt.mode.mode == SEED) { PrintMessage("//  Number of Seed generation                 : %d\n", num_seed); }
	else if (opt.mode.mode == TEST) { PrintMessage("//  Number of Test Pattern                    : %d\n", num_seed); }
	PrintMessage("//  Generated Compatible Fault Set Time       : %.3f sec\n", ((float)time_graph) / CLOCKS_PER_SEC);
	PrintMessage("//  Automatic Seed Generation Time            : %.3f sec\n", ((float)time) / CLOCKS_PER_SEC);
	PrintMessage("//--------------------------------------------------------------------------------\n");



	free(filename);
	free(filename2);
	free(mode);
	free(target);
	return;
}








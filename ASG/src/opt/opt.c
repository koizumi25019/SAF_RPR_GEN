/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	urashima																				 */
/*	file		:	./src/opt/opt.c																	         */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.10.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "./opt.h"
#include "../standard.h"
#include "../asg/read.h"
#include "../lib/lib.h"
#include "../debug/debug.h"


//*************************************************************************************************************
//	@name		：　OPT
//	@function	：	analyze the option
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool OPT(
	int					  argc,			      /**< number of command-arguments */
	char** argv			      /**< command-arguments */
)
{
	/** initialize the options */
	OPTinit();

	/** set the options */
	if (OPTset(argc, argv) != OPT_OKAY)
	{
		/** display the help */
		OPTdispHelp();

		return OPT_ERROR;
	}


#ifdef __DEBUG_OPT__
	_CALL_DEBUG_OPT_(argc);
#endif

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		：　OPTinit
//	@function	：	initialize the options
//	@return		：	(void)
//*************************************************************************************************************
void OPTinit(
	void
)
{
	/** initialize the filename */
	OPTinitFile();


	return;
}

//*************************************************************************************************************
//	@name		：　OPTinitFile
//	@function	：	initialize the filename
//	@return		：	(void)
//*************************************************************************************************************
void OPTinitFile(
	void
)
{
	opt.file.input.fault		= FILE_NOSET;
	opt.file.input.net			= FILE_NOSET;
	opt.file.input.scanchain	= FILE_NOSET;

	opt.file.output.log			= FILE_NOSET;


	return;
}

//*************************************************************************************************************
//	@name		：　OPTset
//	@function	：	set the options
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool OPTset(
	int				      argc,				  /**< number of command-arguments */
	char** argv				  /**< command-arguments */
)
{
	for (int i = 1; i < argc; i++)
	{
		/** help */
		if (strcmp(argv[i], "-help") == 0)
			return OPT_ERROR;

		/** netlist */
		else if (strcmp(argv[i], "-n") == 0)
			opt.file.input.net = _strdup(argv[++i]);

		/** fault-list */
		else if (strcmp(argv[i], "-f") == 0)
			opt.file.input.fault = _strdup(argv[++i]);

		/** scan-chain */
		else if (strcmp(argv[i], "-c") == 0)
			opt.file.input.scanchain = _strdup(argv[++i]);

		/** log */
		else if (strcmp(argv[i], "-l") == 0)
			opt.file.output.log = _strdup(argv[++i]);

		/** pin */
		else if (strcmp(argv[i], "-p") == 0)
			opt.file.output.pin = _strdup(argv[++i]);

		/** read the option */
		else if (strcmp(argv[i], "-z") == 0)
			return OPTread(argv[++i]);

		/** rpr  */
		else if (strcmp(argv[i], "-rpr") == 0)
			opt.file.output.rpr = _strdup(argv[++i]);

		else
		{
			PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
			PrintErrorMessage("%c%s%c is not expected.\n\n", '"', argv[i], '"');
			colorDef
			return OPT_ERROR;
		}
	}

	return OPT_OKAY;
}


//*************************************************************************************************************
//	@name		：　OPTread
//	@function	：	read the option-setting file
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool OPTread(
	char* filename			  /**< filename */
)
{
	FILE* fileptr = (FILE*)NULL;
	char* buffer = (char*)NULL;
	char* context = (char*)NULL;
	char* token1 = (char*)NULL;
	char* token2 = (char*)NULL;

	/** open the "setting file" in read-mode */
	fileOpen(&fileptr, filename, "r");

	buffer = allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** read the option */
	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (buffer[0] == '-')
		{
			token1 = strtok_s(buffer, " \n\0", &context);

			/** help */
			if (strcmp(token1, "-help") == 0) return OPT_ERROR;

			/** netlist */
			else if (strcmp(token1, "-n") == 0)
			{
				token2 = strtok_s(NULL, " \n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.net = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** fault list */
			else if (strcmp(token1, "-f") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.fault = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** scan-chain */
			else if (strcmp(token1, "-c") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.scanchain = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** log */
			else if (strcmp(token1, "-l") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.log = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** pin */
			else if (strcmp(token1, "-p") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.pin = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

				/** mode seed */
			else if (!strcmp(token1, "-m"))
			{
				token2 = strtok_s(NULL, " \n\0", &context);

				for (int i = 0; i < strlen(buffer); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						token2 = strtok_s(&token2[i], " \n\0", &context);

						break;
					}
				}
			}

			/** rpr fault */
			else if (strcmp(token1, "-rpr") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.rpr = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			else
			{
				PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
				PrintErrorMessage("%c%s%c is not expected.\n\n", '"', token1, '"');
				colorDef
				return OPT_ERROR;
			}
		}
	}

	free(buffer);

	fclose(fileptr);

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		：　OPTcheck
//	@function	：	check for essential options
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool OPTcheck(
	void
)
{

	if (OPTcheckFile() != OPT_OKAY)	return OPT_ERROR;

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		：　OPTcheckFile
//	@function	：	check for essential file
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool OPTcheckFile(
	void
)
{
	if (opt.file.input.net == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no netlist file.\n\n");
		colorDef
		return OPT_ERROR;
	}

	if (opt.file.input.fault == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no fault list file.\n\n");
		colorDef
		return OPT_ERROR;
	}

	if (opt.file.input.scanchain == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no scan-chain file.\n\n");
		colorDef
		return OPT_ERROR;
	}

	if (opt.file.output.pin == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no pin file.\n\n");
		colorDef
		return OPT_ERROR;
	}

	if (opt.file.output.log == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no log file.\n\n");
		colorDef
			return OPT_ERROR;
	}


	return	OPT_OKAY;
}

//*************************************************************************************************************
//	@name		：　OPTdispHelp
//	@function	：	display the help
//	@return		：	(void)
//*************************************************************************************************************
void OPTdispHelp(
	void
)
{
	PrintHelpMessage("\n	> help ***********************************************************************************\n\n");

	PrintHelpMessage("		command = urashima -n <.v> -f <.txt> -p <.txt> -t <.txt>\n");

	PrintHelpMessage("\n		>> file option \n");

	PrintHelpMessage("		   -n(essential)   :   netlist <.v> \n");
	PrintHelpMessage("		   -f(essential)   :   fault list <.txt> \n");
	PrintHelpMessage("		   -c(essential)   :   scan-chain <.txt> \n");

	PrintHelpMessage("		   -l              :   log file <.txt>\n");
	PrintHelpMessage("		   -p(essential)   :   pin file <.txt>\n");

	PrintHelpMessage("		   -z              :   setting options by file <.set> \n");

	PrintHelpMessage("\n		>> help \n");
	PrintHelpMessage("		   -help           :   print the help. \n");

	PrintHelpMessage("\n");
	colorDef

		return;
}










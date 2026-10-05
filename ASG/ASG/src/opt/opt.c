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

	/** check for essential options */
	if (OPTcheck() != OPT_OKAY)
	{
		/** display the help */
		OPTdispHelp();

		return OPT_ERROR;
	}


#ifdef __DEBUG_OPT__
	_CALL_DEBUG_OPT_(argc);
#endif


	PrintMessage("	Setup options completed ... \n");

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

	/** initialize the mode */
	OPTinitMode();

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
	opt.file.input.necessary	= FILE_NOSET;
	opt.file.input.xortap		= FILE_NOSET;

	opt.file.output.graphlog	= FILE_NOSET;
	opt.file.output.log			= FILE_NOSET;
	opt.file.output.pin			= FILE_NOSET;
	opt.file.output.seed		= FILE_NOSET;
	opt.file.output.test		= FILE_NOSET;
	opt.file.output.untestable	= FILE_NOSET;

	return;
}

//*************************************************************************************************************
//	@name		：　OPTinitMode
//	@function	：	initialize the mode
//	@return		：	(void)
//*************************************************************************************************************
void OPTinitMode(
	void
)
{
	opt.mode.target = MULTIPLE;
	opt.mode.mode = SEED;

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

		/** xor-tap */
		else if (strcmp(argv[i], "-x") == 0)
			opt.file.input.xortap = _strdup(argv[++i]);

		/** log */
		else if (strcmp(argv[i], "-l") == 0)
			opt.file.output.log = _strdup(argv[++i]);

		/** pin */
		else if (strcmp(argv[i], "-p") == 0)
			opt.file.output.pin = _strdup(argv[++i]);

		/** seed */
		else if (strcmp(argv[i], "-s") == 0)
			opt.file.output.seed = _strdup(argv[++i]);

		/** test pattern */
		else if (strcmp(argv[i], "-t") == 0)
			opt.file.output.test = _strdup(argv[++i]);

		/** untestable fault */
		else if (strcmp(argv[i], "-u") == 0)
			opt.file.output.untestable = _strdup(argv[++i]);

		/** read the option */
		else if (strcmp(argv[i], "-z") == 0)
			return OPTread(argv[++i]);

		/** mode target */
		else if (strcmp(argv[i], "-o") == 0)
		{
			if (strcmp(argv[++i], "SINGLE") == 0)
				opt.mode.target = SINGLE;

			else if (strcmp(argv[i], "MULTIPLE") == 0)
				opt.mode.target = MULTIPLE;

			else
			{
				PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
				PrintErrorMessage("%c-d=%s%c is not expected.\n\n", '"', argv[i], '"');
				colorDef
					return OPT_ERROR;
			}
		}

		/** mode seed */
		else if (strcmp(argv[i], "-m") == 0)
		{
			if (strcmp(argv[++i], "SEED") == 0)
				opt.mode.mode = SEED;

			else if (strcmp(argv[i], "TEST") == 0)
				opt.mode.mode = TEST;

			else
			{
				PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
				PrintErrorMessage("%c-d=%s%c is not expected.\n\n", '"', argv[i], '"');
				colorDef
					return OPT_ERROR;
			}
		}

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

			/** xor-tap */
			else if (strcmp(token1, "-x") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.xortap = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}
			/** graph log */
			else if (strcmp(token1, "-g") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.graphlog = _strdup(strtok_s(&token2[i], " \n\0", &context));
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

			/** seed */
			else if (strcmp(token1, "-s") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.seed = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** test pattern */
			else if (strcmp(token1, "-t") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.test = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** untestable fault */
			else if (strcmp(token1, "-u") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.untestable = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** mode target */
			else if (!strcmp(token1, "-o"))
			{
				token2 = strtok_s(NULL, " \n\0", &context);

				for (int i = 0; i < strlen(buffer); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						token2 = strtok_s(&token2[i], " \n\0", &context);

						if (!strcmp(token2, "SINGLE"))
							opt.mode.target = SINGLE;

						else if (!strcmp(token2, "MULTIPLE"))
							opt.mode.target = MULTIPLE;

						else
						{
							PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
							PrintErrorMessage("%c-d=%s%c is not expected.\n\n", '"', token2, '"');
							colorDef
								return OPT_ERROR;
						}
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

						if (!strcmp(token2, "SEED"))
							opt.mode.mode = SEED;

						else if (!strcmp(token2, "TEST"))
							opt.mode.mode = TEST;

						else
						{
							PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
							PrintErrorMessage("%c-d=%s%c is not expected.\n\n", '"', token2, '"');
							colorDef
								return OPT_ERROR;
						}
						break;
					}
				}
			}

			/** untestable fault */
			else if (strcmp(token1, "-e") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.necessary = _strdup(strtok_s(&token2[i], " \n\0", &context));
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
	if (OPTcheckMode() != OPT_OKAY)	return OPT_ERROR;

	if (OPTcheckFile() != OPT_OKAY)	return OPT_ERROR;

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		：　OPTcheckMode
//	@function	：	check for essential mode
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool OPTcheckMode(
	void
)
{
#ifdef NDEBUG
	assert(opt.mode.drop == DROP_NO || opt.mode.drop == DROP_YES);
#endif

	if (opt.mode.target != SINGLE && opt.mode.target != MULTIPLE)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("%c-m%c is not expected.\n\n", '"', '"');
		colorDef
			return OPT_ERROR;
	}

	return	OPT_OKAY;
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

	if (opt.file.input.xortap == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no xor-tap file.\n\n");
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

	if (opt.file.output.seed == FILE_NOSET)
	{
		PrintErrorMessage("\n	COMMAND ERROR: option setup is failed. ");
		PrintErrorMessage("no seed set file.\n\n");
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
	PrintHelpMessage("		   -x(essential)   :   xor-tap list <.txt> \n");
	PrintHelpMessage("		   -c(essential)   :   scan-chain <.txt> \n");

	PrintHelpMessage("		   -l              :   log file <.txt>\n");
	PrintHelpMessage("		   -p(essential)   :   pin file <.txt>\n");
	PrintHelpMessage("		   -s(essential)   :   seed set file <.txt> \n");
	PrintHelpMessage("		   -t              :   test set file <.txt> \n");
	PrintHelpMessage("		   -u              :   untestable fault list <.txt> \n");

	PrintHelpMessage("		   -z              :   setting options by file <.set> \n");

	PrintHelpMessage("\n		>> mode option \n");
	PrintHelpMessage("		   -m              :   mode . %cSEED%c is default. \n", '"', '"');
	PrintHelpMessage("\n		>> help \n");
	PrintHelpMessage("		   -help           :   print the help. \n");

	PrintHelpMessage("\n");
	colorDef

		return;
}










//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>

#include "./opt.h"
#include "../fdp/read.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		OPT
//	@function	analyze the option
//	@return		(bool) okay, error
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
		return OPT_ERROR;
	}

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		OPTinit
//	@function	initialize the options
//	@return		(void)
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
//	@name		OPTinitFile
//	@function	initialize the filename
//	@return		(void)
//*************************************************************************************************************
void OPTinitFile(
	void
)
{
	opt.file.input.fault		 = FILE_NOSET;
	opt.file.input.net			 = FILE_NOSET;
	opt.file.input.cube_analysis = FILE_NOSET;
	opt.file.output.log			 = FILE_NOSET;

	return;
}

//*************************************************************************************************************
//	@name		F@OPTset
//	@function	F	set the options
//	@return		F	(bool) okay, error
//*************************************************************************************************************
bool OPTset(
	int				      argc,				  /**< number of command-arguments */
	char** argv				  /**< command-arguments */
)
{
	for (int i = 1; i < argc; i++)
	{
		/** netlist */
		if (strcmp(argv[i], "-net") == 0)
			opt.file.input.net = strdup(argv[++i]);

		/** fault-list */
		else if (strcmp(argv[i], "-fault") == 0)
			opt.file.input.fault = strdup(argv[++i]);

	    /** cube analysis mode */
		else if (strcmp(argv[i], "-cube_analysis") == 0)
			opt.file.input.cube_analysis = strdup(argv[++i]);

		/** log */
		else if (strcmp(argv[i], "-log") == 0)
			opt.file.output.log = strdup(argv[++i]);

		/** fdp result  */
		else if (strcmp(argv[i], "-fdp") == 0)
			opt.file.output.fdp = strdup(argv[++i]);

		/** limit  */
		else if (strcmp(argv[i], "-limit") == 0)
			opt.file.input.limit = atoi(argv[++i]);

		/** read the setfile */
		else if (strcmp(argv[i], "-set") == 0)
			return OPTread(argv[++i]);

		else
		{
			printf("\n	COMMAND ERROR: option setup is failed. ");
			printf("%c%s%c is not expected.\n\n", '"', argv[i], '"');
			
			return OPT_ERROR;
		}
	}

	return OPT_OKAY;
}


//*************************************************************************************************************
//	@name		OPTread
//	@function	read the option-setting file
//	@return		(bool) okay, error
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
			token1 = strtok_r(buffer, " \n\0", &context);

			/** netlist */
			if (strcmp(token1, "-net") == 0)
			{
				token2 = strtok_r(NULL, " \n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.net = strdup(strtok_r(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** cube analysis file */
			else if (strcmp(token1, "-cube_analysis") == 0)
			{
				token2 = strtok_r(NULL, " \n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.cube_analysis = strdup(strtok_r(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** fault list */
			else if (strcmp(token1, "-fault") == 0)
			{
				token2 = strtok_r(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.fault = strdup(strtok_r(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** log */
			else if (strcmp(token1, "-log") == 0)
			{
				token2 = strtok_r(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.log = strdup(strtok_r(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** fdp result */
			else if (strcmp(token1, "-fdp") == 0)
			{
				token2 = strtok_r(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.fdp = strdup(strtok_r(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** limit setting */
			else if (strcmp(token1, "-limit") == 0)
			{
				token2 = strtok_r(NULL, " \n\0", &context);

				// 次トークンのNULLチェック
				if (token2 != NULL)
				{
					int val = atoi(token2);

					if (val == 0)
					{
						opt.file.input.limit = 0;
					}
					else
					{
						opt.file.input.limit = val;
					}
				}
			}

			else
			{
				printf("\n	COMMAND ERROR: option setup is failed. ");
				printf("%c%s%c is not expected.\n\n", '"', token1, '"');
				
				return OPT_ERROR;
			}
		}
	}

	free(buffer);

	fclose(fileptr);

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		OPTcheck
//	@function	check for essential options
//	@return		(bool) okay, error
//*************************************************************************************************************
bool OPTcheck(
	void
)
{

	if (OPTcheckFile() != OPT_OKAY)	return OPT_ERROR;

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		OPTcheckFile
//	@function	check for essential file
//	@return		(bool) okay, error
//*************************************************************************************************************
bool OPTcheckFile(
	void
)
{
	if (opt.file.input.net == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no netlist file.\n\n");
		
		return OPT_ERROR;
	}

	if (opt.file.input.fault == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no fault list file.\n\n");
		
		return OPT_ERROR;
	}

	if (opt.file.output.log == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no log file.\n\n");
		
			return OPT_ERROR;
	}

	if (opt.file.output.fdp == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no fdp result file.\n\n");

			return OPT_ERROR;
	}

	return	OPT_OKAY;
}
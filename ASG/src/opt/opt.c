//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "./opt.h"
#include "../asg/read.h"
#include "../lib/lib.h"

//*************************************************************************************************************
//	@name		�F�@OPT
//	@function	�F	analyze the option
//	@return		�F	(bool) okay, error
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

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@OPTinit
//	@function	�F	initialize the options
//	@return		�F	(void)
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
//	@name		�F�@OPTinitFile
//	@function	�F	initialize the filename
//	@return		�F	(void)
//*************************************************************************************************************
void OPTinitFile(
	void
)
{
	opt.file.input.fault		= FILE_NOSET;
	opt.file.input.net			= FILE_NOSET;
	opt.file.output.log			= FILE_NOSET;

	return;
}

//*************************************************************************************************************
//	@name		�F�@OPTset
//	@function	�F	set the options
//	@return		�F	(bool) okay, error
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
		else if (strcmp(argv[i], "-net") == 0)
			opt.file.input.net = _strdup(argv[++i]);

		/** fault-list */
		else if (strcmp(argv[i], "-fault") == 0)
			opt.file.input.fault = _strdup(argv[++i]);

		/** log */
		else if (strcmp(argv[i], "-log") == 0)
			opt.file.output.log = _strdup(argv[++i]);

		/** pin */
		else if (strcmp(argv[i], "-pin") == 0)
			opt.file.output.pin = _strdup(argv[++i]);

		/** result  */
		else if (strcmp(argv[i], "-result") == 0)
			opt.file.output.result = _strdup(argv[++i]);

		/** limit  */
		else if (strcmp(argv[i], "-limit") == 0)
			opt.file.input.limit = atoi(argv[++i]);

		/** random pattern count  */
		else if (strcmp(argv[i], "-rpc") == 0)
		{
			int count = 0;

			char* arg_str = argv[++i];

			char* token = strtok(arg_str, ",");

			while (token != NULL && count < 100) {
				opt.file.input.pattern_num_list[count++] = atoi(token);
				token = strtok(NULL, ",");
			}
			opt.file.input.list_size = count;
		}

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
//	@name		�F�@OPTread
//	@function	�F	read the option-setting file
//	@return		�F	(bool) okay, error
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
			else if (strcmp(token1, "-net") == 0)
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
			else if (strcmp(token1, "-fault") == 0)
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

			/** test relation */
			else if (strcmp(token1, "-relation") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.input.relation = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** log */
			else if (strcmp(token1, "-log") == 0)
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
			else if (strcmp(token1, "-pin") == 0)
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

			/** result fault */
			else if (strcmp(token1, "-result") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						opt.file.output.result = _strdup(strtok_s(&token2[i], " \n\0", &context));
						break;
					}
				}
			}

			/** random pattern list */
			else if (strcmp(token1, "-rp") == 0)
			{
				token2 = strtok_s(NULL, "\n\0", &context);

				for (int i = 0; i < strlen(token2); i++)
				{
					if (token2[i] != ' ' && token2[i] != '\t')
					{
						int count = 0;
						char* inner_context = NULL;
						char* num_token = NULL;

						num_token = strtok_s(&token2[i], ", \n\r", &inner_context);

						while (num_token != NULL && count < 100)
						{
							// �������int�ɕϊ����Ċi�[
							opt.file.input.pattern_num_list[count++] = atoi(num_token);

							// ���̐��l���擾
							num_token = strtok_s(NULL, ", \n\r", &inner_context);
						}

						opt.file.input.list_size = count;
						break;
					}
				}
			}

			/** limit setting */
			else if (strcmp(token1, "-limit") == 0)
			{
				// ���̃g�[�N���i���l�j���擾
				token2 = strtok_s(NULL, " \n\0", &context);

				// �O�̂���NULL�`�F�b�N
				if (token2 != NULL)
				{
					int val = atoi(token2);

					//  0�̏ꍇ�̓��~�b�g�Ȃ��Ƃ���
					if (val == 0)
					{
						opt.file.input.limit = 0;         // �l���ꉞ0�ɂ��Ă���
					}
					else
					{
						opt.file.input.limit = val;        // �w�肳�ꂽ���l���i�[
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
//	@name		�F�@OPTcheck
//	@function	�F	check for essential options
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool OPTcheck(
	void
)
{

	if (OPTcheckFile() != OPT_OKAY)	return OPT_ERROR;

	return OPT_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@OPTcheckFile
//	@function	�F	check for essential file
//	@return		�F	(bool) okay, error
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

	if (opt.file.output.pin == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no pin file.\n\n");
		
		return OPT_ERROR;
	}

	if (opt.file.output.log == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no log file.\n\n");
		
			return OPT_ERROR;
	}

	if (opt.file.output.result == FILE_NOSET)
	{
		printf("\n	COMMAND ERROR: option setup is failed. ");
		printf("no log file.\n\n");
		
			return OPT_ERROR;
	}

	return	OPT_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@OPTdispHelp
//	@function	�F	display the help
//	@return		�F	(void)
//*************************************************************************************************************
void OPTdispHelp(
	void
)
{
	printf("\n	> help ***********************************************************************************\n\n");

	printf("		command = -net <.v> -fault <.txt> -pin <.txt>\n");

	printf("\n		>> file option \n");

	printf("		   -net(essential)   :   netlist <.v> \n");
	printf("		   -fault(essential)   :   fault list <.txt> \n");

	printf("		   -log              :   log file <.txt>\n");
	printf("		   -pin(essential)   :   pin file <.txt>\n");

	printf("		   -set              :   setting options by file <.set> \n");

	printf("\n		>> help \n");
	printf("		   -help           :   print the help. \n");

	printf("\n");
	

		return;
}










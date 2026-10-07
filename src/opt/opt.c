//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "./opt.h"
#include "../fdp/read.h"
#include "../lib/lib.h"

static bool OPTisModeOption(const char* name)
{
    return strcmp(name, "-dc_method") == 0 || strcmp(name, "-dom_reuse") == 0 ||
           strcmp(name, "-core_verify") == 0 || strcmp(name, "-low_power") == 0 ||
           strcmp(name, "-wsa_threshold") == 0 ||
           strcmp(name, "-core_minimize") == 0 || strcmp(name, "-core_recheck") == 0;
}

static bool OPTsetMode(const char* name, const char* value)
{
    if (strcmp(name, "-wsa_threshold") == 0) {
        char* end = NULL;
        errno = 0;
        long percent = value ? strtol(value, &end, 10) : -1;
        if (value && *value && end != value && !*end && !errno && percent >= 0 && percent <= 100) {
            opt.wsa_threshold = (int)percent;
            return OPT_OKAY;
        }
        fprintf(stderr, "COMMAND ERROR: -wsa_threshold requires an integer 0..100\n");
        return OPT_ERROR;
    }
    if (value && strcmp(name, "-dc_method") == 0) {
        if (strcmp(value, "xid") == 0) { opt.dc_method = DC_XID; return OPT_OKAY; }
        if (strcmp(value, "core") == 0) { opt.dc_method = DC_CORE; return OPT_OKAY; }
    } else if (value) {
        int enabled = MODE_NOSET;
        if (strcmp(value, "on") == 0 || strcmp(value, "1") == 0) enabled = YES;
        if (strcmp(value, "off") == 0 || strcmp(value, "0") == 0) enabled = NO;
        if (enabled != MODE_NOSET) {
            if (strcmp(name, "-dom_reuse") == 0) opt.dom_reuse = enabled;
            else if (strcmp(name, "-core_verify") == 0) opt.core_verify = enabled;
            else if (strcmp(name, "-core_minimize") == 0) opt.core_minimize = enabled;
            else if (strcmp(name, "-core_recheck") == 0) opt.core_recheck = enabled;
            else opt.low_power = enabled;
            return OPT_OKAY;
        }
    }
    fprintf(stderr, "COMMAND ERROR: %s requires %s (got %s)\n", name,
            strcmp(name, "-dc_method") == 0 ? "xid|core" : "on|off",
            value ? value : "no value");
    return OPT_ERROR;
}

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

    /* Explicit .set/CLI values override legacy environment switches. */
    if (opt.dc_method == MODE_NOSET) {
        opt.dc_method = getenv("PAPER_CORE") ? DC_CORE : DC_XID;
    }
    if (opt.dom_reuse == MODE_NOSET) {
        opt.dom_reuse = getenv("MDC_NODOM") ? NO : YES;
    }
    if (opt.core_verify == MODE_NOSET) {
        opt.core_verify = getenv("PAPER_CORE_VERIFY") ? YES : NO;
    }
    if (opt.low_power == YES &&
        (opt.fault_model != FM_TDF || opt.dc_method != DC_CORE || opt.wsa_threshold < 0)) {
        fprintf(stderr, "COMMAND ERROR: -low_power on requires -tdf, -dc_method core and -wsa_threshold 0..100\n");
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

	/** fault model: default is stuck-at */
	opt.fault_model = FM_SAF;
    opt.dc_method = MODE_NOSET;
    opt.dom_reuse = MODE_NOSET;
    opt.core_verify = MODE_NOSET;
    opt.core_minimize = YES;
    opt.core_recheck = YES;
    opt.low_power = NO;
    opt.wsa_threshold = -1;

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
		if (OPTisModeOption(argv[i])) {
            const char* name = argv[i];
            const char* value = i + 1 < argc ? argv[++i] : NULL;
            if (!OPTsetMode(name, value)) return OPT_ERROR;
        }
		/** netlist */
		else if (strcmp(argv[i], "-net") == 0)
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

		/** fault model */
		else if (strcmp(argv[i], "-saf") == 0)
			opt.fault_model = FM_SAF;

		else if (strcmp(argv[i], "-tdf") == 0)
			opt.fault_model = FM_TDF;

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
//	@name		OPTreadValue
//	@function	ディレクティブ行から値（先頭の空白を飛ばした最初のトークン）を取り出して複製する
//	@return		(char*) strdup した値。値が無ければ NULL
//	@note		first_delim は最初の取り出しの区切り。"-net"/"-cube_analysis" は空白区切り
//	            (" \n\0")、それ以外は行末まで ("\n\0") という従来挙動をそのまま渡す。
//*************************************************************************************************************
static char* OPTreadValue(
	char** context,			  /**< strtok_r の saveptr */
	const char* first_delim	  /**< 最初の取り出しに使う区切り文字 */
)
{
	char* token = strtok_r(NULL, first_delim, context);
	if (token == NULL) return (char*)NULL;

	for (int i = 0; token[i] != '\0'; i++)
	{
		if (token[i] != ' ' && token[i] != '\t')
			return strdup(strtok_r(&token[i], " \n\0", context));
	}
	return (char*)NULL;
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
			token1 = strtok_r(buffer, " \t\r\n", &context);

            if (OPTisModeOption(token1)) {
                token2 = strtok_r(NULL, " \t\r\n", &context);
                if (!OPTsetMode(token1, token2)) {
                    free(buffer);
                    fclose(fileptr);
                    return OPT_ERROR;
                }
            }

			/** netlist */
			else if (strcmp(token1, "-net") == 0)
				opt.file.input.net = OPTreadValue(&context, " \n\0");

			/** cube analysis file */
			else if (strcmp(token1, "-cube_analysis") == 0)
				opt.file.input.cube_analysis = OPTreadValue(&context, " \n\0");

			/** fault list */
			else if (strcmp(token1, "-fault") == 0)
				opt.file.input.fault = OPTreadValue(&context, "\n\0");

			/** log */
			else if (strcmp(token1, "-log") == 0)
				opt.file.output.log = OPTreadValue(&context, "\n\0");

			/** fdp result */
			else if (strcmp(token1, "-fdp") == 0)
				opt.file.output.fdp = OPTreadValue(&context, "\n\0");

			/** fault model */
			else if (strcmp(token1, "-saf") == 0)
				opt.fault_model = FM_SAF;

			else if (strcmp(token1, "-tdf") == 0)
				opt.fault_model = FM_TDF;

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

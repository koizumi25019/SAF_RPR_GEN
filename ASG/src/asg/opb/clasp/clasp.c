//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "./clasp.h"
#include "../../read.h"
#include "../../../lib/lib.h"

//*************************************************************************************************************
//	@name		�F�@CLASP
//	@function	�F	call the "clasp.exe"
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool CLASP(
	void
)
{
	FILE* fileptr = (FILE*)NULL;
	int resolving_flag = 0;
	/** call the CLASP */
	CALL_CLASP;

	while (true)
	{
		/** open the "clasp log file" in read-mode */
		fileOpen(&fileptr, CLASP_LOG_FILE, "r");


		/** get the objective value */
		CLASPgetObjVal(fileptr);

		if (clasp.status == CLASP_UNSAT)
		{
			/** remove redeundant fault */
			/** close the "clasp log file" in read-mode */
			fclose(fileptr);
			return CLASP_ERROR;
		}
		//SAT�̏ꍇ
		else
		{
			/** get the SCIP solution */
			CLASPgetSol(fileptr);
			break;
		}
	}

	/** close the "scip log file" in read-mode */
	fclose(fileptr);

	assert(remove(CLASP_LOG_FILE) == 0);



	return	CLASP_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@CLASPgetObjVal
//	@function	�F	get the object value
//	@return		�F	(void)
//*************************************************************************************************************
void CLASPgetObjVal(
	FILE* fileptr			  /**< pointer to file */
)
{
	char* buffer = (char*)NULL;
	char* tmptok = (char*)NULL;
	char* context = (char*)NULL;

	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	clasp.status = CLASP_OBJVAL_ERROR;

	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (strstr(buffer, "Answer:") != NULL)
		{
			for (int i = 0; i < strlen(buffer); i++)
			{
				if (isdigit(buffer[i]) != 0)
				{
					clasp.status = atoi(strtok_s(&buffer[i], "\n", &context));
					break;
				}
			}
			break;
		}
		if (strstr(buffer, "UNSATISFIABLE") != NULL)
		{
			clasp.status = CLASP_UNSAT;
			break;
		}
		if (strstr(buffer, "UNKNOWN") != NULL)
		{
			clasp.status = CLASP_STATUS_UNKNOWN;
			break;
		}
	}

	free(buffer);

	return;
}

//*************************************************************************************************************
//	@name		�F�@CLASPgetSol
//	@function	�F	get the solution
//	@return		�F	(void)
//*************************************************************************************************************
void CLASPgetSol(
	FILE* fileptr			  /**< pointer to file */
)
{
	char* buffer = (char*)NULL;
	char* tmptok = (char*)NULL;
	char* context = (char*)NULL;
	int	  count = 0;

	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** initialize the solution to all-zeros */
	//memset(clasp.sol[SOL_SEED], '0', readdata.lfsrnet.num);
	memset(clasp.sol[SOL_TP], '0', n_dff + n_pi);

	/** get the solution */
	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (COMP_NEWLINE(buffer))
		{
			/** get the variable number */
			if (buffer[0] == 'v') {
				tmptok = strtok_s(buffer, " \n",&context);//v���폜����
				while ((tmptok = strtok_s(NULL, " \n", &context)) != NULL) {
					//if (count >= 0 && count < readdata.lfsrnet.num) {
					//	if (tmptok[0] == '-') {
					//	//clasp.sol[SOL_SEED][count] = '0';
					//	}
					//	else {
					//	//clasp.sol[SOL_SEED][count] = '1';
					//	}
					//	count++;
					//}
					if (count >= 0 && count < n_dff + n_pi) {
						if (tmptok[0] == '-') {
							clasp.sol[SOL_TP][count] = '0';
						}
						else {
							clasp.sol[SOL_TP][count] = '1';
						}
						count++;
					}
				}
			}
			
		}
		else
		{
			break;
		}
	}

	free(buffer);

	return;
}







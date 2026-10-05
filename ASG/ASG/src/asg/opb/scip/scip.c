/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	urashima																				 */
/*	file		:	./src/asg/opb/scip/scip.c															     */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.10.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <direct.h>

#include "./scip.h"
#include "../../read.h"
#include "../../../lib/lib.h"
#include "../../../standard.h"
#include "../../../debug/debug.h"


//*************************************************************************************************************
//	@name		：　SCIP
//	@function	：	call the "scip.exe"
//	@return		：	(bool) okay, error
//*************************************************************************************************************
bool SCIP(
	void
)
{
	FILE* fileptr = (FILE*)NULL;

	/** call the SCIP[limits-time=60(s)] */
	_CALL_SCIP_LIMIT_TIME_60_;

	while (true)
	{
		/** open the "scip log file" in read-mode */
		fileOpen(&fileptr, SCIP_LOG_FILE, "r");

		/** get the SCIP status */
		SCIPgetStatus(fileptr);

		if (scip.status == SCIP_STATUS_INFEASIBLE)  break;
		if (scip.status == SCIP_STATUS_ERROR)		return SCIP_ERROR;

		/** get the objective value */
		SCIPgetObjVal(fileptr);

		if (scip.objval == SCIP_OBJVAL_ERROR)
		{
			/** resolving */

			/** close the "scip log file" in read-mode */
			fclose(fileptr);

			/** call the SCIP[limits-soltion=1] */
			_CALL_SCIP_LIMIT_SOL_1_;
		}
		else
		{
			/** get the SCIP solution */
			SCIPgetSol(fileptr);
			break;
		}
	}

	/** close the "scip log file" in read-mode */
	fclose(fileptr);

	assert(remove(SCIP_LOG_FILE) == 0);


#ifdef __DEBUG_SCIP__
	_CALL_DEBUG_SCIP_;
#endif // __DEBUG_SCIP__


	return	SCIP_OKAY;
}

//*************************************************************************************************************
//	@name		：　SCIPgetStatus
//	@function	：	get the status
//	@return		：	(void)
//*************************************************************************************************************
void SCIPgetStatus(
	FILE* fileptr			  /**< pointer to file */
)
{
	char* buffer = (char*)NULL;
	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** initialize the status */
	scip.status = SCIP_STATUS_ERROR;

	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (!strcmp(buffer, "SCIP Status        : unknown\n"))
		{
			scip.status = SCIP_STATUS_UNKNOWN;
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [user interrupt]\n"))
		{
			//scip.status = SCIP_STATUS_USERINTERRUPT;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [node limit reached]\n"))
		{
			//scip.status = SCIP_STATUS_NODELIMIT;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [time limit reached]\n"))
		{
			scip.status = SCIP_STATUS_TIMELIMIT;
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [solution limit reached]\n"))
		{
			scip.status = SCIP_STATUS_SOLLIMIT;
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [memory limit reached]\n"))
		{
			//scip.scip.status = SCIP_STATUS_MEMLIMIT;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [gap limit reached]\n"))
		{
			//scip.status = SCIP_STATUS_GAPLIMIT;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : problem is solved [optimal solution found]\n"))
		{
			scip.status = SCIP_STATUS_OPTIMAL;
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : problem is solved [infeasible]\n"))
		{
			scip.status = SCIP_STATUS_INFEASIBLE;
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : problem is solved [unbounded]\n"))
		{
			//scip.status = SCIP_STATUS_UNBOUNDED;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
		else if (!strcmp(buffer, "SCIP Status        : solving was interrupted [iteration limit reached]\n"))
		{
			//scip.status = SCIP_STATUS_TOTALNODELIMIT;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
		else if (strstr(buffer, "SCIP Status        : unexpected status code") != NULL)
		{
			//scip.status = SCIP_STATUS_SOLLIMIT;
			scip.status = SCIP_STATUS_ERROR;
			PrintErrorMessage("SCIP ERROR: scip error.\n");
			break;
		}
	}

	free(buffer);

	rewind(fileptr);

	return;
}

//*************************************************************************************************************
//	@name		：　SCIPgetObjVal
//	@function	：	get the object value
//	@return		：	(void)
//*************************************************************************************************************
void SCIPgetObjVal(
	FILE* fileptr			  /**< pointer to file */
)
{
	char* buffer = (char*)NULL;
	char* tmptok = (char*)NULL;
	char* context = (char*)NULL;

	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	scip.objval = SCIP_OBJVAL_ERROR;

	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (strstr(buffer, "objective value:") != NULL)
		{
			for (int i = 0; i < strlen(buffer); i++)
			{
				if (isdigit(buffer[i]) != 0)
				{
					scip.objval = atoi(strtok_s(&buffer[i], "\n", &context));
					break;
				}
			}
			break;
		}
		if (strstr(buffer, "no solution available") != NULL)
		{
			break;
		}
	}

	free(buffer);

	//rewind(fileptr);

	return;
}

//*************************************************************************************************************
//	@name		：　SCIPgetSol
//	@function	：	get the solution
//	@return		：	(void)
//*************************************************************************************************************
void SCIPgetSol(
	FILE* fileptr			  /**< pointer to file */
)
{
	char* buffer = (char*)NULL;
	char* tmptok = (char*)NULL;
	char* context = (char*)NULL;
	int	  vars = -1;

	buffer = (char*)allocMemory(MAXSIZE_BUFFER, sizeof(char));

	/** initialize the solution to all-zeros */
	memset(scip.sol[SOL_SEED], '0', readdata.psnet.num);
	memset(scip.sol[SOL_TP], '0', n_dff + n_pi);

	/** get the solution */
	while (COMP_EOF(fgets(buffer, MAXSIZE_BUFFER, fileptr)))
	{
		if (COMP_NEWLINE(buffer))
		{
			/** get the variable number */
			vars = atoi(strtok_s(&buffer[1], " ", &context));

#ifdef NDEBUG
			assert(vars > 0);
#endif

			/** get the solution for seed */
			if (vars >= 1 && vars <= readdata.psnet.num) /** 0~N = seed */
			{
				tmptok = strtok_s(NULL, "\t", &context);

				for (int i = 0; i < strlen(tmptok); i++)
				{
					if (isdigit(tmptok[i]) != 0)
					{
						scip.sol[SOL_SEED][vars - 1] = '1';
						break;
					}
				}
			}

			/** get the solution for test pattern */
			if (vars >= readdata.psnet.num + 1 && vars <= readdata.psnet.num + n_dff + n_pi) /** N+1~N+M = tp */
			{
				tmptok = strtok_s(NULL, "\t", &context);

				for (int i = 0; i < strlen(tmptok); i++)
				{
					if (isdigit(tmptok[i]) != 0)
					{
						scip.sol[SOL_TP][vars - readdata.psnet.num - 1] = '1';
						break;
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










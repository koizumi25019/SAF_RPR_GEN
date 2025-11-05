#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

#include "./standard.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define PROJECT_NAME      "Automatic Seed Generator for RPR fault with Compatible fault sets"		 /**< project name */

#define RETCODE_OKAY	  true				  /**< return code   = okay */
#define RETCODE_ERROR	  false				  /**< return code   = error */

#define READNL_OKAY	      true				  /**< return code   = okay */
#define READNL_ERROR	  false				  /**< return code   = error */

/** warning for netlist */
#define WARNING_NETLIST(numdff)	do																		      \
{																									          \
	if (!numdff)																							  \
	{																										  \
		PrintMessage("	Read the netlists comleted ... \n");										  \
	}																										  \
	else																									  \
	{																										  \
		PrintErrorMessage("\n	FILE ERROR: warning for netlist. read the combination circuit. \n\n");		  \
		return READNL_ERROR;																				  \
	}																										  \
}																											  \
while(false);

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
void OutPIN(
	void
);

void OutLogfile(
	clock_t time
);







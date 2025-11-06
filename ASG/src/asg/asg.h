#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <time.h>
#include <stdio.h>

#include "./target.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define ASG_OKAY	     true				  /**< return code = okay */
#define ASG_ERROR		 false				  /**< return code = error */

/** output test pattern -on */
#define OUTPUT_TEST_PATTERN__ON			if (opt.file.output.test != FILE_NOSET)
/** output test pattern -on */
#define OUTPUT_UNTEST_PATTERN__ON			if (opt.file.output.untestable != FILE_NOSET)
/** output log file -on */
#define OUTPUT_LOGFILE__ON			if (opt.file.output.log != FILE_NOSET)

//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
int						  numtranpo;	      /**< number of transitive primary outputs */
int						  numtrannet;	      /**< number of transitive-nets */
clock_t					  cput;			 	  /**< cpu time */
char*					  net_name;

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** automatic seed generation */
bool ASG(
	void
);

/** output the solution */
void OutSolution(
	TARGET * target			  /**< target fault */
);

/** free the memory */
void FreeMemory(
	TARGET* remain,			  /**< remain fault */
	TARGET* target			  /**< target fault */
);









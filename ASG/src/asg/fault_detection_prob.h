#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <time.h>
#include <stdio.h>

#include "./target_fault.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define AFD_OKAY	     true				  /**< return code = okay */
#define AFD_ERROR		 false				  /**< return code = error */

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
///** analyze the fault density */
bool AnalyzeFaultDensity(
	double* out_time_cadical,
	double* out_time_bdd,
	double* out_time_xid
);

void CallXidSaf(
	const char* net_file, 
	const char* pin_file
);

bool DropDeteFault(
	TARGET * target	
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
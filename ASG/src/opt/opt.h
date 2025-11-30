#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>

#include "../standard.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	OPT_OKAY				  true		  /**< return code   = okay */
#define	OPT_ERROR				  false		  /**< return code   = error */

#define	MAXSIZE_FILENAME		  100		  /**< maximum size of filename */

#define MAX_PATTERN               100         // 最大100個まで指定可能

#define FILE_NOSET			      (char*)NULL /**< initial filename */
#define MODE_NOSET			      -1          /**< initial mode */

#define	PrintHelpMessage		  colorNo4 printf	  /**< help message */


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** input file structre */
typedef struct Input_File
{
	char* net;				  /**< netlist file */
	char* fault;			  /**< fault list file */
	char* relation;           /**< test relation file */
	int pattern_num_list[MAX_PATTERN]; // パターン数を格納する配列
	int list_size;            // パターン数リストのサイズ
}
INPUT;

/** output file structre */
typedef struct Output_File
{
	char* log;			      /**< log file */
	char* pin;                /**< pin file */
	char* result;             //< result file */
}
OUTPUT;

/** file structre */
typedef struct File
{
	INPUT				  input;			  /**< input files */
	OUTPUT				  output;			  /**< output files */
}
FILES;

/** option structre */
typedef struct Option
{
	FILES			      file;			      /**< files */
}
OPTION;


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------
OPTION					   opt;				  /**< option */


//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** set the options */
bool OPT(
	int					  argc,			      /**< number of command-arguments */
	char** argv			      /**< arguments */
);

/** initialize the options */
void OPTinit(
	void
);

/** initialize the files */
void OPTinitFile(
	void
);

/** set the options */
bool OPTset(
	int				      argc,				  /**< number of command-arguments */
	char** argv				  /**< command-arguments */
);

/** read the setting file */
bool OPTread(
	char* filename			  /**< filename */
);

/** check for essential options */
bool OPTcheck(
	void
);

/** check for files */
bool OPTcheckFile(
	void
);

/** display the help */
void OPTdispHelp(
	void
);










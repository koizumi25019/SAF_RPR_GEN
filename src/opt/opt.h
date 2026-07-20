#pragma once
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>

//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define	OPT_OKAY				  true		  /**< return code   = okay */
#define	OPT_ERROR				  false		  /**< return code   = error */

#define	MAXSIZE_FILENAME		  100		  /**< maximum size of filename */

#define MAX_PATTERN               100         // 最大100まで指定可能

#define FILE_NOSET			      (char*)NULL /**< initial filename */
#define MODE_NOSET			      -1          /**< initial mode */

/** fault model (-saf / -tdf) */
#define FM_SAF                    0           /**< stuck-at fault (default) */
#define FM_TDF                    1           /**< transition delay fault (LOC: v1=自由, v2はPI共有+DFF引き継ぎ) */

#define		YES				1
#define		NO				0

//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** input file structre */
typedef struct Input_File
{
	char* net;				  /**< netlist file */
	char* fault;			  /**< fault list file */
	char* cube_analysis;      // cube analysis file
	int limit;                // test generation limit */
}
INPUT;

/** output file structre */
typedef struct Output_File
{
	char* log;			      /**< log file */
	char* fdp;                /**< fdp result file */
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
	int				      fault_model;		  /**< FM_SAF / FM_TDF */
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
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*																											 */
/*	program		:	urashima																				 */
/*	file		:	./src/standard.h																	     */
/*	deginer		:	R.miura																			  		 */
/*	date		:	2022.10.01																  				 */
/*																											 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
//#define NDEBUG								  /**< define for "assert" */

#define SYSTEM_CLS	      system("cls")	      /**< syatem call "crear" */

#define PrintMessage	  colorNo1 printf    /**< print the message */
#define PrintErrorMessage colorNo2 printf    /**< print the error message */
#define PrintCompMessage  colorNo5 printf    /**< print the confirmation message */

#define colorRed         printf("\x1b[31m");
#define colorGreen       printf("\x1b[32m");
#define colorYellow      printf("\x1b[33m");
#define colorBlue        printf("\x1b[34m");
#define colorMagenta     printf("\x1b[35m");
#define colorCyan        printf("\x1b[36m");
#define colorWhite       printf("\x1b[37m");
#define colorDef         colorWhite
#define colorNo1         colorWhite
#define colorNo2         colorRed           /**< for error */
#define colorNo3         colorGreen		  /**< for debug */
#define colorNo4         colorRed  	      /**< for help */
#define colorNo5         colorYellow


//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//	global variable
//-------------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------









#pragma once

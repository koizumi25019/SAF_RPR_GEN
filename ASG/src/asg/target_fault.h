#pragma once
// ==========================================
// Linux移植用 互換定義 (ここから)
// ==========================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h> // bool型を使えるようにする

// Windowsの関数名をLinux標準関数にマッピング
#define _chdir chdir
#define _strdup strdup

// _s系の関数を標準関数に置き換え
// sprintf_s -> snprintf
#define sprintf_s(buf, size, fmt, ...) snprintf(buf, size, fmt, ##__VA_ARGS__)

// strcpy_s -> strncpy (サイズ指定ありコピー)
#define strcpy_s(dest, size, src) strncpy(dest, src, size)

// strtok_s -> strtok_r (スレッドセーフ版トークン分割)
#define strtok_s(str, delims, context) strtok_r(str, delims, context)

// fopen_s -> fopen (戻り値の仕様を合わせる)
// Windows: 成功時0, Linux: 成功時ポインタ / これを 0 or 1 に変換
#define fopen_s(pFile, filename, mode) ((*(pFile) = fopen((filename), (mode))) != NULL ? 0 : 1)

// ==========================================
// Linux移植用 互換定義 (ここまで)
// ==========================================

//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include "./read.h"


//-------------------------------------------------------------------------------------------------------------
//	define
//-------------------------------------------------------------------------------------------------------------
#define TARGET_OKAY	     true				  /**< return code = okay */
#define TARGET_ERROR	 false				  /**< return code = error */

//-------------------------------------------------------------------------------------------------------------
//	structre
//-------------------------------------------------------------------------------------------------------------
/** target fault structure */
typedef struct TargetFault
{
	int					  num;			      /**< number of target faults */
	struct FaultNode** list;				  /**< list */
}
TARGET;

//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------
/** set the target-fault list */
bool SetTarget(
	TARGET* target			  /**< target-fault list */
);

/* set the direct target */
bool DirectInputTarget(
	TARGET* remain,			  /**< remain-fault list */
	TARGET* target			  /**< target-fault list */
);
#pragma once
#include <stdlib.h>
#include <stdio.h>

// 1. 共通のルートディレクトリ（FaultSimの場所）
// 最後のスラッシュを忘れずに！
#define XID_ROOT      "../../src/FaultSim/"

// 2. 各ファイルのディレクトリを定義
// XID_ROOT に続けて書くことで、柔軟に変更できます
#define DIR_CIRCUIT   XID_ROOT "input/circuit/"
#define DIR_TP        XID_ROOT "input/tp/"
#define DIR_PIN       XID_ROOT "input/pin/"
#define DIR_FLIST     XID_ROOT "input/flist/"
#define DIR_OUTPUT    XID_ROOT "output/"

// 3. マクロ本体
// 引数 net, pin は「ファイル名だけ」を受け取ります
#define CALL_XID_SAF(net_filename, pin_filename) do \
{ \
    char* cmd = (char*)malloc(2048 * sizeof(char)); \
    if(cmd == NULL) { exit(EXIT_FAILURE); } \
    \
    snprintf(cmd, 2048, \
        "%sBuild/Release/XID " \
        "-c %s%s "       /* DIR_CIRCUIT + ファイル名 */ \
        "-tx %stest.txt "   /* テストパターンの場所が決まっている場合 */ \
        "-pin %s%s "     /* DIR_PIN + ファイル名 */ \
        "-flist %sxid_fault.txt " \
        "-otx %sxid_tp.txt " \
        "-fm SAF -xid YES -m2008 YES", \
        XID_ROOT,                   /* XID実行ファイル */ \
        DIR_CIRCUIT, net_filename,  /* -c */ \
        XID_ROOT,                   /* -tx (仮) */ \
        DIR_PIN, pin_filename,      /* -pin */ \
        XID_ROOT,                   /* -flist */ \
        XID_ROOT                    /* -otx */ \
    ); \
    \
    int ret = system(cmd); \
    if (ret != 0) { printf("Command Failed: %d\n", ret); } \
    free(cmd); \
} while(false);
//-------------------------------------------------------------------------------------------------------------
//	prototype declaration
//-------------------------------------------------------------------------------------------------------------

/** drop the detected fault */
bool DropDeteFault(
	TARGET * target		  /**< target fault */
);










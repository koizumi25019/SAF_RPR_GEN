#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// CaDiCaL用の定数定義
#define CADICAL_SAT     10
#define CADICAL_UNSAT   20
#define CADICAL_UNKNOWN 0

// 解を格納する場所
typedef struct {
    int status;     // 10 or 20
    char** sol;     // 解のビット列
} SOLVER_LOG;

// グローバル変数
extern SOLVER_LOG solver;

// 関数プロトタイプ
bool RunCaDiCaL(void);
void InitSolverMemory(int size);
void FreeSolverMemory(void);
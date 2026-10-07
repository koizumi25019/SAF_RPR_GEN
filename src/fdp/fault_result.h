#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <gmp.h>
#include "./read.h"

/** 単一故障の計算結果。density は呼び出し側で初期化・解放する。 */
typedef struct FaultResult
{
    const FNODE* target;
    int cube_cnt;
    int seeded_cnt;
    bool complete;
    mpf_t density;
} FaultResult;

/** 代表故障と等価故障の結果を既存の CSV 形式で出力する。 */
void WriteFaultResult(FILE* fp, const FaultResult* result);

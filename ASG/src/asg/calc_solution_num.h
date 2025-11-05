// calc_solution_num.h
#ifndef CALC_SOLUTION_NUM_H
#define CALC_SOLUTION_NUM_H
#define MAX_DIGITS 5000

#include <stddef.h>

typedef struct {
    char total[MAX_DIGITS + 1];
    char threshold[MAX_DIGITS + 1];
} ThresholdNums;

//変数名は extern で宣言だけ
extern ThresholdNums g_thresh;

/**
 * @brief 10進文字列 big を 2 倍する
 * @param big '\0' 終端された 10進数文字列。十分なバッファ長が必要。
 */
void bignum_mul2(char* big);

/**
 * @brief 文字列 add を 10進文字列 sum に加算する
 * @param sum '\0' 終端された 10進数文字列。sum のバッファは大きめに確保しておくこと。
 * @param add '\0' 終端された 10進数文字列を sum に足す
 */
void bignum_add(char* sum, const char* add);

/**
 * @brief 10進文字列 num を divisor で割り、商を quotient に入れる
 * @param num       '\0'終端済みの 10進文字列
 * @param divisor   10 などの小さな正整数
 * @param quotient  商を格納するバッファ。終端も含め十分な長さがあること。
 */
void bignum_div_small(const char* num, int divisor, char* quotient);

/**
 * @brief 10進文字列 a,b を比較する
 * @return  a<b → -1, a==b → 0, a>b → +1
 */
int bignum_cmp(const char* a, const char* b);

#endif // CALC_SOLUTION_NUM_H
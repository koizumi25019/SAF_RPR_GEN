#pragma once
/*
 * calc Fの数値計算部。故障1個ずつFDPを加え、Npごとに検出確率の総和を多倍長で積算する。
 * CSVや出力形式には依存しない（単体テストから直接使う）。
 */
#include <stdbool.h>
#include <gmp.h>

typedef struct {
    const unsigned long* pattern_counts;  /**< Np（昇順・重複なし）。呼び出し側が保持する */
    int n_pattern_counts;
    bool without_replacement;
    mp_bitcnt_t precision;                /**< 全mpf_tの精度ビット数 */
    mpf_t total_patterns;                 /**< N = 2^n_inputs */
    mpf_t* sum_with_replacement;          /**< [Np] Σ 1 - (1 - D)^Np */
    mpf_t* sum_without_replacement;       /**< [Np] Σ 1 - Π(N-Fd-k)/(N-k)。無効時はNULL */
} CoverageModel;

/** n_inputs はFDPの分母の指数（FDP = 検出パターン数 / 2^n_inputs）。 */
bool CoverageModelInit(CoverageModel* model, const unsigned long* pattern_counts,
                       int n_pattern_counts, bool without_replacement, int n_inputs);

/** 検出可能な故障（0 < density <= 1）を1個加える。density = 0 の故障は加えなくてよい。 */
void CoverageModelAddFault(CoverageModel* model, const mpf_t density);

void CoverageModelClear(CoverageModel* model);

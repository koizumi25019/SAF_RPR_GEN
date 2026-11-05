#include <stdlib.h>

#include "coverage/coverage_model.h"

/* 1 - D がD≒2^-n_inputs でも1に丸まらないよう、指数より余裕を持たせる。 */
#define MIN_PRECISION_BITS   2048
#define PRECISION_MARGIN     128

static mpf_t* AllocSums(int n, mp_bitcnt_t precision)
{
    mpf_t* sums = malloc((size_t)n * sizeof(mpf_t));
    if (!sums) return NULL;
    for (int i = 0; i < n; i++) mpf_init2(sums[i], precision);  /* 初期値0 */
    return sums;
}

static void FreeSums(mpf_t* sums, int n)
{
    if (!sums) return;
    for (int i = 0; i < n; i++) mpf_clear(sums[i]);
    free(sums);
}

bool CoverageModelInit(CoverageModel* model, const unsigned long* pattern_counts,
                       int n_pattern_counts, bool without_replacement, int n_inputs)
{
    model->pattern_counts = pattern_counts;
    model->n_pattern_counts = n_pattern_counts;
    model->without_replacement = without_replacement;
    model->precision = (mp_bitcnt_t)n_inputs + PRECISION_MARGIN;
    if (model->precision < MIN_PRECISION_BITS) model->precision = MIN_PRECISION_BITS;

    mpf_init2(model->total_patterns, model->precision);
    mpf_set_ui(model->total_patterns, 1);
    mpf_mul_2exp(model->total_patterns, model->total_patterns, (mp_bitcnt_t)n_inputs);

    model->sum_with_replacement = AllocSums(n_pattern_counts, model->precision);
    model->sum_without_replacement =
        without_replacement ? AllocSums(n_pattern_counts, model->precision) : NULL;
    if (!model->sum_with_replacement || (without_replacement && !model->sum_without_replacement)) {
        CoverageModelClear(model);
        return false;
    }
    return true;
}

void CoverageModelClear(CoverageModel* model)
{
    FreeSums(model->sum_with_replacement, model->n_pattern_counts);
    FreeSums(model->sum_without_replacement, model->n_pattern_counts);
    model->sum_with_replacement = NULL;
    model->sum_without_replacement = NULL;
    mpf_clear(model->total_patterns);
}

/* result = base^exponent（2進べき乗法。mpfにはpowが無い） */
static void PowerUi(mpf_t result, const mpf_t base, unsigned long exponent, mp_bitcnt_t precision)
{
    mpf_t square;
    mpf_init2(square, precision);
    mpf_set(square, base);
    mpf_set_ui(result, 1);
    while (exponent > 0) {
        if (exponent & 1UL) mpf_mul(result, result, square);
        exponent >>= 1;
        if (exponent > 0) mpf_mul(square, square, square);
    }
    mpf_clear(square);
}

/* 復元抽出: 全Npで見逃す確率 (1-D)^Np を、前のNpからの差分のべき乗で積み上げる。 */
static void AddWithReplacement(CoverageModel* model, const mpf_t density)
{
    mpf_t miss, all_miss, step;
    mpf_init2(miss, model->precision);
    mpf_init2(all_miss, model->precision);
    mpf_init2(step, model->precision);

    mpf_ui_sub(miss, 1, density);  /* 1パターンで見逃す確率 */
    mpf_set_ui(all_miss, 1);
    unsigned long applied = 0;
    for (int i = 0; i < model->n_pattern_counts; i++) {
        PowerUi(step, miss, model->pattern_counts[i] - applied, model->precision);
        mpf_mul(all_miss, all_miss, step);
        applied = model->pattern_counts[i];

        mpf_ui_sub(step, 1, all_miss);  /* 検出確率 */
        mpf_add(model->sum_with_replacement[i], model->sum_with_replacement[i], step);
    }
    mpf_clear(miss);
    mpf_clear(all_miss);
    mpf_clear(step);
}

/* 検出パターン数 Fd = round(D * N)。CSVのFDPは有効11桁に丸められているため最近接整数に戻す。
   検出可能な故障なので 1 <= Fd <= N に収める。 */
static void RoundDetectingPatterns(mpf_t detecting, const CoverageModel* model, const mpf_t density)
{
    mpf_t half;
    mpf_init2(half, model->precision);
    mpf_set_d(half, 0.5);
    mpf_mul(detecting, density, model->total_patterns);
    mpf_add(detecting, detecting, half);
    mpf_floor(detecting, detecting);
    if (mpf_cmp_ui(detecting, 1) < 0) mpf_set_ui(detecting, 1);
    if (mpf_cmp(detecting, model->total_patterns) > 0) mpf_set(detecting, model->total_patterns);
    mpf_clear(half);
}

/* 非復元抽出: k番目のパターンも見逃す確率 (N-Fd-k)/(N-k) を、Npに達するまで掛けていく。
   見逃すパターンを使い切ったら必ず検出する（見逃し確率0）。計算量は故障ごとに O(最大Np)。 */
static void AddWithoutReplacement(CoverageModel* model, const mpf_t density)
{
    mpf_t missing, all_miss, numerator, denominator, factor, detect;
    mpf_init2(missing, model->precision);
    mpf_init2(all_miss, model->precision);
    mpf_init2(numerator, model->precision);
    mpf_init2(denominator, model->precision);
    mpf_init2(factor, model->precision);
    mpf_init2(detect, model->precision);

    RoundDetectingPatterns(missing, model, density);
    mpf_sub(missing, model->total_patterns, missing);  /* 見逃すパターン数 N - Fd */
    mpf_set_ui(all_miss, 1);
    bool certain = false;  /* 検出が確定した（all_miss = 0） */
    unsigned long applied = 0;
    for (int i = 0; i < model->n_pattern_counts; i++) {
        for (unsigned long k = applied; !certain && k < model->pattern_counts[i]; k++) {
            mpf_sub_ui(numerator, missing, k);
            if (mpf_sgn(numerator) <= 0) {
                mpf_set_ui(all_miss, 0);
                certain = true;
                break;
            }
            mpf_sub_ui(denominator, model->total_patterns, k);
            mpf_div(factor, numerator, denominator);
            mpf_mul(all_miss, all_miss, factor);
        }
        applied = model->pattern_counts[i];

        mpf_ui_sub(detect, 1, all_miss);
        mpf_add(model->sum_without_replacement[i], model->sum_without_replacement[i], detect);
    }
    mpf_clear(missing);
    mpf_clear(all_miss);
    mpf_clear(numerator);
    mpf_clear(denominator);
    mpf_clear(factor);
    mpf_clear(detect);
}

void CoverageModelAddFault(CoverageModel* model, const mpf_t density)
{
    AddWithReplacement(model, density);
    if (model->without_replacement) AddWithoutReplacement(model, density);
}

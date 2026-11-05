/* calc Fの数値計算部（src/coverage/coverage_model.c）を、手計算できる小さな例で確かめる。 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "coverage/coverage_model.h"

static void assert_close(const mpf_t actual, double expected)
{
    double value = mpf_get_d(actual);
    if (fabs(value - expected) > 1e-15) {
        fprintf(stderr, "expected %.17g, got %.17g\n", expected, value);
        assert(0);
    }
}

static void add_fault(CoverageModel* model, const char* density)
{
    mpf_t value;
    mpf_init2(value, model->precision);
    assert(mpf_set_str(value, density, 10) == 0);
    CoverageModelAddFault(model, value);
    mpf_clear(value);
}

/* 復元抽出: 1 - (1-D)^Np。Npの差分べき乗で積み上げても直接計算と一致する。 */
static void test_with_replacement(void)
{
    const unsigned long counts[] = { 1, 2, 5 };
    CoverageModel model;
    assert(CoverageModelInit(&model, counts, 3, false, 2));
    assert(model.sum_without_replacement == NULL);
    add_fault(&model, "0.25");
    add_fault(&model, "1");
    assert_close(model.sum_with_replacement[0], 0.25 + 1.0);
    assert_close(model.sum_with_replacement[1], 1 - 0.75 * 0.75 + 1.0);
    assert_close(model.sum_with_replacement[2], 1 - pow(0.75, 5) + 1.0);
    CoverageModelClear(&model);
}

/* 非復元抽出: N=4, Fd=1 なら Np 個で検出する確率は Np/4、Np>=N-Fd+1 で1。 */
static void test_without_replacement(void)
{
    const unsigned long counts[] = { 1, 2, 3, 4, 10 };
    CoverageModel model;
    assert(CoverageModelInit(&model, counts, 5, true, 2));
    add_fault(&model, "0.25");
    assert_close(model.sum_without_replacement[0], 0.25);
    assert_close(model.sum_without_replacement[1], 0.5);
    assert_close(model.sum_without_replacement[2], 0.75);
    assert_close(model.sum_without_replacement[3], 1.0);
    assert_close(model.sum_without_replacement[4], 1.0);
    CoverageModelClear(&model);

    /* Fd = round(D*N): 有効11桁に丸めたFDPでも最近接の整数（N=8でFd=3）に戻る。 */
    const unsigned long one[] = { 2 };
    assert(CoverageModelInit(&model, one, 1, true, 3));
    add_fault(&model, "3.7500000000e-01");
    assert_close(model.sum_without_replacement[0], 1 - (5.0 / 8) * (4.0 / 7));
    CoverageModelClear(&model);
}

/* 1 - D が1に丸まらない精度を確保する（D = 2^-3000 でも検出確率が0にならない）。 */
static void test_tiny_density(void)
{
    const unsigned long counts[] = { 1 };
    CoverageModel model;
    assert(CoverageModelInit(&model, counts, 1, false, 3000));
    assert(model.precision >= 3000);
    mpf_t density;
    mpf_init2(density, model.precision);
    mpf_set_ui(density, 1);
    mpf_div_2exp(density, density, 3000);
    CoverageModelAddFault(&model, density);
    assert(mpf_cmp(model.sum_with_replacement[0], density) == 0);
    mpf_clear(density);
    CoverageModelClear(&model);
}

int main(void)
{
    test_with_replacement();
    test_without_replacement();
    test_tiny_density();
    puts("coverage_model_test: ok");
    return 0;
}

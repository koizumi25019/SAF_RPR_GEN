#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <math.h>

/**
 * CUDDから得られた解の個数(文字列)と、全変数数、印加パターン数を受け取り、
 * 「nパターン印加時の検出確率」を計算して返す関数
 */
void calculate_prob_with_gmp(const char* numStr, int nvars, int* pattern_num_list, int list_size, FILE* result_fp, mpf_t* accumulator) {
    mpf_t num, den, density, term, result;

    // 精度設定 (8192ビットあれば2^6666も余裕で扱えます)
    mpf_set_default_prec(8192);

    mpf_init(num);      // 分子 (解の個数)
    mpf_init(den);      // 分母 (全組み合わせ 2^nvars)
    mpf_init(density);  // 1パターンあたりの検出確率 p
    mpf_init(term);     // 計算用一時変数
    mpf_init(result);   // 最終結果

    // --- 1. 分子 (解の個数) の設定 ---
    if (mpf_set_str(num, numStr, 10) != 0) {
        // 変換失敗時は0とみなす
        mpf_set_ui(num, 0);
    }

    // --- 2. 分母 (2^nvars) の設定 ---
    mpf_set_ui(den, 1);
    mpf_mul_2exp(den, den, (unsigned long)nvars); // den = 1 * 2^nvars

    // --- 3. 密度 p = num / den の計算 ---
    mpf_div(density, num, den);

    // ファイル書き込み: まず「密度(density)」を出力
    if (result_fp != NULL) {
        gmp_fprintf(result_fp, "%.10Fe,", density);
    }

    // ===========================================================
    // 計算式: P_det = 1 - (1 - p)^n
    // ===========================================================
    // ランダムパターン数リストの分だけループして確率計算
    for (int i = 0; i < list_size; i++) {

        int n = pattern_num_list[i];

        // term = 1 - p
        mpf_ui_sub(term, 1, density);

        // term = term ^ num_patterns
        // (1-p) を N乗 します
        mpf_pow_ui(term, term, (unsigned long)n);

        // result = 1 - term
        // つまり 1 - (1-p)^N
        mpf_ui_sub(result, 1, term);

        // カンマ区切りで書き出す
        if (result_fp != NULL) {
            // 次にカンマと検出確率
            gmp_fprintf(result_fp, "%.10Fe", result);
            if(i!=list_size-1) fprintf(result_fp, ",");
        }
		// 各ランダムパターン数における故障検出確率を加算
        if (accumulator != NULL) {
            mpf_add(accumulator[i], accumulator[i], result);
        }
    }

    gmp_fprintf(result_fp, "\n");


    // メモリ解放
    mpf_clear(num);
    mpf_clear(den);
    mpf_clear(density);
    mpf_clear(term);
    mpf_clear(result);

    return;
}
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/**
 * CUDD���瓾��ꂽ���̌�(������)�ƁA�S�ϐ����A����p�^�[�������󂯎��A
 * �un�p�^�[��������̌��o�m���v���v�Z���ĕԂ��֐�
 */
/*void calculate_prob_with_gmp(const char* numStr, int nvars, int* pattern_num_list, int list_size, FILE* result_fp, mpf_t* total_prob_sums) {
    mpf_t num, den, density, term, result;

    // ���x�ݒ� (8192�r�b�g�����2^6666���]�T�ň����܂�)
    mpf_set_default_prec(8192);

    mpf_init(num);      // ���q (���̌�)
    mpf_init(den);      // ���� (�S�g�ݍ��킹 2^nvars)
    mpf_init(density);  // 1�p�^�[��������̌��o�m�� p
    mpf_init(term);     // �v�Z�p�ꎞ�ϐ�
    mpf_init(result);   // �ŏI����

    // --- 1. ���q (���̌�) �̐ݒ� ---
    if (mpf_set_str(num, numStr, 10) != 0) {
        // �ϊ����s����0�Ƃ݂Ȃ�
        mpf_set_ui(num, 0);
    }

    // --- 2. ���� (2^nvars) �̐ݒ� ---
    mpf_set_ui(den, 1);
    mpf_mul_2exp(den, den, (unsigned long)nvars); // den = 1 * 2^nvars

    // --- 3. ���x p = num / den �̌v�Z ---
    mpf_div(density, num, den);

    // �t�@�C����������: �܂��u���x(density)�v���o��
    if (result_fp != NULL) {
        gmp_fprintf(result_fp, "%.10Fe,", density);
    }

    // ===========================================================
    // �v�Z��: P_det = 1 - (1 - p)^n
    // ===========================================================
    // �����_���p�^�[�������X�g�̕��������[�v���Ċm���v�Z
    for (int i = 0; i < list_size; i++) {

        int n = pattern_num_list[i];

        // term = 1 - p
        mpf_ui_sub(term, 1, density);

        // term = term ^ num_patterns
        // (1-p) �� N�� ���܂�
        mpf_pow_ui(term, term, (unsigned long)n);

        // result = 1 - term
        // �܂� 1 - (1-p)^N
        mpf_ui_sub(result, 1, term);

        // �J���}��؂�ŏ����o��
        if (result_fp != NULL) {
            // ���ɃJ���}�ƌ��o�m��
            gmp_fprintf(result_fp, "%.10Fe", result);
            if(i!=list_size-1) fprintf(result_fp, ",");
        }
		// �e�����_���p�^�[�����ɂ�����̏ጟ�o�m�������Z
        if (total_prob_sums != NULL) {
            mpf_add(total_prob_sums[i], total_prob_sums[i], result);
        }
    }

    gmp_fprintf(result_fp, "\n");


    // ���������
    mpf_clear(num);
    mpf_clear(den);
    mpf_clear(density);
    mpf_clear(term);
    mpf_clear(result);

    return;
}*/
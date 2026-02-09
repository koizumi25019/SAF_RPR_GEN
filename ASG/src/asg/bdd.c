/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cudd.h>

//�v���g�^�C�v�錾
//void calculate_prob_with_gmp(const char* numStr, int nvars, int* pattern_num_list,int list_size, FILE* result_fp, mpf_t* accumulator);

//�L���[�u��BDD�ɕϊ�
/*DdNode* parseCube(DdManager* gbm, const char* cubeStr, int nvars) {
    // �ύ���BDD�́A�_���́u1�v(Cudd_ReadOne) ����n�߂�
    DdNode* cubeBdd = Cudd_ReadOne(gbm);
    Cudd_Ref(cubeBdd);

    for (int i = 0; i < nvars; i++) {
        char bit = cubeStr[i];
        DdNode* literalBdd = NULL;

        if (bit == '0') {
            // '0' �̏ꍇ: ~x(i+1) �ɑΉ� (CUDD�ł̓C���f�b�N�X i)
            DdNode* varBdd = Cudd_bddIthVar(gbm, i);
            literalBdd = Cudd_Not(varBdd);
        }
        else if (bit == '1') {
            // '1' �̏ꍇ: x(i+1) �ɑΉ� (CUDD�ł̓C���f�b�N�X i)
            literalBdd = Cudd_bddIthVar(gbm, i);
        }
        else if (bit == 'X') {
            // 'X' (Don't Care) �̏ꍇ�A���̕ϐ��͐ύ��Ɋ܂߂Ȃ�
            Cudd_bddIthVar(gbm, i);
            continue;
        }
        else {
            fprintf(stderr, "ERROR: '%c' �͕s���ȕ����ł�\n", bit);
            exit(1);
        }

        // ���݂̐ύ�BDD��AND�Ō���
        DdNode* tmp = Cudd_bddAnd(gbm, cubeBdd, literalBdd);
        Cudd_Ref(tmp);

        // �ȑO��cubeBdd�̎Q�Ƃ����
        Cudd_RecursiveDeref(gbm, cubeBdd);

        // cubeBdd���X�V
        cubeBdd = tmp;
    }

    return cubeBdd;
}*/

// BDD���\�z���A���̌��𐔂��AGMP�Ŋm���v�Z���s���ĕԂ�
/*void RunBDD(DdManager* gbm,int nvars, int* pattern_num_list,int list_size, FILE* result_fp, mpf_t* total_prob_sums) {
    FILE* fp;
    char line[4096]; // �s�o�b�t�@

    DdNode* finalBdd = Cudd_ReadLogicZero(gbm);
    Cudd_Ref(finalBdd);

    //�t�@�C���ǂݍ��݂�BDD�\�z
    if ((fp = fopen("./tools/bdd/bdd_cube_file.txt", "r")) == NULL) {
        fprintf(stderr, "Error: file open error %s\n", "./tools/bdd/bdd_cube_file.txt");
        Cudd_Quit(gbm);
        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = 0; // ���s�폜
        if (strlen(line) == 0) continue;

        DdNode* cubeBdd = parseCube(gbm, line, nvars);
        DdNode* tmp = Cudd_bddOr(gbm, finalBdd, cubeBdd);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(gbm, finalBdd);
        Cudd_RecursiveDeref(gbm, cubeBdd);
        finalBdd = tmp;
    }
    fclose(fp);

    //BDD�ˑ��ϐ����̌v�Z
    int supportSize = Cudd_SupportSize(gbm, finalBdd);
    //BDD�ˑ��ϐ���(supportSize)��CSV�t�@�C���ɒǋL
    fprintf(result_fp, "%d,", supportSize);

    //���̌��J�E���g
    int digits;         // �������󂯎�邽�߂̐����ϐ�
    DdApaNumber count;  // ���ʂ̔z����󂯎�邽�߂̃|�C���^
    count=Cudd_ApaCountMinterm(gbm, finalBdd, nvars, &digits);

    // APA�̌��ʂ𕶎���Ɏ��o��
    FILE* tmp_fp = tmpfile();
    if (!tmp_fp) {
        return 0.0;
    }
    Cudd_ApaPrintDecimal(tmp_fp, digits, count);
    rewind(tmp_fp);

    char countStr[8192]; // �\���ȃT�C�Y
    if (fgets(countStr, sizeof(countStr), tmp_fp) == NULL) {
        strcpy(countStr, "0");
    }
    fclose(tmp_fp);

	// ���{�������z�񃁃������
    free(count);

    //GMP���g���Ċm�����v�Z
    calculate_prob_with_gmp(countStr, nvars, pattern_num_list,list_size, result_fp, total_prob_sums);

    //�I������
    Cudd_RecursiveDeref(gbm, finalBdd);

    return;
}*/
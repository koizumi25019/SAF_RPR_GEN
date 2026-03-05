#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cudd.h>
#include <gmp.h>
#include "./gmp_wrapper.h"
DdNode* parseCube(DdManager* gbm, const char* cubeStr, int nvars) {
    DdNode* cubeBdd = Cudd_ReadOne(gbm);
    Cudd_Ref(cubeBdd);

    for (int i = 0; i < nvars; i++) {
        char bit = cubeStr[i];
        DdNode* literalBdd = NULL;

        if (bit == '0') {
            DdNode* varBdd = Cudd_bddIthVar(gbm, i);
            literalBdd = Cudd_Not(varBdd);
        }
        else if (bit == '1') {
            literalBdd = Cudd_bddIthVar(gbm, i);
        }
        else if (bit == 'X') {
            Cudd_bddIthVar(gbm, i);
            continue;
        }
        else {
            fprintf(stderr, "ERROR: '%c' is not a valid bit\n", bit);
            exit(1);
        }

        DdNode* tmp = Cudd_bddAnd(gbm, cubeBdd, literalBdd);
        Cudd_Ref(tmp);

        Cudd_RecursiveDeref(gbm, cubeBdd);

        cubeBdd = tmp;
    }

    return cubeBdd;
}

// BDD
void RunBDD(DdManager* gbm,int nvars, int* pattern_num_list,int list_size, FILE* result_fp, mpf_t* total_prob_sums) {
    FILE* fp;
    char line[4096]; // �s�o�b�t�@

    DdNode* finalBdd = Cudd_ReadLogicZero(gbm);
    Cudd_Ref(finalBdd);

    //
    if ((fp = fopen("./bdd_cube_file.txt", "r")) == NULL) {
        fprintf(stderr, "Error: file open error %s\n", "./bdd_cube_file.txt");
        Cudd_Quit(gbm);
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

    //BDD
    int supportSize = Cudd_SupportSize(gbm, finalBdd);
    //
    fprintf(result_fp, "%d,", supportSize);

    int digits;         // 
    DdApaNumber count;  // 
    count=Cudd_ApaCountMinterm(gbm, finalBdd, nvars, &digits);

    // APA
    FILE* tmp_fp = tmpfile();
    if (!tmp_fp) {
        fprintf(stderr, "Error: cannot create temporary file\n");
        Cudd_Quit(gbm);
    }
    Cudd_ApaPrintDecimal(tmp_fp, digits, count);
    rewind(tmp_fp);

    char countStr[8192]; // �\���ȃT�C�Y
    if (fgets(countStr, sizeof(countStr), tmp_fp) == NULL) {
        strcpy(countStr, "0");
    }
    fclose(tmp_fp);

	// 
    free(count);

    //GMP
    calculate_prob_with_gmp(countStr, nvars, pattern_num_list,list_size, result_fp, total_prob_sums);

    //
    Cudd_RecursiveDeref(gbm, finalBdd);

    return;
}
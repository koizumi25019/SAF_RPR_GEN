#include <stdio.h>
#include <stdlib.h>
#include "./cudd_wrapper.h"

static DdNode* parseCube(DdManager* gbm, const char* cubeStr, int nvars) {
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

// キューブ和集合の BDD を構築し、検出確率を計算する。
bool RunBDD(DdManager* gbm, int nvars, const CubeSet* cubes, mpf_t density) {
    DdNode* finalBdd = Cudd_ReadLogicZero(gbm);
    Cudd_Ref(finalBdd);

    for (int i = 0; i < cubes->n; i++) {
        DdNode* cubeBdd = parseCube(gbm, cubes->data[i], nvars);
        DdNode* tmp = Cudd_bddOr(gbm, finalBdd, cubeBdd);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(gbm, finalBdd);
        Cudd_RecursiveDeref(gbm, cubeBdd);
        finalBdd = tmp;
    }

    int digits;
    DdApaNumber count = Cudd_ApaCountMinterm(gbm, finalBdd, nvars, &digits);
    Cudd_RecursiveDeref(gbm, finalBdd);
    if (count == NULL) {
        fprintf(stderr, "ERROR: BDD minterm counting failed\n");
        return false;
    }

    // CUDD の APA は上位桁から並ぶ。各桁のバイト順はホストと同じ。
    // 一時ファイル・10進文字列を経由せず、多倍長整数として GMP に直接取り込む。
    mpz_t minterms;
    mpz_init(minterms);
    mpz_import(minterms, (size_t)digits, 1, sizeof(DdApaDigit), 0, 0, count);
    Cudd_FreeApaNumber(count);

    mpf_set_z(density, minterms);
    mpf_div_2exp(density, density, (mp_bitcnt_t)nvars);
    mpz_clear(minterms);
    return true;
}

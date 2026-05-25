#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cudd.h>
#include <gmp.h>
#include "./gmp_wrapper.h"
#include "../opt/opt.h"

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
void RunBDD(DdManager* gbm, int nvars, char** cubes, int n_cubes, FILE* result_fp, FILE* cube_analysis_fp, TARGET* target, int test_loop) {
    DdNode* finalBdd = Cudd_ReadLogicZero(gbm);
    Cudd_Ref(finalBdd);

    for (int i = 0; i < n_cubes; i++) {
        DdNode* cubeBdd = parseCube(gbm, cubes[i], nvars);
        DdNode* tmp = Cudd_bddOr(gbm, finalBdd, cubeBdd);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(gbm, finalBdd);
        Cudd_RecursiveDeref(gbm, cubeBdd);
        finalBdd = tmp;
    }

    //BDD
    int supportSize = Cudd_SupportSize(gbm, finalBdd);
    
    //BDD変数数を記述
    //if(opt.file.input.cube_analysis == FILE_NOSET){
    //    fprintf(result_fp, "%d,", supportSize);
    //};

    int digits;
    DdApaNumber count;
    count=Cudd_ApaCountMinterm(gbm, finalBdd, nvars, &digits);

    // APA
    FILE* tmp_fp = tmpfile();
    if (!tmp_fp) {
        fprintf(stderr, "Error: cannot create temporary file\n");
        Cudd_Quit(gbm);
    }
    Cudd_ApaPrintDecimal(tmp_fp, digits, count);
    rewind(tmp_fp);

    char countStr[8192];
    if (fgets(countStr, sizeof(countStr), tmp_fp) == NULL) {
        strcpy(countStr, "0");
    }
    fclose(tmp_fp);
    free(count);

    //GMP
    calculate_prob_with_gmp(countStr, nvars,result_fp,cube_analysis_fp,target,test_loop);

    Cudd_RecursiveDeref(gbm, finalBdd);

    return;
}
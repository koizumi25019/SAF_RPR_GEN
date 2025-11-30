#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/cudd.h"
#include <gmp.h>

//プロトタイプ宣言
double calculate_prob_with_gmp(const char* numStr, int nvars, int* pattern_num_list,int list_size, FILE* result_fp);

//*************************************************************************************************************
//	@name		：TCD(Cube Truth Density)
//	@function	：	calculation of cube truth density
//	@return		：	(void)
//*************************************************************************************************************
DdNode* parseCube(DdManager* gbm, const char* cubeStr, int nvars) {
    // 積項のBDDは、論理の「1」(Cudd_ReadOne) から始める
    DdNode* cubeBdd = Cudd_ReadOne(gbm);
    Cudd_Ref(cubeBdd);

    for (int i = 0; i < nvars; i++) {
        char bit = cubeStr[i];
        DdNode* literalBdd = NULL;

        if (bit == '0') {
            // '0' の場合: ~x(i+1) に対応 (CUDDではインデックス i)
            DdNode* varBdd = Cudd_bddIthVar(gbm, i);
            literalBdd = Cudd_Not(varBdd);
        }
        else if (bit == '1') {
            // '1' の場合: x(i+1) に対応 (CUDDではインデックス i)
            literalBdd = Cudd_bddIthVar(gbm, i);
        }
        else if (bit == 'X') {
            // 'X' (Don't Care) の場合、この変数は積項に含めない
            Cudd_bddIthVar(gbm, i);
            continue;
        }
        else {
            fprintf(stderr, "ERROR: '%c' は不正な文字です\n", bit);
            exit(1);
        }

        // 現在の積項BDDとANDで結合
        DdNode* tmp = Cudd_bddAnd(gbm, cubeBdd, literalBdd);
        Cudd_Ref(tmp);

        // 以前のcubeBddの参照を解放
        Cudd_RecursiveDeref(gbm, cubeBdd);

        // cubeBddを更新
        cubeBdd = tmp;
    }

    return cubeBdd;
}

// BDDを構築し、解の個数を数え、GMPで確率計算を行って返す

void RunBDD(int nvars, int* pattern_num_list,int list_size, FILE* result_fp) {
    FILE* fp;
    char line[4096]; // 行バッファ

    //CUDDの初期化
    DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

	//SHIFTアルゴリズムを有効化
    Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

    DdNode* finalBdd = Cudd_ReadLogicZero(gbm);
    Cudd_Ref(finalBdd);

    //ファイル読み込みとBDD構築
    if ((fp = fopen("./tools/bdd/bdd_cube_file.txt", "r")) == NULL) {
        fprintf(stderr, "Error: file open error %s\n", "./tools/bdd/bdd_cube_file.txt");
        Cudd_Quit(gbm);
        return 0.0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        line[strcspn(line, "\r\n")] = 0; // 改行削除
        if (strlen(line) == 0) continue;

        DdNode* cubeBdd = parseCube(gbm, line, nvars);
        DdNode* tmp = Cudd_bddOr(gbm, finalBdd, cubeBdd);
        Cudd_Ref(tmp);
        Cudd_RecursiveDeref(gbm, finalBdd);
        Cudd_RecursiveDeref(gbm, cubeBdd);
        finalBdd = tmp;
    }
    fclose(fp);

    //解の個数カウント
    int digits;         // 桁数を受け取るための整数変数
    DdApaNumber count;  // 結果の配列を受け取るためのポインタ
    Cudd_ApaCountMinterm(gbm, finalBdd, nvars, count);

    // APAの結果を文字列に取り出す
    FILE* tmp_fp = tmpfile();
    if (!tmp_fp) {
        return 0.0;
    }
    Cudd_ApaPrintDecimal(tmp_fp, digits, count);
    rewind(tmp_fp);

    char countStr[8192]; // 十分なサイズ
    if (fgets(countStr, sizeof(countStr), tmp_fp) == NULL) {
        strcpy(countStr, "0");
    }
    fclose(tmp_fp);

    //GMPを使って最終確率を計算
    double prob = calculate_prob_with_gmp(countStr, nvars, pattern_num_list,list_size, result_fp);

    //終了処理
    Cudd_RecursiveDeref(gbm, finalBdd);
    Cudd_Quit(gbm);

    return prob;
}
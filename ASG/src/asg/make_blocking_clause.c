#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../netlist/netlist.h"
#include "./MakeBlockingClause.h"
#include "./calc_solution_num.h"
#include "./opb/clasp/clasp.h"
#include "./target.h"

bool make_blocking_clause(TARGET* target)
{
    //XID2 出力パターン読み込み
    char xtp_file[256];
    snprintf(xtp_file, sizeof(xtp_file), "./tools/fsim/xid_tp.txt");
    FILE* fp_xtp = fopen(xtp_file, "r");
    if (!fp_xtp) {
        fprintf(stderr, "【ERROR】: Cannot open %s for blocking clause\n", xtp_file);
        return false;
    }

    // PBO ファイルを a モードで open
    char pbo_file[256];
    snprintf(pbo_file, sizeof(pbo_file), "./tools/clasp/pbo.txt");
    FILE* fp_pbo = fopen(pbo_file, "a");
    if (!fp_pbo) {
        fprintf(stderr, "【ERROR】: Cannot open %s for append\n", pbo_file);
        fclose(fp_xtp);
        return false;
    }

    int n_vars = n_pi;
    //パターンバッファを malloc で確保
    char* x_pattern = malloc(n_vars + 1);
    if (!x_pattern) {
        fprintf(stderr, "【ERROR】: malloc failed for x_pattern\n");
        fclose(fp_xtp);
        fclose(fp_pbo);
        return false;
    }
    if (!fgets(x_pattern, n_vars + 1, fp_xtp)) {
        fprintf(stderr, "【ERROR】: Failed to read pattern from %s\n", xtp_file);
        free(x_pattern);
        fclose(fp_xtp);
        fclose(fp_pbo);
        return false;
    }
    fclose(fp_xtp);

    // 改行文字を除去
    x_pattern[strcspn(x_pattern, "\r\n")] = '\0';

    // --- 追加: 'X' の数を数える ---
    int x_count = 0;
    for (int i = 0; i < n_vars; i++) {
        if (x_pattern[i] == 'X') {
            x_count++;
        }
    }

    //テキストベースで 2^x_count を計算
    char pow2[MAX_DIGITS + 1];
    strcpy(pow2, "1");
    for (int k = 0; k < x_count; ++k) {
        bignum_mul2(pow2);
    }

    //tfl->solution_str に加算
    bignum_add(target->list[0]->solution_str, pow2);

    // 3) 禁止節文字列バッファを malloc で確保
    const size_t BUF_SZ = 50000;
    char* blocking_clause = malloc(BUF_SZ);
    if (!blocking_clause) {
        fprintf(stderr, "【ERROR】: malloc failed for blocking_clause\n");
        free(x_pattern);
        fclose(fp_pbo);
        return false;
    }
    blocking_clause[0] = '\0';
    char* p = blocking_clause;
    size_t rem = BUF_SZ;

    // 4) リテラル生成: 0->1 (x) / 1->0 (~x)
    for (int i = 0; i < n_vars; i++) {
        char bit = x_pattern[i];
        if (bit != '0' && bit != '1') continue;  // 'X' はスキップ

        char term[64];
        const char* sign = (p == blocking_clause) ? "" : " +";
        if (bit == '0') {
            snprintf(term, sizeof(term), "%s1 x%d", sign, i + 1);
        }
        else {
            snprintf(term, sizeof(term), "%s1 ~x%d", sign, i + 1);
        }
        int w = snprintf(p, rem, "%s", term);
        if (w < 0 || (size_t)w >= rem) {
            fprintf(stderr, "【ERROR】: Blocking clause buffer overflow\n");
            free(x_pattern);
            free(blocking_clause);
            fclose(fp_pbo);
            return false;
        }
        p += w;
        rem -= w;
    }

    // 5) ">=1;" を付ける
    int w2 = snprintf(p, rem, " >=1;\n");
    if (w2 < 0 || (size_t)w2 >= rem) {
        fprintf(stderr, "【ERROR】: Failed to append \">=1;\"\n");
        free(x_pattern);
        free(blocking_clause);
        fclose(fp_pbo);
        return false;
    }

    //PBO ファイルに追記
    fprintf(fp_pbo, "%s", blocking_clause);

    //ファイルクローズ
    fclose(fp_pbo);

    //BDD用テストキューブ集合作成
    FILE* fp_bdd_cubes = fopen("./tools/bdd/bdd_cube_file.txt", "a");
    if (!fp_bdd_cubes) {
        free(x_pattern);
        free(blocking_clause);
        return false; // 失敗
    }
    //テストキューブをファイルに書き込む
    fprintf(fp_bdd_cubes, "%s\n", x_pattern);
    fclose(fp_bdd_cubes);


    //片付け
    free(x_pattern);
    free(blocking_clause);

    return true;
}
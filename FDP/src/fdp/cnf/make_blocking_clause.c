#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../netlist/netlist.h"
#include "./target_fault.h"
#include "ccadical.h"

char* make_blocking_clause(CCaDiCaL *solver, TARGET* target)
{
    char xtp_file[256];
    snprintf(xtp_file, sizeof(xtp_file), "./xid_tp.txt");
    FILE* fp_xtp = fopen(xtp_file, "r");
    
    if (!fp_xtp) {
        fprintf(stderr, "【ERROR】: Cannot open %s for blocking clause\n", xtp_file);
        return NULL;
    }

    // 変数設定
    int n_vars = n_pi; // 外部入力数

    // パターンバッファ確保
    char* x_pattern = (char*)malloc(n_vars + 2);
    if (!x_pattern) {
        fprintf(stderr, "【ERROR】: malloc failed for x_pattern\n");
        fclose(fp_xtp);
        return NULL;
    }
    
    // パターン読み込み
    if (!fgets(x_pattern, n_vars + 1, fp_xtp)) { // +1 for safety
        fprintf(stderr, "【ERROR】: Failed to read pattern from %s\n", xtp_file);
        free(x_pattern);
        fclose(fp_xtp);
        return NULL;
    }
    fclose(fp_xtp); // 読み終わったら閉じる

    // 改行除去
    x_pattern[strcspn(x_pattern, "\r\n")] = '\0';
    
    for (int i = 0; i < n_vars; i++) {
        char bit = x_pattern[i];
        
        // 変数番号は 1 始まり
        int var_idx = pi[i]->varsgc;  // varsgcを直接使う
        int lit = 0;

        if (bit == '0') {
            lit = var_idx;      // '0' を禁止するために正のリテラルを加える
        }
        else if (bit == '1') {
            lit = -var_idx;     // '1' を禁止するために負のリテラルを加える
        }
        
        // '0'か'1'の場合のみ追加 ('X'などは無視)
        if (lit != 0) {
            ccadical_add(solver, lit);
        }
    }

    // 節の終わりを示す "0" を追加
    ccadical_add(solver, 0);

    return x_pattern;
}
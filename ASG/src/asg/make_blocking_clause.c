#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../netlist/netlist.h"
// #include "./MakeBlockingClause.h" // 必要に応じて有効化してください
#include "./target.h"

// プロトタイプ宣言（必要に応じて）
// extern int n_pi; 

char* make_blocking_clause(TARGET* target)
{
    // XID2 出力パターン読み込み
    char xtp_file[256];
    snprintf(xtp_file, sizeof(xtp_file), "./xid_tp.txt");
    FILE* fp_xtp = fopen(xtp_file, "r");
    if (!fp_xtp) {
        fprintf(stderr, "【ERROR】: Cannot open %s for blocking clause\n", xtp_file);
        return NULL; // false -> NULL に修正
    }

    // CNF ファイルを a (追記) モードで open
    char cnf_file[256];
    snprintf(cnf_file, sizeof(cnf_file), "./cnf.txt");
    FILE* fp_cnf = fopen(cnf_file, "a");
    if (!fp_cnf) {
        fprintf(stderr, "【ERROR】: Cannot open %s for append\n", cnf_file);
        fclose(fp_xtp);
        return NULL;
    }

    int n_vars = n_pi; // 外部入力数
    
    // パターンバッファを malloc で確保
    char* x_pattern = (char*)malloc(n_vars + 2); // +2 for newline and null terminator safety
    if (!x_pattern) {
        fprintf(stderr, "【ERROR】: malloc failed for x_pattern\n");
        fclose(fp_xtp);
        fclose(fp_cnf);
        return NULL;
    }
    
    if (!fgets(x_pattern, n_vars + 1, fp_xtp)) {
        fprintf(stderr, "【ERROR】: Failed to read pattern from %s\n", xtp_file);
        free(x_pattern);
        fclose(fp_xtp);
        fclose(fp_cnf);
        return NULL;
    }
    fclose(fp_xtp);

    // 改行文字を除去
    x_pattern[strcspn(x_pattern, "\r\n")] = '\0';

    // 3) 禁止節文字列バッファを malloc で確保
    // DIMACS形式は数字とスペースだけなので、サイズは十分足りますが安全のため大きめに確保
    const size_t BUF_SZ = 65536; 
    char* blocking_clause = (char*)malloc(BUF_SZ);
    if (!blocking_clause) {
        fprintf(stderr, "【ERROR】: malloc failed for blocking_clause\n");
        free(x_pattern);
        fclose(fp_cnf);
        return NULL;
    }
    blocking_clause[0] = '\0';
    
    char* p = blocking_clause;
    size_t rem = BUF_SZ;

    // 4) リテラル生成 (DIMACS CNF形式)
    // パターンが '0' なら、それを否定するためにリテラルは正 (例: 1)
    // パターンが '1' なら、それを否定するためにリテラルは負 (例: -1)
    for (int i = 0; i < n_vars; i++) {
        char bit = x_pattern[i];
        if (bit != '0' && bit != '1') continue;  // 'X' はスキップ

        char term[64];
        int var_idx = i + 1; // 1-based index (DIMACSは1から始まる)

        if (bit == '0') {
            // 値が0の場合、禁止するには "1" (True) になればよい -> 正のリテラル
            snprintf(term, sizeof(term), "%d ", var_idx);
        }
        else { // bit == '1'
            // 値が1の場合、禁止するには "0" (False) になればよい -> 負のリテラル
            snprintf(term, sizeof(term), "-%d ", var_idx);
        }

        int w = snprintf(p, rem, "%s", term);
        if (w < 0 || (size_t)w >= rem) {
            fprintf(stderr, "【ERROR】: Blocking clause buffer overflow\n");
            free(x_pattern);
            free(blocking_clause);
            fclose(fp_cnf);
            return NULL;
        }
        p += w;
        rem -= w;
    }

    // 5) DIMACS CNFの行末は "0" と改行
    int w2 = snprintf(p, rem, "0\n");
    if (w2 < 0 || (size_t)w2 >= rem) {
        fprintf(stderr, "【ERROR】: Failed to append \"0\\n\"\n");
        free(x_pattern);
        free(blocking_clause);
        fclose(fp_cnf);
        return NULL;
    }

    // CNFファイルに追記
    fprintf(fp_cnf, "%s", blocking_clause);

    // ファイルクローズ
    fclose(fp_cnf);

    // バッファ解放
    free(blocking_clause);

    return x_pattern;
}
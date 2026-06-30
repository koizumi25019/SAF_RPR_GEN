//-------------------------------------------------------------------------------------------------------------
//	cnf_dump.c : 検出CNFの DIMACS 書き出し（env DUMP_CNF、既定無効。本番動作に影響なし）
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./cnf_dump.h"
#include "../netlist/netlist.h"   /* pi[], n_pi */

/* 節本体は一旦テンポラリへ書き、確定時に「p cnf 変数数 節数」ヘッダと
   射影行(PI good 変数)を前置してから結合する（DIMACSはヘッダが先頭必須のため）。 */
static FILE* body = NULL;
static long  nclause = 0;
static char  out_path[2048];

void cnf_tee_begin(const char* path)
{
    if (body) { fclose(body); body = NULL; }
    strncpy(out_path, path, sizeof(out_path) - 1);
    out_path[sizeof(out_path) - 1] = '\0';
    body = tmpfile();
    nclause = 0;
}

int cnf_tee_active(void) { return body != NULL; }

void cnf_tee_lit(int lit)
{
    if (!body) return;
    if (lit == 0) { fputs("0\n", body); nclause++; }
    else          { fprintf(body, "%d ", lit); }
}

void cnf_tee_end(int nvars)
{
    if (!body) return;
    FILE* out = fopen(out_path, "w");
    if (!out) { fclose(body); body = NULL; return; }

    fprintf(out, "p cnf %d %ld\n", nvars, nclause);
    /* 射影(独立サポート)= PI の good 回路変数。回路は決定的なので
       これらだけで全変数が一意に決まり、射影カウント = Vi。
       Ganak/ApproxMC系は "c ind ... 0"、MCC2022系は "c p show ... 0" を使う。両方出す。 */
    fprintf(out, "c ind");    for (int i = 0; i < n_pi; i++) fprintf(out, " %d", (int)pi[i]->varsgc); fprintf(out, " 0\n");
    fprintf(out, "c p show"); for (int i = 0; i < n_pi; i++) fprintf(out, " %d", (int)pi[i]->varsgc); fprintf(out, " 0\n");

    rewind(body);
    char buf[1 << 16];
    size_t r;
    while ((r = fread(buf, 1, sizeof(buf), body)) > 0) fwrite(buf, 1, r, out);

    fclose(out);
    fclose(body);
    body = NULL;
}

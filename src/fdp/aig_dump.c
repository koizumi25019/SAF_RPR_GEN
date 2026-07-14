//-------------------------------------------------------------------------------------------------------------
//	aig_dump.c : 検証(env AIG_DUMP=path): 検出回路の AIGER 出力（既定無効・比較実験用）
//
//	検出関数 D_f = OR_{PO∈TFO} (good_po XOR fault_po) を AND-Inverter Graph に分解して
//	ASCII AIGER (.aag) で書き出す。変数 1..n_pi が PI（シンボル表に名前つき）、
//	出力リテラルが 1 つ。HALL (Fried+, SAT'23/24) 等の AllSAT-CT ツールへの入力になる。
//	ゲートの AIG 分解は標準的: AND=連鎖 / OR=ド・モルガン / XOR=2AND+1AND(否定) / 定数=0,1。
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "./aig_dump.h"
#include "../netlist/netlist.h"

/* AND ノードのバッファ（lhs は暗黙に 2*(n_pi+1+idx)） */
static unsigned *ad_r0 = NULL, *ad_r1 = NULL;
static int ad_n = 0, ad_cap = 0;
static int ad_nextvar;   /* 次に割り当てる AIG 変数番号 */

/* new AND node, returns its literal。定数畳み込みで自明な場合はノードを作らない */
static unsigned ad_and(unsigned a, unsigned b){
    if (a == 0 || b == 0) return 0;      /* FALSE 吸収 */
    if (a == 1) return b;                /* TRUE 単位元 */
    if (b == 1) return a;
    if (a == b) return a;
    if ((a ^ b) == 1) return 0;          /* x ∧ ¬x */
    if (ad_n >= ad_cap) {
        ad_cap = ad_cap ? ad_cap * 2 : 4096;
        ad_r0 = realloc(ad_r0, ad_cap * sizeof(unsigned));
        ad_r1 = realloc(ad_r1, ad_cap * sizeof(unsigned));
    }
    ad_r0[ad_n] = a; ad_r1[ad_n] = b; ad_n++;
    return (unsigned)(2 * ad_nextvar++);
}

static unsigned ad_or(unsigned a, unsigned b){ return ad_and(a ^ 1, b ^ 1) ^ 1; }
static unsigned ad_xor(unsigned a, unsigned b){
    return ad_or(ad_and(a, b ^ 1), ad_and(a ^ 1, b));
}

/* ゲート1個の AIG リテラル。in[] は入力リテラル */
static unsigned ad_gate(int type, int n_in, const unsigned* in){
    unsigned acc;
    switch (type) {
        case BUF: case FOUT: return in[0];
        case INV:  return in[0] ^ 1;
        case GND:  return 0;
        case ACC:  return 1;
        case AND: case NAND:
            acc = 1;
            for (int k = 0; k < n_in; k++) acc = ad_and(acc, in[k]);
            return (type == AND) ? acc : acc ^ 1;
        case OR: case NOR:
            acc = 0;
            for (int k = 0; k < n_in; k++) acc = ad_or(acc, in[k]);
            return (type == OR) ? acc : acc ^ 1;
        case EXOR: case EXNOR:
            acc = 0;
            for (int k = 0; k < n_in; k++) acc = ad_xor(acc, in[k]);
            return (type == EXOR) ? acc : acc ^ 1;
        default:
            fprintf(stderr, "[AIG_DUMP] unsupported gate type %d\n", type);
            exit(1);
    }
}

bool AIG_Dump(const char* path, FNODE* f){
    int fsig  = (int)(f->netptr - nl);
    int stuck = (f->type == SF0) ? 0 : 1;

    /* TFO 収集 */
    unsigned char* tfo = calloc(n_net, 1);
    int* stk = malloc(n_net * sizeof(int));
    int sp = 0;
    stk[sp++] = fsig; tfo[fsig] = 1;
    while (sp) {
        int i = stk[--sp];
        for (int k = 0; k < nl[i].n_out; k++) {
            int j = nl[i].out[k]->n;
            if (!tfo[j]) { tfo[j] = 1; stk[sp++] = j; }
        }
    }

    /* トポロジカル順（level 昇順）に good/fault のリテラルを構築 */
    int maxlev = 0;
    for (int i = 0; i < n_net; i++) if (nl[i].level > maxlev) maxlev = nl[i].level;

    unsigned* glit = malloc(n_net * sizeof(unsigned));
    unsigned* flit = malloc(n_net * sizeof(unsigned));
    int* piidx = malloc(n_net * sizeof(int));
    for (int i = 0; i < n_net; i++) piidx[i] = -1;
    for (int s = 0; s < n_pi; s++) piidx[pi[s]->n] = s;

    ad_n = 0;
    ad_nextvar = n_pi + 1;           /* 変数 1..n_pi は PI */
    static unsigned in_g[256], in_f[256];

    for (int L = 0; L <= maxlev; L++)
        for (int i = 0; i < n_net; i++) {
            if (nl[i].level != L) continue;
            NLIST* nd = &nl[i];
            if (nd->type == IN || nd->type == DFF) {
                if (piidx[i] < 0) { fprintf(stderr, "[AIG_DUMP] input %s not in pi[]\n", nd->name); exit(1); }
                glit[i] = (unsigned)(2 * (piidx[i] + 1));
            } else {
                if (nd->n_in > 256) { fprintf(stderr, "[AIG_DUMP] >256 inputs\n"); exit(1); }
                for (int k = 0; k < nd->n_in; k++) in_g[k] = glit[nd->in[k]->n];
                glit[i] = ad_gate(nd->type, nd->n_in, in_g);
            }
            if (tfo[i]) {
                if (i == fsig) flit[i] = (unsigned)stuck;
                else {
                    for (int k = 0; k < nd->n_in; k++) {
                        int j = nd->in[k]->n;
                        in_f[k] = tfo[j] ? flit[j] : glit[j];
                    }
                    flit[i] = ad_gate(nd->type, nd->n_in, in_f);
                }
            } else flit[i] = glit[i];
        }

    /* 出力 = OR_{PO∈TFO} (g XOR f) */
    unsigned out = 0;
    for (int i = 0; i < n_net; i++)
        if (tfo[i] && nl[i].n_out == 0)
            out = ad_or(out, ad_xor(glit[i], flit[i]));

    FILE* fp = fopen(path, "w");
    if (!fp) { fprintf(stderr, "[AIG_DUMP] cannot write %s\n", path); return false; }
    fprintf(fp, "aag %d %d 0 1 %d\n", ad_nextvar - 1, n_pi, ad_n);
    for (int s = 0; s < n_pi; s++) fprintf(fp, "%d\n", 2 * (s + 1));
    fprintf(fp, "%u\n", out);
    for (int t = 0; t < ad_n; t++)
        fprintf(fp, "%d %u %u\n", 2 * (n_pi + 1 + t), ad_r0[t], ad_r1[t]);
    for (int s = 0; s < n_pi; s++) fprintf(fp, "i%d %s\n", s, pi[s]->name);
    fprintf(fp, "c\ndetection circuit of %s,%s (D_f over PIs)\n",
            f->name, (f->type == SF0) ? "sa0" : "sa1");
    fclose(fp);

    fprintf(stderr, "[AIG_DUMP] %s,%s -> %s  (inputs=%d ands=%d out=%u)\n",
            f->name, (f->type == SF0) ? "sa0" : "sa1", path, n_pi, ad_n, out);
    free(tfo); free(stk); free(glit); free(flit); free(piidx);
    return true;
}

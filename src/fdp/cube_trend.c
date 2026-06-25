//-------------------------------------------------------------------------------------------------------------
//	cube_trend.c : テストキューブ列の構造傾向を観察する検証ツール（env CUBE_TREND=1）
//
//	本体パイプラインは一切変えない（読むだけ）。仮説：
//	  「キューブ生成回数が極端に多い故障」は、各キューブの X（ドントケア）が少なく、
//	   しかも X の位置が毎回ほぼ同じ（マスク使い回し）で、新しいキューブが空間を
//	   ほとんど広げられない（拡大効果がほぼない）ために UNSAT まで本数が膨らむ。
//	これを各キューブの X 数・X マスク・連続キューブ差分から定量化する。
//	使い方・記録は cube_trend.h と verification/cube_trend/SUMMARY.md を参照。
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./cube_trend.h"
#include "../netlist/netlist.h"   /* n_pi, pi */

/* ===================== 故障横断のバケット集計（仮説検証の本丸） ===================== */
/* 故障ごとに (キューブ数, 平均X率, マスク重複率) を1件記録し、終了時にキューブ数の
   バケットごとへ平均をまとめる。キューブ数が多い帯ほど X率が下がり/マスク重複が
   上がるなら仮説が支持される。 */
typedef struct { long n; double xfrac; double mask_reuse; } CTRec;
static CTRec* recs = NULL;
static int    nrec = 0, reccap = 0;

static void ct_dump(void)
{
    /* キューブ数の対数的バケット境界 */
    static const long edge[] = { 10, 100, 1000, 10000, 100000, -1 };
    fprintf(stderr,
        "\n[CT] fault buckets by cube count (does X shrink / mask reuse rise as cubes explode?)\n");
    fprintf(stderr,
        "[CT]   cube_cnt range :  faults   avg_cubes   avg_X%%   mask_reuse%%\n");
    for (int b = 0; edge[b] != -1 || b == 0; b++) {
        long lo = (b == 0) ? 1 : edge[b-1] + 1;
        long hi = edge[b];                       /* hi==-1 は上限なし */
        long cnt = 0; double sc = 0, sx = 0, sm = 0;
        for (int i = 0; i < nrec; i++) {
            if (recs[i].n < lo) continue;
            if (hi != -1 && recs[i].n > hi) continue;
            cnt++; sc += recs[i].n; sx += recs[i].xfrac; sm += recs[i].mask_reuse;
        }
        if (cnt == 0) { if (hi == -1) break; else continue; }
        char range[32];
        if (hi == -1) snprintf(range, sizeof range, "%6ld+        ", lo);
        else          snprintf(range, sizeof range, "%6ld-%-7ld", lo, hi);
        fprintf(stderr, "[CT]   %s : %6ld   %9.0f   %5.1f   %9.1f\n",
            range, cnt, sc/cnt, 100.0*sx/cnt, 100.0*sm/cnt);
        if (hi == -1) break;
    }
}

static void ct_record(long n, double xfrac, double mask_reuse)
{
    if (nrec == 0) atexit(ct_dump);
    if (nrec == reccap) { reccap = reccap ? reccap*2 : 256; recs = realloc(recs, reccap*sizeof(CTRec)); }
    recs[nrec].n = n; recs[nrec].xfrac = xfrac; recs[nrec].mask_reuse = mask_reuse; nrec++;
}

/* ===================== 明細 CSV（任意・プロット用） ===================== */
static FILE* ct_csv(void)
{
    static FILE* fp = NULL; static int tried = 0;
    if (tried) return fp;
    tried = 1;
    const char* path = getenv("CUBE_TREND_CSV");
    if (path && (fp = fopen(path, "w")))
        fprintf(fp, "fault,f_type,idx,x_count,care_count,same_mask_vs_prev,val_diff_vs_prev\n");
    return fp;
}

static int ull_cmp(const void* a, const void* b)
{
    unsigned long long x = *(const unsigned long long*)a, y = *(const unsigned long long*)b;
    return (x > y) - (x < y);
}

/* ===================== 具体例ダンプ（任意・図解用） =====================
   env CUBE_TREND_DUMP=<故障名の部分一致> の故障について、生成順に各キューブの
   '0'/'1'/'X' 列をそのまま CUBE_TREND_DUMPFILE へ書き出す（idx と並べる）。
   ビット列の具体例を作図するための補助で、未設定なら何もしない。 */
static void ct_dump_cubes(FNODE* f, CubeSet* cubes)
{
    const char* want = getenv("CUBE_TREND_DUMP");
    if (!want || !strstr(f->name, want)) return;
    const char* path = getenv("CUBE_TREND_DUMPFILE");
    FILE* fp = path ? fopen(path, "w") : stderr;
    if (!fp) return;
    fprintf(fp, "# %s %s n_pi=%d cubes=%d\n",
        f->name, (f->type == SF0) ? "sa0" : "sa1", n_pi, cubes->n);
    for (int k = 0; k < cubes->n; k++)
        fprintf(fp, "%d %s\n", k, cubes->data[k]);
    if (fp != stderr) fclose(fp);
}

/* ===================== 1故障ぶんの傾向集計 ===================== */
void CT_Report(FNODE* f, CubeSet* cubes, bool limit_hit)
{
    if (!getenv("CUBE_TREND")) return;
    int n = cubes->n;
    if (n <= 0) return;

    /* PI ごとの X 出現回数（X 位置の集中度を測る）と、各キューブの X マスクのハッシュ */
    static int* xfreq = NULL;                 static int xcap = 0;
    static unsigned long long* mh = NULL;     static int mhcap = 0;
    if (xcap < n_pi) { xfreq = realloc(xfreq, n_pi*sizeof(int)); xcap = n_pi; }
    if (mhcap < n)   { mh    = realloc(mh, (size_t)n*sizeof(unsigned long long)); mhcap = n; }
    for (int i = 0; i < n_pi; i++) xfreq[i] = 0;

    FILE* csv = ct_csv();
    const char* tp = (f->type == SF0) ? "sa0" : "sa1";

    long  sum_x = 0;
    int   head_lim = (n + 9) / 10;            /* 先頭/末尾 10% で X 推移を見る */
    double x_head = 0, x_tail = 0; int head_n = 0, tail_n = 0;
    long  sum_same_mask = 0, sum_val_diff = 0; int n_pair = 0;  /* 連続キューブ差分 */
    char* prev = NULL;

    for (int k = 0; k < n; k++) {
        char* c = cubes->data[k];
        int xc = 0;
        unsigned long long h = 1469598103934665603ULL;   /* FNV-1a 64 で X マスクを要約 */
        for (int i = 0; i < n_pi; i++) {
            int isx = (c[i] == 'X');
            if (isx) { xc++; xfreq[i]++; }
            h = (h ^ (unsigned char)(isx ? 'X' : '.')) * 1099511628211ULL;
        }
        mh[k] = h;
        sum_x += xc;
        if (k <  head_lim)     { x_head += xc; head_n++; }
        if (k >= n - head_lim) { x_tail += xc; tail_n++; }

        int same_mask = -1, val_diff = -1;    /* prev とのマスク一致数・ケア値の相違数 */
        if (prev) {
            int sm = 0, vd = 0;
            for (int i = 0; i < n_pi; i++) {
                if ((prev[i] == 'X') == (c[i] == 'X')) sm++;
                if (prev[i] != 'X' && c[i] != 'X' && prev[i] != c[i]) vd++;
            }
            same_mask = sm; val_diff = vd;
            sum_same_mask += sm; sum_val_diff += vd; n_pair++;
        }
        if (csv)
            fprintf(csv, "%s,%s,%d,%d,%d,%d,%d\n", f->name, tp, k, xc, n_pi - xc, same_mask, val_diff);
        prev = c;
    }

    /* 異なる X マスクの本数（マスク使い回しの指標）。重複率 = 1 - distinct/n。 */
    qsort(mh, n, sizeof(unsigned long long), ull_cmp);
    int distinct = 1;
    for (int k = 1; k < n; k++) if (mh[k] != mh[k-1]) distinct++;
    double mask_reuse = 1.0 - (double)distinct / n;

    /* X 位置の集中度: 何個の PI が一度でも X になったか / 最頻 PI が X になった割合 */
    int support = 0, top = 0;
    for (int i = 0; i < n_pi; i++) { if (xfreq[i] > 0) support++; if (xfreq[i] > top) top = xfreq[i]; }

    double xmean = (double)sum_x / n;
    double xfrac = xmean / n_pi;

    fprintf(stderr,
        "[CT] %-22s %s n=%-6d%s X/cube mean=%.1f(%.0f%%) head=%.1f->tail=%.1f  "
        "mask_reuse=%.0f%% Xsupport=%d/%d topX=%.0f%%  "
        "consec: same_mask=%.0f%% val_diff=%.1f\n",
        f->name, tp, n, limit_hit ? "(capped)" : "",
        xmean, 100.0*xfrac,
        head_n ? x_head/head_n : 0.0, tail_n ? x_tail/tail_n : 0.0,
        100.0*mask_reuse, support, n_pi, 100.0*top/n,
        n_pair ? 100.0*sum_same_mask/n_pair/n_pi : 0.0,
        n_pair ? (double)sum_val_diff/n_pair : 0.0);

    ct_record(n, xfrac, mask_reuse);
    ct_dump_cubes(f, cubes);   /* 図解用: 指定故障のビット列を書き出す（既定無効） */
}

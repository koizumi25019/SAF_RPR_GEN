//-------------------------------------------------------------------------------------------------------------
//	pcount.c : 打ち切り故障の検出パターン数の厳密数え上げ（env PCOUNT=1、既定無効）
//
//	文献準拠の構成（各部品の出典）:
//	  [1] Möhle & Biere, "Dualizing Projected Model Counting", ICTAI 2018 (DUALIZA)
//	      — 双対（P/N）伝播による部分モデルの早期確定。回路に特殊化すると
//	        「3値シミュレーションで PO 差が確定 → 全拡張が検出（2^未割当 を加算）」
//	        「全 TFO PO の一致が確定 → 全拡張が非検出（0）」に相当する。
//	  [2] Huang & Darwiche, "Using DPLL for Efficient OBDD Construction", SAT 2004
//	      — 静的変数順のもと、カット i をまたぐ separator 変数の値をレベル別キーに
//	        する等価キャッシュ。計算量は最大 separator 幅の指数のみ。ここでは
//	        「深さ d までの PI で値が確定し（maxdep<=d）、かつ未確定領域のゲートに
//	        まだ使われる（useu>d）ネット」の3値状態を separator とする回路版。
//	  [3] Bayardo & Pehoushek (AAAI 2000) / Thurley sharpSAT (SAT 2006)
//	      — #SAT における部分問題キャッシュ（[2] と同族。ここでは [2] の方式を採用）。
//	探索は ATPG(PODEM) 流のトップダウン PI 割り当て。SATソルバ・BDD 合成・
//	外部カウンタは使わない。サポート PI をすべて割り当てると全信号が確定するので
//	必ず停止する。割り当て順は故障サイト→TFO PO の fanin DFS 順（構造的局所性）。
//	  env: PCOUNT=1          有効化（limit 打ち切り故障のみ発動）
//	       PCOUNT_MAXNODES   探索ノード上限（既定 2,000,000。超過で NULL=capped 扱い）
//	       PCOUNT_CACHE      メモ化テーブルのエントリ数上限（既定 1<<20）
//	評価は verification/pcount/SUMMARY.md。
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <gmp.h>

#include "./pcount.h"
#include "../netlist/netlist.h"

#define PC_X 2   /* 3値: 0 / 1 / X */

/* ---- 故障ごとの探索コンテキスト ---- */
static int  pc_fsig, pc_stuck;
static int *pc_topo = NULL, pc_ntopo = 0;       /* 関連領域のトポロジカル順 net id */
static unsigned char *pc_tfo = NULL;            /* TFO フラグ（net id 添字） */
static unsigned char *pc_reg = NULL;            /* 関連領域フラグ */
static unsigned char *pc_gv = NULL, *pc_fv = NULL;  /* 3値の正常/故障値 */
static int *pc_sup = NULL, pc_nsup = 0;         /* サポート PI（pi[] index、割り当て順） */
static int *pc_piord = NULL;                    /* net id -> サポート順(1..nsup)、なければ 0 */
static long pc_nodes = 0, pc_maxnodes = 0;
static int  pc_abort = 0;

/* PO の確定状態カウンタ（O(1) の早期終了判定 [1]） */
static int pc_po_open = 0;   /* g か f が X の TFO PO 数 */
static int pc_po_diff = 0;   /* g,f とも確定かつ異なる TFO PO 数 */
static unsigned char *pc_ispo = NULL;   /* TFO PO か */

/* separator キャッシュ [2]: 各ネットの確定深さと最終使用深さ、enter/leave リスト */
static int *pc_maxdep = NULL;   /* 値が必ず確定する深さ（fanin コーン内サポート順の最大） */
static int *pc_useu   = NULL;   /* separator として使われ終わる深さ（fanout の maxdep 最大） */
static int *pc_enter = NULL, *pc_enter_off = NULL;   /* 深さ d で separator に入るネット */
static int *pc_leave = NULL, *pc_leave_off = NULL;   /* 深さ d で separator から出るネット */

/* Zobrist ハッシュ（差分更新、2本で128bit相当） */
static uint64_t pc_h1, pc_h2;
static uint64_t (*pc_zob1)[9] = NULL, (*pc_zob2)[9] = NULL;

/* イベント駆動シミュレーションの undo ログ */
typedef struct { int net; unsigned char og, of; } PCEv;
static PCEv* pc_ev = NULL;
static long  pc_nev = 0, pc_evcap = 0;
static long* pc_evmark = NULL;    /* 深さごとのイベント開始位置 */

/* ---- 統計 ---- */
static long pcs_faults = 0, pcs_done = 0, pcs_bail = 0, pcs_nodes = 0, pcs_hits = 0;
static void pc_dump(void){
    fprintf(stderr, "\n[PCOUNT] faults=%ld  completed=%ld  bailed=%ld  nodes=%ld  cache_hits=%ld\n",
        pcs_faults, pcs_done, pcs_bail, pcs_nodes, pcs_hits);
}

/* ---- メモ化テーブル（開番地法、128bit キー） ---- */
typedef struct { uint64_t h1, h2; mpz_t cnt; int used; } PCEntry;
static PCEntry* pc_tab = NULL;
static size_t   pc_cap = 0, pc_used = 0;

static void pc_tab_init(void){
    size_t cap = getenv("PCOUNT_CACHE") ? (size_t)atol(getenv("PCOUNT_CACHE")) : (1u<<20);
    size_t p = 1; while (p < cap) p <<= 1;
    pc_cap = p;
    pc_tab = calloc(pc_cap, sizeof(PCEntry));
    pc_used = 0;
}
static void pc_tab_reset(void){
    for (size_t i = 0; i < pc_cap; i++)
        if (pc_tab[i].used) { mpz_clear(pc_tab[i].cnt); pc_tab[i].used = 0; }
    pc_used = 0;
}

/* 決定的な擬似乱数（splitmix64）で Zobrist 表を作る */
static uint64_t pc_mix(uint64_t x){
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

/* ---- 3値ゲート評価（入力値は呼び出し側が正常/故障を選んで渡す） ---- */
static unsigned char pc_gate(int type, const NLIST* nd, const unsigned char* in){
    switch (type) {
        case BUF: case FOUT: return in[0];
        case INV:  return in[0] == PC_X ? PC_X : !in[0];
        case GND:  return 0;
        case ACC:  return 1;
        case AND: case NAND: {
            unsigned char a = 1;
            for (int k = 0; k < nd->n_in; k++) {
                if (in[k] == 0) { a = 0; break; }
                if (in[k] == PC_X) a = PC_X;
            }
            if (type == AND) return a;
            return a == PC_X ? PC_X : !a;
        }
        case OR: case NOR: {
            unsigned char a = 0;
            for (int k = 0; k < nd->n_in; k++) {
                if (in[k] == 1) { a = 1; break; }
                if (in[k] == PC_X) a = PC_X;
            }
            if (type == OR) return a;
            return a == PC_X ? PC_X : !a;
        }
        case EXOR: case EXNOR: {
            unsigned char a = 0;
            for (int k = 0; k < nd->n_in; k++) {
                if (in[k] == PC_X) return PC_X;
                a ^= in[k];
            }
            return (type == EXOR) ? a : !a;
        }
        default: return PC_X;
    }
}

/* ネット i の現在値を（入力の gv/fv から）計算する。fault=1 なら故障回路側 */
static unsigned char pc_eval(int i, int fault){
    static unsigned char in[256];
    NLIST* nd = &nl[i];
    if (nd->type == IN || nd->type == DFF) return fault ? pc_fv[i] : pc_gv[i];
    if (fault && i == pc_fsig) return (unsigned char)pc_stuck;
    for (int k = 0; k < nd->n_in; k++) {
        int j = nd->in[k]->n;
        in[k] = (fault && pc_tfo[j]) ? pc_fv[j] : pc_gv[j];
    }
    return pc_gate(nd->type, nd, in);
}

/* PO カウンタの増分更新: ネット i の (g,f) が (og,of)->(ng,nf) に変わった */
static void pc_po_update(int i, unsigned char og, unsigned char of,
                         unsigned char ng, unsigned char nf){
    if (!pc_ispo[i]) return;
    int was_open = (og == PC_X || of == PC_X);
    int was_diff = (!was_open && og != of);
    int is_open  = (ng == PC_X || nf == PC_X);
    int is_diff  = (!is_open && ng != nf);
    pc_po_open += is_open - was_open;
    pc_po_diff += is_diff - was_diff;
}

/* undo ログへ記録して値を更新 */
static void pc_setval(int i, unsigned char ng, unsigned char nf){
    if (pc_nev >= pc_evcap) {
        pc_evcap = pc_evcap ? pc_evcap * 2 : 65536;
        pc_ev = realloc(pc_ev, (size_t)pc_evcap * sizeof(PCEv));
    }
    pc_ev[pc_nev++] = (PCEv){ i, pc_gv[i], pc_fv[i] };
    pc_po_update(i, pc_gv[i], pc_fv[i], ng, nf);
    pc_gv[i] = ng; pc_fv[i] = nf;
}

/* サポート PI d 番目（0-based）に b を割り当て、イベント駆動で領域を伝播する。
   3値は単調（X -> 0/1 のみ）なので、変化したネットの fanout だけ再評価すればよい。 */
static void pc_assign(int depth, unsigned char b){
    pc_evmark[depth] = pc_nev;
    int pin = pi[pc_sup[depth]]->n;
    {
        unsigned char nf = (pin == pc_fsig) ? (unsigned char)pc_stuck : b;
        pc_setval(pin, b, pc_tfo[pin] ? nf : b);
    }
    /* 作業リスト伝播。3値は単調（X→0/1 のみ、確定値は覆らない）なので処理順は
       正しさに影響しない: 早すぎる評価は X のままか確定値になり、入力が後から
       確定したら再エンキューされる。よって FIFO でよい。 */
    static int* q = NULL; static unsigned char* inq = NULL; static int qcap = 0;
    if (qcap == 0) {
        qcap = 4 * n_net + 64;
        q = malloc((size_t)qcap * sizeof(int));
        inq = calloc(n_net, 1);
    }
    int qh = 0, qt = 0;
    #define PC_ENQ(j) do { \
        if (pc_reg[j] && !inq[j]) { \
            if (qt >= qcap) { qcap *= 2; q = realloc(q, (size_t)qcap * sizeof(int)); } \
            inq[j] = 1; q[qt++] = j; \
        } } while (0)
    for (int k = 0; k < nl[pin].n_out; k++) PC_ENQ(nl[pin].out[k]->n);
    while (qh < qt) {
        int i = q[qh++];
        inq[i] = 0;
        unsigned char ng = pc_eval(i, 0);
        unsigned char nf = pc_tfo[i] ? pc_eval(i, 1) : ng;
        if (ng == pc_gv[i] && nf == pc_fv[i]) continue;
        pc_setval(i, ng, nf);
        for (int k = 0; k < nl[i].n_out; k++) PC_ENQ(nl[i].out[k]->n);
    }
    #undef PC_ENQ
    /* separator の差分更新 [2]: 深さ d=depth+1 で出入りするネットを XOR */
    int d = depth + 1;
    for (int t = pc_enter_off[d]; t < pc_enter_off[d + 1]; t++) {
        int i = pc_enter[t];
        int code = pc_gv[i] * 3 + pc_fv[i];
        pc_h1 ^= pc_zob1[i][code]; pc_h2 ^= pc_zob2[i][code];
    }
    for (int t = pc_leave_off[d]; t < pc_leave_off[d + 1]; t++) {
        int i = pc_leave[t];
        int code = pc_gv[i] * 3 + pc_fv[i];
        pc_h1 ^= pc_zob1[i][code]; pc_h2 ^= pc_zob2[i][code];
    }
}

static void pc_undo(int depth){
    /* ハッシュの巻き戻し（XOR は自己逆元。値はこの間不変なので同じ量が消える） */
    int d = depth + 1;
    for (int t = pc_enter_off[d]; t < pc_enter_off[d + 1]; t++) {
        int i = pc_enter[t];
        int code = pc_gv[i] * 3 + pc_fv[i];
        pc_h1 ^= pc_zob1[i][code]; pc_h2 ^= pc_zob2[i][code];
    }
    for (int t = pc_leave_off[d]; t < pc_leave_off[d + 1]; t++) {
        int i = pc_leave[t];
        int code = pc_gv[i] * 3 + pc_fv[i];
        pc_h1 ^= pc_zob1[i][code]; pc_h2 ^= pc_zob2[i][code];
    }
    /* シミュレーションイベントの巻き戻し */
    while (pc_nev > pc_evmark[depth]) {
        PCEv* e = &pc_ev[--pc_nev];
        pc_po_update(e->net, pc_gv[e->net], pc_fv[e->net], e->og, e->of);
        pc_gv[e->net] = e->og; pc_fv[e->net] = e->of;
    }
}

/* 再帰カウント本体。out = サポートの未割当 2^(nsup-depth) 通りのうち検出になる数 */
static void pc_count(int depth, mpz_t out){
    if (pc_abort) { mpz_set_ui(out, 0); return; }
    if (++pc_nodes > pc_maxnodes) { pc_abort = 1; mpz_set_ui(out, 0); return; }

    /* 早期終了 [1]: PO 差が確定 → 全拡張が検出 / 全 PO 一致確定 → 非検出 */
    if (pc_po_diff > 0) { mpz_ui_pow_ui(out, 2, (unsigned long)(pc_nsup - depth)); return; }
    if (pc_po_open == 0) { mpz_set_ui(out, 0); return; }

    /* メモ化 [2]: (深さ, separator 状態) が一致する部分問題は同じ数を返す */
    uint64_t h1 = pc_h1 ^ pc_mix((uint64_t)depth);
    uint64_t h2 = pc_h2 ^ pc_mix(~(uint64_t)depth);
    size_t slot = (size_t)(h1 ^ h2) & (pc_cap - 1);
    while (pc_tab[slot].used) {
        if (pc_tab[slot].h1 == h1 && pc_tab[slot].h2 == h2) {
            mpz_set(out, pc_tab[slot].cnt);
            pcs_hits++;
            return;
        }
        slot = (slot + 1) & (pc_cap - 1);
    }

    mpz_t a, b;
    mpz_init(a); mpz_init(b);
    pc_assign(depth, 0); pc_count(depth + 1, a); pc_undo(depth);
    pc_assign(depth, 1); pc_count(depth + 1, b); pc_undo(depth);
    mpz_add(out, a, b);
    mpz_clear(a); mpz_clear(b);
    if (pc_abort) return;

    if (pc_used < pc_cap / 2) {     /* 半分まで使ったら挿入停止（正しさは不変・速度のみ低下） */
        slot = (size_t)(h1 ^ h2) & (pc_cap - 1);
        while (pc_tab[slot].used) {
            if (pc_tab[slot].h1 == h1 && pc_tab[slot].h2 == h2) return;
            slot = (slot + 1) & (pc_cap - 1);
        }
        pc_tab[slot].h1 = h1; pc_tab[slot].h2 = h2;
        mpz_init_set(pc_tab[slot].cnt, out);
        pc_tab[slot].used = 1;
        pc_used++;
    }
}

char* PC_ExactCountStr(FNODE* f){
    static int inited = 0;
    if (!inited) {
        for (int i = 0; i < n_net; i++)
            if (nl[i].n_in > 256) {
                fprintf(stderr, "[PCOUNT] gate with >256 inputs (%s) — disabled\n", nl[i].name);
                return NULL;
            }
        pc_tab_init();
        pc_tfo    = calloc(n_net, 1);
        pc_reg    = calloc(n_net, 1);
        pc_ispo   = calloc(n_net, 1);
        pc_gv     = malloc(n_net);
        pc_fv     = malloc(n_net);
        pc_topo   = malloc(n_net * sizeof(int));
        pc_sup    = malloc(n_pi  * sizeof(int));
        pc_piord  = malloc(n_net * sizeof(int));
        pc_maxdep = malloc(n_net * sizeof(int));
        pc_useu   = malloc(n_net * sizeof(int));
        pc_enter  = malloc(n_net * sizeof(int));
        pc_leave  = malloc(n_net * sizeof(int));
        pc_enter_off = malloc((n_pi + 2) * sizeof(int));
        pc_leave_off = malloc((n_pi + 2) * sizeof(int));
        pc_evmark = malloc((n_pi + 1) * sizeof(long));
        pc_zob1 = malloc((size_t)n_net * 9 * sizeof(uint64_t));
        pc_zob2 = malloc((size_t)n_net * 9 * sizeof(uint64_t));
        for (int i = 0; i < n_net; i++)
            for (int c = 0; c < 9; c++) {
                pc_zob1[i][c] = pc_mix(((uint64_t)i << 4) + c + 1);
                pc_zob2[i][c] = pc_mix(((uint64_t)i << 4) + c + 0x123456789ull);
            }
        atexit(pc_dump);
        inited = 1;
    }
    pcs_faults++;
    pc_fsig  = (int)(f->netptr - nl);
    pc_stuck = (f->type == SF0) ? 0 : 1;
    pc_maxnodes = getenv("PCOUNT_MAXNODES") ? atol(getenv("PCOUNT_MAXNODES")) : 2000000;
    pc_nodes = 0; pc_abort = 0; pc_nev = 0;
    pc_tab_reset();

    /* TFO（故障サイトから出力辺 DFS）と TFO 内 PO */
    static int* stk = NULL;
    if (!stk) stk = malloc(n_net * sizeof(int));
    memset(pc_tfo,  0, (size_t)n_net);
    memset(pc_ispo, 0, (size_t)n_net);
    int sp = 0, ntpo = 0;
    stk[sp++] = pc_fsig; pc_tfo[pc_fsig] = 1;
    while (sp) {
        int i = stk[--sp];
        if (nl[i].n_out == 0) { pc_ispo[i] = 1; ntpo++; }
        for (int k = 0; k < nl[i].n_out; k++) {
            int j = nl[i].out[k]->n;
            if (!pc_tfo[j]) { pc_tfo[j] = 1; stk[sp++] = j; }
        }
    }
    if (ntpo == 0) { pcs_done++; return strdup("0"); }   /* PO に届かない → 冗長 */

    /* 関連領域 = TFO PO 群の fanin コーン。トポ順は level 昇順 */
    memset(pc_reg, 0, (size_t)n_net);
    for (int i = 0; i < n_net; i++)
        if (pc_ispo[i]) { stk[sp++] = i; pc_reg[i] = 1; }
    while (sp) {
        int i = stk[--sp];
        for (int k = 0; k < nl[i].n_in; k++) {
            int j = nl[i].in[k]->n;
            if (!pc_reg[j]) { pc_reg[j] = 1; stk[sp++] = j; }
        }
    }
    int maxlev = 0;
    for (int i = 0; i < n_net; i++) if (pc_reg[i] && nl[i].level > maxlev) maxlev = nl[i].level;
    pc_ntopo = 0;
    for (int L = 0; L <= maxlev; L++)
        for (int i = 0; i < n_net; i++)
            if (pc_reg[i] && nl[i].level == L) pc_topo[pc_ntopo++] = i;

    /* サポート PI の割り当て順: 故障サイト→各 TFO PO の fanin DFS 順（構造的局所性） */
    static unsigned char* seen = NULL;
    static int* piidx_of_net = NULL;
    if (!seen) {
        seen = calloc(n_net, 1);
        piidx_of_net = malloc(n_net * sizeof(int));
        for (int i = 0; i < n_net; i++) piidx_of_net[i] = -1;
        for (int s = 0; s < n_pi; s++) piidx_of_net[pi[s]->n] = s;
    }
    memset(seen, 0, (size_t)n_net);
    pc_nsup = 0;
    sp = 0; stk[sp++] = pc_fsig; seen[pc_fsig] = 1;
    for (int r = -1; sp || r + 1 < n_net; ) {
        if (!sp) {   /* 次の未訪問 TFO PO を根に */
            while (++r < n_net && !(pc_ispo[r] && !seen[r])) ;
            if (r >= n_net) break;
            stk[sp++] = r; seen[r] = 1;
        }
        while (sp) {
            int i = stk[--sp];
            if (piidx_of_net[i] >= 0 && pc_reg[i]) pc_sup[pc_nsup++] = piidx_of_net[i];
            for (int k = nl[i].n_in - 1; k >= 0; k--) {
                int j = nl[i].in[k]->n;
                if (!seen[j]) { seen[j] = 1; stk[sp++] = j; }
            }
        }
    }
    for (int s = 0; s < n_pi; s++)   /* 取りこぼしを末尾へ */
        if (pc_reg[pi[s]->n] && !seen[pi[s]->n]) pc_sup[pc_nsup++] = s;

    /* separator の静的前計算 [2]:
       maxdep[i] = i の fanin コーン内サポート PI の順位（1..nsup）の最大（=値が必ず確定する深さ）
       useu[i]   = i を入力に持つ領域内ゲートの maxdep の最大（= separator として使われ終わる深さ）
       ネット i は深さ d ∈ [maxdep[i], useu[i]-1] の間 separator に属する。 */
    for (int i = 0; i < n_net; i++) pc_piord[i] = 0;
    for (int s = 0; s < pc_nsup; s++) pc_piord[pi[pc_sup[s]]->n] = s + 1;
    for (int t = 0; t < pc_ntopo; t++) {
        int i = pc_topo[t];
        if (nl[i].type == IN || nl[i].type == DFF) { pc_maxdep[i] = pc_piord[i]; continue; }
        int m = 0;
        for (int k = 0; k < nl[i].n_in; k++) {
            int md = pc_maxdep[nl[i].in[k]->n];
            if (md > m) m = md;
        }
        pc_maxdep[i] = m;
    }
    for (int t = 0; t < pc_ntopo; t++) {
        int i = pc_topo[t];
        int u = pc_maxdep[i];
        for (int k = 0; k < nl[i].n_out; k++) {
            int j = nl[i].out[k]->n;
            if (pc_reg[j] && pc_maxdep[j] > u) u = pc_maxdep[j];
        }
        pc_useu[i] = u;
    }
    /* enter/leave リスト（counting sort で深さ別に並べる） */
    memset(pc_enter_off, 0, (n_pi + 2) * sizeof(int));
    memset(pc_leave_off, 0, (n_pi + 2) * sizeof(int));
    for (int t = 0; t < pc_ntopo; t++) {
        int i = pc_topo[t];
        if (pc_useu[i] > pc_maxdep[i]) {
            pc_enter_off[pc_maxdep[i] + 1]++;
            pc_leave_off[pc_useu[i] + 1]++;
        }
    }
    for (int d = 0; d <= pc_nsup; d++) {
        pc_enter_off[d + 1] += pc_enter_off[d];
        pc_leave_off[d + 1] += pc_leave_off[d];
    }
    static int* fill = NULL;
    if (!fill) fill = malloc((n_pi + 2) * sizeof(int));
    memcpy(fill, pc_enter_off, (pc_nsup + 2) * sizeof(int));
    for (int t = 0; t < pc_ntopo; t++) {
        int i = pc_topo[t];
        if (pc_useu[i] > pc_maxdep[i]) pc_enter[fill[pc_maxdep[i]]++] = i;
    }
    memcpy(fill, pc_leave_off, (pc_nsup + 2) * sizeof(int));
    for (int t = 0; t < pc_ntopo; t++) {
        int i = pc_topo[t];
        if (pc_useu[i] > pc_maxdep[i]) pc_leave[fill[pc_useu[i]]++] = i;
    }

    /* 初期状態: 全サポート X で領域を一度だけ評価（定数の伝播）し、PO カウンタと
       深さ0の separator（定数ネット）ハッシュを初期化 */
    pc_po_open = pc_po_diff = 0;
    pc_h1 = pc_h2 = 0;
    for (int t = 0; t < pc_ntopo; t++) {
        int i = pc_topo[t];
        pc_gv[i] = PC_X; pc_fv[i] = PC_X;   /* pc_eval が入力を読む前に初期化 */
        unsigned char ng = pc_eval(i, 0);
        unsigned char nf = pc_tfo[i] ? pc_eval(i, 1) : ng;
        pc_gv[i] = ng; pc_fv[i] = nf;
        if (pc_ispo[i]) {
            if (ng == PC_X || nf == PC_X) pc_po_open++;
            else if (ng != nf) pc_po_diff++;
        }
    }
    for (int t = pc_enter_off[0]; t < pc_enter_off[1]; t++) {
        int i = pc_enter[t];
        int code = pc_gv[i] * 3 + pc_fv[i];
        pc_h1 ^= pc_zob1[i][code]; pc_h2 ^= pc_zob2[i][code];
    }

    mpz_t vi; mpz_init(vi);
    pc_count(0, vi);
    pcs_nodes += pc_nodes;
    if (pc_abort) { mpz_clear(vi); pcs_bail++; return NULL; }

    /* 領域外 PI は検出に無関係: 2^(n_pi - nsup) 倍 */
    mpz_t mul; mpz_init(mul);
    mpz_ui_pow_ui(mul, 2, (unsigned long)(n_pi - pc_nsup));
    mpz_mul(vi, vi, mul);
    mpz_clear(mul);

    char* s = mpz_get_str(NULL, 10, vi);
    mpz_clear(vi);
    pcs_done++;
    return s;   /* GMP の既定アロケータは malloc なので free 可能 */
}

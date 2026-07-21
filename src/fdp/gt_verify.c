//-------------------------------------------------------------------------------------------------------------
//	gt_verify.c : 独立グラウンドトゥルース検証（env GT_BDD=1）
//
//	SAT/CNF/XID/キューブ列挙/支配流用を一切使わず、ネットリストから
//	正常回路と故障回路の BDD を直接構築して検出関数 D_f を厳密に得る。
//	パイプラインのキューブ和集合と「同一 CUDD マネージャ内のポインタ比較」で照合：
//	  sound : 和集合 ⊆ D_f（非検出ミンタームを含まない）→ 全故障で成立すべき
//	  exact : 和集合 = D_f                              → complete=1 なら成立すべき
//	共有するのはネットリスト構造体と CUDD のみで、疑わしい箇所
//	（TPGモデルCNF・XID・支配流用・キューブ処理）は全て迂回する。
//
//	env:
//	  GT_BDD=1     有効化。不一致故障を stderr に出力し、終了時に [GT] summary。
//	  GT_VERBOSE=1 一致した故障も全て出力。
//	  GT_CUBES=1   非健全キューブを特定し「どのXを1ビット固定すれば健全になるか」
//	               の候補を列挙（バグ箇所の特定用）。
//	  GT_ISOP=1    D_f から Minato-Morreale ISOP を生成し、現行キューブ数と比較。
//	               DNF自体の複雑さと列挙戦略の冗長性を切り分ける診断用。
//	不一致時は独立モンテカルロシミュレーション(FdpBySim)の値も併記する（三重照合）。
//	検証記録: verification/gt_bdd/SUMMARY.md
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <cudd.h>

#include "./gt_verify.h"
#include "../netlist/netlist.h"

/* ============== 独立2値シミュレーション（GT自体の三重照合用） ============== */
static int* sim_topo  = NULL;   /* net id をトポロジカル順（level 昇順）に並べた配列 */
static int  sim_ntopo = 0;

/* level 昇順（PI→PO）に net id を並べた評価順を一度だけ作る。 */
static void sim_build_topo(void)
{
    if (sim_topo) return;
    sim_topo = (int*)malloc(n_net * sizeof(int));

    int maxlev = 0;
    for (int i = 0; i < n_net; i++)
        if (nl[i].level > maxlev) maxlev = nl[i].level;

    int idx = 0;
    for (int L = 0; L <= maxlev; L++)
        for (int i = 0; i < n_net; i++)
            if (nl[i].level == L) sim_topo[idx++] = i;
    sim_ntopo = idx;
}

/* ゲート1個の2値評価。入力値は val[net id] から引く。IN/DFF は acc0（PI値）を保つ。 */
static inline int sim_gate(int type, int acc0, NLIST* nd, const int* val)
{
    switch (type) {
        case BUF: case FOUT:
            return val[nd->in[0]->n];
        case INV:
            return val[nd->in[0]->n] ^ 1;
        case AND: case NAND: {
            int a = 1;
            for (int k = 0; k < nd->n_in; k++) { a &= val[nd->in[k]->n]; if (!a) break; }
            return (type == NAND) ? a ^ 1 : a;
        }
        case OR: case NOR: {
            int a = 0;
            for (int k = 0; k < nd->n_in; k++) { a |= val[nd->in[k]->n]; if (a) break; }
            return (type == NOR) ? a ^ 1 : a;
        }
        case EXOR: case EXNOR: {
            int a = 0;
            for (int k = 0; k < nd->n_in; k++) a ^= val[nd->in[k]->n];
            return (type == EXNOR) ? a ^ 1 : a;
        }
        default:
            return acc0;   /* IN/DFF: keep PI value */
    }
}

/* モンテカルロ真値FDP（SAT/BDD不使用の独立実装）。N回サンプリング。 */
static double FdpBySim(FNODE* f, long N)
{
    sim_build_topo();

    static int *vg = NULL, *vf = NULL;   /* 正常値 / 故障値（net id 添字） */
    if (!vg) {
        vg = malloc(n_net * sizeof(int));
        vf = malloc(n_net * sizeof(int));
    }

    size_t fsig  = (size_t)(f->netptr - nl);
    int    stuck = (f->type == SF0) ? 0 : 1;
    long   det   = 0;

    /* TDF: 励起条件（1時刻目コピー = 初期値）を満たすサンプルだけが検出可能 */
    int excid  = f->exc_netptr ? f->exc_netptr->n : -1;
    int excval = (f->type == SF0) ? 0 : 1;

    for (long s = 0; s < N; s++) {
        /* PI にランダム値を与える（正常・故障とも同じ入力） */
        for (int i = 0; i < n_pi; i++) {
            int b = rand() & 1;
            vg[pi[i]->n] = b;
            vf[pi[i]->n] = b;
        }
        /* 故障なしで全ゲートを評価 */
        for (int t = 0; t < sim_ntopo; t++) {
            int i = sim_topo[t], ty = nl[i].type;
            if (ty == IN || ty == DFF) continue;
            vg[i] = sim_gate(ty, vg[i], &nl[i], vg);
            vf[i] = sim_gate(ty, vf[i], &nl[i], vf);
        }
        /* TDF: 遷移が起動しないサンプルは非検出 */
        if (excid >= 0 && vg[excid] != excval) continue;
        /* 故障サイトに縮退値を注入し、コーンを再評価 */
        vf[fsig] = stuck;
        for (int t = 0; t < sim_ntopo; t++) {
            int i = sim_topo[t], ty = nl[i].type;
            if (ty == IN || ty == DFF || (size_t)i == fsig) continue;
            vf[i] = sim_gate(ty, vf[i], &nl[i], vf);
        }
        /* 観測点(n_out==0 かつ ppo_flag)のどれかで正常≠故障なら検出 */
        int diff = 0;
        for (int i = 0; i < n_net; i++)
            if (nl[i].n_out == 0 && nl[i].ppo_flag && vg[i] != vf[i]) { diff = 1; break; }
        if (diff) det++;
    }
    return (double)det / N;
}

/* ============================ BDD グラウンドトゥルース ============================ */
static DdManager* gt_mgr   = NULL;
static DdNode**   gt_good  = NULL;   /* 正常回路: net id -> BDD（一度だけ構築、常駐） */
static DdNode**   gt_fault = NULL;   /* 故障回路: TFOコーンのみ故障ごとに構築/解放 */
static int*       gt_piidx = NULL;   /* net id -> pi[] index（PIでなければ -1） */
static int*       gt_mark  = NULL;   /* TFOコーン所属フラグ */
static long gt_n=0, gt_unsound=0, gt_inexact=0;

/* Minato-Morreale ISOP 診断（env GT_ISOP=1）。
   Cudd_zddIsop は D_f の BDD から prime かつ irredundant な SOP をZDDで返す。
   minimum SOP ではないが、局所素項化(MAXDC)の列挙結果と「大域的に構成した被覆」の差を
   測るオラクルになる。L=U=D_f なので、返るBDDは必ず D_f と等価でなければならない。 */
static int    gt_isop_ready = 0;
static long   gt_isop_faults = 0, gt_isop_complete = 0, gt_isop_fail = 0;
static double gt_isop_enum_sum = 0.0, gt_isop_cover_sum = 0.0;

static void gt_isop_dump(void){
    if (!gt_isop_faults && !gt_isop_fail) return;
    fprintf(stderr,
        "[GT_ISOP] summary: faults=%ld complete=%ld failures=%ld complete_enum=%.0f isop=%.0f reduction=%.1f%%\n",
        gt_isop_faults, gt_isop_complete, gt_isop_fail,
        gt_isop_enum_sum, gt_isop_cover_sum,
        gt_isop_enum_sum > 0.0 ? 100.0 * (1.0 - gt_isop_cover_sum / gt_isop_enum_sum) : 0.0);
}

static void gt_isop_measure(DdManager* m, FNODE* f, CubeSet* cubes,
                            bool limit_hit, DdNode* det){
    if (!gt_isop_ready){
        /* ISOPはBDD変数iに対しZDD変数2i/2i+1（正/負リテラル）を使う。 */
        Cudd_zddRealignEnable(m);
        Cudd_AutodynDisableZdd(m);
        if (!Cudd_zddVarsFromBddVars(m, 2)){
            fprintf(stderr, "[GT_ISOP] ZDD variable initialization failed\n");
            gt_isop_fail++;
            return;
        }
        gt_isop_ready = 1;
        atexit(gt_isop_dump);
    }

    clock_t t0 = clock();
    DdNode* cover = NULL;
    DdNode* isop_bdd = Cudd_zddIsop(m, det, det, &cover);
    if (!isop_bdd || !cover){
        fprintf(stderr, "[GT_ISOP] %s,%s FAILED\n", f->name, FaultTypeName(f->type));
        gt_isop_fail++;
        return;
    }
    Cudd_Ref(isop_bdd);
    Cudd_Ref(cover);

    double n_isop = Cudd_zddCountDouble(m, cover);
    int exact = (isop_bdd == det);
    int zdd_nodes = Cudd_zddDagSize(cover);
    double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
    double ratio = n_isop > 0.0 ? (double)cubes->n / n_isop : 0.0;

    fprintf(stderr,
        "[GT_ISOP] %s,%s enum=%d complete=%d isop=%.0f enum/isop=%.2f zdd_nodes=%d det_nodes=%d exact=%d time=%.3f\n",
        f->name, FaultTypeName(f->type), cubes->n, !limit_hit, n_isop, ratio,
        zdd_nodes, Cudd_DagSize(det), exact, sec);

    gt_isop_faults++;
    if (!limit_hit){
        gt_isop_complete++;
        gt_isop_enum_sum += cubes->n;
        gt_isop_cover_sum += n_isop;
    }
    if (!exact) gt_isop_fail++;

    Cudd_RecursiveDerefZdd(m, cover);
    Cudd_RecursiveDeref(m, isop_bdd);
}

static void gt_dump(void){
    fprintf(stderr, "[GT] summary: checked=%ld  UNSOUND=%ld  complete-but-NOT-exact=%ld  %s\n",
        gt_n, gt_unsound, gt_inexact,
        (gt_unsound==0 && gt_inexact==0) ? "ALL VERIFIED" : "** MISMATCH **");
}

/* ゲート1個のBDD。入力 j の値は mark[j] が立っていれば fb[j]（故障側）、
   さもなくば gb[j]（正常側）。正常回路構築時は mark=NULL で gb のみ参照。 */
static DdNode* gt_gate_bdd(DdManager* m, NLIST* nd, DdNode** gb, DdNode** fb, const int* mark){
    #define GT_IN(k) ((mark && mark[nd->in[k]->n]) ? fb[nd->in[k]->n] : gb[nd->in[k]->n])
    DdNode *r, *t;
    switch (nd->type) {
        case BUF: case FOUT: r = GT_IN(0);          Cudd_Ref(r); return r;
        case INV:            r = Cudd_Not(GT_IN(0)); Cudd_Ref(r); return r;
        case GND:            r = Cudd_ReadLogicZero(m); Cudd_Ref(r); return r;
        case ACC:            r = Cudd_ReadOne(m);       Cudd_Ref(r); return r;
        case AND: case NAND:
            r = Cudd_ReadOne(m); Cudd_Ref(r);
            for (int k = 0; k < nd->n_in; k++) {
                t = Cudd_bddAnd(m, r, GT_IN(k)); Cudd_Ref(t);
                Cudd_RecursiveDeref(m, r); r = t;
            }
            if (nd->type == NAND) { t = Cudd_Not(r); Cudd_Ref(t); Cudd_RecursiveDeref(m, r); r = t; }
            return r;
        case OR: case NOR:
            r = Cudd_ReadLogicZero(m); Cudd_Ref(r);
            for (int k = 0; k < nd->n_in; k++) {
                t = Cudd_bddOr(m, r, GT_IN(k)); Cudd_Ref(t);
                Cudd_RecursiveDeref(m, r); r = t;
            }
            if (nd->type == NOR) { t = Cudd_Not(r); Cudd_Ref(t); Cudd_RecursiveDeref(m, r); r = t; }
            return r;
        case EXOR: case EXNOR:
            r = Cudd_ReadLogicZero(m); Cudd_Ref(r);
            for (int k = 0; k < nd->n_in; k++) {
                t = Cudd_bddXor(m, r, GT_IN(k)); Cudd_Ref(t);
                Cudd_RecursiveDeref(m, r); r = t;
            }
            if (nd->type == EXNOR) { t = Cudd_Not(r); Cudd_Ref(t); Cudd_RecursiveDeref(m, r); r = t; }
            return r;
        default:
            fprintf(stderr, "[GT] unsupported gate type %d (net %s)\n", nd->type, nd->name);
            exit(1);
    }
    #undef GT_IN
}

/* 初回のみ: 正常回路の全ネットBDDをトポロジカル順に構築する。
   BDD変数 i は pi[i]（cudd_wrapper.c の parseCube と同じ対応）。 */
static void gt_init(void){
    if (gt_mgr) return;
    sim_build_topo();
    gt_mgr   = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    Cudd_AutodynEnable(gt_mgr, CUDD_REORDER_SIFT);
    gt_good  = calloc(n_net, sizeof(DdNode*));
    gt_fault = calloc(n_net, sizeof(DdNode*));
    gt_mark  = calloc(n_net, sizeof(int));
    gt_piidx = malloc(n_net * sizeof(int));
    for (int i=0;i<n_net;i++) gt_piidx[i] = -1;
    for (int i=0;i<n_pi;i++)  gt_piidx[pi[i]->n] = i;
    for (int t=0;t<sim_ntopo;t++){
        int i = sim_topo[t];
        if (gt_piidx[i] >= 0){ gt_good[i] = Cudd_bddIthVar(gt_mgr, gt_piidx[i]); Cudd_Ref(gt_good[i]); }
        else if (nl[i].type==IN || nl[i].type==DFF){
            fprintf(stderr, "[GT] input-like net %s is not in pi[]\n", nl[i].name); exit(1);
        }
        else gt_good[i] = gt_gate_bdd(gt_mgr, &nl[i], gt_good, NULL, NULL);
    }
    /* env GT_NODYN=1: 正常回路の構築後は動的リオーダリングを凍結する。
       故障ごとのコーン再構築が数千回続くと sift が繰り返し走って支配的になるため
       （BDD_EXACT で大量の capped 故障を処理する場合の切り分け/高速化用）。 */
    if (getenv("GT_NODYN")) {
        Cudd_ReduceHeap(gt_mgr, CUDD_REORDER_SIFT, 0);   /* 最後に一度だけ整えて凍結 */
        Cudd_AutodynDisable(gt_mgr);
    }
    atexit(gt_dump);
}

/* キューブ和集合の BDD（変数対応はビット i ↔ pi[i]）。参照済みで返す。 */
static DdNode* gt_cube_bdd(DdManager* m, const char* s){
    DdNode* cb = Cudd_ReadOne(m); Cudd_Ref(cb);
    for (int i=0;i<n_pi;i++){
        DdNode* lit;
        if      (s[i]=='1') lit = Cudd_bddIthVar(m,i);
        else if (s[i]=='0') lit = Cudd_Not(Cudd_bddIthVar(m,i));
        else continue;
        DdNode* t = Cudd_bddAnd(m,cb,lit); Cudd_Ref(t); Cudd_RecursiveDeref(m,cb); cb=t;
    }
    return cb;
}

/* =====================================================================
 *  計測(env GT_GAIN=1): キューブ列挙の「多様性の逓減」を厳密に定量化する。
 *  各キューブ i の限界カバレッジ g_i = |∪_{1..i}| - |∪_{1..i-1}|（ミンターム数、
 *  厳密）を測り、列挙が進むほど新規カバーが減る現象を故障ごとに出力する。
 *  あわせて連続キューブのケアビット類似度（Jaccard: 両方ケアかつ同値 / どちらか
 *  がケア）を測る。支配流用の種が混ざると観察が濁るので MDC_NODOM=1 推奨。
 *    出力: [GAIN] 故障, cubes, k50/k90(累積50%/90%到達に要した本数),
 *           tail1%(限界寄与<1%の本数), jacc(平均類似度)
 *    終了時: [GAIN] summary（全故障集計）
 * ===================================================================== */
static long   gain_faults=0, gain_cubes=0, gain_tail1=0, gain_k90_sum=0, gain_k50_sum=0;
static double gain_jacc_sum=0;

static void gain_dump(void){
    if (!gain_faults) return;
    fprintf(stderr, "[GAIN] summary: faults=%ld  cubes/fault=%.1f  k50=%.1f  k90=%.1f  "
        "tail1%%=%.0f%%  consec_jaccard=%.3f\n",
        gain_faults, (double)gain_cubes/gain_faults,
        (double)gain_k50_sum/gain_faults, (double)gain_k90_sum/gain_faults,
        100.0*gain_tail1/gain_cubes, gain_jacc_sum/gain_faults);
}

/* 計測(env GT_CUBEDUMP): 故障のキューブ列を「先頭H本＋末尾H本」、各行に
   X 数(=ドントケア数)を付けて出力する。スライドの「最初はXが多く、最後は
   ケアビットが大半」を実物で確認するため（H は GT_CUBEDUMP_H、既定15）。
     GT_CUBEDUMP=<net>  指定故障のみ。
     GT_CUBEDUMP=AUTO   complete=1 かつ本数 >= GT_CUBEDUMP_MIN(既定100) の
                        故障を、最初に見つかった1件だけダンプして以降抑制。 */
static int gt_cubedump_done = 0;
static void gt_cubedump(FNODE* f, CubeSet* cubes, bool limit_hit){
    const char* want = getenv("GT_CUBEDUMP");
    if (!want) return;
    int n = cubes->n;
    if (strcmp(want, "AUTO") == 0){
        int mn = getenv("GT_CUBEDUMP_MIN") ? atoi(getenv("GT_CUBEDUMP_MIN")) : 100;
        if (gt_cubedump_done || limit_hit || n < mn) return;
        gt_cubedump_done = 1;   /* 完全列挙できた重い故障を1件だけ */
    } else if (strcmp(f->name, want) != 0) return;
    int H = getenv("GT_CUBEDUMP_H") ? atoi(getenv("GT_CUBEDUMP_H")) : 15;
    fprintf(stderr, "[CUBEDUMP] %s,%s  cubes=%d  (PI順, 各行末は X数/全%d)\n",
        f->name, FaultTypeName(f->type), n, n_pi);
    for (int c=0;c<n;c++){
        if (n > 2*H && c==H){ fprintf(stderr, "   ... (中略 %d本) ...\n", n-2*H); }
        if (n > 2*H && c>=H && c<n-H) continue;
        const char* s = cubes->data[c];
        int nx=0; for (int i=0;i<n_pi;i++) if (s[i]=='X') nx++;
        fprintf(stderr, "  #%-4d %s  X=%d\n", c, s, nx);
    }
}

static void gt_gain_measure(DdManager* m, FNODE* f, CubeSet* cubes, bool limit_hit){
    gt_cubedump(f, cubes, limit_hit);
    int n = cubes->n;
    if (n < 2) return;

    /* 限界カバレッジ列 */
    double* g = malloc(n*sizeof(double));
    DdNode* uni = Cudd_ReadLogicZero(m); Cudd_Ref(uni);
    double prev = 0.0;
    for (int c=0;c<n;c++){
        DdNode* cb = gt_cube_bdd(m, cubes->data[c]);
        DdNode* o = Cudd_bddOr(m,uni,cb); Cudd_Ref(o);
        Cudd_RecursiveDeref(m,uni); Cudd_RecursiveDeref(m,cb); uni=o;
        double cur = Cudd_CountMinterm(m,uni,n_pi);
        g[c] = cur - prev; prev = cur;
    }
    Cudd_RecursiveDeref(m,uni);
    double total = prev;
    if (total <= 0){ free(g); return; }

    /* k50/k90: 累積 50%/90% に達するまでの本数。tail1: 寄与 <1% の本数 */
    int k50=n, k90=n; long tail1=0; double acc=0;
    for (int c=0;c<n;c++){
        acc += g[c];
        if (k50==n && acc >= 0.50*total) k50=c+1;
        if (k90==n && acc >= 0.90*total) k90=c+1;
        if (g[c] < 0.01*total) tail1++;
    }

    /* 連続キューブのケア類似度（位置と値の Jaccard） */
    double jsum=0; int jcnt=0;
    for (int c=1;c<n;c++){
        const char *a=cubes->data[c-1], *b=cubes->data[c];
        int both=0, either=0;
        for (int i=0;i<n_pi;i++){
            int ca=(a[i]!='X'), cb2=(b[i]!='X');
            if (ca||cb2) either++;
            if (ca&&cb2&&a[i]==b[i]) both++;
        }
        if (either){ jsum += (double)both/either; jcnt++; }
    }
    double jacc = jcnt ? jsum/jcnt : 0;

    fprintf(stderr, "[GAIN] %s,%s,cubes=%d,k50=%d,k90=%d,tail1%%=%ld(%.0f%%),jacc=%.3f\n",
        f->name, FaultTypeName(f->type), n, k50, k90, tail1, 100.0*tail1/n, jacc);

    if (!gain_faults) atexit(gain_dump);
    gain_faults++; gain_cubes+=n; gain_tail1+=tail1; gain_k50_sum+=k50; gain_k90_sum+=k90;
    gain_jacc_sum+=jacc;
    free(g);
}

/* 検出関数 D_f を回路から直接構築して返す（参照済み。呼び出し側で deref）。
   GT_Check と BDD 直接法フォールバック（GT_ExactCountStr）が共用する。 */
static DdNode* gt_build_det(FNODE* f){
    gt_init();
    DdManager* m = gt_mgr;
    int fsig = (int)(f->netptr - nl);

    /* 故障サイトの TFO（推移的ファンアウト）コーンを out 辺で DFS 収集する */
    static int *stack = NULL, *cone = NULL;
    if (!stack) {
        stack = malloc(n_net * sizeof(int));
        cone  = malloc(n_net * sizeof(int));
    }
    int ncone = 0, sp = 0;
    stack[sp++] = fsig;
    gt_mark[fsig] = 1;
    while (sp) {
        int i = stack[--sp];
        cone[ncone++] = i;
        for (int k = 0; k < nl[i].n_out; k++) {
            int j = nl[i].out[k]->n;
            if (!gt_mark[j]) { gt_mark[j] = 1; stack[sp++] = j; }
        }
    }

    /* 故障回路: コーン内だけトポロジカル順に再構築（故障サイトは定数） */
    for (int t=0;t<sim_ntopo;t++){
        int i = sim_topo[t];
        if (!gt_mark[i]) continue;
        if (i==fsig){ gt_fault[i] = (f->type==SF0)?Cudd_ReadLogicZero(m):Cudd_ReadOne(m); Cudd_Ref(gt_fault[i]); }
        else gt_fault[i] = gt_gate_bdd(m, &nl[i], gt_good, gt_fault, gt_mark);
    }

    /* D_f = OR_{観測点∈コーン} (good XOR faulty)。コーン外は差分0。
       観測点 = n_out==0 かつ ppo_flag（SAF=全PO、TDF=2時刻目PPOのみ） */
    DdNode* det = Cudd_ReadLogicZero(m); Cudd_Ref(det);
    for (int t=0;t<ncone;t++){
        int i = cone[t];
        if (nl[i].n_out != 0 || !nl[i].ppo_flag) continue;
        DdNode* d = Cudd_bddXor(m, gt_good[i], gt_fault[i]); Cudd_Ref(d);
        DdNode* o = Cudd_bddOr(m, det, d); Cudd_Ref(o);
        Cudd_RecursiveDeref(m,det); Cudd_RecursiveDeref(m,d); det=o;
    }

    /* TDF: 励起条件（1時刻目コピーの正常値 = 初期値）を AND する。
       STR(SF0扱い)は1時刻目=0、STF(SF1扱い)は1時刻目=1 */
    if (f->exc_netptr){
        DdNode* e = gt_good[f->exc_netptr->n];
        if (f->type == SF0) e = Cudd_Not(e);
        DdNode* t2 = Cudd_bddAnd(m, det, e); Cudd_Ref(t2);
        Cudd_RecursiveDeref(m, det); det = t2;
    }

    /* このコーンの故障BDDを解放し、mark をリセット（det は自前の参照を持つ） */
    for (int t = 0; t < ncone; t++) {
        int i = cone[t];
        Cudd_RecursiveDeref(m, gt_fault[i]);
        gt_fault[i] = NULL;
        gt_mark[i]  = 0;
    }
    return det;
}

/* 恒久機能(env BDD_EXACT=1 から使用): D_f のミンターム数を10進文字列で返す
   （malloc 済み、呼び出し側で free）。fdp = 返値/2^n_pi が厳密値。 */
char* GT_ExactCountStr(FNODE* f){
    DdNode* det = gt_build_det(f);
    int digits;
    DdApaNumber count = Cudd_ApaCountMinterm(gt_mgr, det, n_pi, &digits);
    Cudd_RecursiveDeref(gt_mgr, det);
    if (!count) return NULL;

    FILE* tmp = tmpfile();
    if (!tmp) { free(count); return NULL; }
    Cudd_ApaPrintDecimal(tmp, digits, count);
    rewind(tmp);
    char buf[8192];
    if (!fgets(buf, sizeof(buf), tmp)) strcpy(buf, "0");
    fclose(tmp);
    free(count);
    return strdup(buf);
}

void GT_Check(FNODE* f, CubeSet* cubes, bool limit_hit){
    if (getenv("GT_GAIN")){ gt_init(); gt_gain_measure(gt_mgr, f, cubes, limit_hit); }
    int want_bdd = getenv("GT_BDD") != NULL;
    int want_isop = getenv("GT_ISOP") != NULL;
    if (!want_bdd && !want_isop) return;
    DdNode* det = gt_build_det(f);   /* gt_init もここで済む */
    DdManager* m = gt_mgr;

    if (want_isop) gt_isop_measure(m, f, cubes, limit_hit, det);
    if (!want_bdd){
        Cudd_RecursiveDeref(m, det);
        return;
    }

    /* キューブ和集合 */
    DdNode* uni = Cudd_ReadLogicZero(m); Cudd_Ref(uni);
    for (int c=0;c<cubes->n;c++){
        DdNode* cb = gt_cube_bdd(m, cubes->data[c]);
        DdNode* o = Cudd_bddOr(m,uni,cb); Cudd_Ref(o);
        Cudd_RecursiveDeref(m,uni); Cudd_RecursiveDeref(m,cb); uni=o;
    }

    int sound = Cudd_bddLeq(m, uni, det);   /* uni ⇒ det */
    int exact = (uni == det);               /* 同一マネージャ内なので完全等価 ⇔ 同一ノード */

    /* 診断(env GT_COVER=1): D_f の「冗長度」を測る。生成キューブ数に対し、
       D_f の BDD ノード数・1-パス数(=BDD が誘導する disjoint カバーのサイズ)を並べる。
       paths << cubes なら「ほぼ素項なのに重複した冗長カバーを量産」が爆発の主因と確定する。 */
    if (getenv("GT_COVER")){
        double paths = Cudd_CountPath(det);
        int    nodes = Cudd_DagSize(det);
        double pd = ldexp(Cudd_CountMinterm(m,det,n_pi), -n_pi);
        fprintf(stderr, "[GT_COVER] %s,%s cubes=%d det_paths=%.0f det_nodes=%d fdp=%.6f\n",
            f->name, FaultTypeName(f->type), cubes->n, paths, nodes, pd);
    }

    /* 診断(env GT_CUBES=1): 非健全キューブを特定し、X のうち「どれか1ビットを
       固定すれば健全になる」候補（XIDが誤ってXにした必要PI）を列挙する */
    if (!sound && getenv("GT_CUBES")){
        for (int c=0;c<cubes->n;c++){
            const char* s = cubes->data[c];
            DdNode* cb = gt_cube_bdd(m, s);
            if (!Cudd_bddLeq(m, cb, det)){
                fprintf(stderr, "[GT_CUBE] %s,%s cube#%d UNSOUND: %s\n",
                    f->name, FaultTypeName(f->type), c, s);
                for (int i=0;i<n_pi;i++){
                    if (s[i]!='X') continue;
                    DdNode* v = Cudd_bddIthVar(m,i);
                    DdNode* c1 = Cudd_bddAnd(m,cb,v);            Cudd_Ref(c1);
                    DdNode* c0 = Cudd_bddAnd(m,cb,Cudd_Not(v));  Cudd_Ref(c0);
                    if (Cudd_bddLeq(m,c1,det)) fprintf(stderr, "[GT_CUBE]   fix %s=1 would be sound\n", pi[i]->name);
                    if (Cudd_bddLeq(m,c0,det)) fprintf(stderr, "[GT_CUBE]   fix %s=0 would be sound\n", pi[i]->name);
                    Cudd_RecursiveDeref(m,c1); Cudd_RecursiveDeref(m,c0);
                }
            }
            Cudd_RecursiveDeref(m,cb);
        }
    }

    int complete = !limit_hit;
    gt_n++;
    if (!sound) gt_unsound++;
    if (complete && !exact) gt_inexact++;
    /* 完全列挙なのに不一致 / 健全性違反のときだけ詳細を出す（OK行は GT_VERBOSE で）。
       不一致時は独立シミュレーションの値も併記する（GT自体の三重照合）。 */
    int mismatch = !sound || (complete && !exact);
    if (mismatch || getenv("GT_VERBOSE")){
        double pu = ldexp(Cudd_CountMinterm(m,uni,n_pi), -n_pi);
        double pd = ldexp(Cudd_CountMinterm(m,det,n_pi), -n_pi);
        fprintf(stderr, "[GT] %s,%s,cubes=%d,complete=%d,sound=%d,exact=%d,fdp_cube=%.10e,fdp_true=%.10e",
            f->name, FaultTypeName(f->type), cubes->n, complete, sound, exact, pu, pd);
        if (mismatch) fprintf(stderr, ",fdp_sim=%.6f", FdpBySim(f, 200000));
        fputc('\n', stderr);
    }

    /* 後始末（コーンの故障BDDは gt_build_det が解放済み） */
    Cudd_RecursiveDeref(m, det);
    Cudd_RecursiveDeref(m, uni);
}

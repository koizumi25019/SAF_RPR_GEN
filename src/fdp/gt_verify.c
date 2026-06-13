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
//	不一致時は独立モンテカルロシミュレーション(FdpBySim)の値も併記する（三重照合）。
//	検証記録: verification/gt_bdd/SUMMARY.md
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cudd.h>

#include "./gt_verify.h"
#include "../netlist/netlist.h"

/* ============== 独立2値シミュレーション（GT自体の三重照合用） ============== */
static int* sim_topo = NULL; static int sim_ntopo = 0;
static void sim_build_topo(void){
    if (sim_topo) return;
    sim_topo = (int*)malloc(n_net*sizeof(int));
    int maxlev=0; for(int i=0;i<n_net;i++) if(nl[i].level>maxlev) maxlev=nl[i].level;
    int idx=0;
    for(int L=0;L<=maxlev;L++) for(int i=0;i<n_net;i++) if(nl[i].level==L) sim_topo[idx++]=i;
    sim_ntopo=idx;
}
static inline int sim_gate(int type, int acc0, NLIST* nd, const int* val){
    switch(type){
        case BUF: case FOUT: return val[nd->in[0]->n];
        case INV: return val[nd->in[0]->n]^1;
        case AND: case NAND: { int a=1; for(int k=0;k<nd->n_in;k++){a&=val[nd->in[k]->n]; if(!a)break;} return (type==NAND)?a^1:a; }
        case OR:  case NOR:  { int a=0; for(int k=0;k<nd->n_in;k++){a|=val[nd->in[k]->n]; if(a)break;} return (type==NOR)?a^1:a; }
        case EXOR:case EXNOR:{ int a=0; for(int k=0;k<nd->n_in;k++)a^=val[nd->in[k]->n]; return (type==EXNOR)?a^1:a; }
        default: return acc0; /* IN/DFF: keep PI value */
    }
}

/* モンテカルロ真値FDP（SAT/BDD不使用の独立実装）。N回サンプリング。 */
static double FdpBySim(FNODE* f, long N){
    sim_build_topo();
    static int *vg=NULL,*vf=NULL; if(!vg){vg=malloc(n_net*sizeof(int)); vf=malloc(n_net*sizeof(int));}
    size_t fsig=(size_t)(f->netptr - nl); int stuck=(f->type==SF0)?0:1;
    long det=0;
    for(long s=0;s<N;s++){
        for(int i=0;i<n_pi;i++){ int b=rand()&1; vg[pi[i]->n]=b; vf[pi[i]->n]=b; }
        for(int t=0;t<sim_ntopo;t++){ int i=sim_topo[t]; int ty=nl[i].type;
            if(ty==IN||ty==DFF) continue;
            vg[i]=sim_gate(ty,vg[i],&nl[i],vg);
            vf[i]=sim_gate(ty,vf[i],&nl[i],vf);
        }
        vf[fsig]=stuck;                                   /* inject fault */
        for(int t=0;t<sim_ntopo;t++){ int i=sim_topo[t]; int ty=nl[i].type;
            if(ty==IN||ty==DFF||(size_t)i==fsig) continue;
            vf[i]=sim_gate(ty,vf[i],&nl[i],vf);
        }
        int diff=0; for(int i=0;i<n_net;i++) if(nl[i].n_out==0 && vg[i]!=vf[i]){diff=1;break;}
        if(diff) det++;
    }
    return (double)det/N;
}

/* ============================ BDD グラウンドトゥルース ============================ */
static DdManager* gt_mgr   = NULL;
static DdNode**   gt_good  = NULL;   /* 正常回路: net id -> BDD（一度だけ構築、常駐） */
static DdNode**   gt_fault = NULL;   /* 故障回路: TFOコーンのみ故障ごとに構築/解放 */
static int*       gt_piidx = NULL;   /* net id -> pi[] index（PIでなければ -1） */
static int*       gt_mark  = NULL;   /* TFOコーン所属フラグ */
static long gt_n=0, gt_unsound=0, gt_inexact=0;

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
        case BUF: case FOUT: r = GT_IN(0); Cudd_Ref(r); return r;
        case INV:            r = Cudd_Not(GT_IN(0)); Cudd_Ref(r); return r;
        case GND:            r = Cudd_ReadLogicZero(m); Cudd_Ref(r); return r;
        case ACC:            r = Cudd_ReadOne(m); Cudd_Ref(r); return r;
        case AND: case NAND:
            r = Cudd_ReadOne(m); Cudd_Ref(r);
            for (int k=0;k<nd->n_in;k++){ t=Cudd_bddAnd(m,r,GT_IN(k)); Cudd_Ref(t); Cudd_RecursiveDeref(m,r); r=t; }
            if (nd->type==NAND){ t=Cudd_Not(r); Cudd_Ref(t); Cudd_RecursiveDeref(m,r); r=t; }
            return r;
        case OR: case NOR:
            r = Cudd_ReadLogicZero(m); Cudd_Ref(r);
            for (int k=0;k<nd->n_in;k++){ t=Cudd_bddOr(m,r,GT_IN(k)); Cudd_Ref(t); Cudd_RecursiveDeref(m,r); r=t; }
            if (nd->type==NOR){ t=Cudd_Not(r); Cudd_Ref(t); Cudd_RecursiveDeref(m,r); r=t; }
            return r;
        case EXOR: case EXNOR:
            r = Cudd_ReadLogicZero(m); Cudd_Ref(r);
            for (int k=0;k<nd->n_in;k++){ t=Cudd_bddXor(m,r,GT_IN(k)); Cudd_Ref(t); Cudd_RecursiveDeref(m,r); r=t; }
            if (nd->type==EXNOR){ t=Cudd_Not(r); Cudd_Ref(t); Cudd_RecursiveDeref(m,r); r=t; }
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

static void gt_gain_measure(DdManager* m, FNODE* f, CubeSet* cubes){
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
        f->name, (f->type==SF0)?"sa0":"sa1", n, k50, k90, tail1, 100.0*tail1/n, jacc);

    if (!gain_faults) atexit(gain_dump);
    gain_faults++; gain_cubes+=n; gain_tail1+=tail1; gain_k50_sum+=k50; gain_k90_sum+=k90;
    gain_jacc_sum+=jacc;
    free(g);
}

void GT_Check(FNODE* f, CubeSet* cubes, bool limit_hit){
    if (getenv("GT_GAIN")){ gt_init(); gt_gain_measure(gt_mgr, f, cubes); }
    if (!getenv("GT_BDD")) return;
    gt_init();
    DdManager* m = gt_mgr;
    int fsig = (int)(f->netptr - nl);

    /* 故障サイトのTFOコーンを out 辺で収集 */
    static int *stack=NULL, *cone=NULL;
    if (!stack){ stack=malloc(n_net*sizeof(int)); cone=malloc(n_net*sizeof(int)); }
    int ncone=0, sp=0;
    stack[sp++]=fsig; gt_mark[fsig]=1;
    while (sp){
        int i = stack[--sp]; cone[ncone++]=i;
        for (int k=0;k<nl[i].n_out;k++){
            int j = nl[i].out[k]->n;
            if (!gt_mark[j]){ gt_mark[j]=1; stack[sp++]=j; }
        }
    }

    /* 故障回路: コーン内だけトポロジカル順に再構築（故障サイトは定数） */
    for (int t=0;t<sim_ntopo;t++){
        int i = sim_topo[t];
        if (!gt_mark[i]) continue;
        if (i==fsig){ gt_fault[i] = (f->type==SF0)?Cudd_ReadLogicZero(m):Cudd_ReadOne(m); Cudd_Ref(gt_fault[i]); }
        else gt_fault[i] = gt_gate_bdd(m, &nl[i], gt_good, gt_fault, gt_mark);
    }

    /* D_f = OR_{PO∈コーン} (good XOR faulty)。コーン外POは差分0 */
    DdNode* det = Cudd_ReadLogicZero(m); Cudd_Ref(det);
    for (int t=0;t<ncone;t++){
        int i = cone[t];
        if (nl[i].n_out != 0) continue;
        DdNode* d = Cudd_bddXor(m, gt_good[i], gt_fault[i]); Cudd_Ref(d);
        DdNode* o = Cudd_bddOr(m, det, d); Cudd_Ref(o);
        Cudd_RecursiveDeref(m,det); Cudd_RecursiveDeref(m,d); det=o;
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

    /* 診断(env GT_CUBES=1): 非健全キューブを特定し、X のうち「どれか1ビットを
       固定すれば健全になる」候補（XIDが誤ってXにした必要PI）を列挙する */
    if (!sound && getenv("GT_CUBES")){
        for (int c=0;c<cubes->n;c++){
            const char* s = cubes->data[c];
            DdNode* cb = gt_cube_bdd(m, s);
            if (!Cudd_bddLeq(m, cb, det)){
                fprintf(stderr, "[GT_CUBE] %s,%s cube#%d UNSOUND: %s\n",
                    f->name, (f->type==SF0)?"sa0":"sa1", c, s);
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
            f->name, (f->type==SF0)?"sa0":"sa1", cubes->n, complete, sound, exact, pu, pd);
        if (mismatch) fprintf(stderr, ",fdp_sim=%.6f", FdpBySim(f, 200000));
        fputc('\n', stderr);
    }

    Cudd_RecursiveDeref(m,det); Cudd_RecursiveDeref(m,uni);
    for (int t=0;t<ncone;t++){ int i=cone[t]; Cudd_RecursiveDeref(m,gt_fault[i]); gt_fault[i]=NULL; gt_mark[i]=0; }
}

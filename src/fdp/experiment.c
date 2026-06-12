//-------------------------------------------------------------------------------------------------------------
//	experiment.c : 研究用フック（環境変数で制御、デフォルト無効）
//
//	案1 (env MAXDC): 「非検出オラクル」CNF（正常回路 ∧ 故障コーン ∧ サイト縮退
//	  ∧ 全PO一致=z0）を構築し、各 XID キューブのケアビットを
//	  「(cube\b) ∧ 非検出 が UNSAT のまま」である限り貪欲に落として素項へ拡大する。
//	  追加 env: MAXDC_CORE(UNSATコア一括法), MAXDC_NOMUT(計測のみでキューブ不変)。
//	  終了時に [MAXDC] 集計を stderr に出す。
//	案2 (env MAXHAM): 連続キューブのケア領域の値反転が少ない問題に対し、
//	  「前回キューブと >=k ビット違う」制約(Sinz at-most)を活性化リテラルで
//	  ガードして優先 solve する。終了判定は素 solve なので完全性は不変。
//	  追加 env: MAXHAM_K(目標k; 既定 max(2, m/3))。
//
//	実験の評価と結論（案2は却下、案1は要改善）は verification/SUMMARY.md を参照。
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./experiment.h"
#include "./create_TPG_model.h"
#include "./read.h"
#include "./cnf/cnf.h"
#include "../netlist/netlist.h"

/* ================================ 案1: MAXDC ================================ */
static long mdc_cubes=0, mdc_orig=0, mdc_prime=0, mdc_hr_cubes=0, mdc_sanity_fail=0, mdc_revert=0;
static long mdc_hardcubes=0, mdc_hardorig=0, mdc_hardprime=0;   /* capped faults only */
static long mdc_fcubes=0, mdc_forig=0, mdc_fprime=0;            /* per-fault accumulator */

static void mdc_dump(void){
    fprintf(stderr,"\n[MAXDC] cubes=%ld  care/cube: orig=%.2f prime=%.2f  headroom=%.2f bits/cube (%.0f%% of cubes shrink)  sanity_fail=%ld\n",
        mdc_cubes, mdc_cubes?(double)mdc_orig/mdc_cubes:0, mdc_cubes?(double)mdc_prime/mdc_cubes:0,
        mdc_cubes?(double)(mdc_orig-mdc_prime)/mdc_cubes:0, mdc_cubes?100.0*mdc_hr_cubes/mdc_cubes:0, mdc_sanity_fail);
    fprintf(stderr,"[MAXDC] reverts (unsafe expansions caught) = %ld\n", mdc_revert);
    fprintf(stderr,"[MAXDC] capped-fault cubes=%ld  orig=%.2f prime=%.2f headroom=%.2f bits/cube\n",
        mdc_hardcubes, mdc_hardcubes?(double)mdc_hardorig/mdc_hardcubes:0,
        mdc_hardcubes?(double)mdc_hardprime/mdc_hardcubes:0,
        mdc_hardcubes?(double)(mdc_hardorig-mdc_hardprime)/mdc_hardcubes:0);
}

/* 非検出オラクルを構築する（WriteTPGModel 直後に呼ぶこと：
   TFOフラグ/varsfc/numtranpo が当該故障用に設定済みである必要がある） */
static void mdc_build_oracle(CCaDiCaL* u, TARGET* target){
    FNODE* f = target->list[0];
    LoadModelToSolver(u, target);                 /* good circuit (varsgc) */
    for (int j=0;j<n_net;j++){                    /* faulty cone gates (varsfc) */
        if (((nl[j].flag & TFO)==TFO) && ((nl[j].flag & FP)!=FP)){
            switch(nl[j].type){
                case AND:  CreateConsFC_AND (u,&nl[j]); break;
                case NAND: CreateConsFC_NAND(u,&nl[j]); break;
                case OR:   CreateConsFC_OR  (u,&nl[j]); break;
                case NOR:  CreateConsFC_NOR (u,&nl[j]); break;
                case INV:  CreateConsFC_INV (u,&nl[j]); break;
                case BUF:
                case FOUT: CreateConsFC_BUF (u,&nl[j]); break;
                case EXOR: CreateConsFC_XOR (u,&nl[j]); break;
                case EXNOR:CreateConsFC_XNOR(u,&nl[j]); break;
                default: break;
            }
        }
    }
    int fc = f->netptr->varsfc;                   /* fault site stuck value */
    if (f->type==SF0){ ccadical_add(u,-fc); ccadical_add(u,0); }
    else             { ccadical_add(u, fc); ccadical_add(u,0); }
    CreateConsDC_XOR(u);                           /* per-PO diff = gc XOR fc */
    CreateConsDC_OR(u);                            /* z = OR diffs */
    int z = cnf.total.vars;
    ccadical_add(u,-z); ccadical_add(u,0);         /* z=0 : undetection (no PO differs) */
}

CCaDiCaL* EXP_MaybeBuildOracle(TARGET* target){
    if (!getenv("MAXDC")) return NULL;
    if (mdc_cubes==0 && mdc_fcubes==0) atexit(mdc_dump);
    CCaDiCaL* u = ccadical_init();
    ccadical_set_option(u, "factor", 0);
    mdc_build_oracle(u, target);
    mdc_fcubes=0; mdc_forig=0; mdc_fprime=0;
    return u;
}

static inline int mdc_lit(const char* cube, int i){
    return (cube[i]=='1') ? (int)pi[i]->varsgc : -(int)pi[i]->varsgc;
}

/* キューブを素項へ拡大（in place、ケアビット -> 'X'）。
   既定: sound な per-bit 貪欲法（b を抜いても非検出が UNSAT のままなら落とす）。
   MAXDC_CORE: UNSATコア一括法（コア外を一括で落とし、検証+revert）。 */
void EXP_Expand(CCaDiCaL* u, char* cube){
    if (!u) return;
    static int* care=NULL; static char* save=NULL; static int cap=0;
    if (cap<n_pi){ care=realloc(care,n_pi*sizeof(int)); save=realloc(save,n_pi+1); cap=n_pi; }
    int nc=0; for (int i=0;i<n_pi;i++) if (cube[i]!='X') care[nc++]=i;
    if (nc==0) return;
    memcpy(save, cube, n_pi+1);       /* keep original to revert if needed */
    int orig=nc;

    if (getenv("MAXDC_CORE")) {
        /* cheap one-shot: keep only the unsat core, then verify+revert */
        for (int k=0;k<nc;k++){ int i=care[k]; ccadical_assume(u,mdc_lit(cube,i)); }
        if (ccadical_solve(u)==20){
            for (int k=0;k<nc;k++){ int i=care[k]; if (!ccadical_failed(u,mdc_lit(cube,i))) cube[i]='X'; }
            int live=0; for (int k=0;k<nc;k++){ int i=care[k]; if (cube[i]!='X'){ ccadical_assume(u,mdc_lit(cube,i)); live++; } }
            if (live<orig && ccadical_solve(u)!=20){ memcpy(cube,save,n_pi+1); mdc_revert++; }
        } else mdc_sanity_fail++;
    } else {
        /* sound greedy: drop bit b only if (cube\b) still implies detection (UNSAT) */
        /* first confirm the full cube implies detection at all */
        for (int k=0;k<nc;k++){ int i=care[k]; ccadical_assume(u,mdc_lit(cube,i)); }
        if (ccadical_solve(u)!=20){ mdc_sanity_fail++; }
        else {
            for (int b=0;b<nc;b++){
                int ib=care[b];
                for (int k=0;k<nc;k++){ int i=care[k]; if (i==ib||cube[i]=='X') continue; ccadical_assume(u,mdc_lit(cube,i)); }
                if (ccadical_solve(u)==20) cube[ib]='X';   /* still UNSAT without b -> drop */
            }
        }
    }

    if (getenv("MAXDC_NOMUT")) memcpy(cube, save, n_pi+1);  /* diagnostic: measure but don't change cube */

    int prime=0; for (int k=0;k<nc;k++) if (cube[care[k]]!='X') prime++;
    mdc_cubes++; mdc_orig+=orig; mdc_prime+=prime; if (prime<orig) mdc_hr_cubes++;
    mdc_fcubes++; mdc_forig+=orig; mdc_fprime+=prime;
}

void EXP_OracleDone(CCaDiCaL** oracle, bool limit_hit){
    if (!*oracle) return;
    if (limit_hit) { mdc_hardcubes+=mdc_fcubes; mdc_hardorig+=mdc_forig; mdc_hardprime+=mdc_fprime; }
    ccadical_release(*oracle);
    *oracle = NULL;
}

/* ================================ 案2: MAXHAM ================================
 * 各 solve 前に「前回キューブのケア領域と >=k ビット違う」制約を活性化リテラル
 * act でガードし assume(act) でその回だけ有効化する。
 *   完全性: 全節を (-act ∨ ...) でガードし act は恒久 assert しない。よって
 *   正当な検出を恒久的に消すことは不可能。多様性 UNSAT 時は k を下げ、最後の
 *   「多様性なしの素 solve」が UNSAT のときだけ故障完了とみなす。
 *   距離 >=k は at-most-(m-k) (Sinz 逐次カウンタ) でエンコード。
 */
static int maxham_aux = 0;   /* 故障ごとに cnf.total.vars+1 で初期化する aux 採番器 */

void EXP_ResetPerFault(void){
    maxham_aux = cnf.total.vars + 1;
}

/* e[0..m-1] の真を高々 R 個に制限する制約を act でガードして追加 (Sinz LT_SEQ) */
static void maxham_atmost(CCaDiCaL* s, int* e, int m, int R, int act)
{
    if (R >= m) return;                 /* 自明に充足 */
    if (R == 0) {                       /* 全 e_i = false */
        for (int i = 0; i < m; i++) { ccadical_add(s,-act); ccadical_add(s,-e[i]); ccadical_add(s,0); }
        return;
    }
    int base = maxham_aux; maxham_aux += m * R;   /* SV(i,j)=base + i*R + (j-1), 1<=j<=R */
    #define SV(i,j) (base + (i)*R + ((j)-1))
    ccadical_add(s,-act); ccadical_add(s,-e[0]); ccadical_add(s,SV(0,1)); ccadical_add(s,0);
    for (int j = 2; j <= R; j++) { ccadical_add(s,-act); ccadical_add(s,-SV(0,j)); ccadical_add(s,0); }
    for (int i = 1; i < m; i++) {
        ccadical_add(s,-act); ccadical_add(s,-e[i]);      ccadical_add(s,SV(i,1)); ccadical_add(s,0);
        ccadical_add(s,-act); ccadical_add(s,-SV(i-1,1)); ccadical_add(s,SV(i,1)); ccadical_add(s,0);
        for (int j = 2; j <= R; j++) {
            ccadical_add(s,-act); ccadical_add(s,-e[i]); ccadical_add(s,-SV(i-1,j-1)); ccadical_add(s,SV(i,j)); ccadical_add(s,0);
            ccadical_add(s,-act); ccadical_add(s,-SV(i-1,j)); ccadical_add(s,SV(i,j)); ccadical_add(s,0);
        }
        ccadical_add(s,-act); ccadical_add(s,-e[i]); ccadical_add(s,-SV(i-1,R)); ccadical_add(s,0);
    }
    #undef SV
}

/* 多様性付き solve。prev のケア領域と離れた解を優先しつつ、終了判定は素 solve。 */
int EXP_Solve(CCaDiCaL* s, const char* prev)
{
    if (!getenv("MAXHAM") || !prev) return ccadical_solve(s);

    static int* e = NULL; static int cap = 0;
    if (cap < n_pi) { e = realloc(e, n_pi * sizeof(int)); cap = n_pi; }
    int m = 0;
    for (int p = 0; p < n_pi; p++) {           /* e_i = 「prev と同じ値」リテラル */
        if      (prev[p] == '1') e[m++] =  (int)pi[p]->varsgc;
        else if (prev[p] == '0') e[m++] = -(int)pi[p]->varsgc;
    }
    if (m == 0) return ccadical_solve(s);

    int K = getenv("MAXHAM_K") ? atoi(getenv("MAXHAM_K")) : (m/3 > 2 ? m/3 : 2);
    if (K > m) K = m;

    for (int k = K; k >= 2; k /= 2) {          /* 幾何降下: K, K/2, ... ,2 */
        int act = maxham_aux++;
        maxham_atmost(s, e, m, m - k, act);    /* >=k 違う = 同じが高々 m-k */
        ccadical_assume(s, act);
        if (ccadical_solve(s) == 10) return 10;
        /* UNSAT: act は二度と assume しない -> 当該節は無効化 */
    }
    return ccadical_solve(s);                   /* 多様性なしの素 solve(終了判定の根拠) */
}

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
#include "./fault_detection_prob.h"   /* numtranpo */
#include "./read.h"
#include "./cnf/cnf.h"
#include "../netlist/netlist.h"

/* ================================ 案1: MAXDC ================================ */
static long mdc_cubes=0, mdc_orig=0, mdc_prime=0, mdc_hr_cubes=0, mdc_sanity_fail=0, mdc_revert=0;
static long mdc_hardcubes=0, mdc_hardorig=0, mdc_hardprime=0;   /* capped faults only */
static long mdc_fcubes=0, mdc_forig=0, mdc_fprime=0;            /* per-fault accumulator */

/* 0除算を避ける平均（分母0なら0） */
static double mdc_avg(double num, long den){ return den ? num / den : 0.0; }

static void mdc_dump(void){
    fprintf(stderr,
        "\n[MAXDC] cubes=%ld  care/cube: orig=%.2f prime=%.2f  "
        "headroom=%.2f bits/cube (%.0f%% of cubes shrink)  sanity_fail=%ld\n",
        mdc_cubes,
        mdc_avg(mdc_orig,  mdc_cubes),
        mdc_avg(mdc_prime, mdc_cubes),
        mdc_avg(mdc_orig - mdc_prime, mdc_cubes),
        100.0 * mdc_avg(mdc_hr_cubes, mdc_cubes),
        mdc_sanity_fail);
    fprintf(stderr, "[MAXDC] reverts (unsafe expansions caught) = %ld\n", mdc_revert);
    fprintf(stderr,
        "[MAXDC] capped-fault cubes=%ld  orig=%.2f prime=%.2f headroom=%.2f bits/cube\n",
        mdc_hardcubes,
        mdc_avg(mdc_hardorig,  mdc_hardcubes),
        mdc_avg(mdc_hardprime, mdc_hardcubes),
        mdc_avg(mdc_hardorig - mdc_hardprime, mdc_hardcubes));
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

/* DIVPO 用: WriteTPGModel 直後の最終変数番号(=検出フラグ z)と伝播PO数を捕捉。
   diff変数(各POの gc XOR fc)は z-numtranpo .. z-1 の連番（detection_circuit.c）。
   本関数は MAXDC 不使用でも main から毎故障呼ばれるため、ここで捕捉する。 */
static int divpo_z = 0, divpo_n = 0;
static int* divpo_netid = NULL;            /* p 番目の diff 変数に対応する PO の net id */

CCaDiCaL* EXP_MaybeBuildOracle(TARGET* target){
    divpo_z = cnf.total.vars;
    divpo_n = numtranpo;
    if (getenv("DIVPO") && divpo_n > 0){
        /* diff変数は CreateConsDC_XOR が nl[] を昇順走査して TPO ネットに
           割り当てた連番。同じ順で走査して p 番目 -> net id の対応を作る */
        static int netid_cap = 0;
        if (netid_cap < divpo_n){ divpo_netid = realloc(divpo_netid, divpo_n*sizeof(int)); netid_cap = divpo_n; }
        int k = 0;
        for (int i = 0; i < n_net && k < divpo_n; i++)
            if ((nl[i].flag & TPO) == TPO) divpo_netid[k++] = i;
    }
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
    int nc = 0;
    for (int i = 0; i < n_pi; i++)
        if (cube[i] != 'X') care[nc++] = i;
    if (nc == 0) return;
    memcpy(save, cube, n_pi+1);       /* keep original to revert if needed */
    int orig = nc;

    if (getenv("MAXDC_CORE")) {
        /* cheap one-shot: keep only the unsat core, then verify+revert */
        for (int k = 0; k < nc; k++)
            ccadical_assume(u, mdc_lit(cube, care[k]));

        if (ccadical_solve(u) == 20) {
            /* コア外（failed でない）ビットは検出に不要なので X に落とす */
            for (int k = 0; k < nc; k++) {
                int i = care[k];
                if (!ccadical_failed(u, mdc_lit(cube, i))) cube[i] = 'X';
            }
            /* 念のため：残ったケアだけで本当に非検出が UNSAT か再確認し、駄目なら戻す */
            int live = 0;
            for (int k = 0; k < nc; k++) {
                int i = care[k];
                if (cube[i] != 'X') { ccadical_assume(u, mdc_lit(cube, i)); live++; }
            }
            if (live < orig && ccadical_solve(u) != 20) { memcpy(cube, save, n_pi+1); mdc_revert++; }
        } else {
            mdc_sanity_fail++;
        }
    } else {
        /* sound greedy: ビット b を抜いても (cube\b) が検出を含意（非検出が UNSAT）なら落とす。
           まず完全キューブが検出を含意することを確認する。 */
        for (int k = 0; k < nc; k++)
            ccadical_assume(u, mdc_lit(cube, care[k]));

        if (ccadical_solve(u) != 20) {
            mdc_sanity_fail++;
        } else {
            for (int b = 0; b < nc; b++) {
                int ib = care[b];
                /* ib 以外の生きているケアビットだけを assume */
                for (int k = 0; k < nc; k++) {
                    int i = care[k];
                    if (i == ib || cube[i] == 'X') continue;
                    ccadical_assume(u, mdc_lit(cube, i));
                }
                if (ccadical_solve(u) == 20) cube[ib] = 'X';   /* b 無しでも UNSAT → 落とす */
            }
        }
    }

    if (getenv("MAXDC_NOMUT")) memcpy(cube, save, n_pi+1);  /* diagnostic: measure but don't change cube */

    int prime = 0;
    for (int k = 0; k < nc; k++)
        if (cube[care[k]] != 'X') prime++;

    /* 全体集計 */
    mdc_cubes++; mdc_orig += orig; mdc_prime += prime;
    if (prime < orig) mdc_hr_cubes++;
    /* この故障ぶんの集計（capped 判定で hardcubes へ繰り入れる） */
    mdc_fcubes++; mdc_forig += orig; mdc_fprime += prime;
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

/* ===================== 多様化案A (env DIVPO=1): 検出PO指定 =====================
 * 検出条件は z = OR(diff_po) で「どのPOで検出するか」をソルバ任せにしているが、
 * これを assume(diff_p) で1リテラル指定し、POを round-robin で巡回する。
 * 異なるPOへの伝搬は構造的に異なる正当化を要求するため、入力空間の離れた
 * 領域から解が出る（=多様なキューブ）ことを狙う。あるPOが(累積ブロッキング下で)
 * UNSATになったらそのPOは打ち止め。完全性: T(f)=∪_p T_p(f) なので全PO打ち止め後の
 * 素solveがUNSATなら従来と同じ完全列挙。assume方式なので恒久的な制約は残らない。
 */
static unsigned char* divpo_dead = NULL;   /* PO p が UNSAT 済みか（故障ごとにリセット） */
static int divpo_cap = 0, divpo_idx = 0;
static int divpo_last = -1;                /* 直前の solve で assume した PO の net id（なければ -1） */
static char* divpo_prevcube = NULL;        /* 直前キューブ（DIVPO_JACC モードの類似度判定用） */
static int  divpo_pcap = 0;

/* 連続キューブのケア類似度（両方ケア&同値 / どちらかケア）。gt_verify.c の定義と同じ。 */
static double cube_jaccard(const char* a, const char* b){
    int both = 0, either = 0;
    for (int i = 0; i < n_pi; i++){
        int ca = (a[i] != 'X'), cb = (b[i] != 'X');
        if (ca || cb) either++;
        if (ca && cb && a[i] == b[i]) both++;
    }
    return either ? (double)both / either : 0.0;
}

/* PO指定solveの中核。
 *   advance_on_sat=1: SAT毎に次POへ進む（従来の DIVPO=毎キューブ round-robin）。
 *   advance_on_sat=0: SATでも同じPOに留まる（DIVPO_JACC モード）。
 *   force_advance=1 : この回だけ強制で次POへ（直前2キューブの類似度が閾値超のとき）。
 * UNSATのPOは恒久に打ち止め、全PO打ち止め後の素solveで完全性を確定するのは従来通り。 */
static int divpo_solve_core(CCaDiCaL* s, int advance_on_sat, int force_advance){
    divpo_last = -1;
    if (divpo_n <= 1 || !divpo_dead) return ccadical_solve(s);
    if (force_advance) divpo_idx = (divpo_idx + 1) % divpo_n;
    for (int tried = 0; tried < divpo_n; tried++){
        int p = (divpo_idx + tried) % divpo_n;
        if (divpo_dead[p]) continue;
        ccadical_assume(s, divpo_z - divpo_n + p);
        if (ccadical_solve(s) == 10){
            divpo_idx = advance_on_sat ? (p + 1) % divpo_n : p;
            divpo_last = divpo_netid ? divpo_netid[p] : -1;
            return 10;
        }
        divpo_dead[p] = 1;             /* ブロッキングは増える一方なので恒久にUNSAT */
    }
    return ccadical_solve(s);          /* 全PO打ち止め → 素solveで完全性を確定 */
}

/* 直前の solve で検出先に指定した PO の net id（XID の正当化先を揃えるため）。
   指定なし（DIVPO無効/素solve）なら -1。 */
int EXP_PreferredPONet(void){ return divpo_last; }

/* ============== 多様化案B (env DIVPHASE=1): PI位相ランダム化 ==============
 * CDCL の phase saving は前回の解の極性を保持するため、ブロッキング節1本では
 * 「前回の近傍解」に落ちやすい。各 solve 前に PI 変数の優先位相をランダムに
 * 設定し、毎回解空間の別の隅から探索させる。制約は一切加えないので
 * 健全性・完全性とも自明に不変。 */
static void divphase_randomize(CCaDiCaL* s){
    for (int i = 0; i < n_pi; i++){
        int v = (int)pi[i]->varsgc;
        if (v) ccadical_phase(s, (rand() & 1) ? v : -v);
    }
}

void EXP_ResetPerFault(void){
    maxham_aux = cnf.total.vars + 1;
    divpo_idx = 0;
    if (divpo_n > 0){
        if (divpo_cap < divpo_n){ divpo_dead = realloc(divpo_dead, divpo_n); divpo_cap = divpo_n; }
        memset(divpo_dead, 0, divpo_n);
        if (divpo_pcap < n_pi + 1){ divpo_prevcube = realloc(divpo_prevcube, n_pi + 1); divpo_pcap = n_pi + 1; }
        divpo_prevcube[0] = '\0';      /* 故障の先頭では比較対象なし */
    }
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
    if (getenv("DIVPHASE")) divphase_randomize(s);   /* 案B: 位相ランダム化 */
    if (getenv("DIVPO")){                            /* 案A: 検出PO指定 */
        const char* jt = getenv("DIVPO_JACC");
        if (!jt) return divpo_solve_core(s, 1, 0);   /* 従来: 毎キューブ巡回 */
        /* DIVPO_JACC モード: 直前2キューブの Jaccard が閾値超のときだけ PO 切替 */
        double thr = atof(jt);
        int adv = (prev && divpo_prevcube && divpo_prevcube[0]
                   && cube_jaccard(divpo_prevcube, prev) > thr);
        int r = divpo_solve_core(s, 0, adv);
        if (prev && divpo_prevcube){ memcpy(divpo_prevcube, prev, n_pi); divpo_prevcube[n_pi] = '\0'; }
        return r;
    }
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

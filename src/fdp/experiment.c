//-------------------------------------------------------------------------------------------------------------
//	experiment.c : 研究用フック（環境変数で制御、デフォルト無効）
//
//	案1 (env MAXDC): 「非検出オラクル」CNF（正常回路 ∧ 故障コーン ∧ サイト縮退
//	  ∧ 全PO一致=z0）を構築し、各 XID キューブのケアビットを
//	  「(cube\b) ∧ 非検出 が UNSAT のまま」である限り貪欲に落として素項へ拡大する。
//	  追加 env: MAXDC_CORE(UNSATコア一括法), MAXDC_NOMUT(計測のみでキューブ不変),
//	  MAXDC_QX_MULTI=N(QuickXplainを異なる順序でN回実行し最短の素項を採用)。
//	  終了時に [MAXDC] 集計を stderr に出す。
//	案2 (env MAXHAM): 連続キューブのケア領域の値反転が少ない問題に対し、
//	  「前回キューブと >=k ビット違う」制約(Sinz at-most)を活性化リテラルで
//	  ガードして優先 solve する。終了判定は素 solve なので完全性は不変。
//	  追加 env: MAXHAM_K(目標k; 既定 max(2, m/3))。
//	案3 (env DUAL): 双対列挙。非検出空間 ¬D_f のキューブも並行列挙し、
//	  U∪V の閉包 or ¬D_f 列挙完了で det 側の UNSAT を待たずに完了する。
//	  詳細はファイル末尾の 案3 セクション冒頭コメントを参照。
//	案4 (env SPLIT): Shannon 分割による完全化。打ち切り故障の未被覆空間を PI で
//	  二分しながら両側（検出/非検出）を SAT 列挙し、必ず停止して complete にする。
//	  詳細はファイル末尾の 案4 セクション冒頭コメントを参照。
//
//	実験の評価と結論（案2は却下、案1は要改善）は verification/SUMMARY.md を参照。
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <math.h>

#include "./experiment.h"
#include "./create_TPG_model.h"
#include "./fault_detection_prob.h"   /* numtranpo */
#include "./read.h"
#include "./cnf/cnf.h"
#include "./cudd_wrapper.h"           /* parseCube (案3 DUAL / 案4 SPLIT) */
#include "./xid/XID.h"                /* InlineXID (案4 SPLIT) */
#include "../netlist/netlist.h"
#include "../opt/opt.h"

/* ================================ 案1: MAXDC ================================ */
static long mdc_cubes=0, mdc_orig=0, mdc_prime=0, mdc_hr_cubes=0, mdc_sanity_fail=0, mdc_revert=0;
static long mdc_hardcubes=0, mdc_hardorig=0, mdc_hardprime=0;   /* capped faults only */
static long mdc_fcubes=0, mdc_forig=0, mdc_fprime=0;            /* per-fault accumulator */
static long mdc_multi_cubes=0, mdc_multi_improved=0, mdc_multi_saved=0, mdc_multi_trials=0;

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
    if (mdc_multi_cubes){
        fprintf(stderr,
            "[MAXDC_QX_MULTI] cubes=%ld avg_trials=%.1f improved=%ld(%.0f%%) saved_literals=%ld\n",
            mdc_multi_cubes, mdc_avg(mdc_multi_trials, mdc_multi_cubes),
            mdc_multi_improved, 100.0 * mdc_avg(mdc_multi_improved, mdc_multi_cubes),
            mdc_multi_saved);
    }
    fprintf(stderr,
        "[MAXDC] capped-fault cubes=%ld  orig=%.2f prime=%.2f headroom=%.2f bits/cube\n",
        mdc_hardcubes,
        mdc_avg(mdc_hardorig,  mdc_hardcubes),
        mdc_avg(mdc_hardprime, mdc_hardcubes),
        mdc_avg(mdc_hardorig - mdc_hardprime, mdc_hardcubes));
}

/* 検出/非検出オラクルを構築する（WriteTPGModel 直後に呼ぶこと：
   TFOフラグ/varsfc/numtranpo が当該故障用に設定済みである必要がある）。
   detect=0: z=0 を assert（非検出空間 ¬D_f）、detect=1: z=1（検出空間 D_f）。 */
static void mdc_build_oracle(CCaDiCaL* u, TARGET* target, int detect){
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
    if (f->exc_netptr){
        /* TDF: 検出条件は z ∧ 励起（1時刻目コピー=初期値）。
           非検出オラクルは ¬(z∧exc) = (¬z ∨ ¬exc)、検出オラクルは両ユニット節 */
        int exc_lit = (f->type==SF0) ? -(int)f->exc_netptr->varsgc
                                     :  (int)f->exc_netptr->varsgc;
        if (detect){
            ccadical_add(u, z);       ccadical_add(u,0);
            ccadical_add(u, exc_lit); ccadical_add(u,0);
        } else {
            ccadical_add(u,-z); ccadical_add(u,-exc_lit); ccadical_add(u,0);
        }
    } else {
        ccadical_add(u, detect ? z : -z); ccadical_add(u,0);   /* z=0: 非検出 / z=1: 検出 */
    }
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
    mdc_build_oracle(u, target, 0);
    mdc_fcubes=0; mdc_forig=0; mdc_fprime=0;
    return u;
}

static inline int mdc_lit(const char* cube, int i){
    return (cube[i]=='1') ? (int)pi[i]->varsgc : -(int)pi[i]->varsgc;
}

/* 非検出オラクル u に対し、care 添字集合 sub[0..n) を assume して UNSAT(=検出を含意)か。 */
static int mdc_unsat(CCaDiCaL* u, const char* cube, const int* sub, int n){
    for (int k = 0; k < n; k++) ccadical_assume(u, mdc_lit(cube, sub[k]));
    return ccadical_solve(u) == 20;
}

/* QuickXplain (Junker 2004): B∪S が UNSAT のとき、B∪D が UNSAT となる極小な D⊆S を返す。
   ここで「B∪D が UNSAT」= 「D のケアビットだけで検出が含意される」＝ D が素項。
   out に D の添字を書き、|D| を返す。solve 回数は O(|D|·log(|S|/|D|))、revert 不要で常に健全。 */
static int mdc_qx(CCaDiCaL* u, const char* cube,
                  const int* B, int nb, const int* S, int ns, int* out){
    /* B だけで既に UNSAT なら S からは何も要らない */
    if (nb > 0 && mdc_unsat(u, cube, B, nb)) return 0;
    if (ns == 1) { out[0] = S[0]; return 1; }

    int half = ns / 2;
    int* scratch = malloc((size_t)(nb + ns) * sizeof(int));
    int* D1      = malloc((size_t)ns * sizeof(int));

    /* D1 = QX(B∪S1, S2) */
    if (nb) memcpy(scratch, B, (size_t)nb * sizeof(int));   /* B==NULL,nb==0 の memcpy 回避 */
    memcpy(scratch + nb, S, (size_t)half * sizeof(int));
    int nd1 = mdc_qx(u, cube, scratch, nb + half, S + half, ns - half, D1);

    /* D2 = QX(B∪D1, S1)（out に直接書く） */
    memcpy(scratch + nb, D1, (size_t)nd1 * sizeof(int));
    int nd2 = mdc_qx(u, cube, scratch, nb + nd1, S, half, out);

    memcpy(out + nd2, D1, (size_t)nd1 * sizeof(int));
    free(scratch); free(D1);
    return nd2 + nd1;
}

/* HALL/MARS の「短い implicant を優先する」着想を、外部MaxSATなしで近似する実験。
   QuickXplain は集合極小(MUS)を返すが、分割順により異なる極小集合になり得る。
   care の順序を決定的にシャッフルして複数回抽出し、リテラル数最小の素項を選ぶ。
   各候補は mdc_qx が返す正当な素項なので、選択しても健全性・完全性は変わらない。
   これは cardinality-minimum MUS の厳密解ではなく、低コストなmulti-start近似。 */
static int mdc_qx_multistart(CCaDiCaL* u, const char* cube,
                             const int* care, int nc, int* out){
    int best_n = mdc_qx(u, cube, NULL, 0, care, nc, out);
    int first_n = best_n;
    const char* s = getenv("MAXDC_QX_MULTI");
    int trials = s ? atoi(s) : 1;
    if (trials < 2) return best_n;
    if (trials > 32) trials = 32;   /* SATコストの暴走防止 */

    int* perm = malloc((size_t)nc * sizeof(int));
    int* cand = malloc((size_t)nc * sizeof(int));
    uint32_t state = 2166136261u;
    for (int i = 0; i < n_pi; i++){
        state ^= (unsigned char)cube[i];
        state *= 16777619u;
    }
    if (state == 0) state = 0x9e3779b9u;

    for (int t = 1; t < trials; t++){
        memcpy(perm, care, (size_t)nc * sizeof(int));
        for (int i = nc - 1; i > 0; i--){
            state ^= state << 13; state ^= state >> 17; state ^= state << 5;
            int j = (int)(state % (uint32_t)(i + 1));
            int tmp = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
        }
        int nk = mdc_qx(u, cube, NULL, 0, perm, nc, cand);
        if (nk < best_n){
            memcpy(out, cand, (size_t)nk * sizeof(int));
            best_n = nk;
        }
    }
    free(perm);
    free(cand);

    mdc_multi_cubes++;
    mdc_multi_trials += trials;
    if (best_n < first_n){
        mdc_multi_improved++;
        mdc_multi_saved += first_n - best_n;
    }
    return best_n;
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

    if (getenv("MAXDC_QX")) {
        /* QuickXplain で真の極小素項を求める（revert 不要・常に健全）。
           まず完全キューブが検出を含意すること（B∪S が UNSAT）を確認する。 */
        if (!mdc_unsat(u, cube, care, nc)) {
            mdc_sanity_fail++;
        } else {
            static int* keep = NULL; static int kcap = 0;
            if (kcap < n_pi) { keep = realloc(keep, n_pi * sizeof(int)); kcap = n_pi; }
            int nk = mdc_qx_multistart(u, cube, care, nc, keep);
            /* 一旦すべての care を X にし、keep に残った素項ビットだけ元値を復元 */
            for (int k = 0; k < nc; k++) cube[care[k]] = 'X';
            for (int k = 0; k < nk; k++) cube[keep[k]] = save[keep[k]];
        }
    } else if (getenv("MAXDC_CORE")) {
        /* cheap one-shot: keep only the unsat core, then verify+revert。
           MAXDC_HYB=1 のとき、revert する代わりに QuickXplain で救済する
           （coreの1-solveで済む大多数は高速のまま、core が外した分だけ minimal 化）。 */
        int hyb = getenv("MAXDC_HYB") != NULL;
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
            if (live < orig && ccadical_solve(u) != 20) {
                memcpy(cube, save, n_pi+1);
                if (hyb) {
                    /* QuickXplain で救済（save=完全キューブから minimal prime を求める） */
                    static int* keep = NULL; static int kcap = 0;
                    if (kcap < n_pi) { keep = realloc(keep, n_pi * sizeof(int)); kcap = n_pi; }
                    int nk = mdc_qx(u, cube, NULL, 0, care, nc, keep);
                    for (int k = 0; k < nc; k++) cube[care[k]] = 'X';
                    for (int k = 0; k < nk; k++) cube[keep[k]] = save[keep[k]];
                } else {
                    mdc_revert++;
                }
            }
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

/* ================================ 案3: DUAL ================================
 * 双対列挙 (env DUAL=1): 検出空間 D_f のキューブ列挙と 1:1 で、非検出空間
 * ¬D_f のキューブも並行して列挙する（cf. Möhle & Biere の dual reasoning）。
 *   U = 検出側キューブの和集合（従来の cubes、種キューブ含む）
 *   V = 非検出側キューブの和集合（非検出CNF z=0 の解を、検出CNF z=1 を
 *       オラクルに UNSATコア→QuickXplain 救済で ¬D_f の素項へ拡大したもの）
 * U ⊆ D_f と V ⊆ ¬D_f は互いに素なので、
 *   (a) 閉包: U∪V が恒真になった時点で U = D_f が確定
 *   (b) V完了: 非検出側が UNSAT（V = ¬D_f 確定）なら、残り D_f\U = ¬(U∪V) を
 *       BDD の disjoint パスとして取り出し U へ補充（SAT solve 不要）
 * どちらも det 側の UNSAT を待たずに complete=1 で終われ、終了時に
 * U = D_f が保証されるため fdp / GT_BDD / 支配流用の不変条件は保たれる。
 * fdp が 1 に近い故障ほど ¬D_f 側が小さく (b) が早く効く。
 *
 * コスト制御（b12 での初期実験で判明した2つの落とし穴への対策）:
 *   - 遅延起動: det 側が DUAL_START 本（既定16）以内で終わる易しい故障では
 *     V 側を一切動かさない（ソルバ構築も遅延）。V 側 solve の浪費を防ぐ。
 *   - remainder 上限: (b) のパス数が生成済みキューブ数に対して大きすぎる場合は
 *     取り出しを見送り det 列挙を続行する（U が育てば残りは縮むので毎回再判定）。
 *     無制限に取り出すと disjoint パスが数百万本になり、cube_cnt と
 *     支配流用の種を爆発させる（実測 b12 で 1,637万本）。
 */
static TARGET*    dual_target = NULL;   /* 対象故障（NULL なら DUAL 無効） */
static CCaDiCaL*  dual_nd  = NULL;   /* 非検出空間の列挙ソルバ（Vの禁止節が溜まる） */
static CCaDiCaL*  dual_det = NULL;   /* 検出オラクル（Vキューブの素項展開用・不変） */
static DdManager* dual_gbm = NULL;
static DdNode*    dual_u   = NULL;
static DdNode*    dual_v   = NULL;
static int dual_steps = 0, dual_vcomplete = 0, dual_vcap = 0, dual_nv = 0, dual_ended = 0;
static long dl_closed=0, dl_vdone=0, dl_det=0, dl_capped=0;
static long dl_vcubes=0, dl_remcubes=0, dl_remskip=0, dl_sanity=0;

static void dual_dump(void){
    fprintf(stderr,
        "\n[DUAL] faults: closed(U∪V=1)=%ld  vcomplete(¬D_f確定)=%ld  det_unsat=%ld  capped=%ld\n"
        "[DUAL] v_cubes=%ld  remainder_cubes=%ld  remainder_skips=%ld  sanity_fail=%ld\n",
        dl_closed, dl_vdone, dl_det, dl_capped, dl_vcubes, dl_remcubes, dl_remskip, dl_sanity);
}

/* acc |= cube（参照カウントを維持したまま OR で置き換える） */
static void exp_or_into(DdManager* m, DdNode** acc, const char* cube){
    DdNode* c = parseCube(m, cube, n_pi);
    DdNode* t = Cudd_bddOr(m, *acc, c);
    Cudd_Ref(t);
    Cudd_RecursiveDeref(m, *acc);
    Cudd_RecursiveDeref(m, c);
    *acc = t;
}

/* ソルバ s にキューブの禁止節（否定）を追加する */
static void exp_block(CCaDiCaL* s, const char* cube){
    for (int i = 0; i < n_pi; i++) {
        if      (cube[i] == '0') ccadical_add(s,  (int)pi[i]->varsgc);
        else if (cube[i] == '1') ccadical_add(s, -(int)pi[i]->varsgc);
    }
    ccadical_add(s, 0);
}

/* 非検出キューブ（初期は全ビットケアの最小項）を ¬D_f の素項へ拡大する。
   「検出CNF(det_oracle) ∧ cube が UNSAT」= cube ⊆ ¬D_f を保つ範囲でケアビットを落とす。
   UNSATコア一括→検証、破れたら QuickXplain で救済（revert なし・常に健全）。 */
static void dual_expand_v(CCaDiCaL* det_oracle, char* cube){
    static int *care=NULL, *keep=NULL; static char* save=NULL; static int cap=0;
    if (cap<n_pi){ care=realloc(care,n_pi*sizeof(int)); keep=realloc(keep,n_pi*sizeof(int));
                   save=realloc(save,n_pi+1); cap=n_pi; }
    int nc = 0;
    for (int i = 0; i < n_pi; i++)
        if (cube[i] != 'X') care[nc++] = i;
    if (nc == 0) return;
    memcpy(save, cube, n_pi+1);

    if (!mdc_unsat(det_oracle, cube, care, nc)) { dl_sanity++; return; }

    /* コア外（failed でない）ビットを一括で落とす */
    for (int k = 0; k < nc; k++)
        if (!ccadical_failed(det_oracle, mdc_lit(cube, care[k]))) cube[care[k]] = 'X';

    /* 残ったケアだけで本当に UNSAT か検証。破れたら QX で救済 */
    int live = 0;
    for (int k = 0; k < nc; k++)
        if (cube[care[k]] != 'X') { ccadical_assume(det_oracle, mdc_lit(cube, care[k])); live++; }
    if (live < nc && ccadical_solve(det_oracle) != 20) {
        memcpy(cube, save, n_pi+1);
        int nk = mdc_qx(det_oracle, cube, NULL, 0, care, nc, keep);
        for (int k = 0; k < nc; k++) cube[care[k]] = 'X';
        for (int k = 0; k < nk; k++) cube[keep[k]] = save[keep[k]];
    }
}

void EXP_DualInit(DdManager* gbm, TARGET* target){
    if (!getenv("DUAL")) return;
    static int registered = 0;
    if (!registered) { atexit(dual_dump); registered = 1; }
    dual_gbm = gbm;
    dual_target = target;   /* ソルバ/BDD の構築は V 側の初回起動まで遅延する */
    dual_steps = dual_vcomplete = dual_vcap = dual_nv = dual_ended = 0;
}

static void dual_vstep(void);
static bool dual_tryfinish(CubeSet* cubes);

bool EXP_DualStep(CubeSet* cubes, const char* latest){
    if (!dual_target) return false;

    /* 遅延起動: det 側が DUAL_START 本以内で終わる故障では V 側を動かさない */
    if (!dual_nd) {
        const char* s = getenv("DUAL_START");
        if (++dual_steps < (s ? atoi(s) : 16)) return false;
        dual_nd  = ccadical_init(); ccadical_set_option(dual_nd,  "factor", 0);
        dual_det = ccadical_init(); ccadical_set_option(dual_det, "factor", 0);
        mdc_build_oracle(dual_nd,  dual_target, 0);
        mdc_build_oracle(dual_det, dual_target, 1);
        dual_u = Cudd_ReadLogicZero(dual_gbm); Cudd_Ref(dual_u);
        dual_v = Cudd_ReadLogicZero(dual_gbm); Cudd_Ref(dual_v);
        for (int m = 0; m < cubes->n; m++)   /* 種＋ここまでの det キューブ（latest 含む） */
            exp_or_into(dual_gbm, &dual_u, cubes->data[m]);
    } else {
        exp_or_into(dual_gbm, &dual_u, latest);
    }

    /* 非検出側を1本進める（V が確定/上限到達済みならスキップ） */
    if (!dual_vcomplete && !dual_vcap) dual_vstep();

    return dual_tryfinish(cubes);
}

/* 非検出側を1本進める（呼び出し側で !vcomplete && !vcap を確認すること） */
static void dual_vstep(void){
    if (ccadical_solve(dual_nd) == 20) {
        dual_vcomplete = 1;
        return;
    }
    char* vc = (char*)malloc((size_t)n_pi + 1);
    for (int i = 0; i < n_pi; i++)
        vc[i] = (ccadical_val(dual_nd, (int)pi[i]->varsgc) > 0) ? '1' : '0';
    vc[n_pi] = '\0';
    dual_expand_v(dual_det, vc);
    exp_block(dual_nd, vc);                    /* nd 列挙ソルバに禁止節を追加 */
    exp_or_into(dual_gbm, &dual_v, vc);
    free(vc);
    dual_nv++; dl_vcubes++;
    /* V 側の上限。V キューブは成果物でなく閉包判定用の内部データなので、
       env DUAL_VLIMIT で det 側の -limit と独立に大きくできる（既定は -limit と同じ）。 */
    const char* vs = getenv("DUAL_VLIMIT");
    int vlimit = vs ? atoi(vs) : opt.file.input.limit;
    if (vlimit > 0 && dual_nv >= vlimit) dual_vcap = 1;
}

/* 完了判定: V 確定なら残り D_f\U をパス補充して完了、さもなくば U∪V の閉包を見る */
static bool dual_tryfinish(CubeSet* cubes){
    DdNode* uv = Cudd_bddOr(dual_gbm, dual_u, dual_v);
    Cudd_Ref(uv);

    if (dual_vcomplete) {
        /* V = ¬D_f 確定。残り D_f\U = ¬(U∪V) を disjoint パスで取り出し U へ補充する。
           ただしパス数が生成済みキューブ数に対して大きすぎる場合は見送る
           （U が育てば残りは縮むので、次の機会に再判定）。 */
        DdNode* rem = Cudd_Not(uv);
        const char* rs = getenv("DUAL_REMCAP");
        double  cap = rs ? atof(rs) : 4.0 * cubes->n + 64.0;
        if (Cudd_CountPathsToNonZero(rem) > cap) {
            Cudd_RecursiveDeref(dual_gbm, uv);
            dl_remskip++;
            return false;
        }
        DdGen* gen; int* cu; CUDD_VALUE_TYPE val;
        Cudd_ForeachCube(dual_gbm, rem, gen, cu, val) {
            char* s = (char*)malloc((size_t)n_pi + 1);
            for (int i = 0; i < n_pi; i++)
                s[i] = (cu[i] == 1) ? '1' : (cu[i] == 0) ? '0' : 'X';
            s[n_pi] = '\0';
            cubeset_push(cubes, s);
            dl_remcubes++;
        }
        Cudd_RecursiveDeref(dual_gbm, uv);
        dl_vdone++; dual_ended = 1;
        return true;
    }

    bool closed = (uv == Cudd_ReadOne(dual_gbm));   /* 閉包: U∪V 恒真 → U = D_f */
    Cudd_RecursiveDeref(dual_gbm, uv);
    if (closed) { dl_closed++; dual_ended = 1; }
    return closed;
}

/* det 側が limit で打ち切られた後、V 側だけを DUAL_VLIMIT まで回して完了を狙う
   （「ドレイン」）。det 側 1:1 の交互制約を外すことで、¬D_f の被覆が中規模の
   故障は det 30 本のままでも complete にできる。閉包判定は 32 本ごとに間引く。 */
bool EXP_DualDrain(CubeSet* cubes){
    if (!dual_target || !dual_nd) return false;   /* DUAL 無効 or V 側未起動 */
    while (1) {
        if (dual_tryfinish(cubes)) return true;
        if (dual_vcomplete || dual_vcap) return false;  /* これ以上 V は増やせない */
        for (int k = 0; k < 32 && !dual_vcomplete && !dual_vcap; k++)
            dual_vstep();
    }
}

void EXP_DualDone(FNODE* f, bool limit_hit){
    if (!dual_target) return;
    dual_target = NULL;
    if (limit_hit) dl_capped++;
    if (!dual_nd) return;               /* V 側未起動（DUAL_START 未満で完了） */
    if (limit_hit) {
        /* 打ち切り: 双対列挙の副産物として fdp の anytime 上下界を報告できる */
        double lo = ldexp(Cudd_CountMinterm(dual_gbm, dual_u, n_pi), -n_pi);
        double hi = 1.0 - ldexp(Cudd_CountMinterm(dual_gbm, dual_v, n_pi), -n_pi);
        fprintf(stderr, "[DUAL] capped %s,%s v_cubes=%d%s bounds=[%.6f, %.6f]\n",
                f->name, (f->type == SF0) ? "sa0" : "sa1",
                dual_nv, dual_vcomplete ? "(complete)" : "", lo, hi);
    } else if (!dual_ended) {
        dl_det++;                       /* 従来どおり det 側 UNSAT で完了 */
    }
    ccadical_release(dual_nd);  dual_nd  = NULL;
    ccadical_release(dual_det); dual_det = NULL;
    Cudd_RecursiveDeref(dual_gbm, dual_u); dual_u = NULL;
    Cudd_RecursiveDeref(dual_gbm, dual_v); dual_v = NULL;
}

/* ================================ 案4: SPLIT ================================
 * Shannon 分割による完全化 (env SPLIT=1): 打ち切りになった故障の「未被覆空間」を
 * PI で二分しながら、各部分空間で検出側(U)と非検出側(V)のキューブを少量ずつ
 * SAT 列挙する（cf. #SAT の CDP [Birnbaum&Lozinskii 1999] / DPLL-trace 圧縮
 * [Huang&Darwiche 2004] の分割統治を、無改造 CaDiCaL の assumption で実現）。
 *   - 部分空間で det/非det の両ソルバが UNSAT → その空間は U∪V で被覆済み（閉包）
 *   - 閉じなければ rem = path ∧ ¬(U∪V) のサポート変数で二分して再帰
 *   - path は深さ ≤ n_pi で必ず単一ミンタームに達して閉じる ＝ 必ず停止する
 * 終了時 U∪V=1 かつ U⊆D_f, V⊆¬D_f より U = D_f。追加された U キューブは
 * 本物のテストキューブ（XID + MAXDC 併用可）で、fdp は従来の RunBDD 経路のまま
 * 厳密・complete=1 になる。D_f を回路から直接構成する従来手法（BDD直接法）とは
 * 異なり、すべての情報は SAT 列挙から得る（BDD はキューブ和集合の管理のみ）。
 *   env: SPLIT_BUDGET   ノードあたり両側それぞれの列挙本数（既定8）
 *        SPLIT_MAXNODES 分割ノード数の安全弁（超えたら諦めて従来の capped、既定1000000）
 */
static long sp_faults=0, sp_done=0, sp_bail=0, sp_nodes=0, sp_maxdepth=0, sp_ucubes=0, sp_vcubes=0;

static void split_dump(void){
    fprintf(stderr,
        "\n[SPLIT] faults=%ld  completed=%ld  bailed=%ld  nodes=%ld  max_depth=%ld\n"
        "[SPLIT] added u_cubes=%ld  v_cubes=%ld\n",
        sp_faults, sp_done, sp_bail, sp_nodes, sp_maxdepth, sp_ucubes, sp_vcubes);
}

/* path（'0'/'1'/'X'）の割り当て済みビットを assumption として積む */
static void sp_assume_path(CCaDiCaL* s, const char* path){
    for (int i = 0; i < n_pi; i++) {
        if      (path[i] == '1') ccadical_assume(s,  (int)pi[i]->varsgc);
        else if (path[i] == '0') ccadical_assume(s, -(int)pi[i]->varsgc);
    }
}

bool EXP_SplitFinish(DdManager* gbm, CCaDiCaL* det, CCaDiCaL* u_oracle,
                     CubeSet* cubes, TARGET* target){
    if (!getenv("SPLIT")) return false;
    FNODE* f = target->list[0];
    static int registered = 0;
    if (!registered) { atexit(split_dump); registered = 1; }
    sp_faults++;

    int  budget   = getenv("SPLIT_BUDGET")   ? atoi(getenv("SPLIT_BUDGET"))   : 8;
    long maxnodes = getenv("SPLIT_MAXNODES") ? atol(getenv("SPLIT_MAXNODES")) : 1000000;
    if (budget < 1) budget = 1;

    /* 非検出列挙ソルバ nd と、非検出キューブ素項展開用の検出オラクル dor */
    CCaDiCaL* nd  = ccadical_init(); ccadical_set_option(nd,  "factor", 0);
    CCaDiCaL* dor = ccadical_init(); ccadical_set_option(dor, "factor", 0);
    mdc_build_oracle(nd,  target, 0);
    mdc_build_oracle(dor, target, 1);

    /* U（既存キューブの和集合）と V の BDD */
    DdNode* u = Cudd_ReadLogicZero(gbm); Cudd_Ref(u);
    DdNode* v = Cudd_ReadLogicZero(gbm); Cudd_Ref(v);
    for (int m = 0; m < cubes->n; m++) exp_or_into(gbm, &u, cubes->data[m]);

    /* DFS スタック（深さ ≤ n_pi なので同時保持は高々 n_pi+1 ノード） */
    int cap = n_pi + 2, sp = 0;
    char** stk = malloc((size_t)cap * sizeof(char*));
    char* root = malloc((size_t)n_pi + 1);
    memset(root, 'X', (size_t)n_pi); root[n_pi] = '\0';
    stk[sp++] = root;

    long nodes = 0;
    bool ok = true;
    while (sp > 0) {
        if (++nodes > maxnodes) { ok = false; break; }
        char* path = stk[--sp];

        /* この部分空間で両側を budget 本ずつ列挙する */
        int det_dead = 0, nd_dead = 0;
        for (int k = 0; k < budget && !(det_dead && nd_dead); k++) {
            if (!det_dead) {
                sp_assume_path(det, path);
                if (ccadical_solve(det) == 10) {
                    char* xc = InlineXID(det, f->netptr, -1, NULL);
                    EXP_Expand(u_oracle, xc);          /* MAXDC 有効時のみ素項化 */
                    exp_block(det, xc);
                    exp_or_into(gbm, &u, xc);
                    cubeset_push(cubes, xc);
                    sp_ucubes++;
                } else det_dead = 1;                    /* path∧D_f ⊆ U */
            }
            if (!nd_dead) {
                sp_assume_path(nd, path);
                if (ccadical_solve(nd) == 10) {
                    char* vc = (char*)malloc((size_t)n_pi + 1);
                    for (int i = 0; i < n_pi; i++)
                        vc[i] = (ccadical_val(nd, (int)pi[i]->varsgc) > 0) ? '1' : '0';
                    vc[n_pi] = '\0';
                    dual_expand_v(dor, vc);
                    exp_block(nd, vc);
                    exp_or_into(gbm, &v, vc);
                    free(vc);
                    sp_vcubes++;
                } else nd_dead = 1;                     /* path∧¬D_f ⊆ V */
            }
        }
        if (det_dead && nd_dead) { free(path); continue; }   /* 閉包 */

        /* 未被覆 rem = path ∧ ¬(U∪V)。空なら閉包、非空ならそのサポート変数で二分 */
        DdNode* uv = Cudd_bddOr(gbm, u, v); Cudd_Ref(uv);
        DdNode* pb = parseCube(gbm, path, n_pi);
        DdNode* rem = Cudd_bddAnd(gbm, pb, Cudd_Not(uv)); Cudd_Ref(rem);
        Cudd_RecursiveDeref(gbm, uv);
        Cudd_RecursiveDeref(gbm, pb);
        if (rem == Cudd_ReadLogicZero(gbm)) {
            Cudd_RecursiveDeref(gbm, rem);
            free(path);
            continue;
        }
        int var = -1;
        int* sup = Cudd_SupportIndex(gbm, rem);
        if (sup) {
            for (int i = 0; i < Cudd_ReadSize(gbm) && var < 0; i++)
                if (sup[i] && i < n_pi && path[i] == 'X') var = i;
            free(sup);
        }
        Cudd_RecursiveDeref(gbm, rem);
        if (var < 0)   /* 保険: rem のサポートが全て割り当て済みなら任意の未割り当てPI */
            for (int i = 0; i < n_pi && var < 0; i++)
                if (path[i] == 'X') var = i;
        if (var < 0) { ok = false; free(path); break; }   /* 全割り当てで未被覆は起こらないはず */

        long depth = 1;
        for (int i = 0; i < n_pi; i++) if (path[i] != 'X') depth++;
        if (depth > sp_maxdepth) sp_maxdepth = depth;

        char* c0 = malloc((size_t)n_pi + 1);
        memcpy(c0, path, (size_t)n_pi + 1);
        c0[var] = '0';
        path[var] = '1';                 /* path を '1' 側の子として再利用 */
        if (sp + 2 > cap) { cap *= 2; stk = realloc(stk, (size_t)cap * sizeof(char*)); }
        stk[sp++] = c0;
        stk[sp++] = path;
    }
    while (sp > 0) free(stk[--sp]);
    free(stk);
    ccadical_release(nd);
    ccadical_release(dor);
    Cudd_RecursiveDeref(gbm, u);
    Cudd_RecursiveDeref(gbm, v);
    sp_nodes += nodes;
    if (ok) sp_done++; else sp_bail++;
    return ok;
}

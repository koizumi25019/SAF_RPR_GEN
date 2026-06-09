//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/resource.h>
#include <cudd.h>
#include <gmp.h>

#include <stdlib.h>
#include "ccadical.h"
#include "./create_TPG_model.h"
#include "./fault_detection_prob.h"
#include "./init.h"
#include "./read.h"
#include "./cube_set.h"
#include "./cnf/cnf.h"
#include "../opt/opt.h"
#include "./cudd_wrapper.h"
#include "./xid/XID.h"

//*************************************************************************************************************
//	@name		AddBlockingClauseFromCube
//	@function	キューブ文字列（'0'/'1'/'X' を n_pi 文字）からブロッキング節をソルバに追加する
//*************************************************************************************************************
static void AddBlockingClauseFromCube(CCaDiCaL* solver, const char* cube)
{
    for (int i = 0; i < n_pi; i++)
    {
        int lit = 0;
        if      (cube[i] == '0') lit =  (int)pi[i]->varsgc;
        else if (cube[i] == '1') lit = -(int)pi[i]->varsgc;
        if (lit != 0) ccadical_add(solver, lit);
    }
    ccadical_add(solver, 0);
}

/* =====================================================================
 *  EXPERIMENT (env MAXDC_MEASURE): don't-care headroom measurement.
 *  Builds an "undetection oracle" CNF (good ^ faulty-cone ^ fc[site]=stuck
 *  ^ all-PO-equal i.e. z=0) and, for each XID cube, greedily drops care
 *  bits while (cube\b ^ undetection) stays UNSAT  ==> prime implicant.
 *  Reports how many MORE bits could be X'd (the headroom for 案1).
 *  Inert unless MAXDC_MEASURE is set; production behaviour unchanged.
 * ===================================================================== */
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

/* INDEPENDENT ground-truth: Monte-Carlo true FDP via direct logic simulation
   (no SAT, no oracle). Evaluates good vs faulty circuit, detect = any PO differs. */
static int* sim_topo=NULL; static int sim_ntopo=0;
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

/* build the undetection oracle for the CURRENT target (call right after
   WriteTPGModel so TFO flags / varsfc / numtranpo are set for this fault) */
static void MDC_BuildOracle(CCaDiCaL* u, TARGET* target){
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

static inline int mdc_lit(const char* cube, int i){
    return (cube[i]=='1') ? (int)pi[i]->varsgc : -(int)pi[i]->varsgc;
}

/* 案1: expand `cube` toward a prime implicant IN PLACE (care bits -> 'X').
   Cheap method: assume all care literals once; on UNSAT, the unsat core
   (ccadical_failed) is a sufficient subset, so every care bit NOT in the
   core can be dropped to X simultaneously. Optional extra greedy rounds
   minimise further (env MAXDC_ROUNDS, default 1 = core-only).            */
static void MDC_Expand(CCaDiCaL* u, char* cube){
    static int* care=NULL; static char* save=NULL; static int cap=0;
    if (cap<n_pi){ care=realloc(care,n_pi*sizeof(int)); save=realloc(save,n_pi+1); cap=n_pi; }
    int nc=0; for (int i=0;i<n_pi;i++) if (cube[i]!='X') care[nc++]=i;
    if (nc==0) return;
    memcpy(save, cube, n_pi+1);       /* keep original to revert if needed */
    int orig=nc;
    int use_core = getenv("MAXDC_CORE") ? 1 : 0;  /* default: sound per-bit greedy */

    if (use_core) {
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

/* ================== 案2: 多様解(最大ハミング距離)による多様化 ==================
 * 連続キューブのケア領域の値反転が極端に少ない問題を直接攻める。各 solve 前に
 * 「前回キューブのケア領域と >=k ビット違う」制約を活性化リテラル act でガードし
 * assume(act) でその回だけ有効化する。
 *   exact 安全性: 全節を (-act ∨ ...) でガードし act は恒久 assert しない。よって
 *   正当な検出を恒久的に消すことは不可能。多様性 UNSAT 時は k を下げ、最後に
 *   「多様性なしの素 solve」が UNSAT のときだけ故障完了とみなす。
 *   距離 >=k は at-most-(m-k) (Sinz 逐次カウンタ) でエンコード。
 * env: MAXHAM(有効化), MAXHAM_K(目標 k; 既定 max(2, m/3)).
 */
static int maxham_aux = 0;   /* 故障ごとに cnf.total.vars+1 で初期化する aux 採番器 */

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
static int MaxHamSolve(CCaDiCaL* s, const char* prev)
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

//*************************************************************************************************************
//	@name	    @AnalyzeFaultDensity
//	@function   analyze the fault detection probability
//	@return		(bool) okay, error
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	double* out_time_cadical,
    double* out_time_bdd,
    double* out_time_xid,
    double* out_time_read
)
{
	TARGET	target;
	FILE* bdd_result = (FILE*)NULL;
	FILE* cube_analysis_fp = (FILE*)NULL;

	int count = 0;

	// ===== CPU時間計測用変数 =====
    clock_t t_start, t_end;
    double time_cadical = 0.0;
    double time_bdd     = 0.0;
    double time_xid     = 0.0;
    double time_read    = 0.0;
    // ============================

	//キューブ分析用ファイルオープン
	if (opt.file.input.cube_analysis != FILE_NOSET) {
		fileOpen(&cube_analysis_fp, opt.file.input.cube_analysis, "w");
	}

	//CUDD初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	//result file open
	fileOpen(&bdd_result, opt.file.output.fdp, "w");
	fprintf(bdd_result, "net_name,f_type,cube_cnt,complete,fdp\n");

	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

    t_start = clock();
	printf("Reading fault data...\n");
	if (ReadFault() != READ_OKAY) return READ_ERROR;
    t_end = clock();
    time_read = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	printf("ReadFault: %.3f sec\n", time_read);

	if (CreateConsGC() != true) return AFD_ERROR;

	while (readdata.fault.numrema != 0)
	{
		// ソルバの初期化
		CCaDiCaL* solver = ccadical_init();
		ccadical_set_option(solver, "factor", 0);

		count++;
		SetTarget(&target);
		FNODE* f = target.list[0];

		if (WriteTPGModel(solver, &target) != true) return AFD_ERROR;

		// 案1: 素項展開用 非検出オラクル（env MAXDC 指定時のみ。未指定なら従来動作）
		CCaDiCaL* u_oracle = NULL;
		if (getenv("MAXDC") || (getenv("MDC_MC") && strcmp(f->name,getenv("MDC_MC"))==0)) {
			if (mdc_cubes==0 && mdc_fcubes==0) atexit(mdc_dump);
			u_oracle = ccadical_init();
			ccadical_set_option(u_oracle, "factor", 0);
			MDC_BuildOracle(u_oracle, &target);
			mdc_fcubes=0; mdc_forig=0; mdc_fprime=0;
		}

		// 診断: 真のFDP(検出ソルバ) と オラクルの非検出率 をモンテカルロで突き合わせ
		if (getenv("MDC_MC") && strcmp(f->name, getenv("MDC_MC"))==0) {
			long N = 100000, det = 0, oun = 0;
			for (long s=0;s<N;s++){
				/* same random PI pattern for both solvers */
				for (int i=0;i<n_pi;i++){ int v=(int)pi[i]->varsgc; if(v){ int L=(rand()&1)?v:-v; ccadical_assume(solver,L); if(u_oracle) ccadical_assume(u_oracle,L);} }
				if (ccadical_solve(solver)==10) det++;
				if (u_oracle && ccadical_solve(u_oracle)==10) oun++;
			}
			double sim = FdpBySim(f, 200000);   /* independent ground truth */
			fprintf(stderr,"[MC] fault=%s/%s  SIM_TRUE_FDP=%.6f | detSolver=%.6f oracle_detect=%.6f (det+undet=%.4f)\n",
				f->name,(f->type==SF0)?"sa0":"sa1", sim, (double)det/N, 1.0-(double)oun/N, (double)(det+oun)/N);
		}

		// f のテストキューブを集める集合
		CubeSet cubes;
		cubeset_init(&cubes, (opt.file.input.limit > 0) ? opt.file.input.limit : 30);

		// 部分集合側の故障（subset_faults）のキューブを種＋禁止節として流用する。
		// T(subset) ⊆ T(f) なので、これらは f の正当なテストであり、
		// solver は差分 T(f)\∪T(subset) だけを探索すればよい。
		// MDC_NODOM をセットすると流用を止め、ゼロから完全列挙する（支配解析の検証用）。
		bool nodom = getenv("MDC_NODOM");
		for (int k = 0; k < f->n_subset_faults; k++)
		{
			FNODE* src = f->subset_faults[k];

			if (!nodom)
			{
				for (int m = 0; m < src->cubes.n; m++)
				{
					cubeset_push(&cubes, strdup(src->cubes.data[m]));
					AddBlockingClauseFromCube(solver, src->cubes.data[m]);
				}
			}

			// この親で src のキューブを使い切る。最後の消費者ならここで解放
			if (--src->n_pending == 0)
				cubeset_free(&src->cubes);
		}

		if (opt.file.input.cube_analysis != FILE_NOSET) {
			fprintf(cube_analysis_fp, "%s", f->name);
			fprintf(cube_analysis_fp, (f->type == SF0) ? ",sa0" : ",sa1");
		}

		// 案2: 多様化の参照(前回キューブ)と aux 採番器を故障ごとに初期化
		char* prev_cube = NULL;
		maxham_aux = cnf.total.vars + 1;

		// limit <= 0 は「上限なし（無制限）」を意味し、UNSAT まで完全列挙する
		bool unlimited = (opt.file.input.limit <= 0);

		// UNSAT もしくは limit 到達でテスト生成を終了する
		while (1) {
            t_start = clock();
            int res = MaxHamSolve(solver, prev_cube);
            t_end   = clock();
            time_cadical += ((double)(t_end - t_start)) / CLOCKS_PER_SEC;

            if (res == 20 || (!unlimited && cubes.n >= opt.file.input.limit)) {
                bool limit_hit = (!unlimited && cubes.n >= opt.file.input.limit && res != 20);

				if (opt.file.input.cube_analysis != FILE_NOSET) {
					fprintf(cube_analysis_fp, "\n");
				}

                t_start = clock();
                RunBDD(gbm, n_pi, cubes.data, cubes.n, bdd_result, NULL, &target, cubes.n, limit_hit);
                t_end   = clock();
                time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				// 伸び代測定：この故障が capped なら hard 集計に加算し、オラクルを解放
				if (u_oracle) {
					if (limit_hit) { mdc_hardcubes+=mdc_fcubes; mdc_hardorig+=mdc_forig; mdc_hardprime+=mdc_fprime; }
					ccadical_release(u_oracle); u_oracle=NULL;
				}

				// キューブの所有権を故障へ移す（深いコピーはしない）。
				// 流用する親が残っていなければ即解放し、メモリを生存集合だけに保つ。
				f->cubes = cubes;
				if (f->n_pending == 0)
					cubeset_free(&f->cubes);

				DropDeteFault(&target);
				FreeMemory(&target);
				break;
			}
			// SAT → InlineXID でドントケアを埋め、キューブ追加＋禁止節
			else {
				printf("\rProgress >> %d/%d", count, readdata.fault.numinit);

                t_start = clock();
                char* x_pattern = InlineXID(solver, f->netptr);
                t_end   = clock();
                time_xid += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				if (u_oracle) MDC_Expand(u_oracle, x_pattern);  // 案1: キューブを素項へ拡大（in place）

				AddBlockingClauseFromCube(solver, x_pattern);
				cubeset_push(&cubes, x_pattern);
				prev_cube = x_pattern;   // 案2: 次回 solve の多様化参照

				// 計測: 特定故障のキューブ列を stderr にダンプ（env CUBE_DUMP=<net>）
				if (getenv("CUBE_DUMP") && strcmp(f->name, getenv("CUBE_DUMP"))==0)
					fprintf(stderr, "[CUBE] %s\n", x_pattern);

                if (opt.file.input.cube_analysis != FILE_NOSET) {
                    t_start = clock();
                    RunBDD(gbm, n_pi, cubes.data, cubes.n, NULL, cube_analysis_fp, &target, cubes.n, false);
                    t_end   = clock();
                    time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;
                }
			}
		}
		ccadical_release(solver);
	}

	// ===== CPU time =====
    *out_time_cadical = time_cadical;
    *out_time_bdd     = time_bdd;
    *out_time_xid     = time_xid;
    *out_time_read    = time_read;

	return AFD_OKAY;
}

//*************************************************************************************************************
//	@name		@FreeMemory
//	@function	free the memory
//	@return		(void)
//*************************************************************************************************************
void FreeMemory(
	TARGET* target
)
{
	free(target->list);
	return;
}

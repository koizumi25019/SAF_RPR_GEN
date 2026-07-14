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
#include "./gmp_wrapper.h"   /* 厳密化フォールバック(env PCOUNT/BDD_EXACT)の行出力 */
#include "./pcount.h"        /* 案5: PODEM型入力空間探索の厳密数え上げ(env PCOUNT=1) */
#include "./aig_dump.h"      /* 検証: env AIG_DUMP で検出回路をAIGER出力(既定無効) */
#include "./xid/XID.h"
#include "./gt_verify.h"     /* 検証: env GT_BDD=1 で厳密照合（既定無効） */
#include "./cnf_dump.h"      /* 検証: env DUMP_CNF で検出CNFをDIMACS出力（既定無効） */
#include "./cube_trend.h"    /* 検証: env CUBE_TREND=1 でキューブ列の傾向観察（既定無効） */
#include "./experiment.h"    /* 研究: env MAXDC / MAXHAM（既定無効） */

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
 *  EXPERIMENT (env XID_EXTERNAL=<bin>): 旧・外部実行体XID(Miyase2004)で
 *  ドントケアを埋める。現行 InlineXID(故障値考慮) との「X判定単体効果」を
 *  比較するため、モデル/ソルバ/回路/limit/ブロッキング帰還を固定したままX判定だけ差替え。
 *  返り値は InlineXID と同じ malloc 済み char[n_pi+1]（'0'/'1'/'X'）。
 *  PINファイル（pi[] 順の信号名）は初回のみ書き出してキャッシュする。
 * ===================================================================== */
static const char* s_xid_pin = NULL;

static char* ExternalXID(CCaDiCaL* solver, FNODE* f)
{
    const char* bin = getenv("XID_EXTERNAL");
    const char* tp  = "./xidext_tp.txt";
    const char* fl  = "./xidext_flist.txt";
    const char* ot  = "./xidext_otx.txt";

    if (!s_xid_pin) {
        FILE* p = fopen("./xidext_pin.txt", "w");
        if (!p) { fprintf(stderr, "ExternalXID: cannot write pin file\n"); exit(1); }
        for (int i = 0; i < n_pi; i++) fprintf(p, "%s\n", pi[i]->name);
        fclose(p);
        s_xid_pin = "./xidext_pin.txt";
    }

    FILE* fp = fopen(tp, "w");
    if (!fp) { fprintf(stderr, "ExternalXID: cannot write tp\n"); exit(1); }
    for (int i = 0; i < n_pi; i++)
        fputc((ccadical_val(solver, (int)pi[i]->varsgc) > 0) ? '1' : '0', fp);
    fputc('\n', fp);
    fclose(fp);

    fp = fopen(fl, "w");
    fprintf(fp, "%s %s\n", (f->type == SF0) ? "SF0" : "SF1", f->name);
    fclose(fp);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "%s -c %s -tx %s -pin %s -flist %s -otx %s -fm SAF -xid YES -m2004 YES > /dev/null 2>&1",
        bin, opt.file.input.net, tp, s_xid_pin, fl, ot);
    if (system(cmd) != 0) fprintf(stderr, "ExternalXID: invocation failed\n");

    char* result = (char*)malloc((size_t)n_pi + 1);
    if (!result) { fprintf(stderr, "ExternalXID: malloc failed\n"); exit(1); }
    fp = fopen(ot, "r");
    if (!fp) { fprintf(stderr, "ExternalXID: cannot read otx\n"); exit(1); }
    int c, k = 0;
    while (k < n_pi && (c = fgetc(fp)) != EOF) {
        if (c == '0' || c == '1') result[k++] = (char)c;
        else if (c == 'X' || c == 'x') result[k++] = 'X';
    }
    fclose(fp);
    while (k < n_pi) result[k++] = 'X';
    result[n_pi] = '\0';
    return result;
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
    // X率計測（env XSTAT=1 のときだけ集計。キューブ全体のXビット率）
    long xstat_bits = 0, xstat_x = 0;

    // 支配流用サマリー用アキュムレータ
    long dom_total_cubes  = 0;
    long dom_seeded_cubes = 0;

	//キューブ分析用ファイルオープン
	if (opt.file.input.cube_analysis != FILE_NOSET) {
		fileOpen(&cube_analysis_fp, opt.file.input.cube_analysis, "w");
	}

	//CUDD初期化
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	//result file open
	fileOpen(&bdd_result, opt.file.output.fdp, "w");
	fprintf(bdd_result, "net_name,f_type,cube_cnt,complete,fdp,seeded_cnt\n");

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

		// 検証(env AIG_DUMP=path): この故障の検出回路を AIGER 出力して即終了。
		// AllSAT-CT ツール（HALL 等）との同一インスタンス比較用。
		const char* aig_path = getenv("AIG_DUMP");
		if (aig_path) {
			AIG_Dump(aig_path, f);
			ccadical_release(solver);
			exit(0);   // DUMP_CNF と同様、単一故障 flist で回す前提
		}

		// 検証(env DUMP_CNF=dir): この故障の検出CNFを DIMACS 出力して即終了。
		// 厳密モデルカウンタで Vi=#SAT を直接数え、キューブ列挙と比較するため。
		const char* dump_dir = getenv("DUMP_CNF");
		if (dump_dir) {
			char path[2048];
			snprintf(path, sizeof(path), "%s/%s_%s.cnf",
			         dump_dir, f->name, (f->type == SF0) ? "sa0" : "sa1");
			cnf_tee_begin(path);
		}

		if (WriteTPGModel(solver, &target) != true) return AFD_ERROR;

		if (dump_dir) {
			cnf_tee_end(cnf.total.vars);
			fprintf(stderr, "[DUMP_CNF] %s_%s -> n_pi=%d vars=%d (Vi=#SAT, FDP=Vi/2^n_pi)\n",
			        f->name, (f->type == SF0) ? "sa0" : "sa1", n_pi, cnf.total.vars);
			ccadical_release(solver);
			exit(0);   // 対象は単一故障flistで回す前提。最初の故障を出して終了
		}

		// 案1(env MAXDC): 素項展開用 非検出オラクル。未設定なら NULL で従来動作
		CCaDiCaL* u_oracle = EXP_MaybeBuildOracle(&target);

		// f のテストキューブを集める集合
		CubeSet cubes;
		cubeset_init(&cubes, (opt.file.input.limit > 0) ? opt.file.input.limit : 30);

		// 部分集合側の故障（subset_faults）のキューブを種＋禁止節として流用する。
		// T(subset) ⊆ T(f) なので、これらは f の正当なテストであり、
		// solver は差分 T(f)\∪T(subset) だけを探索すればよい。
		// MDC_NODOM をセットすると流用を止め、ゼロから完全列挙する（支配解析の検証用）。
		bool nodom = getenv("MDC_NODOM");
		int seeded_cnt = 0;
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
				seeded_cnt += src->cubes.n;
			}

			// この親で src のキューブを使い切る。最後の消費者ならここで解放
			if (--src->n_pending == 0)
				cubeset_free(&src->cubes);
		}

		if (opt.file.input.cube_analysis != FILE_NOSET) {
			fprintf(cube_analysis_fp, "%s", f->name);
			fprintf(cube_analysis_fp, (f->type == SF0) ? ",sa0" : ",sa1");
		}

		// 案2(env MAXHAM): 多様化の参照(前回キューブ)と aux 採番器を故障ごとに初期化
		char* prev_cube = NULL;
		EXP_ResetPerFault();

		// 案3(env DUAL): 双対列挙を初期化（実体の構築は V 側の初回起動まで遅延）
		bool dual_fin = false;
		EXP_DualInit(gbm, &target);

		// limit <= 0 は「上限なし（無制限）」を意味し、UNSAT まで完全列挙する
		bool unlimited = (opt.file.input.limit <= 0);

		// UNSAT・limit 到達・双対列挙の完了 のいずれかでテスト生成を終了する
		while (1) {
            int res;
            if (dual_fin) {
                // 案3: 双対列挙で U=D_f が確定済み。det 側の追加 solve は不要（UNSAT と同じ完了処理へ）
                res = 20;
            } else {
                t_start = clock();
                res = EXP_Solve(solver, prev_cube);   // MAXHAM 未設定なら素の solve
                t_end   = clock();
                time_cadical += ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
            }

            if (res == 20 || (!unlimited && cubes.n >= opt.file.input.limit)) {
                bool limit_hit = (!unlimited && cubes.n >= opt.file.input.limit && res != 20);

                // 案3(env DUAL): det 打ち切り後に V 側だけ回すドレインで完了を狙う
                if (limit_hit && EXP_DualDrain(&cubes))
                    limit_hit = false;

                // 案4(env SPLIT=1): 打ち切りを Shannon 分割で完全列挙まで持っていけたら
                // complete に昇格する（cubes の和集合 = D_f になる。未設定なら何もしない）
                if (limit_hit && EXP_SplitFinish(gbm, solver, u_oracle, &cubes, &target))
                    limit_hit = false;

				if (opt.file.input.cube_analysis != FILE_NOSET) {
					fprintf(cube_analysis_fp, "\n");
				}

                dom_total_cubes  += cubes.n;
                dom_seeded_cubes += seeded_cnt;

                t_start = clock();
                // 打ち切り故障の厳密化フォールバック（どちらも complete=1 で報告、
                // キューブは部分被覆のまま残り、支配流用・DropDeteFault にそのまま使える）:
                //   PCOUNT=1    PODEM型入力空間探索＋メモ化による厳密数え上げ（案5）
                //   BDD_EXACT=1 検出関数 D_f の直接BDD構築（従来手法・検証/参照値用）
                char* exact_cnt = NULL;
                if (limit_hit && getenv("PCOUNT"))                 exact_cnt = PC_ExactCountStr(f);
                if (limit_hit && !exact_cnt && getenv("BDD_EXACT")) exact_cnt = GT_ExactCountStr(f);
                if (exact_cnt) {
                    calculate_prob_with_gmp(exact_cnt, n_pi, bdd_result, NULL, &target,
                                            cubes.n, seeded_cnt, false);
                    free(exact_cnt);
                } else {
                    RunBDD(gbm, n_pi, cubes.data, cubes.n, bdd_result, NULL, &target, cubes.n, seeded_cnt, limit_hit);
                }
                t_end   = clock();
                time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				// 検証(env GT_BDD=1): 回路から直接構築した検出関数とキューブ和集合を厳密照合
				GT_Check(f, &cubes, limit_hit);

				// 検証(env CUBE_TREND=1): キューブ列の X 数・マスク重複など生成傾向を観察
				CT_Report(f, &cubes, limit_hit);

				// 案1: capped 故障の集計とオラクル解放（NULL なら何もしない）
				EXP_OracleDone(&u_oracle, limit_hit);

				// 案3: 双対列挙の集計と資源解放（打ち切り時は fdp の上下界も報告）
				EXP_DualDone(f, limit_hit);

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
                char* x_pattern = getenv("XID_EXTERNAL")
                                    ? ExternalXID(solver, f)
                                    : InlineXID(solver, f->netptr, EXP_PreferredPONet());
                t_end   = clock();
                time_xid += (double)(t_end - t_start) / CLOCKS_PER_SEC;

				EXP_Expand(u_oracle, x_pattern);  // 案1: キューブを素項へ拡大（in place）

				AddBlockingClauseFromCube(solver, x_pattern);
				cubeset_push(&cubes, x_pattern);
				prev_cube = x_pattern;   // 案2: 次回 solve の多様化参照

				// 案3: 非検出側を1本進め、U∪V の閉包で完了を判定（DUAL 未設定なら常に false）
				dual_fin = EXP_DualStep(&cubes, x_pattern);

				// X率計測: このキューブのXビット数を集計
				for (int xi = 0; xi < n_pi; xi++) if (x_pattern[xi] == 'X') xstat_x++;
				xstat_bits += n_pi;

                if (opt.file.input.cube_analysis != FILE_NOSET) {
                    t_start = clock();
                    RunBDD(gbm, n_pi, cubes.data, cubes.n, NULL, cube_analysis_fp, &target, cubes.n, 0, false);
                    t_end   = clock();
                    time_bdd += (double)(t_end - t_start) / CLOCKS_PER_SEC;
                }
			}
		}
		ccadical_release(solver);
	}

	// ===== 支配流用サマリー =====
	{
		long sat_calls = dom_total_cubes - dom_seeded_cubes;
		double reduction = dom_total_cubes > 0
			? 100.0 * dom_seeded_cubes / dom_total_cubes : 0.0;
		printf("\n[DOM] total_cubes=%ld  seeded=%ld  sat_calls=%ld  reduction=%.1f%%\n",
			dom_total_cubes, dom_seeded_cubes, sat_calls, reduction);
	}

	// ===== CPU time =====
    *out_time_cadical = time_cadical;
    *out_time_bdd     = time_bdd;
    *out_time_xid     = time_xid;
    *out_time_read    = time_read;

	// X率サマリー（env XSTAT=1 のときだけ。比較用の一時計装）
	if (getenv("XSTAT"))
		fprintf(stderr, "[XSTAT] xid=%s cubes_bits=%ld x_bits=%ld x_ratio=%.4f\n",
		        getenv("XID_EXTERNAL") ? "external" : "inline",
		        xstat_bits, xstat_x,
		        xstat_bits ? (double)xstat_x / (double)xstat_bits : 0.0);

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

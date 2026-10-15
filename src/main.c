#define _POSIX_C_SOURCE 200809L
//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "io/report.h"
#include "cli/options.h"
#include "netlist/netlist.h"
#include "fdp/analysis.h"

#include "fdp/worker_pool.h"


//*************************************************************************************************************
//	@name		main
//	@function	main
//	@return		(void)
//*************************************************************************************************************
int main(
	int					  argc,				 /**< コマンド引数の数 */
	char** argv								 /**< コマンド引数 */
)
{
	struct timespec wall_start, wall_end;
	clock_t cpu_start, cpu_end; // CPU時間計測用

	// CPU時間受け取り用
	double time_cadical = 0.0;
	double time_bdd     = 0.0;
	double time_xid     = 0.0;
	double time_read    = 0.0;

	clock_gettime(CLOCK_MONOTONIC, &wall_start);
	cpu_start = clock(); // CPU時間の計測開始

	// オプションを設定する
	if (OPT(argc, argv) != OPT_OKAY) return EXIT_FAILURE;

	/** ネットリストを読み込む */
	read_nl(opt.file.input.net);

	/** TDF モード：順序回路（DFF入り）を LOC 方式の2時刻展開に組み替える */
	if (opt.fault_model == FM_TDF)
	{
		if (n_rdff + n_dffs + n_rdffs > 0)
		{
			printf("\n	COMMAND ERROR: -tdf は素の DFF のみ対応です。");
			printf("RDFF/DFFS/RDFFS を含む回路は指定できません。\n\n");
			return EXIT_FAILURE;
		}
		if (n_dff == 0)
		{
			printf("\n	COMMAND ERROR: -tdf には順序回路が必要です。");
			printf("DFF を含まない回路が指定されました。順序回路にしてください。\n\n");
			return EXIT_FAILURE;
		}
		expand_tdf_netlist();
	}

	// 故障検出確率を算出する
	if (AnalyzeFaultDensity(&time_cadical, &time_bdd, &time_xid, &time_read) != AFD_OKAY) return EXIT_FAILURE;

	clock_gettime(CLOCK_MONOTONIC, &wall_end);
	cpu_end = clock(); // CPU時間の計測終了

	// CPU時間を計算
	double cpu_time = (double)(cpu_end - cpu_start) / CLOCKS_PER_SEC + FaultPoolChildCPU();
    double wall_time = wall_end.tv_sec - wall_start.tv_sec +
                       (wall_end.tv_nsec - wall_start.tv_nsec) / 1e9;

	bool log_okay = OutLogfile(wall_time, cpu_time, time_cadical, time_bdd, time_xid, time_read);

	free_netlist();

	return log_okay ? EXIT_SUCCESS : EXIT_FAILURE;
}

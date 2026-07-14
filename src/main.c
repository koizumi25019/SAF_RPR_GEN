//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./main.h"
#include "./opt/opt.h"
#include "./netlist/netlist.h"
#include "./fdp/fault_detection_prob.h"
#include "./lib/lib.h"


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
	clock_t cpu_start, cpu_end; // CPU時間計測用

	// CPU時間受け取り用
	double time_cadical = 0.0;
	double time_bdd     = 0.0;
	double time_xid     = 0.0;
	double time_read    = 0.0;

	cpu_start = clock(); // CPU時間の計測開始

	// オプションを設定する
	if (OPT(argc, argv) != OPT_OKAY) return RETCODE_ERROR;

	/** ネットリストを読み込む */
	read_nl(opt.file.input.net);

	// 故障検出確率を算出する
	if (AnalyzeFaultDensity(&time_cadical, &time_bdd, &time_xid, &time_read) != AFD_OKAY) return RETCODE_ERROR;

	cpu_end = clock(); // CPU時間の計測終了

	// CPU時間を計算
	double cpu_time = (double)(cpu_end - cpu_start) / CLOCKS_PER_SEC;

	OutLogfile(cpu_time, time_cadical, time_bdd, time_xid, time_read);

	free_netlist();

	return 0;
}

//*************************************************************************************************************
//	@name		OutLogfile
//	@function	output the log
//	@return		(bool) okay, error
//*************************************************************************************************************
static void WriteReport(
	FILE* fp,
	double cpu_time,
	double time_cadical,
	double time_bdd,
	double time_xid,
	double time_read
)
{
	fprintf(fp, "//--------------------------------------------------------------------------------\n");
	fprintf(fp, "//                AnalyzeFaultDetectionProbability Information\n");
	fprintf(fp, "//--------------------------------------------------------------------------------\n");
	fprintf(fp, "//  Target Circuit                            : %s\n", net_name);
	fprintf(fp, "//  Name of Target Fault File                 : %s\n", opt.file.input.fault);
	fprintf(fp, "//  Number of Target Faults                   : %d\n", readdata.fault.numinit);
	fprintf(fp, "//  CPU Time                                  : %.3f sec\n", cpu_time);
	fprintf(fp, "//  CPU Time (CaDiCaL)                        : %.3f sec\n", time_cadical);
	fprintf(fp, "//  CPU Time (BDD)                            : %.3f sec\n", time_bdd);
	fprintf(fp, "//  CPU Time (Don't care)                     : %.3f sec\n", time_xid);
	fprintf(fp, "//  CPU Time (Read Fault)                     : %.3f sec\n", time_read);
	fprintf(fp, "//--------------------------------------------------------------------------------\n");
}

void OutLogfile(
	double cpu_time,
	double time_cadical,
	double time_bdd,
	double time_xid,
	double time_read
)
{
	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, opt.file.output.log, "w");
	WriteReport(fileptr, cpu_time, time_cadical, time_bdd, time_xid, time_read);

	printf("\n\n");
	WriteReport(stdout, cpu_time, time_cadical, time_bdd, time_xid, time_read);

	return;
}

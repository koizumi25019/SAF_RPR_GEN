#pragma once
#include "read.h"

/* 1故障で増えた集計値。並列では子が返信し、親がFaultStatsAddで合計する。
 * cadical/bdd/xidはCPU秒であり、実経過秒ではない。
 */
typedef struct {
    double cadical;
    double bdd;
    double xid;
    long total_cubes;
    long seeded_cubes;
    long xstat_bits;
    long xstat_x;
    long gt_checked;
    long gt_unsound;
    long gt_inexact;
} FaultStats;

/* 1故障の全処理: CNF→SAT・キューブ列挙→XID/CORE→BDD・FDP→検証。
 * 直列と各ワーカーで同じ関数を使う。
 * 並列時のcsv_output/analysis_outputは子専用メモリストリーム。
 * worker_contextはfork後の各プロセスに独立した作業領域となる。
 */
typedef bool (*FaultTask)(FNODE* fault, int fault_number,
                          FILE* csv_output, FILE* analysis_output,
                          FaultStats* stats, void* worker_context);
/* 同じワーカーで全担当故障を処理し終えた後の解放・診断。 */
typedef void (*FaultFinish)(void* worker_context);

/* 故障順を一度決める。従来SetTargetと同じlevel→ハッシュ走査順。
 * 戻り値の配列は呼出側がfreeする。FNODE本体の所有権は移さない。
 */
FNODE** FaultPoolOrder(int* count);
void FaultStatsAdd(FaultStats* sum, const FaultStats* part);

/* 親が配分・出力・回収を担当し、子がtaskを実行する。falseは処理失敗。
 * faults/context/出力FILEは呼出側が所有する。内部の通信・結果バッファはここで解放。
 */
bool FaultPoolRun(FNODE** faults, int count, int jobs, FaultTask task,
                  FaultFinish finish, void* context, FILE* csv, FILE* analysis,
                  FaultStats* stats);
/* 回収した全ワーカーのuser+system CPU秒。親のCPU秒はmain側で加算する。 */
double FaultPoolChildCPU(void);
int FaultPoolWorkers(void);

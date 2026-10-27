#pragma once
#include "fault/fault_list.h"
#include "io/cube_text.h"

/* 1故障のCPU秒・キューブ数。並列では親が全ワーカー分を合計する。 */
typedef struct {
    double cadical;
    double bdd;
    double xid;
    long total_cubes;
    long seeded_cubes;
} FaultStats;

/* 直列とワーカーで共通の1故障処理。並列時の出力先は子専用メモリストリーム。 */
typedef bool (*FaultTask)(FNODE* fault, int fault_number, FILE* csv_output,
                          FILE* cube_output, FaultStats* stats, void* worker_context);
typedef void (*FaultFinish)(void* worker_context);

void FaultStatsAdd(FaultStats* sum, const FaultStats* part);
/* 親が配分・ファイル出力・回収、子がtaskを担当する。渡した配列・出力情報の所有権は移さない。 */
bool FaultPoolRun(FNODE** faults, int count, int jobs, FaultTask task,
                  FaultFinish finish, void* context, FILE* csv, CubeOutput* cubes,
                  FaultStats* stats);
/* 回収済みの全ワーカーのuser+system CPU秒。親のCPU秒はmainで加算する。 */
double FaultPoolChildCPU(void);
int FaultPoolWorkers(void);

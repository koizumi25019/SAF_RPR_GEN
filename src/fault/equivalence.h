#pragma once

#include "fault/fault_list.h"

/** CSVへ出力する故障。リストは初期化完了後から解放まで読み取り専用。 */
typedef struct EquivalentFault {
    NLIST* net;
    int type;
    const struct EquivalentFault* next;
} EquivalentFault;

typedef struct FaultEquivalenceStats {
    size_t input_pairs;
    int targets_before_external;
    int targets_after_external;
    int output_faults;
} FaultEquivalenceStats;

/** 全故障の集合と構造による等価関係を作り、自動生成用のtest_sa0/1を設定する。 */
bool InitFaultEquivalence(void);

/** 外部ペアを統合し、計算対象を代表故障だけに絞って出力メンバーを確定する。 */
bool FinalizeFaultEquivalence(void);

const EquivalentFault* GetEquivalentFaults(const FNODE* representative);
const FaultEquivalenceStats* GetFaultEquivalenceStats(void);
void FreeFaultEquivalence(void);

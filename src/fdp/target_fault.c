//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "./target_fault.h"

static FNODE** target_order = NULL;
static size_t target_count = 0;
static size_t target_cursor = 0;

void FreeTargetOrder(void)
{
    free(target_order);
    target_order = NULL;
    target_count = 0;
    target_cursor = 0;
}

bool InitTargetOrder(void)
{
    FreeTargetOrder();

    size_t count = 0;
    for (int i = 0; i < MAXSIZE_HASH; i++)
        for (FNODE* p = readdata.fault.list[i]; p != NULL; p = p->nextptr)
            count++;

    if (count == 0) return true;

    target_order = malloc(count * sizeof(*target_order));
    if (target_order == NULL) {
        fprintf(stderr, "ERROR: failed to allocate target selection order.\n");
        return false;
    }

    size_t rank = 0;
    for (int i = 0; i < MAXSIZE_HASH; i++) {
        for (FNODE* p = readdata.fault.list[i]; p != NULL; p = p->nextptr) {
            target_order[rank] = p;
            rank++;
        }
    }

    target_count = count;
    return true;
}


//*************************************************************************************************************
//	@name		F@SetTarget
//	@function	F	単一の対象故障を選択する
//	@return		F	(FNODE*) 対象故障、なければ NULL
//*************************************************************************************************************
FNODE* SetTarget(void)
{
    while (target_cursor < target_count) {
        FNODE* target = target_order[target_cursor];
        /* 処理済みになるまでは同じ対象を返す。レベルでは並べ替えない。 */
        if (target->detect == UNDETECTED)
            return target;
        target_cursor++;
    }

    return NULL;
}

/* 並列でも同じ一覧順を使い、選択ロジックを二重に実装しない。 */
FNODE** CopyTargetOrder(int* count)
{
    *count = 0;
    for (size_t index = 0; index < target_count; index++) {
        FNODE* fault = target_order[index];
        if (fault->detect != UNDETECTED) continue;
        (*count)++;
    }
    FNODE** faults = malloc((size_t)(*count ? *count : 1) * sizeof(*faults));
    if (!faults) return NULL;
    int copied = 0;
    for (size_t index = 0; index < target_count; index++) {
        FNODE* fault = target_order[index];
        if (fault->detect == UNDETECTED) faults[copied++] = fault;
    }
    return faults;
}

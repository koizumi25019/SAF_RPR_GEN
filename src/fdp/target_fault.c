//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "./target_fault.h"

typedef struct {
    FNODE* fault;
    size_t rank;  /* 同レベルでは元のハッシュ表の走査順を維持する */
} TargetEntry;

static TargetEntry* target_order = NULL;
static size_t target_count = 0;
static size_t target_cursor = 0;

static int CompareTarget(const void* lhs, const void* rhs)
{
    const TargetEntry* a = lhs;
    const TargetEntry* b = rhs;
    int a_level = a->fault->netptr->level;
    int b_level = b->fault->netptr->level;

    if (a_level != b_level) return (a_level > b_level) ? 1 : -1;
    return (a->rank > b->rank) - (a->rank < b->rank);
}

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
            target_order[rank] = (TargetEntry){.fault = p, .rank = rank};
            rank++;
        }
    }

    qsort(target_order, count, sizeof(*target_order), CompareTarget);
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
        FNODE* target = target_order[target_cursor].fault;
        /* 処理済みになるまでは同じ対象を返す。INT_MAX は旧実装でも対象外。 */
        if (target->detect == UNDETECTED && target->netptr->level < INT_MAX)
            return target;
        target_cursor++;
    }

    return NULL;
}

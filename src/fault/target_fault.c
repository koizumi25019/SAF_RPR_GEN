//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "fault/target_fault.h"

typedef struct {
    FNODE* fault;
    size_t rank;  /* 同レベルでは元のハッシュ表の走査順を維持する */
} TargetEntry;

static TargetEntry* target_order = NULL;
static size_t target_count = 0;

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


/* 直列・並列で同じ整列順を使い、選択ロジックを二重に実装しない。 */
FNODE** CopyTargetOrder(int* count)
{
    *count = (int)target_count;
    FNODE** faults = malloc((target_count ? target_count : 1) * sizeof(*faults));
    if (!faults) return NULL;
    for (size_t index = 0; index < target_count; index++) faults[index] = target_order[index].fault;
    return faults;
}

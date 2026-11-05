#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "fault/target_fault.h"

enum { COUNT = 1000 };

/* 整列とは独立した全走査を選択順のオラクルとして使う。
 * 未選択の故障のうち最小レベル、同レベルでは故障表の走査で最初のものを選ぶ。 */
static FNODE* scan_target(const FNODE* faults, const bool* selected)
{
    FNODE* best = NULL;
    for (int i = 0; i < MAXSIZE_HASH; i++) {
        for (FNODE* f = readdata.fault.list[i]; f; f = f->nextptr) {
            if (selected[f - faults]) continue;
            if (best == NULL || f->netptr->level < best->netptr->level) best = f;
        }
    }
    return best;
}

static void check_order(void)
{
    static FNODE faults[COUNT];
    static NLIST nets[COUNT];
    memset(faults, 0, sizeof(faults));
    memset(nets, 0, sizeof(nets));
    memset(&readdata, 0, sizeof(readdata));

    unsigned state = 42;
    for (int i = 0; i < COUNT; i++) {
        state = state * 1664525u + 1013904223u;
        int bucket = (state >> 16) % MAXSIZE_HASH;
        nets[i].level = (state >> 8) % 17;  /* 多数の同レベルとバケット内衝突 */
        faults[i].netptr = &nets[i];
        faults[i].nextptr = readdata.fault.list[bucket];
        readdata.fault.list[bucket] = &faults[i];
    }
    nets[1].level = INT_MAX;      /* 極端なレベルでも比較が溢れない */
    nets[2].level = INT_MAX - 1;

    /* 再初期化でも旧配列を持ち越さない。 */
    for (int pass = 0; pass < 2; pass++) {
        assert(InitTargetOrder());
        int count = -1;
        FNODE** order = CopyTargetOrder(&count);
        assert(order != NULL && count == COUNT);
        bool selected[COUNT] = {false};
        for (int index = 0; index < count; index++) {
            FNODE* expected = scan_target(faults, selected);
            assert(order[index] == expected);
            selected[expected - faults] = true;
        }
        assert(scan_target(faults, selected) == NULL);
        free(order);
    }

    FreeTargetOrder();
    FreeTargetOrder();
    memset(&readdata, 0, sizeof(readdata));
}

static void check_empty(void)
{
    assert(InitTargetOrder());
    int count = -1;
    FNODE** order = CopyTargetOrder(&count);
    assert(order != NULL && count == 0);
    free(order);
    FreeTargetOrder();
}

int main(void)
{
    check_empty();
    check_order();
    check_empty();
    return 0;
}

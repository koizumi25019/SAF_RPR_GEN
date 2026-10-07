#include <assert.h>
#include <limits.h>
#include <string.h>
#include "target_fault.h"

/* 整列とは独立した、変更前の全走査を選択順のオラクルとして使う。 */
static FNODE* scan_target(void)
{
    FNODE* best = NULL;
    int level = INT_MAX;
    for (int i = 0; i < MAXSIZE_HASH; i++) {
        for (FNODE* f = readdata.fault.list[i]; f; f = f->nextptr) {
            if (f->detect == UNDETECTED && f->netptr->level < level) {
                best = f;
                level = f->netptr->level;
            }
        }
    }
    return best;
}

static void check_selection(void)
{
    enum { COUNT = 1000 };
    FNODE faults[COUNT] = {0};
    NLIST nets[COUNT] = {0};
    memset(&readdata, 0, sizeof(readdata));

    unsigned state = 42;
    for (int i = 0; i < COUNT; i++) {
        state = state * 1664525u + 1013904223u;
        int bucket = (state >> 16) % MAXSIZE_HASH;
        nets[i].level = (state >> 8) % 17;  /* 多数の同レベルとバケット内衝突 */
        faults[i].netptr = &nets[i];
        faults[i].detect = (i % 11 == 0) ? DETECTED : UNDETECTED;
        faults[i].nextptr = readdata.fault.list[bucket];
        readdata.fault.list[bucket] = &faults[i];
    }
    nets[1].level = INT_MAX;
    nets[2].level = INT_MAX - 1;

    /* 再初期化でも旧配列やカーソルを持ち越さない。 */
    for (int pass = 0; pass < 2; pass++) {
        assert(InitTargetOrder());
        int selected = 0;
        FNODE* expected;
        while ((expected = scan_target()) != NULL) {
            assert(SetTarget() == expected);
            assert(SetTarget() == expected);  /* マーク前には同じ対象 */
            expected->detect = DETECTED;
            selected++;
            /* 先の対象が別の処理で終了していてもスキップする。 */
            if (selected % 7 == 0) faults[(selected * 13) % COUNT].detect = DETECTED;
        }
        assert(selected > 0);
        assert(SetTarget() == NULL);
        assert(SetTarget() == NULL);
        for (int i = 0; i < COUNT; i++) faults[i].detect = UNDETECTED;
    }

    FreeTargetOrder();
    FreeTargetOrder();
    assert(SetTarget() == NULL);
    memset(&readdata, 0, sizeof(readdata));
}

int main(void)
{
    assert(InitTargetOrder());
    assert(SetTarget() == NULL);
    FreeTargetOrder();
    check_selection();
    assert(InitTargetOrder());
    assert(SetTarget() == NULL);
    FreeTargetOrder();
    return 0;
}

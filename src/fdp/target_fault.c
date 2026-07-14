//-------------------------------------------------------------------------------------------------------------
//	インクルード
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#include "./target_fault.h"
#include "./read.h"
#include "../lib/lib.h"


//*************************************************************************************************************
//	@name		F@SetTarget
//	@function	F	対象故障リストを設定する
//	@return		F	(bool) 正常, 異常
//*************************************************************************************************************
bool SetTarget(TARGET* target)
{
    FNODE* best       = (FNODE*)NULL;
    int    best_level = INT_MAX;

    for (int i = 0; i < MAXSIZE_HASH; i++)
    {
        for (FNODE* p = readdata.fault.list[i]; p != NULL; p = p->nextptr)
        {
            if (p->detect == UNDETECTED && p->netptr->level < best_level)
            {
                best_level = p->netptr->level;
                best       = p;
            }
        }
    }

    if (best == (FNODE*)NULL) return TARGET_ERROR;

    target->num     = 1;
    target->list    = (FNODE**)allocMemory(1, sizeof(FNODE*));
    target->list[0] = best;
    return TARGET_OKAY;
}
//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "./target_fault.h"
#include "./read.h"
#include "../lib/lib.h"


//*************************************************************************************************************
//	@name		F@SetTarget
//	@function	F	set the target-fault list
//	@return		F	(bool) okay, error
//*************************************************************************************************************
bool SetTarget(TARGET* target)
{
    for (int i = 0; i < MAXSIZE_HASH; i++)
    {
        for (FNODE* p = readdata.fault.list[i]; p != NULL; p = p->nextptr)
        {
            if (p->detect == UNDETECTED)
            {
                target->num     = 1;
                target->list    = (FNODE**)allocMemory(1, sizeof(FNODE*));
                target->list[0] = p;
                return TARGET_OKAY;
            }
        }
    }
    return TARGET_ERROR;
}
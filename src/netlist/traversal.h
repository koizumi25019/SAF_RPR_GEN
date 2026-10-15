#pragma once
#include "netlist/netlist.h"
#define MAXSIZE_STACK 100
#define MAXSIZE_QUEUE 100
#define QUE_MODE_ONE 1
#define QUE_MODE_ALLIN 2
#define QUE_MODE_ALLOUT 3
#define POSSIBLE 1
#define UNPOSSIBLE 0
typedef struct Stack { int maxnum; int ptr; NLIST** stk; } STACK;
typedef struct Queue { int maxnum; int num; int front; int rear; NLIST** que; } QUE;
extern STACK stack;
extern QUE que;

void stackPUSH(NLIST* netptr);
NLIST* stackPOP(void);
void queENQ(NLIST* netptr, int mode);
NLIST* queDEQ(void);

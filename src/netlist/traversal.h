#pragma once
#include "netlist/netlist.h"
#define MAXSIZE_STACK 100
#define MAXSIZE_QUEUE 100
typedef struct Stack { int maxnum; int ptr; NLIST** stk; } STACK;
typedef struct Queue { int maxnum; int num; int front; int rear; NLIST** que; } QUE;
extern STACK stack;
extern QUE que;

void stackPUSH(NLIST* netptr);
NLIST* stackPOP(void);
void queENQ(NLIST* netptr);
NLIST* queDEQ(void);

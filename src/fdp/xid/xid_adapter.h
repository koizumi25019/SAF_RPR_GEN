#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

/* ASG netlist: provides NLIST, nl, n_net, pi, n_pi, po, n_po, etc. */
#include "netlist/netlist.h"

/* CaDiCaL SAT solver */
#include "ccadical.h"

/* NLIST_t: alias for struct _Netlist_Format_ (same as ASG's NLIST) */
typedef struct _Netlist_Format_ NLIST_t;

/* Gate type constants */
#define IN_G          IN   /* IN=0 in netlist.h */
#define MAX_GATE_TYPE 27   /* ACC=26 is highest gate type */

/* PRId8 fallback (node->type is int in ASG, not int8_t) */
#ifndef PRId8
#define PRId8 "d"
#endif

/* ------------------------------------------------------------------ */
/* Queue                                                               */
/* ------------------------------------------------------------------ */
typedef struct XID_Queue {
    void**  items;
    size_t  front;
    size_t  rear;
    size_t  count;
    size_t  capacity;
} Queue_t;

extern Queue_t* createQueue(size_t capacity);
extern void     destroyQueue(Queue_t* q);
extern void     enqueue(Queue_t* q, void* item);
extern void*    dequeue(Queue_t* q);
extern int      isQueueEmpty(const Queue_t* q);

/* ------------------------------------------------------------------ */
/* Level stack (replaces CompletionNetlist level stack)                */
/* ------------------------------------------------------------------ */
typedef size_t vsize_t;
#define VSIZE_INVALID SIZE_MAX

typedef struct LevStack {
    NLIST_t** net;
    size_t    n_net;
} LEV;

extern LEV*   xid_lev;
extern size_t xid_max_level;

extern void     xid_lev_init(void);
extern void     xid_lev_free(void);
extern size_t   get_total_net_count(void);
extern void     push_lev_net(NLIST_t* net);
extern NLIST_t* pop_lev_net(size_t lev);
extern size_t   check_lev(size_t lev);
extern vsize_t  check_lev_reverse(vsize_t lev);
extern void     Reset_lev_tmp(size_t event_lev);

/* ------------------------------------------------------------------ */
/* Memory helpers                                                      */
/* ------------------------------------------------------------------ */
static inline void* xid_alloc_mem(size_t size) {
    void* p = malloc(size);
    if (!p) { fprintf(stderr, "XID: malloc failed\n"); exit(1); }
    return p;
}
static inline void* xid_alloc_con(size_t count, size_t size) {
    void* p = calloc(count, size);
    if (!p) { fprintf(stderr, "XID: calloc failed\n"); exit(1); }
    return p;
}
#define ALLOC_MEM(sz)       xid_alloc_mem(sz)
#define ALLOC_CON(cnt, sz)  xid_alloc_con((cnt), (sz))
#define FREE(x)             (free(x), (x) = NULL)

/* ------------------------------------------------------------------ */
/* Debug helpers                                                        */
/* ------------------------------------------------------------------ */
#define ERROR_PRINTF(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define DEBUG_PRINTF(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)  ((void)0)
#endif
#define DEBUG_ASSERT(cond)      assert(cond)

/* ------------------------------------------------------------------ */
/* InlineXID: PI don't-care filling (replaces external XID call)       */
/* Returns malloc'd char[] of length n_pi+1: '0','1','X' per PI, '\0' */
/* Caller must free() the returned string.                             */
/* Also adds the blocking clause to solver.                            */
/* ------------------------------------------------------------------ */
extern char* InlineXID(CCaDiCaL* solver, NLIST* fault_net);

#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

/* ASG ネットリスト：NLIST, nl, n_net, pi, n_pi, po, n_po などを提供する */
#include "netlist/netlist.h"

/* CaDiCaL SATソルバ */
#include "ccadical.h"

/* NLIST_t: struct _Netlist_Format_ の別名（ASG の NLIST と同じ） */
typedef struct _Netlist_Format_ NLIST_t;

/* ゲートタイプ定数 */
#define IN_G          IN   /* netlist.h では IN=0 */
#define MAX_GATE_TYPE 27   /* ACC=26 が最大のゲートタイプ */

/* PRId8 のフォールバック（ASG では node->type は int8_t ではなく int） */
#ifndef PRId8
#define PRId8 "d"
#endif

/* ------------------------------------------------------------------ */
/* キュー                                                              */
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
extern void     resetQueue(Queue_t* q);

/* ------------------------------------------------------------------ */
/* レベルスタック（CompletionNetlist のレベルスタックの代替）         */
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
/* メモリ確保ヘルパー                                                  */
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
/* デバッグヘルパー                                                    */
/* ------------------------------------------------------------------ */
#define ERROR_PRINTF(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define DEBUG_PRINTF(fmt, ...)  fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)  ((void)0)
#endif
#define DEBUG_ASSERT(cond)      assert(cond)

/* InlineXID のプロトタイプは XID.h 側にある（重複宣言しない） */

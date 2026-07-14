#include "xid_adapter.h"

/* ================================================================== */
/* キュー                                                              */
/* ================================================================== */

Queue_t* createQueue(size_t capacity) {
    Queue_t* q = malloc(sizeof(struct XID_Queue));
    if (!q) { fprintf(stderr, "createQueue: malloc failed\n"); exit(1); }
    q->items = malloc(capacity * sizeof(void*));
    if (!q->items) { fprintf(stderr, "createQueue: malloc items failed\n"); exit(1); }
    q->front    = 0;
    q->rear     = 0;
    q->count    = 0;
    q->capacity = capacity;
    return q;
}

void destroyQueue(Queue_t* q) {
    if (q) { free(q->items); free(q); }
}

void enqueue(Queue_t* q, void* item) {
    if (q->count >= q->capacity) {
        fprintf(stderr, "enqueue: queue overflow (cap=%zu)\n", q->capacity);
        exit(1);
    }
    q->items[q->rear] = item;
    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
}

void* dequeue(Queue_t* q) {
    if (q->count == 0) return NULL;
    void* item  = q->items[q->front];
    q->front    = (q->front + 1) % q->capacity;
    q->count--;
    return item;
}

int isQueueEmpty(const Queue_t* q) {
    return q->count == 0;
}

void resetQueue(Queue_t* q) {
    q->front = 0;
    q->rear  = 0;
    q->count = 0;
}

/* ================================================================== */
/* レベルスタック                                                      */
/* ================================================================== */

LEV*   xid_lev       = NULL;
size_t xid_max_level = 0;
static size_t xid_lev_total = 0;

void xid_lev_init(void) {
    xid_max_level = 0;
    for (int i = 0; i < n_net; i++) {
        if ((size_t)nl[i].level > xid_max_level)
            xid_max_level = (size_t)nl[i].level;
    }

    xid_lev = calloc(xid_max_level + 1, sizeof(LEV));
    if (!xid_lev) { fprintf(stderr, "xid_lev_init: calloc failed\n"); exit(1); }

    for (size_t i = 0; i <= xid_max_level; i++) {
        xid_lev[i].net = malloc((size_t)n_net * sizeof(NLIST_t*));
        if (!xid_lev[i].net) { fprintf(stderr, "xid_lev_init: net malloc failed\n"); exit(1); }
        xid_lev[i].n_net = 0;
    }
    xid_lev_total = 0;
}

void xid_lev_free(void) {
    if (xid_lev) {
        for (size_t i = 0; i <= xid_max_level; i++)
            free(xid_lev[i].net);
        free(xid_lev);
        xid_lev = NULL;
    }
    xid_lev_total = 0;
}

size_t get_total_net_count(void) {
    return xid_lev_total;
}

void push_lev_net(NLIST_t* net) {
    size_t lev = (size_t)net->level;
    xid_lev[lev].net[xid_lev[lev].n_net++] = net;
    xid_lev_total++;
}

NLIST_t* pop_lev_net(size_t lev) {
    if (xid_lev[lev].n_net == 0) return NULL;
    NLIST_t* net = xid_lev[lev].net[--xid_lev[lev].n_net];
    xid_lev_total--;
    return net;
}

size_t check_lev(size_t lev) {
    while (lev <= xid_max_level && xid_lev[lev].n_net == 0) lev++;
    return lev;
}

vsize_t check_lev_reverse(vsize_t lev) {
    while (lev > 0 && xid_lev[lev].n_net == 0) lev--;
    return lev;
}

void Reset_lev_tmp(size_t event_lev) {
    for (size_t i = event_lev; i <= xid_max_level; i++) {
        xid_lev_total -= xid_lev[i].n_net;
        xid_lev[i].n_net = 0;
    }
}


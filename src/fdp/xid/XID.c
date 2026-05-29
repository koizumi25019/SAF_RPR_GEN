#include "XID.h"

#define DEBUG_XFILL       0
#define DEBUG_XFILL_SHORT 0
#define MODE_XID_LOOP_PO  0

/* forward declarations */
static void ED_push_Xpath(NLIST_t* net, size_t xid_tag_base, XID_VAR_INFO* var_info);

/* -----------------------------------------------------------------------
 * Persistent scratch buffers (allocated once, reused across InlineXID calls)
 * to avoid per-call malloc/free churn. The XID algorithm fully resets
 * var_info each call and drains the level stack / queues to empty, so the
 * buffers are safe to reuse without per-call reallocation.
 * ----------------------------------------------------------------------- */
static XID_VAR_INFO* s_var_info  = NULL;
static size_t*       s_po_id     = NULL;
static Queue_t*      s_fwd_q     = NULL;
static Queue_t*      s_bwd_q     = NULL;
static Queue_t*      s_jus_q     = NULL;
static _Bool         s_xid_ready = 0;

static void xid_ensure_scratch(void) {
    if (s_xid_ready) return;
    init_xid_fpath_table();
    init_xid_bimp_table();
    init_xid_fimp_table();
    init_xid_bimp_limited_table();
    s_var_info = (XID_VAR_INFO*)ALLOC_MEM((size_t)n_net * sizeof(XID_VAR_INFO));
    s_po_id    = (size_t*)ALLOC_CON((size_t)n_po, sizeof(size_t));
    s_fwd_q    = createQueue((size_t)n_net);
    s_bwd_q    = createQueue((size_t)n_net);
    s_jus_q    = createQueue((size_t)n_net);
    xid_lev_init();   /* levels are fixed after ComputeLevels; init once */
    s_xid_ready = 1;
}

/* -----------------------------------------------------------------------
 * xid_fpath: fault propagation path selection (backward from fault site)
 * ----------------------------------------------------------------------- */
typedef void (*xid_fpath_func_t)(Queue_t* imp_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info);

static void xid_fpath_unsupported(Queue_t* imp_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    (void)imp_q; (void)xid_tag; (void)var_info;
    ERROR_PRINTF("Unsupported gate type in XID fault propagation: %s (type)%" PRId8 "\n", node->name, node->type);
    DEBUG_ASSERT(0);
}
static void xid_fpath_skip(Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    (void)bwd_q; (void)node; (void)xid_tag; (void)var_info;
}
static void xid_fpath_buf(Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    (void)bwd_q; (void)xid_tag;
    ED_push_Xpath(node->in[0], xid_tag, var_info);
}

static void gate_common_AND_OR_like(
    Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info,
    int ctrl_value, int invert_output)
{
    int non_ctrl_value = ctrl_value ^ 1;
    vsize_t in_lev   = VSIZE_INVALID;
    vsize_t in_index = VSIZE_INVALID;
    vsize_t cbit_index = VSIZE_INVALID;

    size_t n_in = (size_t)node->n_in;
    for (size_t i = 0; i < n_in; ++i) {
        NLIST_t* in_i = node->in[i];
        vsize_t  in_i_lev = (vsize_t)in_i->level;
        XID_VAR_INFO* in_info = &var_info[in_i->n];

        if (in_info->normal_2value == in_info->fault_2value) continue;

        size_t current_tag = in_info->xid_tag;
        if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }

        if (!(current_tag & XID_FLAG_NORMAL)) {
            if (in_lev > in_i_lev) {
                in_lev   = in_i_lev;
                in_index = i;
            }
        } else {
            cbit_index = i;
            break;
        }
    }

    int n2v = var_info[node->n].normal_2value;
    if (invert_output) n2v ^= 1;
    size_t select_id = (cbit_index == VSIZE_INVALID) ? in_index : cbit_index;

    for (size_t i = 0; i < n_in; ++i) {
        NLIST_t* in_i = node->in[i];
        XID_VAR_INFO* in_info = &var_info[in_i->n];

        if (i == select_id) {
            ED_push_Xpath(in_i, xid_tag_base, var_info);
        } else {
            size_t current_tag = in_info->xid_tag;
            _Bool is_updated = 0;
            if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }

            if (n2v == ctrl_value) {
                if (!(current_tag & XID_FLAG_FAULT)) {
                    in_info->fault_3value = non_ctrl_value;
                    current_tag |= XID_FLAG_FAULT;
                    is_updated = 1;
                }
            } else if (n2v == non_ctrl_value) {
                if (!(current_tag & XID_FLAG_NORMAL)) {
                    in_info->normal_3value = non_ctrl_value;
                    current_tag |= XID_FLAG_NORMAL;
                    is_updated = 1;
                }
            }

            if (is_updated) {
                in_info->xid_tag = current_tag;
                enqueue(bwd_q, in_i);
            }
        }
    }
}

static void xid_fpath_and (Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    gate_common_AND_OR_like(bwd_q, node, xid_tag, var_info, XID_ZERO, 0);
}
static void xid_fpath_nand(Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    gate_common_AND_OR_like(bwd_q, node, xid_tag, var_info, XID_ZERO, 1);
}
static void xid_fpath_or  (Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    gate_common_AND_OR_like(bwd_q, node, xid_tag, var_info, XID_ONE, 0);
}
static void xid_fpath_nor (Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag, XID_VAR_INFO* var_info) {
    gate_common_AND_OR_like(bwd_q, node, xid_tag, var_info, XID_ONE, 1);
}

static void xid_fpath_exor(Queue_t* bwd_q, const NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info) {
    size_t n_in = (size_t)node->n_in;
    DEBUG_ASSERT(n_in == 2);
    for (size_t i = 0; i < n_in; ++i) {
        NLIST_t* in_i = node->in[i];
        XID_VAR_INFO* in_info = &var_info[in_i->n];

        if (in_info->normal_2value == in_info->fault_2value) {
            size_t current_tag = in_info->xid_tag;
            _Bool is_updated = 0;
            if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }

            if (!(current_tag & XID_FLAG_NORMAL)) {
                in_info->normal_3value = in_info->normal_2value;
                current_tag |= XID_FLAG_NORMAL;
                is_updated = 1;
            }
            if (!(current_tag & XID_FLAG_FAULT)) {
                in_info->fault_3value = in_info->fault_2value;
                current_tag |= XID_FLAG_FAULT;
                is_updated = 1;
            }
            if (is_updated) {
                in_info->xid_tag = current_tag;
                enqueue(bwd_q, in_i);
            }
        } else {
            ED_push_Xpath(in_i, xid_tag_base, var_info);
        }
    }
}

static xid_fpath_func_t xid_fpath[MAX_GATE_TYPE] = { 0 };
void init_xid_fpath_table(void) {
    static _Bool initialized = 0;
    if (initialized) return;
    initialized = 1;
    for (size_t i = 0; i < MAX_GATE_TYPE; ++i) xid_fpath[i] = xid_fpath_unsupported;
    xid_fpath[IN_G]  = xid_fpath_skip;
    xid_fpath[BUF]   = xid_fpath_buf;
    xid_fpath[FOUT]  = xid_fpath_buf;
    xid_fpath[INV]   = xid_fpath_buf;
    xid_fpath[AND]   = xid_fpath_and;
    xid_fpath[NAND]  = xid_fpath_nand;
    xid_fpath[OR]    = xid_fpath_or;
    xid_fpath[NOR]   = xid_fpath_nor;
    xid_fpath[EXOR]  = xid_fpath_exor;
    xid_fpath[EXNOR] = xid_fpath_exor;
}

/* -----------------------------------------------------------------------
 * ED_push_Xpath: mark a net as being on the fault-propagation path
 * ----------------------------------------------------------------------- */
static void ED_push_Xpath(NLIST_t* net, size_t xid_tag_base, XID_VAR_INFO* var_info) {
    size_t id = net->n;
    size_t current_tag = var_info[id].xid_tag;

    if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
    if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }

    push_lev_net(net);

    if (!(current_tag & XID_FLAG_NORMAL)) {
        var_info[id].normal_3value = var_info[id].normal_2value;
        current_tag |= XID_FLAG_NORMAL;
    }
    if (!(current_tag & XID_FLAG_FAULT)) {
        var_info[id].fault_3value = var_info[id].fault_2value;
        current_tag |= XID_FLAG_FAULT;
    }
    var_info[id].xid_tag = current_tag;
}

/* -----------------------------------------------------------------------
 * Xfilling: 3-value X-filling from detecting PO back to fault site
 * ----------------------------------------------------------------------- */
static _Bool Xfilling(size_t fsigID, XID_VAR_INFO* var_info, size_t po_id, size_t xid_tag_base) {
    NLIST_t* tmp_net = &nl[po_id];
    ED_push_Xpath(tmp_net, xid_tag_base, var_info);

    Queue_t* fwd_q = s_fwd_q;
    Queue_t* bwd_q = s_bwd_q;
    Queue_t* jus_q = s_jus_q;
    resetQueue(fwd_q);
    resetQueue(bwd_q);
    resetQueue(jus_q);

    vsize_t event_lev = (vsize_t)tmp_net->level;
    _Bool stop_xpath = 0;

    while (!isQueueEmpty(fwd_q) || !isQueueEmpty(bwd_q) ||
           (!stop_xpath && get_total_net_count()) || !isQueueEmpty(jus_q))
    {
        if (!isQueueEmpty(fwd_q)) {
            NLIST_t* net = (NLIST_t*)dequeue(fwd_q);
            xid_forward_imp(fwd_q, net, xid_tag_base, var_info);
        }
        else if (!isQueueEmpty(bwd_q)) {
            NLIST_t* net = (NLIST_t*)dequeue(bwd_q);
            xid_backward_imp(bwd_q, jus_q, net, xid_tag_base, var_info);
            if (net->n_out >= 2) {
                xid_forward_imp(fwd_q, net, xid_tag_base, var_info);
            }
        }
        else if (!stop_xpath && get_total_net_count()) {
            event_lev = check_lev_reverse(event_lev);
            tmp_net   = pop_lev_net(event_lev);

            if (tmp_net->n_out >= 2) { enqueue(fwd_q, tmp_net); }

            if ((size_t)tmp_net->n == fsigID) {
                enqueue(bwd_q, tmp_net);
                stop_xpath = 1;
                while (get_total_net_count()) {
                    event_lev = check_lev_reverse(event_lev);
                    (void)pop_lev_net(event_lev);
                }
            } else {
                (*xid_fpath[tmp_net->type])(bwd_q, tmp_net, xid_tag_base, var_info);
            }
        }
        else {
            NLIST_t* jus_net = (NLIST_t*)dequeue(jus_q);
            xid_backward_imp_limited(bwd_q, jus_net, xid_tag_base, var_info);
        }
    }

    /* queues are persistent scratch; the loop above left them empty */

    /* fix up: signals not on influence cone get normal value */
    for (int i = 0; i < n_net; ++i) {
        size_t current_tag = var_info[i].xid_tag;
        if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
        int n3v = (current_tag & XID_FLAG_NORMAL) ? var_info[i].normal_3value : XID_X;
        int f3v = (current_tag & XID_FLAG_FAULT)  ? var_info[i].fault_3value  : XID_X;

        if (f3v != XID_X && n3v == XID_X) {
            if (var_info[i].ed_tag == XID_UNTAGGED) {
                var_info[i].normal_3value = var_info[i].normal_2value;
                var_info[i].xid_tag |= XID_FLAG_NORMAL;
            }
        }
    }
    return 1;
}

/* -----------------------------------------------------------------------
 * InlineXID: PI don't-care filling (replaces external XID process call)
 * Returns malloc'd char[n_pi+1]: '0'/'1'/'X' per PI + '\0'.
 * The blocking clause is added by the caller (AddBlockingClauseFromCube).
 * Caller must free() the returned string.
 * ----------------------------------------------------------------------- */
char* InlineXID(CCaDiCaL* solver, NLIST* fault_net) {
    xid_ensure_scratch();

    size_t fsigID = (size_t)(fault_net - nl);

    XID_VAR_INFO* var_info = s_var_info;
    DETECT_PO detect_po = { 0, s_po_id };

    /* reset per-call state: init every signal to X, then overwrite the
       good-circuit value from the SAT model in a single pass */
    for (int i = 0; i < n_net; ++i) {
        var_info[i].ed_tag        = 0;
        var_info[i].edx_tag       = 0;
        var_info[i].xid_tag       = 0;
        var_info[i].normal_3value = XID_X;
        var_info[i].fault_3value  = XID_X;

        unsigned int var = nl[i].varsgc;
        if (var != 0) {
            int v = (ccadical_val(solver, (int)var) > 0) ? XID_ONE : XID_ZERO;
            var_info[i].normal_2value = v;
            var_info[i].fault_2value  = v;
        } else {
            var_info[i].normal_2value = XID_X;
            var_info[i].fault_2value  = XID_X;
        }
    }

    /* 2-value fault simulation (drains the level stack back to empty) */
    xid_fsim(fsigID, var_info, &detect_po);

    /* X-filling toward the first detecting PO */
    if (detect_po.n_det_po > 0) {
        Xfilling(fsigID, var_info, detect_po.po_id[0], 0);
    }

    /* build result string: '0'/'1'/'X' per PI ('\0'-terminated, no trailing newline) */
    char* result = (char*)malloc((size_t)n_pi + 1);
    if (!result) { fprintf(stderr, "InlineXID: malloc failed\n"); exit(1); }

    for (int i = 0; i < n_pi; ++i) {
        size_t sigID       = (size_t)pi[i]->n;
        size_t current_tag = var_info[sigID].xid_tag;
        int n3v = (current_tag & XID_FLAG_NORMAL) ? var_info[sigID].normal_3value : XID_X;
        result[i] = (n3v == XID_ZERO) ? '0' : (n3v == XID_ONE) ? '1' : 'X';
    }
    result[n_pi] = '\0';

    return result;
}

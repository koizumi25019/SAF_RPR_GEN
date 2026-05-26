#include "XID.h"


typedef void (*xid_fimp_func_t)(Queue_t* fwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v);

static void xid_fimp_unsupported(Queue_t* fwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)fwd_q; (void)var_info; (void)n3v; (void)f3v;
	ERROR_PRINTF("Unsupported gate type in forward implication: (type)%" PRId8 "\n", t_net->type);
	DEBUG_ASSERT(0);
}

// -------------------------------------------------------------
// BUF / INV
// -------------------------------------------------------------
static void xid_fimp_buf(Queue_t* fwd_q, NLIST_t* out, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	XID_VAR_INFO* out_info = &var_info[out->n];
	size_t current_tag = out_info->xid_tag;
	if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }

	if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
	_Bool updated = 0;

	if (n3v != XID_X && !(current_tag & XID_FLAG_NORMAL)) {
		out_info->normal_3value = n3v;
		current_tag |= XID_FLAG_NORMAL;
		updated = 1;
	}
	if (f3v != XID_X && !(current_tag & XID_FLAG_FAULT)) {
		out_info->fault_3value = f3v;
		current_tag |= XID_FLAG_FAULT;
		updated = 1;
	}
	
	if (updated) { 
		out_info->xid_tag = current_tag; // タグ書き戻し
		enqueue(fwd_q, out);
	}
}

static void xid_fimp_inv(Queue_t* fwd_q, NLIST_t* out, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	XID_VAR_INFO* out_info = &var_info[out->n];
	size_t current_tag = out_info->xid_tag;
	if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }

	if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
	_Bool updated = 0;

	if (n3v != XID_X && !(current_tag & XID_FLAG_NORMAL)) {
		out_info->normal_3value = n3v ^ 1;
		current_tag |= XID_FLAG_NORMAL;
		updated = 1;
	}
	if (f3v != XID_X && !(current_tag & XID_FLAG_FAULT)) {
		out_info->fault_3value = f3v ^ 1;
		current_tag |= XID_FLAG_FAULT;
		updated = 1;
	}
	
	if (updated) { 
		out_info->xid_tag = current_tag; // タグ書き戻し
		enqueue(fwd_q, out);
	}
}

// -------------------------------------------------------------
// AND / NAND / OR / NOR
// -------------------------------------------------------------
typedef struct {
	_Bool		n_unknown_found;
	_Bool		n_control_found;
	_Bool		f_unknown_found;
	_Bool		f_control_found;
} XidFimpScan;

static XidFimpScan scan_inputs(const NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int c_val) {
	XidFimpScan scan = { .n_unknown_found = 0, .n_control_found = 0, .f_unknown_found = 0, .f_control_found = 0 };
	NLIST_t** in = node->in;

	for (size_t i = 0; i < node->n_in; ++i) {
		XID_VAR_INFO* in_info = &var_info[in[i]->n];
		
		// 入力の現在の有効なフラグを取得
		size_t in_tag = in_info->xid_tag;
		size_t valid_flags = (in_tag >= xid_tag_base) ? (in_tag & XID_FLAG_BOTH) : XID_FLAG_NONE;

		// 正常値の判定
		if (!(valid_flags & XID_FLAG_NORMAL)) { scan.n_unknown_found = 1; }
		else if (in_info->normal_3value == c_val) { scan.n_control_found = 1; }

		// 故障値の判定
		if (!(valid_flags & XID_FLAG_FAULT)) { scan.f_unknown_found = 1; }
		else if (in_info->fault_3value == c_val) { scan.f_control_found = 1; }
	}
	return scan;
}

static void xid_fimp_universal(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int c_val, _Bool is_inv) {
	XID_VAR_INFO* info = &var_info[node->n];
	size_t current_tag = info->xid_tag;
	if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }
	if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }

	XidFimpScan scan = scan_inputs(node, xid_tag_base, var_info, c_val);
	int nc_val = c_val ^ 1;
	_Bool updated = 0;

	// 正常値の前方含意
	if (!(current_tag & XID_FLAG_NORMAL)) {
		if (scan.n_control_found) {
			info->normal_3value = (is_inv) ? nc_val : c_val;
			current_tag |= XID_FLAG_NORMAL;
			updated = 1;
		}
		else if (!scan.n_unknown_found) {
			info->normal_3value = (is_inv) ? c_val : nc_val;
			current_tag |= XID_FLAG_NORMAL;
			updated = 1;
		}
	}
	
	// 故障値の前方含意
	if (!(current_tag & XID_FLAG_FAULT)) {
		if (scan.f_control_found) {
			info->fault_3value = (is_inv) ? nc_val : c_val;
			current_tag |= XID_FLAG_FAULT;
			updated = 1;
		}
		else if (!scan.f_unknown_found) {
			info->fault_3value = (is_inv) ? c_val : nc_val;
			current_tag |= XID_FLAG_FAULT;
			updated = 1;
		}
	}
	
	if (updated) { 
		info->xid_tag = current_tag;
		enqueue(fwd_q, node);
	}
}

static void xid_fimp_and(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)n3v; (void)f3v; xid_fimp_universal(fwd_q, node, xid_tag_base, var_info, XID_ZERO, 0);
}
static void xid_fimp_nand(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)n3v; (void)f3v; xid_fimp_universal(fwd_q, node, xid_tag_base, var_info, XID_ZERO, 1);
}
static void xid_fimp_or(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)n3v; (void)f3v; xid_fimp_universal(fwd_q, node, xid_tag_base, var_info, XID_ONE, 0);
}
static void xid_fimp_nor(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)n3v; (void)f3v; xid_fimp_universal(fwd_q, node, xid_tag_base, var_info, XID_ONE, 1);
}

// -------------------------------------------------------------
// EXOR / EXNOR
// -------------------------------------------------------------
typedef struct {
	_Bool		n_unknown_found;
	size_t		n_one_count;
	_Bool		f_unknown_found;
	size_t		f_one_count;
} XidFimpScanXor;

static XidFimpScanXor scan_inputs_exor(const NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info) {
	XidFimpScanXor scan = { .n_unknown_found = 0, .n_one_count = 0, .f_unknown_found = 0, .f_one_count = 0 };
	NLIST_t** in = node->in;

	for (size_t i = 0; i < node->n_in; ++i) {
		XID_VAR_INFO* in_info = &var_info[in[i]->n];
		size_t in_tag = in_info->xid_tag;
		size_t valid_flags = (in_tag >= xid_tag_base) ? (in_tag & XID_FLAG_BOTH) : XID_FLAG_NONE;

		// 正常値
		if (!(valid_flags & XID_FLAG_NORMAL)) { scan.n_unknown_found = 1; }
		else if (in_info->normal_3value == XID_ONE) { scan.n_one_count++; }
		
		// 故障値
		if (!(valid_flags & XID_FLAG_FAULT)) { scan.f_unknown_found = 1; }
		else if (in_info->fault_3value == XID_ONE) { scan.f_one_count++; }
	}
	return scan;
}

static void xid_fimp_univ_exor(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, _Bool is_inv) {
	XID_VAR_INFO* info = &var_info[node->n];
	size_t current_tag = info->xid_tag;
	if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }
	if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }

	XidFimpScanXor scan = scan_inputs_exor(node, xid_tag_base, var_info);
	_Bool updated = 0;

	// 正常値の前方含意（未知入力がない場合のみ確定）
	if (!(current_tag & XID_FLAG_NORMAL) && !scan.n_unknown_found) {
		int out_val = (scan.n_one_count & 1ULL) ? XID_ONE : XID_ZERO;
		info->normal_3value = (is_inv) ? (out_val ^ 1) : out_val;
		current_tag |= XID_FLAG_NORMAL;
		updated = 1;
	}
	
	// 故障値の前方含意
	if (!(current_tag & XID_FLAG_FAULT) && !scan.f_unknown_found) {
		int out_val = (scan.f_one_count & 1ULL) ? XID_ONE : XID_ZERO;
		info->fault_3value = (is_inv) ? (out_val ^ 1) : out_val;
		current_tag |= XID_FLAG_FAULT;
		updated = 1;
	}
	
	if (updated) { 
		info->xid_tag = current_tag;
		enqueue(fwd_q, node);
	}
}

static void xid_fimp_exor(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)n3v; (void)f3v; xid_fimp_univ_exor(fwd_q, node, xid_tag_base, var_info, 0);
}
static void xid_fimp_exnor(Queue_t* fwd_q, NLIST_t* node, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)n3v; (void)f3v; xid_fimp_univ_exor(fwd_q, node, xid_tag_base, var_info, 1);
}

// -------------------------------------------------------------
// Table & Entry
// -------------------------------------------------------------
static xid_fimp_func_t xid_fimp[MAX_GATE_TYPE] = { 0 };

void init_xid_fimp_table(void) {
	static _Bool initialized = 0;
	if (initialized) return;
	initialized = 1;

	for (size_t i = 0; i < MAX_GATE_TYPE; ++i) xid_fimp[i] = xid_fimp_unsupported;
	xid_fimp[BUF] = xid_fimp_buf;
	xid_fimp[FOUT] = xid_fimp_buf;
	xid_fimp[INV] = xid_fimp_inv;
	xid_fimp[AND] = xid_fimp_and;
	xid_fimp[NAND] = xid_fimp_nand;
	xid_fimp[OR] = xid_fimp_or;
	xid_fimp[NOR] = xid_fimp_nor;
	xid_fimp[EXOR] = xid_fimp_exor;
	xid_fimp[EXNOR] = xid_fimp_exnor;
}

// t_net -> out 方向
// t_netのn3v/f3vが{0,1}に決まっているとき、out方向のn3v/f3vを更新する
void xid_forward_imp(Queue_t* fwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info) {

	// t_netの現在の有効なフラグから値を抽出
	size_t t_tag = var_info[t_net->n].xid_tag;
	size_t valid_flags = (t_tag >= xid_tag_base) ? (t_tag & XID_FLAG_BOTH) : XID_FLAG_NONE;

	// フラグが立っていない場合は XID_X として扱う
	int n3v = (valid_flags & XID_FLAG_NORMAL) ? var_info[t_net->n].normal_3value : XID_X;
	int f3v = (valid_flags & XID_FLAG_FAULT) ? var_info[t_net->n].fault_3value : XID_X;

	// 両方とも未確定の場合は伝搬するものがない
	if (n3v == XID_X && f3v == XID_X) return;

	for (size_t i = 0; i < t_net->n_out; ++i) {
		NLIST_t* out_i = t_net->out[i];

		size_t out_tag = var_info[out_i->n].xid_tag;
		if (out_tag == (xid_tag_base | XID_FLAG_BOTH)) { continue; }

		(*xid_fimp[out_i->type])(fwd_q, out_i, xid_tag_base, var_info, n3v, f3v);
	}
}
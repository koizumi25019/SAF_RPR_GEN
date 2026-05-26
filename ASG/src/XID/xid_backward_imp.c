#include "XID.h"


// ===================================================================
// ゲート別後方含意関数のヘルパー関数(XID用)
// ===================================================================

typedef void (*xid_bimp_func_t)(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v);

static void xid_bimp_unsupported(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)bwd_q; (void)jus_q; (void*)var_info; (void)xid_tag_base; (void)n3v; (void)f3v;
	ERROR_PRINTF("Unsupported gate type in backward implication: (type)%" PRId8 "\n", t_net->type);
	DEBUG_ASSERT(0);
}

static void xid_bimp_skip(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)bwd_q; (void)jus_q; (void)t_net; (void)var_info; (void)xid_tag_base; (void)n3v; (void)f3v;
	// 何もしない
}

// -------------------------------------------------------------
// BUF / INV
// -------------------------------------------------------------
static void xid_bimp_buf(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)jus_q;
	NLIST_t* in = t_net->in[0];
	XID_VAR_INFO* in_info = &var_info[in->n];
	size_t current_tag = in_info->xid_tag;
	if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }

	if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
	_Bool updated = 0;

	if (n3v != XID_X && !(current_tag & XID_FLAG_NORMAL)) {
		in_info->normal_3value = n3v;
		current_tag |= XID_FLAG_NORMAL;
		updated = 1;
	}
	if (f3v != XID_X && !(current_tag & XID_FLAG_FAULT)) {
		in_info->fault_3value = f3v;
		current_tag |= XID_FLAG_FAULT;
		updated = 1;
	}

	if (updated) {
		in_info->xid_tag = current_tag;
		enqueue(bwd_q, in);
	}
}

static void xid_bimp_inv(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)jus_q;
	NLIST_t* in = t_net->in[0];
	XID_VAR_INFO* in_info = &var_info[in->n];
	size_t current_tag = in_info->xid_tag;
	if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { return; }

	if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
	_Bool updated = 0;

	if (n3v != XID_X && !(current_tag & XID_FLAG_NORMAL)) {
		in_info->normal_3value = n3v ^ 1;
		current_tag |= XID_FLAG_NORMAL;
		updated = 1;
	}
	if (f3v != XID_X && !(current_tag & XID_FLAG_FAULT)) {
		in_info->fault_3value = f3v ^ 1;
		current_tag |= XID_FLAG_FAULT;
		updated = 1;
	}

	if (updated) {
		in_info->xid_tag = current_tag;
		enqueue(bwd_q, in);
	}
}

// -------------------------------------------------------------
// AND / NAND / OR / NOR の汎用後方含意
// -------------------------------------------------------------
// c_val  : 制御値 (AND/NAND=0, OR/NOR=1)
// is_inv : 出力反転 (AND/OR=0, NAND/NOR=1)
static void xid_bimp_universal(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v, int c_val, int is_inv) {
	const int nc_val = c_val ^ 1;
	const int trigger_force = (is_inv) ? c_val : nc_val;
	const int trigger_search = (is_inv) ? nc_val : c_val;

	NLIST_t* n_cand = NULL, * f_cand = NULL;
	int n_cnt = 0, f_cnt = 0;

	// 出力が「探索トリガー」でなければ、探索完了(done)扱い
	int n_done = (n3v != trigger_search);
	int f_done = (f3v != trigger_search);

	// -------------------------------------------------------------
	// メインループ (Forceの処理と、Searchの候補探し)
	// -------------------------------------------------------------
	for (size_t i = 0; i < t_net->n_in; ++i) {
		NLIST_t* in_node = t_net->in[i];
		XID_VAR_INFO* in_info = &var_info[in_node->n];

		size_t current_tag = in_info->xid_tag;
		if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
		_Bool updated = 0;

		// --- 正常値 (Normal) ---
		if (n3v == trigger_force) {
			// Force: 入力をすべて「非制御値」にする
			if (!(current_tag & XID_FLAG_NORMAL)) {
				in_info->normal_3value = nc_val;
				current_tag |= XID_FLAG_NORMAL;
				updated = 1;
			}
		}
		else if (n3v == trigger_search) {
			// Search: 入力の「制御値」を探す
			if (current_tag & XID_FLAG_NORMAL) {
				if (in_info->normal_3value == c_val) { n_done = 1; }
			}
			else if (in_info->normal_2value == c_val) { // XID_X の場合
				n_cand = in_node;
				n_cnt++;
			}
		}

		// --- 故障値 (Fault) ---
		if (f3v == trigger_force) {
			if (!(current_tag & XID_FLAG_FAULT)) {
				in_info->fault_3value = nc_val;
				current_tag |= XID_FLAG_FAULT;
				updated = 1;
			}
		}
		else if (f3v == trigger_search) {
			if (current_tag & XID_FLAG_FAULT) {
				if (in_info->fault_3value == c_val) { f_done = 1; }
			}
			else if (in_info->fault_2value == c_val) { // XID_X の場合
				f_cand = in_node;
				f_cnt++;
			}
		}

		// Force処理で値が更新された場合のみ書き戻してエンキュー
		if (updated) {
			in_info->xid_tag = current_tag;
			enqueue(bwd_q, in_node);
		}
	}

	// -------------------------------------------------------------
	// 限定含意
	// -------------------------------------------------------------
	NLIST_t* last_enqueued = NULL;

	// 正常値
	if (!n_done && n_cnt == 1) {
		n_done = 1;
		XID_VAR_INFO* c_info = &var_info[n_cand->n];
		size_t c_tag = c_info->xid_tag;
		if (c_tag < xid_tag_base) { c_tag = xid_tag_base; }

		// ループ内のFault側Forceでキューに積まれている可能性もあるが
		// タグを読んで未確定なら設定する
		if (!(c_tag & XID_FLAG_NORMAL)) {
			c_info->normal_3value = c_val;
			c_tag |= XID_FLAG_NORMAL;
			c_info->xid_tag = c_tag;
			enqueue(bwd_q, n_cand);
			last_enqueued = n_cand;
		}
	}

	// 故障値
	if (!f_done && f_cnt == 1) {
		f_done = 1;
		XID_VAR_INFO* c_info = &var_info[f_cand->n];
		size_t c_tag = c_info->xid_tag;
		if (c_tag < xid_tag_base) { c_tag = xid_tag_base; }

		if (!(c_tag & XID_FLAG_FAULT)) {
			c_info->fault_3value = c_val;
			c_tag |= XID_FLAG_FAULT;
			c_info->xid_tag = c_tag;
			if (f_cand != last_enqueued) {
				enqueue(bwd_q, f_cand);
			}
		}
	}

	// -------------------------------------------------------------
	// J-Frontier 登録
	// -------------------------------------------------------------
	if (jus_q != NULL) {
		if ((!n_done && n_cnt >= 2) || (!f_done && f_cnt >= 2)) {
			enqueue(jus_q, t_net);
		}
	}
}

// ゲートタイプ別の呼び出しラッパー
static void xid_bimp_and(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_universal(bwd_q, jus_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ZERO, 0);
}
static void xid_bimp_nand(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_universal(bwd_q, jus_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ZERO, 1);
}
static void xid_bimp_or(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_universal(bwd_q, jus_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ONE, 0);
}
static void xid_bimp_nor(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_universal(bwd_q, jus_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ONE, 1);
}

// -------------------------------------------------------------
// EXOR / EXNOR
// -------------------------------------------------------------
static void xid_bimp_exor(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)jus_q; (void)n3v; (void)f3v;
	for (size_t i = 0; i < t_net->n_in; ++i) {
		NLIST_t* in_i = t_net->in[i];
		XID_VAR_INFO* in_info = &var_info[in_i->n];
		size_t current_tag = in_info->xid_tag;
		if (current_tag == (xid_tag_base | XID_FLAG_BOTH)) { continue; }

		if (current_tag < xid_tag_base) { current_tag = xid_tag_base; }
		_Bool updated = 0;

		// XID_X の代わりにフラグを確認して、2値(正解値)を割り当てる
		if (!(current_tag & XID_FLAG_NORMAL)) {
			in_info->normal_3value = in_info->normal_2value;
			current_tag |= XID_FLAG_NORMAL;
			updated = 1;
		}
		if (!(current_tag & XID_FLAG_FAULT)) {
			in_info->fault_3value = in_info->fault_2value;
			current_tag |= XID_FLAG_FAULT;
			updated = 1;
		}

		if (updated) {
			in_info->xid_tag = current_tag;
			enqueue(bwd_q, in_i);
		}
	}
}

// -------------------------------------------------------------
// Table & Entry
// -------------------------------------------------------------
static xid_bimp_func_t xid_bimp[MAX_GATE_TYPE] = { 0 };

void init_xid_bimp_table(void) {
	static _Bool initialized = 0;
	if (initialized) return;
	initialized = 1;

	for (size_t i = 0; i < MAX_GATE_TYPE; ++i) xid_bimp[i] = xid_bimp_unsupported;
	xid_bimp[IN_G] = xid_bimp_skip;
	xid_bimp[BUF] = xid_bimp_buf;
	xid_bimp[FOUT] = xid_bimp_buf;
	xid_bimp[INV] = xid_bimp_inv;
	xid_bimp[AND] = xid_bimp_and;
	xid_bimp[NAND] = xid_bimp_nand;
	xid_bimp[OR] = xid_bimp_or;
	xid_bimp[NOR] = xid_bimp_nor;
	xid_bimp[EXOR] = xid_bimp_exor;
	xid_bimp[EXNOR] = xid_bimp_exor;
}

// in <- t_net 方向
// t_netのn3v/f3vが{0,1}に決まっているとき、in方向のn3v/f3vを更新する
void xid_backward_imp(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info) {

	size_t t_tag = var_info[t_net->n].xid_tag;
	size_t valid_flags = (t_tag >= xid_tag_base) ? (t_tag & XID_FLAG_BOTH) : XID_FLAG_NONE;

	int n3v = (valid_flags & XID_FLAG_NORMAL) ? var_info[t_net->n].normal_3value : XID_X;
	int f3v = (valid_flags & XID_FLAG_FAULT) ? var_info[t_net->n].fault_3value : XID_X;

	if (n3v == XID_X && f3v == XID_X) return; // 後方含意早期終了

	(*xid_bimp[t_net->type])(bwd_q, jus_q, t_net, xid_tag_base, var_info, n3v, f3v);
}
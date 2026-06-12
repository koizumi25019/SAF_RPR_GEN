#include "XID.h"


// 関数ポインタの型に xid_tag_base を追加
typedef void (*xid_bimp_limited_func_t)(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v);

static void xid_bimp_unsupported(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	(void)bwd_q; (void)var_info; (void)xid_tag_base;
	ERROR_PRINTF("Unsupported gate type in limited backward implication: (type)%" PRId8 "\n", t_net->type);
	DEBUG_ASSERT(0);
}

// AND, NAND 制御値0を複数持つ
// OR, NOR 制御値1を複数持つ
static void xid_bimp_limited_univ(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v, int c_val, int is_inv) {
	// -------------------------------------------------------------
	// 変数初期化
	// -------------------------------------------------------------
	vsize_t n_lev = VSIZE_INVALID;
	vsize_t f_lev = VSIZE_INVALID;

	NLIST_t* n_cand = NULL, * f_cand = NULL;  // 最小レベルを持つ入力候補を保存
	size_t n_cnt = 0, f_cnt = 0;

	// is_inv を使って出力側のターゲット値（o_val）を算出
	int o_val = (is_inv) ? (c_val ^ 1) : c_val;

	// 出力が o_val でない（Xや非制御値）場合は、最初からスキップ
	_Bool n_done = (n3v != o_val);
	_Bool f_done = (f3v != o_val);

	// -------------------------------------------------------------
	// メインループ: 候補探索
	// 正常側と故障側は独立に探索する。片側だけ制御値を持つ入力
	// （Dを運ぶ入力など）も、その側の正当化候補としては有効。
	// 候補の3値割り当てはモデルの2値に常に一致するため、コミットして
	// も他の要求と矛盾しない（モデル誘導なので健全・完全）。
	// -------------------------------------------------------------
	for (size_t i = 0; i < t_net->n_in; ++i) {
		if (n_done && f_done) { break; }

		NLIST_t* in_i = t_net->in[i];
		XID_VAR_INFO* in_info = &var_info[in_i->n];
		vsize_t in_i_lev = in_i->level;

		// タグから現在の有効なフラグを取得
		size_t in_tag = in_info->xid_tag;
		size_t valid_flags = (in_tag >= xid_tag_base) ? (in_tag & XID_FLAG_BOTH) : XID_FLAG_NONE;

		// --- 正常値 最小レベル選択 ---
		if (!n_done) {
			if (valid_flags & XID_FLAG_NORMAL) {
				if (in_info->normal_3value == c_val) { n_done = 1; }
			}
			else if (in_info->normal_2value == c_val) { // XID_X扱い
				if (n_lev > in_i_lev) {
					n_lev = in_i_lev;
					n_cand = in_i;
				}
				n_cnt++;
			}
		}

		// --- 故障値 最小レベル選択 ---
		if (!f_done) {
			if (valid_flags & XID_FLAG_FAULT) {
				if (in_info->fault_3value == c_val) { f_done = 1; }
			}
			else if (in_info->fault_2value == c_val) { // XID_X扱い
				if (f_lev > in_i_lev) {
					f_lev = in_i_lev;
					f_cand = in_i;
				}
				f_cnt++;
			}
		}
	}
	if (n_done && f_done) { return; }

	// -------------------------------------------------------------
	// 最小レベルを選択して確定
	// 候補が1つでも残っていれば必ずコミットする。ここは J-frontier の
	// 最終正当化なので、コミットしないと出力の要求が黙って失われ、
	// 必須PIがXのまま出力される（過大評価=非健全キューブの原因）。
	// -------------------------------------------------------------
	NLIST_t* last_enqueued = NULL;

	// 正常値
	if (!n_done && n_cand != NULL && n_cnt >= 1) {
		XID_VAR_INFO* c_info = &var_info[n_cand->n];
		size_t c_tag = c_info->xid_tag;

		if (c_tag < xid_tag_base) { c_tag = xid_tag_base; }

		// 安全のためのフラグ確認
		if (!(c_tag & XID_FLAG_NORMAL)) {
			c_info->normal_3value = c_val; // 制御値に確定
			c_tag |= XID_FLAG_NORMAL;
			c_info->xid_tag = c_tag;

			enqueue(bwd_q, n_cand);
			last_enqueued = n_cand;
		}
	}

	// 故障値
	if (!f_done && f_cand != NULL && f_cnt >= 1) {
		XID_VAR_INFO* c_info = &var_info[f_cand->n];
		size_t c_tag = c_info->xid_tag;
		if (c_tag < xid_tag_base) { c_tag = xid_tag_base; }

		if (!(c_tag & XID_FLAG_FAULT)) {
			c_info->fault_3value = c_val; // 制御値に確定
			c_tag |= XID_FLAG_FAULT;
			c_info->xid_tag = c_tag;

			if (f_cand != last_enqueued) {
				enqueue(bwd_q, f_cand);
			}
		}
	}
}

// -------------------------------------------------------------
// ラッパー関数
// -------------------------------------------------------------
static void xid_bimp_limited_and(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_limited_univ(bwd_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ZERO, 0);
}
static void xid_bimp_limited_nand(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_limited_univ(bwd_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ZERO, 1);
}
static void xid_bimp_limited_or(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_limited_univ(bwd_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ONE, 0);
}
static void xid_bimp_limited_nor(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, int n3v, int f3v) {
	xid_bimp_limited_univ(bwd_q, t_net, xid_tag_base, var_info, n3v, f3v, XID_ONE, 1);
}


// -------------------------------------------------------------
// Table & Entry
// -------------------------------------------------------------
static xid_bimp_limited_func_t xid_bimp_limited[MAX_GATE_TYPE] = { 0 };

void init_xid_bimp_limited_table(void) {
	static _Bool initialized = 0;
	if (initialized) return;
	initialized = 1;

	for (size_t i = 0; i < MAX_GATE_TYPE; ++i) xid_bimp_limited[i] = xid_bimp_unsupported;
	xid_bimp_limited[AND] = xid_bimp_limited_and;
	xid_bimp_limited[NAND] = xid_bimp_limited_nand;
	xid_bimp_limited[OR] = xid_bimp_limited_or;
	xid_bimp_limited[NOR] = xid_bimp_limited_nor;
}

// 限定正当化
void xid_backward_imp_limited(Queue_t* bwd_q, NLIST_t* t_net, size_t xid_tag_base, XID_VAR_INFO* var_info, size_t fsig_id) {

	size_t t_tag = var_info[t_net->n].xid_tag;
	size_t valid_flags = (t_tag >= xid_tag_base) ? (t_tag & XID_FLAG_BOTH) : XID_FLAG_NONE;

	int n3v = (valid_flags & XID_FLAG_NORMAL) ? var_info[t_net->n].normal_3value : XID_X;
	int f3v = (valid_flags & XID_FLAG_FAULT) ? var_info[t_net->n].fault_3value : XID_X;

	/* 故障サイトの故障側は正当化対象外（xid_backward_imp と同じ理由） */
	if ((size_t)t_net->n == fsig_id) f3v = XID_X;

	if (n3v == XID_X && f3v == XID_X) return; // 後方含意早期終了

	(*xid_bimp_limited[t_net->type])(bwd_q, t_net, xid_tag_base, var_info, n3v, f3v);
}

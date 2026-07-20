#include "XID.h"

#define DEBUG_XID_FSIM 0
//===================================================================
// ゲートイベントドリブンシミュレーション関数のヘルパー関数
//===================================================================
static inline int in_val(size_t in_id, size_t ed_tag, XID_VAR_INFO* var_info) {
	// 計算済み or 前置換サイト(置換サイトは未計算) fault_value を有効化
	return (var_info[in_id].ed_tag == ed_tag)
		? var_info[in_id].fault_2value
		: var_info[in_id].normal_2value;
}

static inline int in_val_and(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) {
	NLIST_t** in = node->in;
	size_t n_in = node->n_in;
	if (n_in == 2) return in_val(in[0]->n, ed_tag, var_info) & in_val(in[1]->n, ed_tag, var_info);

	int acc = in_val(in[0]->n, ed_tag, var_info);
	for (size_t i = 1; i < n_in; ++i) {
		acc &= in_val(in[i]->n, ed_tag, var_info);
		if (acc == XID_ZERO) break;
	}
	return acc;
}

static inline int in_val_or(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) {
	NLIST_t** in = node->in;
	size_t n_in = node->n_in;
	if (n_in == 2) return in_val(in[0]->n, ed_tag, var_info) | in_val(in[1]->n, ed_tag, var_info);

	int acc = in_val(in[0]->n, ed_tag, var_info);
	for (size_t i = 1; i < n_in; ++i) {
		acc |= in_val(in[i]->n, ed_tag, var_info);
		if (acc == XID_ONE) break;
	}
	return acc;
}

static inline int in_val_xor(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) {
	NLIST_t** in = node->in;
	size_t n_in = node->n_in;
	if (n_in == 2) return in_val(in[0]->n, ed_tag, var_info) ^ in_val(in[1]->n, ed_tag, var_info);

	int acc = in_val(in[0]->n, ed_tag, var_info);
	for (size_t i = 1; i < n_in; ++i) {
		acc ^= in_val(in[i]->n, ed_tag, var_info);
	}
	return acc;
}

typedef void (*ed_fsim_func_t)(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info);
static inline void ed_fsim_buf(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val(node->in[0]->n, ed_tag, var_info); }
static inline void ed_fsim_inv(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val(node->in[0]->n, ed_tag, var_info) ^ 1; }
static inline void ed_fsim_and(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val_and(node, node_id, ed_tag, var_info); }
static inline void ed_fsim_nand(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val_and(node, node_id, ed_tag, var_info) ^ 1; }
static inline void ed_fsim_or(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val_or(node, node_id, ed_tag, var_info); }
static inline void ed_fsim_nor(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val_or(node, node_id, ed_tag, var_info) ^ 1; }
static inline void ed_fsim_exor(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val_xor(node, node_id, ed_tag, var_info); }
static inline void ed_fsim_exnor(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) { var_info[node_id].fault_2value = in_val_xor(node, node_id, ed_tag, var_info) ^ 1; }
static inline void ed_fsim_unsupported(const NLIST_t* node, size_t node_id, size_t ed_tag, XID_VAR_INFO* var_info) {
	(void)node_id; (void)ed_tag; (void)var_info;
	ERROR_PRINTF("Unsupported gate type in event-driven fault simulation: (type)%" PRId8 "\n", node->type);
	DEBUG_ASSERT(0);
}

static ed_fsim_func_t ed_fsim[MAX_GATE_TYPE] = { 0 };
static void init_ed_sppfp_table(void) {
	static _Bool initialized = 0;
	if (initialized) return;
	initialized = 1;

	for (size_t i = 0; i < MAX_GATE_TYPE; ++i) ed_fsim[i] = ed_fsim_unsupported;

	ed_fsim[BUF] = ed_fsim_buf;
	ed_fsim[FOUT] = ed_fsim_buf;
	ed_fsim[INV] = ed_fsim_inv;
	ed_fsim[AND] = ed_fsim_and;
	ed_fsim[NAND] = ed_fsim_nand;
	ed_fsim[OR] = ed_fsim_or;
	ed_fsim[NOR] = ed_fsim_nor;
	ed_fsim[EXOR] = ed_fsim_exor;
	ed_fsim[EXNOR] = ed_fsim_exnor;

	// DFF/RDFF/DFFS/RDFFS/GND/ACC は仕様未対応のため非対応（必要ないため）
}

static void ED_push_out(NLIST_t* net, size_t ed_tag, XID_VAR_INFO* var_info) {
	for (size_t i = 0; i < net->n_out; ++i) {
		XID_VAR_INFO* info = &var_info[net->out[i]->n];

		if (info->ed_tag != ed_tag && info->ed_tag != ed_tag - 1) {
			info->ed_tag = ed_tag - 1;
			push_lev_net(net->out[i]);
		}
	}
}

void xid_fsim(size_t fsigID, XID_VAR_INFO* var_info, DETECT_PO* detect_po) {
	init_ed_sppfp_table();

	/* -------------------- イベントドリブン処理 ------------------------ */
	size_t ed_tag = 2;
	// 故障値の設定（正常値を反転）
	var_info[fsigID].fault_2value = var_info[fsigID].normal_2value ^ 1;
	var_info[fsigID].ed_tag = ed_tag;
	NLIST_t* tmp_net = &nl[fsigID];
	if (tmp_net->n_out == 0 && tmp_net->ppo_flag) {
		detect_po->po_id[detect_po->n_det_po++] = tmp_net->n;
	}
	ED_push_out(tmp_net, ed_tag, var_info);

	// 新しい値を計算
	vsize_t event_lev = tmp_net->level;
	while (get_total_net_count()) {
		event_lev = check_lev(event_lev);
		tmp_net = pop_lev_net(event_lev);

		// 演算
		size_t tmp_id = tmp_net->n;
		var_info[tmp_id].ed_tag = ed_tag;
		(*ed_fsim[tmp_net->type])(tmp_net, tmp_id, ed_tag, var_info);

#if DEBUG_XID_FSIM
		DEBUG_PRINTF("xid_fsim:%s\t%d/%d\n", tmp_net->name,
			var_info[tmp_id].normal_2value, var_info[tmp_id].fault_2value);
#endif

		// 変化なし
		if (var_info[tmp_id].fault_2value == var_info[tmp_id].normal_2value) { continue; }

		// 変化あり（観測できる端点だけを検出POとして記録する）
		if (tmp_net->n_out == 0) {
			if (tmp_net->ppo_flag)
				detect_po->po_id[detect_po->n_det_po++] = tmp_net->n;
		}
		else {
			ED_push_out(tmp_net, ed_tag, var_info);
		}

	}
	return;
}
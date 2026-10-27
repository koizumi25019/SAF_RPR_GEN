//--------------------------------------------------------------------------------------------------------------------
// LOC方式の2時刻展開。PIは両時刻で共有し、DFFは自由な初期状態とキャプチャ用BUFに置き換える。
// 観測点は2時刻目のDFF D入力（PPO）のみ。2時刻目は元の名前、1時刻目は「_1t」付きの名前を使う。
// peer_1t は遷移故障の励起条件を課す1時刻目の信号を指す（共有PIは自分自身）。
//--------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netlist/netlist.h"

#define TDF_YES 1   /* netlist.h には YES/NO が無いので test_sa0/test_sa1 用に定義 */

/* DFFへ向かう辺は1時刻目だけ残し、2時刻目のBUFへつなぐ。
   2時刻目には次のDFFがないため辺を落とし、キャプチャ点はPPOとして別途記録する。 */
static int count_frame_outputs(const NLIST* original_net, int frame)
{
	int output_count = 0;
	for (int k = 0; k < original_net->n_out; k++) {
		if (original_net->out[k]->type != DFF || frame == 1) output_count++;
	}
	return output_count;
}

/* 各出力を同じ時刻のコピーへ接続する。DFFへの辺だけは時刻をまたぐ。 */
static void connect_frame_outputs(
	NLIST** outputs, const NLIST* original_net, int frame,
	NLIST* expanded_nets, const int* frame1_net_ids, const int* frame2_net_ids
)
{
	const int* frame_net_ids = frame == 1 ? frame1_net_ids : frame2_net_ids;
	int output_index = 0;
	for (int k = 0; k < original_net->n_out; k++) {
		const NLIST* output_net = original_net->out[k];
		if (output_net->type == DFF) {
			if (frame == 1) outputs[output_index++] = &expanded_nets[frame2_net_ids[output_net->n]];
		}
		else {
			outputs[output_index++] = &expanded_nets[frame_net_ids[output_net->n]];
		}
	}
}

static char* duplicate_frame_name(const char* original_name, int frame)
{
	if (frame == 2) return strdup(original_name);
	size_t name_length = strlen(original_name);
	char* frame_name = (char*)malloc(name_length + 4);            /* "_1t" + '\0' */
	memcpy(frame_name, original_name, name_length);
	memcpy(frame_name + name_length, "_1t", 4);
	return frame_name;
}

/* 新ノードの共通フィールド初期化（calloc 済み前提で残りを埋める） */
static void init_expanded_net(NLIST* expanded_net, int id, int type, char* name)
{
	expanded_net->n        = id;
	expanded_net->type     = type;
	expanded_net->name     = name;
	expanded_net->name_ins = strdup(name);
	expanded_net->name_port = (char*)NULL;
	/* 故障読込時に等価故障を整理するまでは、全てを対象にする。 */
	expanded_net->test_sa0 = TDF_YES;
	expanded_net->test_sa1 = TDF_YES;
	expanded_net->ppo_flag = 0;         /* 観測点は build_scan_ports が PPO にだけ立てる */
}

/* 疑似外部入出力の配列を作り、
   観測点フラグ(ppo_flag)を立てる。展開回路上では
     ppi[] = DFF状態の1時刻目コピー（自由入力＝スキャンで設定される状態変数）
     ppo[] = DFFのD入力の2時刻目コピー（FFへのキャプチャ点＝唯一の観測点）
   PO は at-speed でストローブしないので ppo_flag を立てない。 */
static void build_scan_ports(
	const NLIST* original_nets, int original_net_count,
	NLIST* expanded_nets, const int* frame1_net_ids, const int* frame2_net_ids
)
{
	ppi = (NLIST**)malloc(sizeof(NLIST*) * n_dff);
	ppo = (NLIST**)malloc(sizeof(NLIST*) * n_dff);
	if (ppi == NULL || ppo == NULL) {
		printf("build_scan_ports: allocation failed\n");
		exit(EXIT_FAILURE);
	}
	n_ppi = 0;
	n_ppo = 0;

	for (int i = 0; i < original_net_count; i++) {
		if (original_nets[i].type != DFF) continue;
		int data_net_id = original_nets[i].in[0]->n;
		ppi[n_ppi++] = &expanded_nets[frame1_net_ids[i]];
		ppo[n_ppo] = &expanded_nets[frame2_net_ids[data_net_id]];
		ppo[n_ppo]->ppo_flag = 1;
		n_ppo++;
	}
}

void expand_tdf_netlist(void)
{
	NLIST* original_nets = nl;
	int original_net_count = n_net;
	NLIST** original_pi = pi;
	int original_pi_count = n_pi;
	NLIST** original_po = po;
	int original_po_count = n_po;

	/* 新ノード ID の割り当て：PI は両時刻共有で1個、それ以外は各時刻1個ずつ */
	int* frame1_net_ids = (int*)malloc(original_net_count * sizeof(int));   /* 元ID -> 1時刻目コピーの新ID */
	int* frame2_net_ids = (int*)malloc(original_net_count * sizeof(int));   /* 元ID -> 2時刻目コピーの新ID */
	int expanded_net_count = 0;
	for (int i = 0; i < original_net_count; i++) {
		frame1_net_ids[i] = expanded_net_count++;
		if (original_nets[i].type == IN) {
			frame2_net_ids[i] = frame1_net_ids[i];
		}
		else {
			frame2_net_ids[i] = expanded_net_count++;
		}
	}

	NLIST* expanded_nets = (NLIST*)calloc((size_t)expanded_net_count, sizeof(NLIST));
	if (expanded_nets == NULL) {
		printf("expand_tdf_netlist: allocation failed\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < original_net_count; i++) {
		const NLIST* original_net = &original_nets[i];

		if (original_net->type == IN) {
			/* 両時刻共有の PI。出力は両フレームのコピーへつながる */
			NLIST* expanded_net = &expanded_nets[frame1_net_ids[i]];
			init_expanded_net(expanded_net, frame1_net_ids[i], IN, duplicate_frame_name(original_net->name, 2));
			expanded_net->peer_1t = expanded_net;                       /* PI の1時刻目=自分自身 */
			expanded_net->n_out = count_frame_outputs(original_net, 1) + count_frame_outputs(original_net, 2);
			if (expanded_net->n_out > 0) {
				expanded_net->out = (NLIST**)malloc(expanded_net->n_out * sizeof(NLIST*));
				connect_frame_outputs(expanded_net->out, original_net, 1, expanded_nets, frame1_net_ids, frame2_net_ids);
				connect_frame_outputs(expanded_net->out + count_frame_outputs(original_net, 1),
					original_net, 2, expanded_nets, frame1_net_ids, frame2_net_ids);
			}
		}
		else if (original_net->type == DFF) {
			/* 1時刻目コピー：自由入力（擬似PI＝スキャン設定される状態変数） */
			NLIST* initial_state = &expanded_nets[frame1_net_ids[i]];
			init_expanded_net(initial_state, frame1_net_ids[i], IN, duplicate_frame_name(original_net->name, 1));
			initial_state->n_out = count_frame_outputs(original_net, 1);
			if (initial_state->n_out > 0) {
				initial_state->out = (NLIST**)malloc(initial_state->n_out * sizeof(NLIST*));
				connect_frame_outputs(initial_state->out, original_net, 1, expanded_nets, frame1_net_ids, frame2_net_ids);
			}

			/* 2時刻目コピー：1時刻目の D 入力を受ける BUF（値引き継ぎ） */
			NLIST* capture_state = &expanded_nets[frame2_net_ids[i]];
			init_expanded_net(capture_state, frame2_net_ids[i], BUF, duplicate_frame_name(original_net->name, 2));
			capture_state->peer_1t = initial_state;
			capture_state->n_in = 1;
			capture_state->in = (NLIST**)malloc(sizeof(NLIST*));
			capture_state->in[0] = &expanded_nets[frame1_net_ids[original_net->in[0]->n]];
			capture_state->n_out = count_frame_outputs(original_net, 2);
			if (capture_state->n_out > 0) {
				capture_state->out = (NLIST**)malloc(capture_state->n_out * sizeof(NLIST*));
				connect_frame_outputs(capture_state->out, original_net, 2, expanded_nets, frame1_net_ids, frame2_net_ids);
			}
		}
		else {
			/* 通常ゲート（FOUT 分岐含む）：各時刻に同型コピー */
			for (int frame = 1; frame <= 2; frame++) {
				const int* frame_net_ids = frame == 1 ? frame1_net_ids : frame2_net_ids;
				int net_id = frame_net_ids[i];
				NLIST* expanded_net = &expanded_nets[net_id];
				init_expanded_net(expanded_net, net_id, original_net->type,
					duplicate_frame_name(original_net->name, frame));
				if (frame == 2) expanded_net->peer_1t = &expanded_nets[frame1_net_ids[i]];
				expanded_net->n_in = original_net->n_in;
				if (expanded_net->n_in > 0) {
					expanded_net->in = (NLIST**)malloc(expanded_net->n_in * sizeof(NLIST*));
					for (int j = 0; j < original_net->n_in; j++) {
						int input_net_id = original_net->in[j]->n;
						expanded_net->in[j] = &expanded_nets[frame_net_ids[input_net_id]];
					}
				}
				expanded_net->n_out = count_frame_outputs(original_net, frame);
				if (expanded_net->n_out > 0) {
					expanded_net->out = (NLIST**)malloc(expanded_net->n_out * sizeof(NLIST*));
					connect_frame_outputs(expanded_net->out, original_net, frame, expanded_nets, frame1_net_ids, frame2_net_ids);
				}
			}
		}
	}

	build_scan_ports(original_nets, original_net_count, expanded_nets, frame1_net_ids, frame2_net_ids);

	/* pi[]：元の PI（共有ノード）＋ ppi（DFF の1時刻目コピー＝擬似PI）。
	   この順がキューブ文字列・BDD変数の並びになる */
	int expanded_pi_count = original_pi_count + n_ppi;
	NLIST** new_pi = (NLIST**)malloc(expanded_pi_count * sizeof(NLIST*));
	for (int k = 0; k < original_pi_count; k++) new_pi[k] = &expanded_nets[frame1_net_ids[original_pi[k]->n]];
	for (int d = 0; d < n_ppi; d++)  new_pi[original_pi_count + d] = ppi[d];

	/* po[]：2時刻目の PO コピー（非観測。観測点は ppo[] 側） */
	NLIST** new_po = (NLIST**)malloc(original_po_count * sizeof(NLIST*));
	for (int k = 0; k < original_po_count; k++) new_po[k] = &expanded_nets[frame2_net_ids[original_po[k]->n]];

	/* 旧ネットリストを解放して差し替え */
	for (int i = 0; i < original_net_count; i++) {
		free(original_nets[i].name);
		free(original_nets[i].in);
		free(original_nets[i].out);
		free(original_nets[i].name_ins);
		free(original_nets[i].name_port);
	}
	free(original_nets);
	free(original_pi);
	free(original_po);
	free(dff);
	dff = NULL;
	free(rdff);
	rdff = NULL;
	free(dffs);
	dffs = NULL;
	free(rdffs);
	rdffs = NULL;
	free(assign);
	assign = NULL;

	nl = expanded_nets;
	n_net = expanded_net_count;
	pi = new_pi;
	n_pi = expanded_pi_count;
	po = new_po;
	n_po = original_po_count;
	n_dff = 0;
	n_rdff = 0;
	n_dffs = 0;
	n_rdffs = 0;
	n_assign = 0;

	free(frame1_net_ids);
	free(frame2_net_ids);

	printf("	TDF time-expansion: nets=%d (was %d), pi=%d (PI %d + ppi %d), po=%d (unobserved), ppo=%d\n",
	       n_net, original_net_count, n_pi, original_pi_count, n_ppi, n_po, n_ppo);
}

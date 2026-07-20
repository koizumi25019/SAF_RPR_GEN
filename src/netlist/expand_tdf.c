//--------------------------------------------------------------------------------------------------------------------
//	expand_tdf.c : 遷移故障（TDF, LOC方式）用の2時刻展開ネットリスト構築
//
//	read_nl() が構築した DFF 入りの順序回路グラフを、1時刻目＋2時刻目の
//	組み合わせ回路（時間展開モデル）に組み替えて nl/pi/po を差し替える。
//	以降のパイプライン（CNF生成・キューブ列挙・BDD・GT検証）は
//	「入力 n_pi+n_dff 本の組み合わせ回路」として無修正で動作する。
//
//	LOC（launch-on-capture）の意味論：
//	  - PI は両時刻で共通（1ノードを共有。v2 の PI = v1 の PI）
//	  - DFF の Q は、1時刻目コピー＝自由入力（擬似PI、スキャンで設定される状態）
//	                2時刻目コピー＝1時刻目の D 入力を受ける BUF（DFF の値引き継ぎ）
//	  - 観測点は 2時刻目の PO と PPO（DFF の D 入力）。展開後はどちらも
//	    n_out==0 の端点になるので、既存の観測点判定（SearchTFO / gt_verify）が
//	    そのまま成立する。1時刻目の PO 端点は正常・故障で常に一致（故障は
//	    2時刻目コーンのみ）なので検出に寄与しない。
//
//	TDF 故障 (L, STR) は「2時刻目コピー L の sa0 ＋ 励起条件 L の1時刻目=0」に
//	帰着する（STF は sa1 ＋ 1時刻目=1）。励起条件用に、2時刻目コピーから
//	1時刻目コピーへのポインタを peer_1t に持たせる（PI は自分自身）。
//	信号線名は 2時刻目コピーが元の名前を保持し（故障リスト・CSV は元の名前で
//	引ける）、1時刻目コピーには "_1t" を付ける。
//--------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./netlist.h"

#define TDF_YES 1   /* netlist.h には YES/NO が無いので test_sa0/test_sa1 用に定義 */

/* 元ノード o のフレーム f (1/2) コピーが持つ出力本数。
   DFF へ向かう辺は f=1 なら「2時刻目の BUF へ」付け替え、f=2 なら（3時刻目が
   無いので）落とす。落とした結果 n_out==0 になった信号線が PPO 端点になる。 */
static int count_out(const NLIST* o, int f)
{
	int c = 0;
	for (int k = 0; k < o->n_out; k++) {
		if (o->out[k]->type == DFF) { if (f == 1) c++; }
		else c++;
	}
	return c;
}

/* 元ノード o のフレーム f の出力配列を dst に書く（count_out と同じ規則） */
static void fill_out(NLIST** dst, const NLIST* o, int f, NLIST* nn, const int* m1, const int* m2)
{
	int c = 0;
	for (int k = 0; k < o->n_out; k++) {
		const NLIST* t = o->out[k];
		if (t->type == DFF) {
			if (f == 1) dst[c++] = &nn[m2[t->n]];
		}
		else {
			dst[c++] = &nn[(f == 1) ? m1[t->n] : m2[t->n]];
		}
	}
}

static char* dup_name(const char* base, int frame)
{
	if (frame == 2) return strdup(base);
	size_t len = strlen(base);
	char* s = (char*)malloc(len + 4);            /* "_1t" + '\0' */
	memcpy(s, base, len);
	memcpy(s + len, "_1t", 4);
	return s;
}

/* 新ノードの共通フィールド初期化（calloc 済み前提で残りを埋める） */
static void init_node(NLIST* nd, int id, int type, char* name)
{
	nd->n        = id;
	nd->type     = type;
	nd->name     = name;
	nd->name_ins = strdup(name);
	nd->name_port = (char*)NULL;
	nd->test_sa0 = TDF_YES;   /* 等価故障エコー(OutputEquivFaults)を確実に無効化 */
	nd->test_sa1 = TDF_YES;
	nd->ppo_flag = 0;         /* 観測点は make_ppo_ppi が PPO にだけ立てる */
}

/* NEW_RPR_FAULT の make_ppo_ppi と同じ役割：疑似外部入出力の配列を作り、
   観測点フラグ(ppo_flag)を立てる。展開回路上では
     ppi[] = DFF状態の1時刻目コピー（自由入力＝スキャンで設定される状態変数）
     ppo[] = DFFのD入力の2時刻目コピー（FFへのキャプチャ点＝唯一の観測点）
   PO は at-speed でストローブしないので ppo_flag を立てない（旧ツールと同じ意味論）。 */
static void make_ppo_ppi(const NLIST* old, int old_n, NLIST* nn, const int* m1, const int* m2)
{
	ppi = (NLIST**)malloc(sizeof(NLIST*) * n_dff);
	ppo = (NLIST**)malloc(sizeof(NLIST*) * n_dff);
	if (ppi == NULL || ppo == NULL) { printf("make_ppo_ppi: allocation failed\n"); exit(1); }
	n_ppi = 0;
	n_ppo = 0;

	for (int i = 0; i < old_n; i++) {
		if (old[i].type != DFF) continue;
		ppi[n_ppi++] = &nn[m1[i]];                    /* Q の1時刻目コピー */
		ppo[n_ppo]   = &nn[m2[old[i].in[0]->n]];      /* D 入力の2時刻目コピー */
		ppo[n_ppo]->ppo_flag = 1;
		n_ppo++;
	}
}

void expand_tdf_netlist(void)
{
	NLIST*  old      = nl;
	int     old_n    = n_net;
	NLIST** old_pi   = pi;
	int     old_npi  = n_pi;
	NLIST** old_po   = po;
	int     old_npo  = n_po;

	/* 新ノード ID の割り当て：PI は両時刻共有で1個、それ以外は各時刻1個ずつ */
	int* m1 = (int*)malloc(old_n * sizeof(int));   /* 元ID -> 1時刻目コピーの新ID */
	int* m2 = (int*)malloc(old_n * sizeof(int));   /* 元ID -> 2時刻目コピーの新ID */
	int  new_n = 0;
	for (int i = 0; i < old_n; i++) {
		if (old[i].type == IN) { m1[i] = m2[i] = new_n++; }
		else                   { m1[i] = new_n++; m2[i] = new_n++; }
	}

	NLIST* nn = (NLIST*)calloc((size_t)new_n, sizeof(NLIST));
	if (nn == NULL) { printf("expand_tdf_netlist: allocation failed\n"); exit(1); }

	for (int i = 0; i < old_n; i++) {
		const NLIST* o = &old[i];

		if (o->type == IN) {
			/* 両時刻共有の PI。出力は両フレームのコピーへつながる */
			NLIST* nd = &nn[m1[i]];
			init_node(nd, m1[i], IN, dup_name(o->name, 2));
			nd->peer_1t = nd;                       /* PI の1時刻目=自分自身 */
			nd->n_out = count_out(o, 1) + count_out(o, 2);
			if (nd->n_out > 0) {
				nd->out = (NLIST**)malloc(nd->n_out * sizeof(NLIST*));
				fill_out(nd->out, o, 1, nn, m1, m2);
				fill_out(nd->out + count_out(o, 1), o, 2, nn, m1, m2);
			}
		}
		else if (o->type == DFF) {
			/* 1時刻目コピー：自由入力（擬似PI＝スキャン設定される状態変数） */
			NLIST* d1 = &nn[m1[i]];
			init_node(d1, m1[i], IN, dup_name(o->name, 1));
			d1->n_out = count_out(o, 1);
			if (d1->n_out > 0) {
				d1->out = (NLIST**)malloc(d1->n_out * sizeof(NLIST*));
				fill_out(d1->out, o, 1, nn, m1, m2);
			}

			/* 2時刻目コピー：1時刻目の D 入力を受ける BUF（値引き継ぎ） */
			NLIST* d2 = &nn[m2[i]];
			init_node(d2, m2[i], BUF, dup_name(o->name, 2));
			d2->peer_1t = d1;
			d2->n_in = 1;
			d2->in = (NLIST**)malloc(sizeof(NLIST*));
			d2->in[0] = &nn[m1[o->in[0]->n]];
			d2->n_out = count_out(o, 2);
			if (d2->n_out > 0) {
				d2->out = (NLIST**)malloc(d2->n_out * sizeof(NLIST*));
				fill_out(d2->out, o, 2, nn, m1, m2);
			}
		}
		else {
			/* 通常ゲート（FOUT 分岐含む）：各時刻に同型コピー */
			for (int f = 1; f <= 2; f++) {
				int id = (f == 1) ? m1[i] : m2[i];
				NLIST* nd = &nn[id];
				init_node(nd, id, o->type, dup_name(o->name, f));
				if (f == 2) nd->peer_1t = &nn[m1[i]];
				nd->n_in = o->n_in;
				if (nd->n_in > 0) {
					nd->in = (NLIST**)malloc(nd->n_in * sizeof(NLIST*));
					for (int j = 0; j < o->n_in; j++)
						nd->in[j] = &nn[(f == 1) ? m1[o->in[j]->n] : m2[o->in[j]->n]];
				}
				nd->n_out = count_out(o, f);
				if (nd->n_out > 0) {
					nd->out = (NLIST**)malloc(nd->n_out * sizeof(NLIST*));
					fill_out(nd->out, o, f, nn, m1, m2);
				}
			}
		}
	}

	/* 疑似外部入出力の構築と観測点フラグ設定（NEW_RPR_FAULT の make_ppo_ppi 相当）。
	   DFF へ向かう辺は f=2 で落とされているので、ppo[] の各ノードは n_out==0 の端点 */
	make_ppo_ppi(old, old_n, nn, m1, m2);

	/* pi[]：元の PI（共有ノード）＋ ppi（DFF の1時刻目コピー＝擬似PI）。
	   この順がキューブ文字列・BDD変数の並びになる */
	int new_npi = old_npi + n_ppi;
	NLIST** new_pi = (NLIST**)malloc(new_npi * sizeof(NLIST*));
	for (int k = 0; k < old_npi; k++) new_pi[k] = &nn[m1[old_pi[k]->n]];
	for (int d = 0; d < n_ppi; d++)  new_pi[old_npi + d] = ppi[d];

	/* po[]：2時刻目の PO コピー（非観測。観測点は ppo[] 側） */
	NLIST** new_po = (NLIST**)malloc(old_npo * sizeof(NLIST*));
	for (int k = 0; k < old_npo; k++) new_po[k] = &nn[m2[old_po[k]->n]];
	int new_npo = old_npo;

	/* 旧ネットリストを解放して差し替え */
	for (int i = 0; i < old_n; i++) {
		free(old[i].name);
		free(old[i].in);
		free(old[i].out);
		free(old[i].name_ins);
		free(old[i].name_port);
	}
	free(old);
	free(old_pi);
	free(old_po);
	free(dff);   dff   = (NLIST**)NULL;
	free(rdff);  rdff  = (NLIST**)NULL;
	free(dffs);  dffs  = (NLIST**)NULL;
	free(rdffs); rdffs = (NLIST**)NULL;
	free(assign); assign = (NLIST**)NULL;

	nl    = nn;     n_net = new_n;
	pi    = new_pi; n_pi  = new_npi;
	po    = new_po; n_po  = new_npo;
	n_dff = 0; n_rdff = 0; n_dffs = 0; n_rdffs = 0; n_assign = 0;

	free(m1);
	free(m2);

	printf("	TDF time-expansion: nets=%d (was %d), pi=%d (PI %d + ppi %d), po=%d (unobserved), ppo=%d\n",
	       n_net, old_n, n_pi, old_npi, n_ppi, n_po, n_ppo);
}

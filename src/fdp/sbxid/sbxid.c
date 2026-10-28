/* Port of SB-XID/src/saxid.c, v1 without CEGAR.
 * Reference: koizumi25019/SAF_RPR_GEN@52bb7ac268d2425ab2a8089c7b929fa87d262322
 * The multi-fault formula is called with K=1. Signals outside each fault's
 * TFO reuse V variables and justification, retaining both objective weights.
 * See README.md for the changes from the source.
 */
#define _POSIX_C_SOURCE 200809L
#include "sbxid.h"
#include "../../netlist/netlist.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sbxid_fatal(const char* reason)
{
    fprintf(stderr, "[SB-XID] %s\n", reason);
    exit(EXIT_FAILURE);
}
static void* sbxid_malloc(size_t bytes)
{
    void* p = malloc(bytes ? bytes : 1);
    if (!p) sbxid_fatal("memory allocation failed");
    return p;
}
static void* sbxid_calloc(size_t count, size_t width)
{
    if (width && count > SIZE_MAX / width) sbxid_fatal("allocation size overflow");
    void* p = calloc(count ? count : 1, width ? width : 1);
    if (!p) sbxid_fatal("memory allocation failed");
    return p;
}
typedef struct {
    char name[256];
    int  net_id;   // get_net_indexの結果。-1なら未解決（エラー）
    int  type;     // 0 or 1 (S-A-0 / S-A-1)
} SbxidFaultSpec;

typedef struct {
    SbxidFaultSpec spec;
    int*  fault_values;      // [n_net] この故障単独を注入したときのsignal値
    char* is_path;      // [n_net] この故障単独のPATHメンバーシップ(0/1)
    char* is_justify;   // [n_net] この故障単独のJUSTIFYメンバーシップ(0/1)
    int*  varS;         // [n_net] この故障のS側OPB変数番号(0=未割当)
} SbxidFaultContext;

// ケース全体（1パターン×複数故障）のコンテキスト。
typedef struct {
    SbxidFaultContext* f;         // [n_faults]
    int n_faults;
    char* union_path;      // [n_net] いずれかの故障のPATHに属するか(V側の膜)
    char* union_justify;   // [n_net] いずれかの故障のJUSTIFYに属するか(V側の膜)
} SbxidCase;

typedef struct { int max, num, front, rear; NLIST** que; } SbxidQueue;
static SbxidQueue sbxid_queue;
static int* sbxid_index;
static int sbxid_index_size;
static char** sbxid_index_keys;
static int* sbxid_pi_of_net;
static unsigned char* sbxid_fault_tfo;
static int sbxid_cached_fault_net_id = -1;
static const int sbxid_emit_var_map = 0;
static void sbxid_load_fault_signal(int k, SbxidCase* mc) {
    int* sig = mc->f[k].fault_values;
    for (int i = 0; i < n_net; i++) nl[i].sbxid_fault_value = sig[i];
}

static int sbxid_ternary_not(int v) {
    if (v == SBXID_X) return SBXID_X;
    return v ? 0 : 1;
}

static void sbxid_init_queue(int size) {
    sbxid_queue.max = size; sbxid_queue.num = 0; sbxid_queue.front = 0; sbxid_queue.rear = 0;
    sbxid_queue.que = (NLIST**)sbxid_malloc(sizeof(NLIST*) * size);
}

static void sbxid_enqueue(NLIST* node) {
    if (sbxid_queue.num >= sbxid_queue.max) sbxid_fatal("simulation queue overflow");
    sbxid_queue.que[sbxid_queue.rear] = node;
    sbxid_queue.rear = (sbxid_queue.rear + 1) % sbxid_queue.max;
    sbxid_queue.num++;
}

static NLIST* sbxid_dequeue() {
    if (sbxid_queue.num <= 0) return NULL;
    NLIST* node = sbxid_queue.que[sbxid_queue.front];
    sbxid_queue.front = (sbxid_queue.front + 1) % sbxid_queue.max;
    sbxid_queue.num--;
    return node;
}

static int sbxid_is_queue_empty() { return (sbxid_queue.num <= 0); }

/* 各worker専用。構造上のTFOはseed・縮退値によらないため故障点ごとに再利用する。 */
static const unsigned char* sbxid_get_fault_tfo(int fault_net_id) {
    if (sbxid_cached_fault_net_id == fault_net_id) return sbxid_fault_tfo;
    if (!sbxid_fault_tfo) sbxid_fault_tfo = sbxid_calloc((size_t)n_net, 1);
    else memset(sbxid_fault_tfo, 0, (size_t)n_net);
    sbxid_init_queue(n_net);
    sbxid_fault_tfo[fault_net_id] = 1;
    sbxid_enqueue(&nl[fault_net_id]);
    while (!sbxid_is_queue_empty()) {
        NLIST* node = sbxid_dequeue();
        for (int j = 0; j < node->n_out; ++j) {
            NLIST* out = node->out[j];
            if (sbxid_fault_tfo[out->n]) continue;
            sbxid_fault_tfo[out->n] = 1;
            sbxid_enqueue(out);
        }
    }
    free(sbxid_queue.que);
    sbxid_queue.que = NULL;
    sbxid_cached_fault_net_id = fault_net_id;
    return sbxid_fault_tfo;
}

static unsigned long sbxid_djb2_hash(const char* s) {
    unsigned long h = 5381;
    for (; *s; s++) h = ((h << 5) + h) + (unsigned char)*s; // h*33 + c
    return h;
}

static void sbxid_build_net_index_table(void) {
    free(sbxid_index); free(sbxid_index_keys);
    sbxid_index_size = 1;
    while (sbxid_index_size < n_net * 2) sbxid_index_size <<= 1; // 荷重率 < 0.5
    if (sbxid_index_size < 8) sbxid_index_size = 8;
    sbxid_index = (int*)sbxid_malloc(sizeof(int) * sbxid_index_size);
    sbxid_index_keys = (char**)sbxid_malloc(sizeof(char*) * sbxid_index_size);
    for (int i = 0; i < sbxid_index_size; i++) { sbxid_index[i] = -1; sbxid_index_keys[i] = NULL; }

    unsigned int mask = (unsigned int)sbxid_index_size - 1;
    for (int i = 0; i < n_net; i++) {
        unsigned int slot = (unsigned int)sbxid_djb2_hash(nl[i].name) & mask;
        while (sbxid_index_keys[slot] != NULL) slot = (slot + 1) & mask; // 線形探査
        sbxid_index_keys[slot] = nl[i].name;
        sbxid_index[slot] = i;
    }
}

static int sbxid_get_net_index(const char* name) {
    if (sbxid_index == NULL) {
        // 表未構築時のフォールバック（従来動作）
        for (int i = 0; i < n_net; i++)
            if (strcmp(nl[i].name, name) == 0) return i;
        return -1;
    }
    unsigned int mask = (unsigned int)sbxid_index_size - 1;
    unsigned int slot = (unsigned int)sbxid_djb2_hash(name) & mask;
    while (sbxid_index_keys[slot] != NULL) {
        if (strcmp(sbxid_index_keys[slot], name) == 0) return sbxid_index[slot];
        slot = (slot + 1) & mask;
    }
    return -1;
}

static void sbxid_build_pi_of_net_table(void) {
    free(sbxid_pi_of_net);
    sbxid_pi_of_net = (int*)sbxid_malloc(sizeof(int) * n_net);
    for (int i = 0; i < n_net; i++) sbxid_pi_of_net[i] = -1;
    for (int j = 0; j < n_pi; j++) {
        int idx = sbxid_get_net_index(pi[j]->name);
        if (idx >= 0) sbxid_pi_of_net[idx] = j;
    }
}

static void sbxid_assign_pi_values(const char* pattern_str) {
    for (int i = 0; i < n_pi; i++) {
        char c = pattern_str[i];
        int val;
        if (c == '-' || c == 'X' || c == 'x') {
            val = SBXID_X;
        }
        else {
            val = (c == '1') ? 1 : 0;
        }
        pi[i]->sbxid_normal_value = val;
        pi[i]->sbxid_fault_value = val;
    }
}

static int sbxid_evaluate_gate_logic(NLIST* node, int is_fault_mode) {
    int type = node->type;
    int n_in = node->n_in;
    NLIST** in = node->in;
    int has_0 = 0, has_1 = 0, has_x = 0;
    for (int i = 0; i < n_in; i++) {
        int in_val = is_fault_mode ? in[i]->sbxid_fault_value : in[i]->sbxid_normal_value;
        if (in_val == 0) has_0 = 1;
        else if (in_val == 1) has_1 = 1;
        else has_x = 1;
    }
    int res = 0;
    switch (type) {
    case AND:   res = has_0 ? 0 : (has_x ? SBXID_X : 1); break;
    case NAND:  res = has_0 ? 1 : (has_x ? SBXID_X : 0); break;
    case OR:    res = has_1 ? 1 : (has_x ? SBXID_X : 0); break;
    case NOR:   res = has_1 ? 0 : (has_x ? SBXID_X : 1); break;
    case EXOR: {
        if (has_x) res = SBXID_X;
        else {
            int parity = 0;
            for (int i = 0; i < n_in; i++) {
                int in_val = is_fault_mode ? in[i]->sbxid_fault_value : in[i]->sbxid_normal_value;
                parity ^= (in_val & 1);
            }
            res = parity;
        }
    } break;
    case EXNOR: {
        if (has_x) res = SBXID_X;
        else {
            int parity = 0;
            for (int i = 0; i < n_in; i++) {
                int in_val = is_fault_mode ? in[i]->sbxid_fault_value : in[i]->sbxid_normal_value;
                parity ^= (in_val & 1);
            }
            res = !parity;
        }
    } break;
    case INV: {
        int v = is_fault_mode ? in[0]->sbxid_fault_value : in[0]->sbxid_normal_value;
        res = sbxid_ternary_not(v);
    } break;
    case BUF:
    case FOUT:  res = is_fault_mode ? in[0]->sbxid_fault_value : in[0]->sbxid_normal_value; break;
    case GND:   res = 0; break;
    case ACC:   res = 1; break;
    default:    sbxid_fatal("unsupported simulated gate");
    }
    return res;
}

static void sbxid_clear_all_flags() {
    for (int i = 0; i < n_net; i++) nl[i].sbxid_flag = 0;
}

static void sbxid_run_simulation_logic(int fault_net_id, int fault_type) {
    // 探索フラグ(PATH, JUSTIFY)を維持し、評価フラグのみクリア
    for (int i = 0; i < n_net; i++) nl[i].sbxid_flag &= ~SBXID_FLAG_EVALUATED;
    sbxid_init_queue(n_net * 10);
    for (int i = 0; i < n_pi; i++) sbxid_enqueue(pi[i]);

    while (!sbxid_is_queue_empty()) {
        NLIST* node = sbxid_dequeue();
        int old_val = node->sbxid_normal_value;
        int old_sig = node->sbxid_fault_value;

        if (node->type != IN) {
            node->sbxid_normal_value = sbxid_evaluate_gate_logic(node, 0);
            node->sbxid_fault_value = sbxid_evaluate_gate_logic(node, 1);
        }

        if (fault_net_id != -1 && node == &nl[fault_net_id]) {
            node->sbxid_fault_value = fault_type;
        }

        if (!(node->sbxid_flag & SBXID_FLAG_EVALUATED) || old_val != node->sbxid_normal_value || old_sig != node->sbxid_fault_value) {
            node->sbxid_flag |= SBXID_FLAG_EVALUATED;
            for (int i = 0; i < node->n_out; i++) {
                sbxid_enqueue(node->out[i]);
            }
        }
    }
    free(sbxid_queue.que);
    sbxid_queue.que = NULL;
}

static int sbxid_verify_pattern_detects_fault(const char* pat, int fault_net_id, int fault_type) {
    // 固定長 1024 では b17_C.v（入力1452本）でスタックを踏み越えるため、
    // 入力数から必要量を確保する（n_pi は起動時に確定する厳密値）。
    char* buf = (char*)sbxid_malloc((size_t)n_pi + 1);
    if (!buf) return 0;
    size_t plen = strlen(pat);
    if (plen > (size_t)n_pi) plen = (size_t)n_pi;
    memcpy(buf, pat, plen);
    for (size_t i = plen; i < (size_t)n_pi; i++) buf[i] = '-';
    buf[n_pi] = '\0';

    sbxid_assign_pi_values(buf);
    sbxid_run_simulation_logic(fault_net_id, fault_type);
    free(buf);

    for (int i = 0; i < n_po; i++) {
        if (po[i]->sbxid_normal_value != SBXID_X && po[i]->sbxid_fault_value != SBXID_X && po[i]->sbxid_normal_value != po[i]->sbxid_fault_value) {
            return 1;
        }
    }
    return 0;
}

static int sbxid_extract_single_bundle_path(int fault_net_id, int silent) {
    sbxid_init_queue(n_net * 10);
    int found_po = 0;

    for (int i = 0; i < n_po; i++) {
        if (po[i]->sbxid_normal_value != po[i]->sbxid_fault_value) {
            sbxid_enqueue(po[i]);
            found_po++;
        }
    }

    if (found_po == 0) {
        free(sbxid_queue.que);
        sbxid_queue.que = NULL;
        return 0;
    }

    if (!silent) printf("【パス抽出】故障が観測された %d 個の外部出力を起点に、伝搬経路全体を抽出します。\n", found_po);

    while (!sbxid_is_queue_empty()) {
        NLIST* current = sbxid_dequeue();
        if (current->sbxid_flag & SBXID_FLAG_PATH) continue;
        current->sbxid_flag |= SBXID_FLAG_PATH;

        if (current == &nl[fault_net_id]) continue;

        for (int i = 0; i < current->n_in; i++) {
            if (current->in[i]->sbxid_normal_value != current->in[i]->sbxid_fault_value) {
                sbxid_enqueue(current->in[i]);
            }
        }
    }
    free(sbxid_queue.que);
    sbxid_queue.que = NULL;
    return 1;
}

static int sbxid_get_controlling_value(int gate_type) {
    switch (gate_type) {
    case AND: case NAND: return 0;
    case OR:  case NOR:  return 1;
    default: return -1;
    }
}

static int sbxid_get_out_controlling_value(int gate_type, int c_val) {
    if (gate_type == NAND || gate_type == NOR) return !c_val;
    return c_val;
}

static void sbxid_extract_justification_cone(int fault_net_id) {
    (void)fault_net_id;
    sbxid_init_queue(n_net * 10);

    for (int i = 0; i < n_net; i++) {
        if (nl[i].sbxid_flag & SBXID_FLAG_PATH) {
            for (int j = 0; j < nl[i].n_in; j++) {
                if (!(nl[i].in[j]->sbxid_flag & SBXID_FLAG_PATH)) {
                    sbxid_enqueue(nl[i].in[j]);
                }
            }
        }
    }
    while (!sbxid_is_queue_empty()) {
        NLIST* node = sbxid_dequeue();
        if (node->sbxid_flag & SBXID_FLAG_PATH) continue;
        if (node->sbxid_flag & SBXID_FLAG_JUSTIFY) continue;

        node->sbxid_flag |= SBXID_FLAG_JUSTIFY;
        for (int i = 0; i < node->n_in; i++) sbxid_enqueue(node->in[i]);
    }
    free(sbxid_queue.que);
    sbxid_queue.que = NULL;
}

static int sbxid_build_multi_case(SbxidCase* mc, SbxidFaultSpec* specs, int n_faults, const char* pat_str) {
    mc->n_faults = n_faults;
    mc->f = (SbxidFaultContext*)sbxid_calloc((size_t)n_faults, sizeof(SbxidFaultContext));
    mc->union_path = (char*)sbxid_calloc((size_t)n_net, 1);
    mc->union_justify = (char*)sbxid_calloc((size_t)n_net, 1);
    if (!mc->f || !mc->union_path || !mc->union_justify) return 0;

    int all_detected = 1;
    for (int k = 0; k < n_faults; k++) {
        mc->f[k].spec = specs[k];
        mc->f[k].fault_values = (int*)sbxid_calloc((size_t)n_net, sizeof(int));
        mc->f[k].is_path = (char*)sbxid_calloc((size_t)n_net, 1);
        mc->f[k].is_justify = (char*)sbxid_calloc((size_t)n_net, 1);
        mc->f[k].varS = NULL; // generate_opb_for_multi_bundleで割り当てる
        sbxid_assign_pi_values(pat_str);
        sbxid_clear_all_flags();
        sbxid_run_simulation_logic(specs[k].net_id, specs[k].type);
        for (int i = 0; i < n_net; i++) mc->f[k].fault_values[i] = nl[i].sbxid_fault_value;

        sbxid_clear_all_flags();
        int got = 0;
        for (int p = 0; p < n_po; ++p)
            if (po[p]->sbxid_normal_value != SBXID_X && po[p]->sbxid_fault_value != SBXID_X &&
                po[p]->sbxid_normal_value != po[p]->sbxid_fault_value) got = 1;
        if (got) got = sbxid_extract_single_bundle_path(specs[k].net_id, 1);
        if (!got) { all_detected = 0; continue; } // このパターンは故障kを検出しない→入力不整合
        sbxid_extract_justification_cone(specs[k].net_id);
        for (int i = 0; i < n_net; i++) {
            int is_p = (nl[i].sbxid_flag & SBXID_FLAG_PATH) ? 1 : 0;
            int is_j = (nl[i].sbxid_flag & SBXID_FLAG_JUSTIFY) ? 1 : 0;
            mc->f[k].is_path[i] = (char)is_p;
            mc->f[k].is_justify[i] = (char)is_j;
            if (is_p) mc->union_path[i] = 1;
            if (is_j) mc->union_justify[i] = 1;
        }
    }
    for (int i = 0; i < n_net; i++) {
        if (mc->union_path[i]) mc->union_justify[i] = 0;
    }
    return all_detected;
}

static void sbxid_free_multi_case(SbxidCase* mc) {
    if (!mc->f) return;
    for (int k = 0; k < mc->n_faults; k++) {
        free(mc->f[k].fault_values);
        free(mc->f[k].is_path);
        free(mc->f[k].is_justify);
        free(mc->f[k].varS);
    }
    free(mc->f); mc->f = NULL;
    free(mc->union_path); mc->union_path = NULL;
    free(mc->union_justify); mc->union_justify = NULL;
}

static void sbxid_CreateConsEss_StandardGate(FILE* fp, NLIST* g, int c_val, int out_c_val, int target_val, int use_signal, int* varess, int* cons_count) {
    int g_idx = sbxid_get_net_index(g->name);
    int out_v = varess[g_idx];
    if (out_v <= 0) return;

    if (target_val == out_c_val) {
        fprintf(fp, "-1 x%d ", out_v);
        for (int i = 0; i < g->n_in; i++) {
            int in_idx = sbxid_get_net_index(g->in[i]->name);
            int in_v = varess[in_idx];
            int in_side_val = use_signal ? g->in[i]->sbxid_fault_value : g->in[i]->sbxid_normal_value;
            if (in_v > 0 && in_side_val == c_val) {
                fprintf(fp, "+1 x%d ", in_v);
            }
        }
        fprintf(fp, ">= 0;  * Justify(OR) Target=%d Ch=%s\n", target_val, use_signal ? "S" : "V");
        (*cons_count)++;
    }
    else {
        for (int i = 0; i < g->n_in; i++) {
            int in_idx = sbxid_get_net_index(g->in[i]->name);
            int in_v = varess[in_idx];
            if (in_v <= 0) continue;
            fprintf(fp, "-1 x%d +1 x%d >= 0;  * Justify(AND) Target=%d Ch=%s\n", out_v, in_v, target_val, use_signal ? "S" : "V");
            (*cons_count)++;
        }
    }
}

static void sbxid_CreateConsEss_SpecialGate(FILE* fp, NLIST* g, int* varess, int* cons_count) {
    int g_idx = sbxid_get_net_index(g->name);
    int out_v = varess[g_idx];
    switch (g->type) {
    case EXOR:
    case EXNOR:
        for (int i = 0; i < g->n_in; i++) {
            int in_idx = sbxid_get_net_index(g->in[i]->name);
            int in_v = varess[in_idx];
            if (in_v > 0) {
                fprintf(fp, "-1 x%d +1 x%d >= 0;\n", out_v, in_v);
                (*cons_count)++;
            }
        }
        break;
    case INV:
    case BUF:
    case FOUT:
        if (g->n_in > 0) {
            int in_idx = sbxid_get_net_index(g->in[0]->name);
            int in_v = varess[in_idx];
            if (in_v > 0) {
                fprintf(fp, "-1 x%d +1 x%d >= 0;\n", out_v, in_v);
                (*cons_count)++;
            }
        }
        break;
    }
}

static const char* sbxid_gate_type_name(int type) {
    switch (type) {
    case AND:   return "AND";
    case NAND:  return "NAND";
    case OR:    return "OR";
    case NOR:   return "NOR";
    case EXOR:  return "EXOR";
    case EXNOR: return "EXNOR";
    case INV:   return "INV";
    case BUF:   return "BUF";
    case FOUT:  return "FOUT";
    case GND:   return "GND";
    case ACC:   return "ACC";
    case IN:    return "IN";
    default:    return "UNK";
    }
}

static bool sbxid_generate_opb_for_multi_bundle(FILE* tmp_fp, int** out_var_to_net, int* out_total_vars, int* out_cons_count,
    SbxidCase* mc, int** out_varV, const unsigned char* pi_scope) {
    int K = mc->n_faults;
    int* varV = (int*)sbxid_calloc(n_net, sizeof(int));
    int vars = 0;

    // PI：故障によらず共有1変数（単一故障版と同じ）
    for (int i = 0; i < n_pi; i++) {
        int idx = sbxid_get_net_index(pi[i]->name);
        if (idx != -1) varV[idx] = ++vars;
    }
    // 内部ノード：V側はunion_path/union_justifyのどちらかに属すれば1変数
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN) continue;
        if (varV[i] == 0 && (mc->union_path[i] || mc->union_justify[i])) varV[i] = ++vars;
    }
    // 内部ノード：TFO内だけS側専用変数を作り、範囲外はV側を参照する。
    for (int k = 0; k < K; k++) {
        const unsigned char* fault_tfo = sbxid_get_fault_tfo(mc->f[k].spec.net_id);
        mc->f[k].varS = (int*)sbxid_calloc(n_net, sizeof(int));
        for (int i = 0; i < n_pi; i++) {
            int idx = sbxid_get_net_index(pi[i]->name);
            if (idx != -1) mc->f[k].varS[idx] = varV[idx]; // PIはV側と同じ変数を共有
        }
        for (int i = 0; i < n_net; i++) {
            if (nl[i].type == IN) continue;
            if (mc->f[k].varS[i] == 0 && (mc->f[k].is_path[i] || mc->f[k].is_justify[i]))
                mc->f[k].varS[i] = fault_tfo[i] ? ++vars : varV[i];
        }
    }

    // PO観測制約の補助変数：故障ごとに独立（その故障自身のPATH上のPOのみが候補）
    int** po_cand = (int**)sbxid_malloc(sizeof(int*) * (size_t)K);
    int** po_aux = (int**)sbxid_malloc(sizeof(int*) * (size_t)K);
    int* n_po_cand = (int*)sbxid_calloc((size_t)K, sizeof(int));
    for (int k = 0; k < K; k++) {
        po_cand[k] = (int*)sbxid_malloc(sizeof(int) * (size_t)(n_po > 0 ? n_po : 1));
        po_aux[k] = (int*)sbxid_malloc(sizeof(int) * (size_t)(n_po > 0 ? n_po : 1));
        for (int i = 0; i < n_po; i++) {
            int po_idx = sbxid_get_net_index(po[i]->name);
            if (po_idx != -1 && mc->f[k].is_path[po_idx] && varV[po_idx] > 0) {
                po_cand[k][n_po_cand[k]++] = po_idx;
            }
        }
        for (int j = 0; j < n_po_cand[k]; j++) po_aux[k][j] = ++vars;
    }

    int* var_to_net = (int*)sbxid_malloc(sizeof(int) * (size_t)(vars + 1));
    for (int i = 0; i <= vars; i++) var_to_net[i] = -1;
    for (int i = 0; i < n_net; i++) if (varV[i] > 0) var_to_net[varV[i]] = i;
    for (int k = 0; k < K; k++)
        for (int i = 0; i < n_net; i++)
            if (mc->f[k].varS[i] > 0) var_to_net[mc->f[k].varS[i]] = i;
    *out_var_to_net = var_to_net;
    *out_total_vars = vars;

    fprintf(tmp_fp, "* #variable= %-12d #constraint= %-12d\n\n", 0, 0);

    // 目的関数：PI確定数の最小化（主目的、PIは共有変数なので1回だけ計上）
    int weight_pi = (K + 1) * n_net + 1;
    fprintf(tmp_fp, "min: ");
    for (int i = 0; i < n_pi; i++) {
        if (pi_scope && !pi_scope[i]) continue;
        int idx = sbxid_get_net_index(pi[i]->name);
        if (idx != -1 && varV[idx] > 0) fprintf(tmp_fp, "+%d x%d ", weight_pi, varV[idx]);
    }
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type != IN && varV[i] > 0) fprintf(tmp_fp, "-1 x%d ", varV[i]);
    }
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < n_net; i++) {
            // V側を参照する信号も両側分を計上する（K=1の共有変数では合計-2）。
            if (nl[i].type != IN && mc->f[k].varS[i] > 0) fprintf(tmp_fp, "-1 x%d ", mc->f[k].varS[i]);
        }
    }
    fprintf(tmp_fp, ";\n\n");

    int cons_count = 0;

    // 故障起点のhard-fix：故障ごとにV側・S_k側を確定
    for (int k = 0; k < K; k++) {
        int origin = mc->f[k].spec.net_id;
        if (varV[origin] > 0) {
            fprintf(tmp_fp, "1 x%d >= 1;  * Hard fix Fault Origin(V): %s\n", varV[origin], nl[origin].name);
            cons_count++;
        }
        if (mc->f[k].varS[origin] > 0 && mc->f[k].varS[origin] != varV[origin]) {
            fprintf(tmp_fp, "1 x%d >= 1;  * Hard fix Fault Origin(S%d): %s\n", mc->f[k].varS[origin], k, nl[origin].name);
            cons_count++;
        }
    }

    // PO観測制約：故障ごとに独立した「最低1PO」制約（K本、全てAND的に要求される）
    for (int k = 0; k < K; k++) {
        if (n_po_cand[k] == 0) continue;
        for (int j = 0; j < n_po_cand[k]; j++) {
            int po_idx = po_cand[k][j];
            int y = po_aux[k][j];
            fprintf(tmp_fp, "-1 x%d +1 x%d >= 0;  * PO-AND(V) aux fault%d for %s\n", y, varV[po_idx], k, nl[po_idx].name);
            cons_count++;
            fprintf(tmp_fp, "-1 x%d +1 x%d >= 0;  * PO-AND(S%d) aux for %s\n", y, mc->f[k].varS[po_idx], k, nl[po_idx].name);
            cons_count++;
        }
        for (int j = 0; j < n_po_cand[k]; j++) fprintf(tmp_fp, "+1 x%d ", po_aux[k][j]);
        fprintf(tmp_fp, ">= 1;  * Observe at least one PO for fault%d (V and S%d both fixed)\n", k, k);
        cons_count++;
    }

    // 後方含意制約（パスの一意化）：V側はunion_path、S側は故障ごとのis_path
    for (int i = 0; i < n_net; i++) {
        if (mc->union_path[i] && varV[i] > 0) {
            size_t capacity = (size_t)nl[i].n_in * 32 + 64;
            char* lineV = sbxid_malloc(capacity);
            int lenV = snprintf(lineV, capacity, "-1 x%d ", varV[i]);
            int has_in = 0;
            for (int j = 0; j < nl[i].n_in; j++) {
                int in_idx = sbxid_get_net_index(nl[i].in[j]->name);
                if (in_idx != -1 && mc->union_path[in_idx] && varV[in_idx] > 0) {
                    lenV += snprintf(lineV + lenV, capacity - lenV, "+1 x%d ", varV[in_idx]);
                    has_in = 1;
                }
            }
            if (has_in) { fprintf(tmp_fp, "%s>= 0;  * Backward OR(V)\n", lineV); cons_count++; }
            free(lineV);
        }
    }
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < n_net; i++) {
            if (mc->f[k].is_path[i] && mc->f[k].varS[i] > 0) {
                size_t capacity = (size_t)nl[i].n_in * 32 + 64;
                char* lineS = sbxid_malloc(capacity);
                int lenS = snprintf(lineS, capacity, "-1 x%d ", mc->f[k].varS[i]);
                int has_in = 0;
                for (int j = 0; j < nl[i].n_in; j++) {
                    int in_idx = sbxid_get_net_index(nl[i].in[j]->name);
                    if (in_idx != -1 && mc->f[k].is_path[in_idx] && mc->f[k].varS[in_idx] > 0) {
                        lenS += snprintf(lineS + lenS, capacity - lenS, "+1 x%d ", mc->f[k].varS[in_idx]);
                        has_in = 1;
                    }
                }
                if (has_in) { fprintf(tmp_fp, "%s>= 0;  * Backward OR(S%d)\n", lineS, k); cons_count++; }
                free(lineS);
            }
        }
    }

    // 正当化制約：V側を生成し、S側は専用変数を持つTFO内だけ生成する。
    // sbxid_CreateConsEss_StandardGate/SpecialGateは無改造のまま呼ぶ。
    for (int i = 0; i < n_net; i++) {
        if (varV[i] <= 0 || !(mc->union_path[i] || mc->union_justify[i])) continue;
        int c_val = sbxid_get_controlling_value(nl[i].type);
        if (c_val != -1) {
            int out_c_val = sbxid_get_out_controlling_value(nl[i].type, c_val);
            sbxid_CreateConsEss_StandardGate(tmp_fp, &nl[i], c_val, out_c_val, nl[i].sbxid_normal_value, 0, varV, &cons_count);
        }
        else {
            sbxid_CreateConsEss_SpecialGate(tmp_fp, &nl[i], varV, &cons_count);
        }
    }
    for (int k = 0; k < K; k++) {
        sbxid_load_fault_signal(k, mc);
        for (int i = 0; i < n_net; i++) {
            if (mc->f[k].varS[i] <= 0 || !(mc->f[k].is_path[i] || mc->f[k].is_justify[i])) continue;
            if (mc->f[k].varS[i] == varV[i]) continue; // TFO外の正当化はV側で生成済み
            if (i == mc->f[k].spec.net_id) continue; // 故障起点のS側は入力由来の正当化を生成しない(hard-fixのみ)
            int c_val = sbxid_get_controlling_value(nl[i].type);
            if (c_val != -1) {
                int out_c_val = sbxid_get_out_controlling_value(nl[i].type, c_val);
                sbxid_CreateConsEss_StandardGate(tmp_fp, &nl[i], c_val, out_c_val, nl[i].sbxid_fault_value, 1, mc->f[k].varS, &cons_count);
            }
            else {
                sbxid_CreateConsEss_SpecialGate(tmp_fp, &nl[i], mc->f[k].varS, &cons_count);
            }
        }
    }

    if (sbxid_emit_var_map) {
        fprintf(tmp_fp, "\n* --- Variable Mapping for Standalone Parser (multi-fault) ---\n");
        for (int i = 1; i <= vars; i++) {
            int net_idx = var_to_net[i];
            if (net_idx < 0) { fprintf(tmp_fp, "* VAR_MAP: %d (PO-AND-aux)\n", i); continue; }
            const char* ch = "?";
            char chbuf[16];
            if (varV[net_idx] == i) ch = "V";
            else {
                for (int k = 0; k < K; k++) {
                    if (mc->f[k].varS[net_idx] == i) { snprintf(chbuf, sizeof(chbuf), "S%d", k); ch = chbuf; break; }
                }
            }
            fprintf(tmp_fp, "* VAR_MAP: %d %s %s\t%s\tCh=%s\n",
                i, nl[net_idx].name, (nl[net_idx].type == IN) ? "IN" : "", sbxid_gate_type_name(nl[net_idx].type), ch);
        }
    }

    long end_position = ftell(tmp_fp);
    bool okay = end_position >= 0 && fflush(tmp_fp) == 0 && fseek(tmp_fp, 0, SEEK_SET) == 0;
    fprintf(tmp_fp, "* #variable= %-12d #constraint= %-12d\n\n", vars, cons_count);
    /* open_memstream must finish at EOF after the header backpatch. */
    okay = !ferror(tmp_fp) && fseek(tmp_fp, end_position, SEEK_SET) == 0 && okay;

    *out_cons_count = cons_count;
    if (out_varV) *out_varV = varV; else free(varV);
    for (int k = 0; k < K; k++) { free(po_cand[k]); free(po_aux[k]); }
    free(po_cand); free(po_aux); free(n_po_cand);
    return okay;
}

static void sbxid_extract_pattern_from_solution(const char* pat_str, int* var_to_net, int total_vars, int* var_values,
    const unsigned char* pi_scope, char* out_pat) {
    for (int i = 0; i < n_pi; i++) out_pat[i] = '-';
    out_pat[n_pi] = '\0';
    for (int i = 1; i <= total_vars; i++) {
        int net_idx = var_to_net[i];
        if (net_idx < 0) continue; // PO補助変数（実ネットを持たない）
        if (nl[net_idx].type == IN) {
            int pi_idx = sbxid_pi_of_net[net_idx];
            if (pi_idx != -1 && (!pi_scope || pi_scope[pi_idx]) && var_values[i] == 1)
                out_pat[pi_idx] = pat_str[pi_idx];
        }
    }
}
static bool sbxid_initialize(void)
{
    if (sbxid_index) return true;
    if (!nl || n_net <= 0 || n_net > INT_MAX / 10 || n_pi < 1 || n_po < 1) {
        fprintf(stderr, "[SB-XID] invalid circuit dimensions\n");
        return false;
    }
    for (int i = 0; i < n_net; ++i) {
        switch (nl[i].type) {
        case IN: case BUF: case INV: case FOUT: case AND: case NAND:
        case OR: case NOR: case EXOR: case EXNOR: break;
        default:
            fprintf(stderr, "[SB-XID] unsupported gate %s (type=%d)\n", nl[i].name, nl[i].type);
            return false;
        }
    }
    sbxid_build_net_index_table();
    sbxid_build_pi_of_net_table();
    return true;
}

bool sbxid_generalize(const char* seed, int fault_net_id, int stuck_value,
                     const unsigned char* pi_scope, double limit_seconds,
                     char* cube, SbxidInfo* info)
{
    if (!info) return false;
    memset(info, 0, sizeof(*info));
    info->solve.status = SBXID_SOLVE_ERROR;
    if (!seed || !cube || !sbxid_initialize() || fault_net_id < 0 ||
        fault_net_id >= n_net || (stuck_value != 0 && stuck_value != 1) ||
        strlen(seed) != (size_t)n_pi) return false;
    for (int p = 0; p < n_pi; ++p)
        if (seed[p] != '0' && seed[p] != '1') return false;
    cube[0] = '\0';
    for (int i = 0; i < n_net; ++i) {
        nl[i].sbxid_normal_value = nl[i].sbxid_fault_value = SBXID_X;
        nl[i].sbxid_flag = 0;
    }

    SbxidFaultSpec fault = { .net_id = fault_net_id, .type = stuck_value };
    SbxidCase mc = {0};
    int *var_to_net = NULL, *varV = NULL, *values = NULL;
    char* opb = NULL;
    size_t opb_length = 0;
    bool okay = false;
    if (!sbxid_build_multi_case(&mc, &fault, 1, seed)) {
        fprintf(stderr, "[SB-XID] original binary pattern does not detect %s sa%d\n",
                nl[fault_net_id].name, stuck_value);
        goto cleanup;
    }
    /* A host scope must contain the complete SB support. A bad mask must not
     * turn a required PI into a free, uncharged care variable. */
    for (int p = 0; pi_scope && p < n_pi; ++p) {
        int net = pi[p]->n;
        if (!pi_scope[p] && (mc.union_path[net] || mc.union_justify[net])) {
            fprintf(stderr, "[SB-XID] PI scope omits required input %s\n", pi[p]->name);
            goto cleanup;
        }
    }
    FILE* stream = open_memstream(&opb, &opb_length);
    if (!stream) {
        fprintf(stderr, "[SB-XID] cannot open OPB memory stream\n");
        goto cleanup;
    }
    bool written = sbxid_generate_opb_for_multi_bundle(stream, &var_to_net,
        &info->variables, &info->constraints, &mc, &varV, pi_scope);
    if (fclose(stream) != 0) written = false;
    if (!written) {
        fprintf(stderr, "[SB-XID] cannot generate OPB constraints\n");
        goto cleanup;
    }
    values = sbxid_calloc((size_t)info->variables + 1, sizeof(*values));
    info->solve = sbxid_solve_opb(opb, opb_length, info->variables, values,
                                limit_seconds);
    if (info->solve.status == SBXID_SOLVE_UNKNOWN) {
        memcpy(cube, seed, (size_t)n_pi + 1);
        okay = true;
        goto cleanup;
    }
    if (info->solve.status != SBXID_SOLVE_OPTIMAL &&
        info->solve.status != SBXID_SOLVE_FEASIBLE) {
        fprintf(stderr, "[SB-XID] clasp %s for %s sa%d: %s\n",
                info->solve.status == SBXID_SOLVE_UNSAT ? "UNSAT" : "error",
                nl[fault_net_id].name, stuck_value, info->solve.error);
        goto cleanup;
    }
    sbxid_extract_pattern_from_solution(seed, var_to_net, info->variables, values, pi_scope, cube);
    for (int p = 0; p < n_pi; ++p) if (cube[p] == '-') cube[p] = 'X';
    const char* verify = getenv("FDP_SBXID_VERIFY");
    if (verify && strcmp(verify, "0") != 0) {
        info->verified = true;
        if (!sbxid_verify_pattern_detects_fault(cube, fault_net_id, stuck_value)) {
            info->verification_failed = true;
            memcpy(cube, seed, (size_t)n_pi + 1);
        }
        /* Keep seed values available for FDP_NORMAL_SCOPE_VALIDATE. */
        sbxid_assign_pi_values(seed);
        sbxid_run_simulation_logic(fault_net_id, stuck_value);
    }
    okay = true;
cleanup:
    free(values); free(opb); free(varV); free(var_to_net);
    sbxid_free_multi_case(&mc);
    return okay;
}

void sbxid_release(void)
{
    free(sbxid_index); free(sbxid_index_keys); free(sbxid_pi_of_net);
    free(sbxid_fault_tfo);
    free(sbxid_queue.que);
    sbxid_index = NULL; sbxid_index_keys = NULL; sbxid_pi_of_net = NULL;
    sbxid_index_size = 0;
    sbxid_fault_tfo = NULL;
    sbxid_cached_fault_net_id = -1;
    memset(&sbxid_queue, 0, sizeof(sbxid_queue));
}

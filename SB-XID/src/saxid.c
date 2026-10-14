#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include "Netlist.h"
// ============================================================================
// saxid.c : SAT ベースのドントケア(X)判定 SB-XID (saxid v1)
//   テストパターン(0/1)と、そのパターンで検出させたい縮退故障(1つ以上)を与えると、
//   全故障を検出したまま、できるだけ多くの外部入力をドントケア(X)にしたパターンを求める。
//   (1) 元のパターンで各故障が検出されることを3値故障シミュレーションで確認する
//   (2) 故障ごとの伝搬経路・正当化コーンを擬似ブール制約(OPB)にし、
//       「ケアビットにする外部入力の数を最小化」する問題として clasp で1回解く
//   (3) 得られたパターンを3値故障シミュレーションで検証し、Detected / NOT-Detected を記録する
//       （未検出でも補修・代替はしない）
//   開発版 main9_multi_nocegar.c (sbxid_v1.exe) を整理したもの。処理内容・出力は同じ。
//
// 
//  コマンドライン
//  <回路.v> <故障ファイル> <テストパターンファイル> <結果ファイル名> <X抽出後パターン名> --batch [オプション]
//   故障ファイルは1行に複数故障を列挙できる。;セミコロン区切り。
//   受け付ける書式: "name,0"(従来) / "name sa0"(XID形式、区切りは空白かタブ) / "name 0"
//   オプションは main() 先頭のコメント、または README.txt を参照。
// 
// .exeでの使い方
//   例:
//      複数の故障 
//      saxid_v1.exe circuit\s5378_C.v tp\mfmp\s5378_C_tp_fault.txt tp\mfmp\s5378_C_tp.txt s5378_mfmp_result.txt s5378_mfmp_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s9234_C.v tp\mfmp\s9234_C_tp_fault.txt tp\mfmp\s9234_C_tp.txt s9234_mfmp_result.txt s9234_mfmp_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s13207_C.v tp\mfmp\s13207_C_tp_fault.txt tp\mfmp\s13207_C_tp.txt s13207_mfmp_result.txt s13207_mfmp_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s15850_C.v tp\mfmp\s15850_C_tp_fault.txt tp\mfmp\s15850_C_tp.txt s15850_mfmp_result.txt s15850_mfmp_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s35932_C.v tp\mfmp\s35932_C_tp_fault.txt tp\mfmp\s35932_C_tp.txt s35932_mfmp_result.txt s35932_mfmp_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s38417_C.v tp\mfmp\s38417_C_tp_fault.txt tp\mfmp\s38417_C_tp.txt s38417_mfmp_result.txt s38417_mfmp_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s38584_C.v tp\mfmp\s38584_C_tp_fault.txt tp\mfmp\s38584_C_tp.txt s38584_mfmp_result.txt s38584_mfmp_xtp.txt --batch --threads=16
// 
//      １つの故障
//      saxid_v1.exe circuit\s5378_C.v tp\1f1p\s5378_C_1f1p_fault.txt tp\1f1p\s5378_C_1f1p_tp.txt s5378_1f1p_result.txt s5378_1f1p_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s9234_C.v tp\1f1p\s9234_C_1f1p_fault.txt tp\1f1p\s9234_C_1f1p_tp.txt s9234_1f1p_result.txt s9234_1f1p_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s13207_C.v tp\1f1p\s13207_C_1f1p_fault.txt tp\1f1p\s13207_C_1f1p_tp.txt s13207_1f1p_result.txt s13207_1f1p_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s15850_C.v tp\1f1p\s15850_C_1f1p_fault.txt tp\1f1p\s15850_C_1f1p_tp.txt s15850_1f1p_result.txt s15850_1f1p_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s35932_C.v tp\1f1p\s35932_C_1f1p_fault.txt tp\1f1p\s35932_C_1f1p_tp.txt s35932_1f1p_result.txt s35932_1f1p_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s38417_C.v tp\1f1p\s38417_C_1f1p_fault.txt tp\1f1p\s38417_C_1f1p_tp.txt s38417_1f1p_result.txt s38417_1f1p_xtp.txt --batch --threads=16
//      saxid_v1.exe circuit\s38584_C.v tp\1f1p\s38584_C_1f1p_fault.txt tp\1f1p\s38584_C_1f1p_tp.txt s38584_1f1p_result.txt s38584_1f1p_xtp.txt --batch --threads=16
// 
// ============================================================================
//              オプション（どの位置に書いてもよい）
// ============================================================================
//--batch                   起動時のモード入力を省略してバッチ処理を行う（付けないと 0 / 1 の入力を待つ）
//--interactive             対話モード（単一故障）ユーザ入力で故障を1つずつ指定して X を求める。--batch と排他
//--threads = N             clasp のスレッド数 1〜64（既定 1）。2 以上で並列探索。要確認
//                          並列だと同点の最適解のどれが選ばれるかが実行ごとに変わることがある
//                          （X の数は同じでケアビットの位置が変わる）。1 なら毎回同じ結果。
//--max - faults = N        1行あたりの故障数の上限の指定（既定 64）。超える行は ERROR(TooManyFaults:件数)
//--keep - opb              各ケースの OPB ファイルを OPB_Files\<回路のパス>\ に残す（既定は削除）
//--varmap                  残す OPB の末尾に変数番号と信号線名の対応表を付ける（--keep - opb と併用）
//--debug / --no - debug    進捗表示の ON(既定) / OFF
//--no-result               結果ファイルを出力しない。位置引数は通常の5つのままでも
//                          (結果ファイル名は無視)、<結果ファイル名> を省いた4つでもよい
//                          （結果ファイルにしか使わない検証・故障名の連結もしない。--debug 時は検証して表示する）
//-pin <pinファイル>        パターンの文字の並び順を pinファイル（外部入力名を1行に1つ）で指定する。
//                          X抽出後パターンも同じ順で出力する。省略時は回路の input 宣言の順
// ============================================================================
// 
//
// 結果ファイルの内容(タブ区切り): Case Fault Result Verify Xcount Diff Time_ms Clasp_ms Vars Cons
// ============================================================================

#define FLAG_EVALUATED (1 << 0)
#define FLAG_PATH      (1 << 1)
#define FLAG_JUSTIFY   (1 << 2)

#define XVAL 2

int g_opb_write_failed = 0;
int debug_printf = 1;
const char* g_pin_file = NULL;  // -pin <ファイル> で指定した pinファイル（NULL なら input 宣言の順）
int g_write_result = 1;   // 1: 結果ファイルを出力する(既定) / 0: 出力しない(--no-result)
#define DBG(...) do { if (debug_printf) { printf(__VA_ARGS__); fflush(stdout); } } while (0)

double g_last_clasp_ms = 0.0;  // 直近の clasp 実行時間(ms)
double g_case_clasp_ms = 0.0;  // このcaseの clasp 合計時間(ms)

// 結果ファイルに1case分の行を書く(改行まで出力する)
//   Case Fault Result Verify Xcount Diff Time_ms Clasp_ms Vars Cons
//   fp が NULL(--no-result)なら何もしない（引数の式も評価しない）
#define WRITE_RESULT_ROW(fp, case_num, fault_str, result, verify, xcount, diff, case_t0, vars, cons) do { \
    if ((fp) == NULL) break; \
    double _cms = (double)(clock() - (case_t0)) * 1000.0 / CLOCKS_PER_SEC; \
    fprintf((fp), "Case %d\t%s\t%s\t%s\t%d\t%+d\t%.1f\t%.1f\t%d\t%d\n", \
        (case_num), (fault_str), (result), (verify), (xcount), (diff), _cms, g_case_clasp_ms, (vars), (cons)); \
} while (0)

// [main9 バグ修正] OPB変数総数(total_vars)の構造的上限
#define MAX_OPB_VARS ((size_t)(2 * n_net + n_po + 8))

// [main9_multi バグ修正] 複数故障でのOPB変数総数(total_vars)の構造的上限
#define MAX_OPB_VARS_MULTI(K) ((size_t)(((K) + 1) * n_net + ((K) + 1) * n_po + n_pi + 16))

// コマンドラインオプション
int g_threads = 1;      // clasp のスレッド数 (--threads=N, 1..64)。1 なら並列探索しない
int g_mode_flag = -1;   // --batch で0、--interactive で1。-1 なら起動時に入力を求める
int g_keep_opb = 0;     // 1: OPBファイルを残す(--keep-opb) / 0: 各case後に削除(既定)
int g_emit_var_map = 0; // 1: OPB末尾に変数番号と信号線名の対応表を付ける(--varmap)

static const char* gate_type_name(int type); // 対応表でゲート種別を出すため前方宣言
int get_net_index(char* name); // parse_fault_lineから使うための前方宣言

#define MAX_FAULTS_PER_CASE 64   // 1caseあたりの故障数の既定上限（--max-faults=N で変更可）
int g_max_faults = MAX_FAULTS_PER_CASE;

// 1行を長さ無制限で読む（戻り値は malloc した文字列、EOFなら NULL）。
static char* read_line_dyn(FILE* fp) {
    size_t cap = 4096, len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return NULL;
    while (fgets(buf + len, (int)(cap - len), fp)) {
        len += strlen(buf + len);
        if (len > 0 && buf[len - 1] == '\n') return buf;
        if (cap - len < 2) {
            char* nb = (char*)realloc(buf, cap * 2);
            if (!nb) { free(buf); return NULL; }
            buf = nb; cap *= 2;
        }
    }
    if (len == 0) { free(buf); return NULL; }
    return buf;   // 最終行に改行が無い場合
}

// 故障リスト1行に含まれる故障の数（;区切りの空でない要素の数）
static int count_fault_items(const char* line) {
    int n = 0, in_item = 0;
    for (const char* c = line; ; c++) {
        if (*c == ';' || *c == '\0') { if (in_item) n++; in_item = 0; if (*c == '\0') break; }
        else if (*c != ' ' && *c != '\t' && *c != '\r' && *c != '\n') in_item = 1;
    }
    return n;
}

typedef struct {
    char name[256];
    int  net_id;   // get_net_indexの結果。-1なら未解決（エラー）
    int  type;     // 0 or 1 (S-A-0 / S-A-1)
} FAULT_SPEC;

typedef struct {
    FAULT_SPEC spec;
    int*  signal;      // [n_net] この故障単独を注入したときのsignal値
    char* is_path;      // [n_net] この故障単独のPATHメンバーシップ(0/1)
    char* is_justify;   // [n_net] この故障単独のJUSTIFYメンバーシップ(0/1)
    int*  varS;         // [n_net] この故障のS側OPB変数番号(0=未割当)
} FAULT_CTX;

// ケース全体（1パターン×複数故障）のコンテキスト。
typedef struct {
    FAULT_CTX* f;         // [n_faults]
    int n_faults;
    char* union_path;      // [n_net] いずれかの故障のPATHに属するか(V側の膜)
    char* union_justify;   // [n_net] いずれかの故障のJUSTIFYに属するか(V側の膜)
} MULTI_CASE;

// 故障kのsignal値を全ノードへロードする
static void load_fault_signal(int k, MULTI_CASE* mc) {
    int* sig = mc->f[k].signal;
    for (int i = 0; i < n_net; i++) nl[i].signal = sig[i];
}
static int parse_fault_line(const char* line, FAULT_SPEC* out, int max_faults) {
    size_t buf_len = strlen(line) + 1;
    char* buf = (char*)malloc(buf_len);
    if (!buf) return 0;
    memcpy(buf, line, buf_len);

    int n = 0;
    char* ctx = NULL;
    char* tok = strtok_s(buf, ";", &ctx);
    while (tok && n < max_faults) {
        while (*tok == ' ' || *tok == '\t') tok++;
        char name[256]; int type;
        if (sscanf(tok, "%255[^, \t],%d", name, &type) == 2 ||
            sscanf(tok, "%255s sa%d", name, &type) == 2 ||
            sscanf(tok, "%255s %d", name, &type) == 2) {
            strncpy(out[n].name, name, sizeof(out[n].name) - 1);
            out[n].name[sizeof(out[n].name) - 1] = '\0';
            out[n].net_id = get_net_index(name);
            out[n].type = type;
            n++;
        }
        tok = strtok_s(NULL, ";", &ctx);
    }
    free(buf);
    return n;
}

static int ternary_not(int v) {
    if (v == XVAL) return XVAL;
    return v ? 0 : 1;
}

typedef struct queue {
    int max; int num; int front; int rear; NLIST** que;
} Que;
Que Queue;

void init_queue(int size) {
    Queue.max = size; Queue.num = 0; Queue.front = 0; Queue.rear = 0;
    Queue.que = (NLIST**)malloc(sizeof(NLIST*) * size);
}
void enqueue(NLIST* node) {
    if (Queue.num >= Queue.max) return;
    Queue.que[Queue.rear] = node;
    Queue.rear = (Queue.rear + 1) % Queue.max;
    Queue.num++;
}
NLIST* dequeue() {
    if (Queue.num <= 0) return NULL;
    NLIST* node = Queue.que[Queue.front];
    Queue.front = (Queue.front + 1) % Queue.max;
    Queue.num--;
    return node;
}

int is_queue_empty() { return (Queue.num <= 0); }

static void sanitize_filename(const char* src, char* dst, size_t dst_size) {
    size_t k = 0;
    for (size_t i = 0; src[i] != '\0' && k + 1 < dst_size; i++) {
        char c = src[i];
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') c = '_';
        dst[k++] = c;
    }
    dst[k] = '\0';
}

static int* g_htab = NULL;   // ハッシュスロット → ネット添字（空は -1）
static int   g_htab_size = 0; // 2の冪
static char** g_htab_key = NULL; // スロットの鍵（nl[idx].name のコピーではなくポインタ）

static unsigned long djb2_hash(const char* s) {
    unsigned long h = 5381;
    for (; *s; s++) h = ((h << 5) + h) + (unsigned char)*s; // h*33 + c
    return h;
}

// nl[] 構築後に呼ぶ。以降 get_net_index はこの表を使う。
void build_net_index_table(void) {
    free(g_htab); free(g_htab_key);
    g_htab_size = 1;
    while (g_htab_size < n_net * 2) g_htab_size <<= 1; // 荷重率 < 0.5
    if (g_htab_size < 8) g_htab_size = 8;
    g_htab = (int*)malloc(sizeof(int) * g_htab_size);
    g_htab_key = (char**)malloc(sizeof(char*) * g_htab_size);
    for (int i = 0; i < g_htab_size; i++) { g_htab[i] = -1; g_htab_key[i] = NULL; }

    unsigned int mask = (unsigned int)g_htab_size - 1;
    for (int i = 0; i < n_net; i++) {
        unsigned int slot = (unsigned int)djb2_hash(nl[i].name) & mask;
        while (g_htab_key[slot] != NULL) slot = (slot + 1) & mask; // 線形探査
        g_htab_key[slot] = nl[i].name;
        g_htab[slot] = i;
    }
}

int get_net_index(char* name) {
    if (g_htab == NULL) {
        // 表未構築時のフォールバック（従来動作）
        for (int i = 0; i < n_net; i++)
            if (strcmp(nl[i].name, name) == 0) return i;
        return -1;
    }
    unsigned int mask = (unsigned int)g_htab_size - 1;
    unsigned int slot = (unsigned int)djb2_hash(name) & mask;
    while (g_htab_key[slot] != NULL) {
        if (strcmp(g_htab_key[slot], name) == 0) return g_htab[slot];
        slot = (slot + 1) & mask;
    }
    return -1;
}

// ネット添字 → PI番号（PIでなければ -1）。パターン構築の内側ループを潰すため。
static int* g_pi_of_net = NULL;
void build_pi_of_net_table(void) {
    free(g_pi_of_net);
    g_pi_of_net = (int*)malloc(sizeof(int) * n_net);
    for (int i = 0; i < n_net; i++) g_pi_of_net[i] = -1;
    for (int j = 0; j < n_pi; j++) {
        int idx = get_net_index(pi[j]->name);
        if (idx >= 0) g_pi_of_net[idx] = j;
    }
}

// ---- -pin <ファイル>：パターンの文字の並び順を指定する ----
// pinファイルは外部入力名を1行に1つ（空白区切りでも可）、パターンの1文字目から順に並べたもの
// （PBO_MTTG の pin_<回路>.txt と同じ形式）。指定しなければ、パターンの文字の順 = 回路の input 宣言の順。
// g_pin_perm[j] = パターンの j 文字目に対応する外部入力の番号（回路の input 宣言順の添字）
static int* g_pin_perm = NULL;

static int load_pin_file(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) { printf("ERROR: pinファイルを開けません: %s\n", path); return 0; }
    g_pin_perm = (int*)malloc(sizeof(int) * (size_t)(n_pi > 0 ? n_pi : 1));
    char* used = (char*)calloc((size_t)(n_pi > 0 ? n_pi : 1), 1);
    if (!g_pin_perm || !used) { fclose(fp); free(used); return 0; }
    char name[1024];
    int n = 0, ok = 1;
    while (fscanf(fp, "%1023s", name) == 1) {
        if (n >= n_pi) { printf("ERROR: pinファイルの入力数が回路の外部入力数 %d より多いです\n", n_pi); ok = 0; break; }
        int idx = get_net_index(name);
        int p = (idx >= 0) ? g_pi_of_net[idx] : -1;
        if (p < 0) { printf("ERROR: pinファイルの %d 個目 \"%s\" は回路の外部入力ではありません\n", n + 1, name); ok = 0; break; }
        if (used[p]) { printf("ERROR: pinファイルで外部入力 \"%s\" が重複しています\n", name); ok = 0; break; }
        used[p] = 1;
        g_pin_perm[n++] = p;
    }
    fclose(fp);
    free(used);
    if (ok && n != n_pi) { printf("ERROR: pinファイルの入力数 %d が回路の外部入力数 %d と一致しません\n", n, n_pi); ok = 0; }
    if (!ok) { free(g_pin_perm); g_pin_perm = NULL; }
    return ok;
}

// パターン(pinファイルの順) → 回路の input 宣言の順。足りない文字は X(-) とする。戻り値は malloc した文字列
static char* pin_to_internal(const char* pat) {
    char* out = (char*)malloc((size_t)n_pi + 1);
    if (!out) return NULL;
    size_t len = strlen(pat);
    for (int j = 0; j < n_pi; j++) out[g_pin_perm[j]] = ((size_t)j < len) ? pat[j] : '-';
    out[n_pi] = '\0';
    return out;
}

// 回路の input 宣言の順 → パターン(pinファイルの順)。戻り値は malloc した文字列
static char* internal_to_pin(const char* pat) {
    char* out = (char*)malloc((size_t)n_pi + 1);
    if (!out) return NULL;
    size_t len = strlen(pat);
    for (int j = 0; j < n_pi; j++) out[j] = ((size_t)g_pin_perm[j] < len) ? pat[g_pin_perm[j]] : '-';
    out[n_pi] = '\0';
    return out;
}

// X抽出後パターンを1行書く（-pin 指定時は pinファイルの順に戻す）
static void write_xtp(FILE* fp, const char* pat) {
    if (!g_pin_perm) { fprintf(fp, "%s\n", pat); return; }
    char* o = internal_to_pin(pat);
    fprintf(fp, "%s\n", o ? o : pat);
    free(o);
}

void assign_pi_values(char* pattern_str) {
    for (int i = 0; i < n_pi; i++) {
        char c = pattern_str[i];
        int val;
        if (c == '-' || c == 'X' || c == 'x') {
            val = XVAL;
        }
        else {
            val = (c == '1') ? 1 : 0;
        }
        pi[i]->value = val;
        pi[i]->signal = val;
    }
}

int evaluate_gate_logic(NLIST* node, int is_fault_mode) {
    int type = node->type;
    int n_in = node->n_in;
    NLIST** in = node->in;
    int has_0 = 0, has_1 = 0, has_x = 0;
    for (int i = 0; i < n_in; i++) {
        int in_val = is_fault_mode ? in[i]->signal : in[i]->value;
        if (in_val == 0) has_0 = 1;
        else if (in_val == 1) has_1 = 1;
        else has_x = 1;
    }
    int res = 0;
    switch (type) {
    case AND:   res = has_0 ? 0 : (has_x ? XVAL : 1); break;
    case NAND:  res = has_0 ? 1 : (has_x ? XVAL : 0); break;
    case OR:    res = has_1 ? 1 : (has_x ? XVAL : 0); break;
    case NOR:   res = has_1 ? 0 : (has_x ? XVAL : 1); break;
    case EXOR: {
        if (has_x) res = XVAL;
        else {
            int parity = 0;
            for (int i = 0; i < n_in; i++) {
                int in_val = is_fault_mode ? in[i]->signal : in[i]->value;
                parity ^= (in_val & 1);
            }
            res = parity;
        }
    } break;
    case EXNOR: {
        if (has_x) res = XVAL;
        else {
            int parity = 0;
            for (int i = 0; i < n_in; i++) {
                int in_val = is_fault_mode ? in[i]->signal : in[i]->value;
                parity ^= (in_val & 1);
            }
            res = !parity;
        }
    } break;
    case INV: {
        int v = is_fault_mode ? in[0]->signal : in[0]->value;
        res = ternary_not(v);
    } break;
    case BUF:
    case FOUT:  res = is_fault_mode ? in[0]->signal : in[0]->value; break;
    case GND:   res = 0; break;
    case ACC:   res = 1; break;
    default:    res = 0; break;
    }
    return res;
}

void clear_all_flags() {
    for (int i = 0; i < n_net; i++) nl[i].flag = 0;
}

void run_simulation_logic(int fault_net_id, int fault_type) {
    // 探索フラグ(PATH, JUSTIFY)を維持し、評価フラグのみクリア
    for (int i = 0; i < n_net; i++) nl[i].flag &= ~FLAG_EVALUATED;
    init_queue(n_net * 10);
    for (int i = 0; i < n_pi; i++) enqueue(pi[i]);

    while (!is_queue_empty()) {
        NLIST* node = dequeue();
        int old_val = node->value;
        int old_sig = node->signal;

        if (node->type != IN) {
            node->value = evaluate_gate_logic(node, 0);
            node->signal = evaluate_gate_logic(node, 1);
        }

        if (fault_net_id != -1 && node == &nl[fault_net_id]) {
            node->signal = fault_type;
        }

        if (!(node->flag & FLAG_EVALUATED) || old_val != node->value || old_sig != node->signal) {
            node->flag |= FLAG_EVALUATED;
            for (int i = 0; i < node->n_out; i++) {
                enqueue(node->out[i]);
            }
        }
    }
    free(Queue.que);
}

int verify_pattern_detects_fault(const char* pat, int fault_net_id, int fault_type) {
    // 固定長 1024 では b17_C.v（入力1452本）でスタックを踏み越えるため、
    // 入力数から必要量を確保する（n_pi は起動時に確定する厳密値）。
    char* buf = (char*)malloc((size_t)n_pi + 1);
    if (!buf) return 0;
    size_t plen = strlen(pat);
    if (plen > (size_t)n_pi) plen = (size_t)n_pi;
    memcpy(buf, pat, plen);
    for (size_t i = plen; i < (size_t)n_pi; i++) buf[i] = '-';
    buf[n_pi] = '\0';

    assign_pi_values(buf);
    run_simulation_logic(fault_net_id, fault_type);
    free(buf);

    for (int i = 0; i < n_po; i++) {
        if (po[i]->value != XVAL && po[i]->signal != XVAL && po[i]->value != po[i]->signal) {
            return 1;
        }
    }
    return 0;
}

int extract_single_bundle_path(int fault_net_id, int silent) {
    init_queue(n_net * 10);
    int found_po = 0;

    for (int i = 0; i < n_po; i++) {
        if (po[i]->value != po[i]->signal) {
            enqueue(po[i]);
            found_po++;
        }
    }

    if (found_po == 0) {
        free(Queue.que);
        return 0;
    }

    if (!silent) printf("【パス抽出】故障が観測された %d 個の外部出力を起点に、伝搬経路全体を抽出します。\n", found_po);

    while (!is_queue_empty()) {
        NLIST* current = dequeue();
        if (current->flag & FLAG_PATH) continue;
        current->flag |= FLAG_PATH;

        if (current == &nl[fault_net_id]) continue;

        for (int i = 0; i < current->n_in; i++) {
            if (current->in[i]->value != current->in[i]->signal) {
                enqueue(current->in[i]);
            }
        }
    }
    free(Queue.que);
    return 1;
}

static int get_controlling_value(int gate_type) {
    switch (gate_type) {
    case AND: case NAND: return 0;
    case OR:  case NOR:  return 1;
    default: return -1;
    }
}

// 制御値(c_val)が入力に現れたときにゲート出力が取る値（NAND/NORは反転）
static int get_out_controlling_value(int gate_type, int c_val) {
    if (gate_type == NAND || gate_type == NOR) return !c_val;
    return c_val;
}

void extract_justification_cone(int fault_net_id) {
    init_queue(n_net * 10);

    for (int i = 0; i < n_net; i++) {
        if (nl[i].flag & FLAG_PATH) {
            for (int j = 0; j < nl[i].n_in; j++) {
                if (!(nl[i].in[j]->flag & FLAG_PATH)) {
                    enqueue(nl[i].in[j]);
                }
            }
        }
    }
    while (!is_queue_empty()) {
        NLIST* node = dequeue();
        if (node->flag & FLAG_PATH) continue;
        if (node->flag & FLAG_JUSTIFY) continue;

        node->flag |= FLAG_JUSTIFY;
        for (int i = 0; i < node->n_in; i++) enqueue(node->in[i]);
    }
    free(Queue.que);
}

// [main9_multi] 与えられたパターンが、faults[]の故障全てを個別に検出するか
int verify_pattern_detects_all_faults(const char* pat, FAULT_CTX* f, int n_faults) {
    for (int k = 0; k < n_faults; k++) {
        if (!verify_pattern_detects_fault(pat, f[k].spec.net_id, f[k].spec.type)) return 0;
    }
    return 1;
}

// [main9_multi] 1ケース(1パターン×複数故障)ぶんのMULTI_CASEを構築する。
// 故障ごとに独立して3値シミュレーション・PATH/JUSTIFY抽出を行い、
// (1)故障ごとのsignal値・PATH/JUSTIFYメンバーシップ、
// (2)V側で使う和集合メンバーシップ
// をmcに詰める。戻り値0は「リスト中の故障が1つでもこのパターンで検出でき
// なかった」ことを示す（入力データの不整合。呼び出し側でエラー扱いする）。
// 呼び出し前提：assign_pi_values(pat_str) 済みであること。
// 確保した内部バッファはfree_multi_caseで解放する。
int build_multi_case(MULTI_CASE* mc, FAULT_SPEC* specs, int n_faults, char* pat_str) {
    mc->n_faults = n_faults;
    mc->f = (FAULT_CTX*)calloc((size_t)n_faults, sizeof(FAULT_CTX));
    mc->union_path = (char*)calloc((size_t)n_net, 1);
    mc->union_justify = (char*)calloc((size_t)n_net, 1);
    if (!mc->f || !mc->union_path || !mc->union_justify) return 0;

    int all_detected = 1;
    for (int k = 0; k < n_faults; k++) {
        mc->f[k].spec = specs[k];
        mc->f[k].signal = (int*)calloc((size_t)n_net, sizeof(int));
        mc->f[k].is_path = (char*)calloc((size_t)n_net, 1);
        mc->f[k].is_justify = (char*)calloc((size_t)n_net, 1);
        mc->f[k].varS = NULL; // generate_opb_for_multi_bundleで割り当てる
        assign_pi_values(pat_str);
        clear_all_flags();
        run_simulation_logic(specs[k].net_id, specs[k].type);
        for (int i = 0; i < n_net; i++) mc->f[k].signal[i] = nl[i].signal;

        clear_all_flags();
        int got = extract_single_bundle_path(specs[k].net_id, 1);
        if (!got) { all_detected = 0; continue; } // このパターンは故障kを検出しない→入力不整合
        extract_justification_cone(specs[k].net_id);
        for (int i = 0; i < n_net; i++) {
            int is_p = (nl[i].flag & FLAG_PATH) ? 1 : 0;
            int is_j = (nl[i].flag & FLAG_JUSTIFY) ? 1 : 0;
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

void free_multi_case(MULTI_CASE* mc) {
    if (!mc->f) return;
    for (int k = 0; k < mc->n_faults; k++) {
        free(mc->f[k].signal);
        free(mc->f[k].is_path);
        free(mc->f[k].is_justify);
        free(mc->f[k].varS);
    }
    free(mc->f); mc->f = NULL;
    free(mc->union_path); mc->union_path = NULL;
    free(mc->union_justify); mc->union_justify = NULL;
}

// ====================================================================
// OPB & CNF 式生成モジュール群
// ====================================================================
static void CreateConsEss_StandardGate(FILE* fp, NLIST* g, int c_val, int out_c_val, int target_val, int use_signal, int* varess, int* cons_count) {
    int g_idx = get_net_index(g->name);
    int out_v = varess[g_idx];
    if (out_v <= 0) return;

    if (target_val == out_c_val) {
        fprintf(fp, "-1 x%d ", out_v);
        for (int i = 0; i < g->n_in; i++) {
            int in_idx = get_net_index(g->in[i]->name);
            int in_v = varess[in_idx];
            int in_side_val = use_signal ? g->in[i]->signal : g->in[i]->value;
            if (in_v > 0 && in_side_val == c_val) {
                fprintf(fp, "+1 x%d ", in_v);
            }
        }
        fprintf(fp, ">= 0;  * Justify(OR) Target=%d Ch=%s\n", target_val, use_signal ? "S" : "V");
        (*cons_count)++;
    }
    else {
        for (int i = 0; i < g->n_in; i++) {
            int in_idx = get_net_index(g->in[i]->name);
            int in_v = varess[in_idx];
            if (in_v <= 0) continue;
            fprintf(fp, "-1 x%d +1 x%d >= 0;  * Justify(AND) Target=%d Ch=%s\n", out_v, in_v, target_val, use_signal ? "S" : "V");
            (*cons_count)++;
        }
    }
}

static void CreateConsEss_SpecialGate(FILE* fp, NLIST* g, int* varess, int* cons_count) {
    int g_idx = get_net_index(g->name);
    int out_v = varess[g_idx];
    switch (g->type) {
    case EXOR:
    case EXNOR:
        for (int i = 0; i < g->n_in; i++) {
            int in_idx = get_net_index(g->in[i]->name);
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
            int in_idx = get_net_index(g->in[0]->name);
            int in_v = varess[in_idx];
            if (in_v > 0) {
                fprintf(fp, "-1 x%d +1 x%d >= 0;\n", out_v, in_v);
                (*cons_count)++;
            }
        }
        break;
    }
}

// [main9] varV[i]/varS[i]：ノードiのvalue側/signal側の確定変数番号。
// PI（外部入力）は正常機械・故障機械で同一の物理ビットなのでvarV==varS
// （共有1変数）。内部ノード(PATH/JUSTIFY)はvarV/varSが別々の変数番号を持ち、
// 正常機械のvalue正当化と故障機械のsignal正当化を独立に扱えるようにする。
// out_varV/out_varS: ネット添字→変数番号の対応表を返す（PIはvarV==varS）。不要ならNULLを渡してよい。
void generate_opb_for_single_bundle(const char* opb_file, int** out_var_to_net, int* out_total_vars, int* out_cons_count, int fault_net_id, int** out_varV, int** out_varS) {
    int* varV = (int*)calloc(n_net, sizeof(int));
    int* varS = (int*)calloc(n_net, sizeof(int));
    int vars = 0;

    // PI：共有1変数
    for (int i = 0; i < n_pi; i++) {
        int idx = get_net_index(pi[i]->name);
        if (idx != -1) { varV[idx] = varS[idx] = ++vars; }
    }
    // 内部ノード(PATH/JUSTIFY)：value側・signal側の2変数
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN) continue;
        if (varV[i] == 0 && ((nl[i].flag & FLAG_PATH) || (nl[i].flag & FLAG_JUSTIFY))) {
            varV[i] = ++vars;
            varS[i] = ++vars;
        }
    }
    // 観測制約で使う「POのvalueとsignalが両方確定している」ことを表す補助変数(AND線形化)
    int po_candidates[4096];
    int n_po_candidates = 0;
    for (int i = 0; i < n_po && n_po_candidates < 4096; i++) {
        int po_idx = get_net_index(po[i]->name);
        if (po_idx != -1 && (nl[po_idx].flag & FLAG_PATH) && varV[po_idx] > 0) {
            po_candidates[n_po_candidates++] = po_idx;
        }
    }
    int po_aux_var[4096];
    for (int k = 0; k < n_po_candidates; k++) po_aux_var[k] = ++vars;

    // var_to_net: 変数番号→ネット添字。POの補助変数(実ネットを持たない)は-1。
    int* var_to_net = (int*)malloc(sizeof(int) * (size_t)(vars + 1));
    for (int i = 0; i <= vars; i++) var_to_net[i] = -1;
    for (int i = 0; i < n_net; i++) {
        if (varV[i] > 0) var_to_net[varV[i]] = i;
        if (varS[i] > 0) var_to_net[varS[i]] = i;
    }
    *out_var_to_net = var_to_net;
    *out_total_vars = vars;

    FILE* tmp_fp = fopen(opb_file, "wb+");
    if (!tmp_fp) {
        fprintf(stderr, "[ERROR] OPBファイルを開けません: \"%s\" (パス長 %zu)\n",
            opb_file, strlen(opb_file));
        g_opb_write_failed = 1;
        free(varV); free(varS);
        if (out_varV) *out_varV = NULL;
        if (out_varS) *out_varS = NULL;
        return;
    }
    // 大きめの出力バッファで、多数の小さな fprintf による syscall を減らす
    static char opb_iobuf[1 << 20];
    setvbuf(tmp_fp, opb_iobuf, _IOFBF, sizeof(opb_iobuf));
    fprintf(tmp_fp, "* #variable= %-12d #constraint= %-12d\n\n", 0, 0);

    // 目的関数：PI確定数の最小化（主目的）＋内部信号(value側・signal側それぞれ)の
    // 確定数を最大化（タイブレーク、main6/main8の-1反転を踏襲）。
    int weight_pi = n_net + 1;
    fprintf(tmp_fp, "min: ");
    for (int i = 0; i < n_pi; i++) {
        int idx = get_net_index(pi[i]->name);
        if (idx != -1 && varV[idx] > 0) fprintf(tmp_fp, "+%d x%d ", weight_pi, varV[idx]);
    }
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type != IN) {
            if (varV[i] > 0) fprintf(tmp_fp, "-1 x%d ", varV[i]);
            if (varS[i] > 0) fprintf(tmp_fp, "-1 x%d ", varS[i]);
        }
    }
    fprintf(tmp_fp, ";\n\n");

    int cons_count = 0;

    // 故障箇所必須割当（伝搬の起点）：value側・signal側とも確定
    if (varV[fault_net_id] > 0) {
        fprintf(tmp_fp, "1 x%d >= 1;  * Hard fix Fault Origin(V): %s\n", varV[fault_net_id], nl[fault_net_id].name);
        cons_count++;
    }
    if (varS[fault_net_id] > 0 && varS[fault_net_id] != varV[fault_net_id]) {
        fprintf(tmp_fp, "1 x%d >= 1;  * Hard fix Fault Origin(S): %s\n", varS[fault_net_id], nl[fault_net_id].name);
        cons_count++;
    }

    // 故障伝搬箇所制約（PO到達制約）：
    // 「あるPOでvalueとsignalが両方確定している」ことを、補助変数y_po<=x_V,
    // y_po<=x_S で線形化し、Σy_po>=1 とする（=少なくとも1つのPOで両側とも
    // 確定していなければ検出とみなさない、という健全性要件をそのまま表現）。
    if (n_po_candidates > 0) {
        for (int k = 0; k < n_po_candidates; k++) {
            int po_idx = po_candidates[k];
            int y = po_aux_var[k];
            fprintf(tmp_fp, "-1 x%d +1 x%d >= 0;  * PO-AND(V) aux for %s\n", y, varV[po_idx], nl[po_idx].name);
            cons_count++;
            fprintf(tmp_fp, "-1 x%d +1 x%d >= 0;  * PO-AND(S) aux for %s\n", y, varS[po_idx], nl[po_idx].name);
            cons_count++;
        }
        for (int k = 0; k < n_po_candidates; k++) fprintf(tmp_fp, "+1 x%d ", po_aux_var[k]);
        fprintf(tmp_fp, ">= 1;  * Observe at least one PO (V and S both fixed)\n");
        cons_count++;
    }

    // 故障伝搬経路の後方含意制約（パスの一意化に必須）：value側・signal側を独立に。
    for (int i = 0; i < n_net; i++) {
        if ((nl[i].flag & FLAG_PATH) && i != fault_net_id && varV[i] > 0) {
            char lineV[2048], lineS[2048];
            int lenV = snprintf(lineV, sizeof(lineV), "-1 x%d ", varV[i]);
            int lenS = snprintf(lineS, sizeof(lineS), "-1 x%d ", varS[i]);
            int has_path_in = 0;
            for (int j = 0; j < nl[i].n_in; j++) {
                int in_idx = get_net_index(nl[i].in[j]->name);
                if (in_idx != -1 && (nl[in_idx].flag & FLAG_PATH) && varV[in_idx] > 0) {
                    lenV += snprintf(lineV + lenV, sizeof(lineV) - lenV, "+1 x%d ", varV[in_idx]);
                    lenS += snprintf(lineS + lenS, sizeof(lineS) - lenS, "+1 x%d ", varS[in_idx]);
                    has_path_in = 1;
                }
            }
            if (has_path_in) {
                fprintf(tmp_fp, "%s>= 0;  * Backward OR(V)\n", lineV);
                cons_count++;
                fprintf(tmp_fp, "%s>= 0;  * Backward OR(S)\n", lineS);
                cons_count++;
            }
        }
    }

    // オフパス制約・正当化制約：value側・signal側を独立に2回生成する。
    // (対象ゲートの全入力は1.の変数番号割当フェーズで変数化済みであることが前提)
    for (int i = 0; i < n_net; i++) {
        if (varV[i] > 0 && ((nl[i].flag & FLAG_PATH) || (nl[i].flag & FLAG_JUSTIFY) || i == fault_net_id)) {
            int c_val = get_controlling_value(nl[i].type);
            int is_fault_origin = (i == fault_net_id);
            if (c_val != -1) {
                int out_c_val = get_out_controlling_value(nl[i].type, c_val);
                CreateConsEss_StandardGate(tmp_fp, &nl[i], c_val, out_c_val, nl[i].value, 0, varV, &cons_count);
                if (!is_fault_origin) {
                    CreateConsEss_StandardGate(tmp_fp, &nl[i], c_val, out_c_val, nl[i].signal, 1, varS, &cons_count);
                }
            }
            else {
                CreateConsEss_SpecialGate(tmp_fp, &nl[i], varV, &cons_count);
                if (!is_fault_origin) {
                    CreateConsEss_SpecialGate(tmp_fp, &nl[i], varS, &cons_count);
                }
            }
        }
    }

    // 変数番号と信号線名の対応表（OPBコメントとして末尾に付す）。
    if (g_emit_var_map) {
        fprintf(tmp_fp, "\n* --- Variable Mapping for Standalone Parser ---\n");
        for (int i = 1; i <= vars; i++) {
            int net_idx = var_to_net[i];
            if (net_idx < 0) {
                fprintf(tmp_fp, "* VAR_MAP: %d (PO-AND-aux)\n", i);
                continue;
            }
            char region[32];
            int is_p = (nl[net_idx].flag & FLAG_PATH) != 0;
            int is_j = (nl[net_idx].flag & FLAG_JUSTIFY) != 0;
            if (is_p && is_j)      strcpy(region, "PATH+JUSTIFY");
            else if (is_p)         strcpy(region, "PATH");
            else if (is_j)         strcpy(region, "JUSTIFY");
            else                   strcpy(region, "-");
            const char* ch = (varV[net_idx] == i) ? "V" : (varS[net_idx] == i ? "S" : "?");

            fprintf(tmp_fp, "* VAR_MAP: %d %s %s\t%s\tCh=%s\t%s%s\n",
                i,
                nl[net_idx].name,
                (nl[net_idx].type == IN) ? "IN" : "",
                gate_type_name(nl[net_idx].type),
                ch,
                region,
                (net_idx == fault_net_id) ? "\tFAULT" : "");
        }
    }

    // 末尾で先頭にシークし、ヘッダを実値へ上書きする。
    fflush(tmp_fp);
    fseek(tmp_fp, 0, SEEK_SET);
    fprintf(tmp_fp, "* #variable= %-12d #constraint= %-12d\n\n", vars, cons_count);
    fclose(tmp_fp);

    *out_cons_count = cons_count;
    if (out_varV) *out_varV = varV; else free(varV);
    if (out_varS) *out_varS = varS; else free(varS);
}

// [main9_multi] generate_opb_for_single_bundleの複数故障版。
// XID的な必須故障の優先度づけは行わない：K個の故障を完全に対称に扱う。
void generate_opb_for_multi_bundle(const char* opb_file, int** out_var_to_net, int* out_total_vars, int* out_cons_count,
    MULTI_CASE* mc, int** out_varV) {
    int K = mc->n_faults;
    int* varV = (int*)calloc(n_net, sizeof(int));
    int vars = 0;

    // PI：故障によらず共有1変数（単一故障版と同じ）
    for (int i = 0; i < n_pi; i++) {
        int idx = get_net_index(pi[i]->name);
        if (idx != -1) varV[idx] = ++vars;
    }
    // 内部ノード：V側はunion_path/union_justifyのどちらかに属すれば1変数
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN) continue;
        if (varV[i] == 0 && (mc->union_path[i] || mc->union_justify[i])) varV[i] = ++vars;
    }
    // 内部ノード：S側は故障ごとに、その故障自身のPATH/JUSTIFYに属す場合のみ変数を割当
    for (int k = 0; k < K; k++) {
        mc->f[k].varS = (int*)calloc(n_net, sizeof(int));
        for (int i = 0; i < n_pi; i++) {
            int idx = get_net_index(pi[i]->name);
            if (idx != -1) mc->f[k].varS[idx] = varV[idx]; // PIはV側と同じ変数を共有
        }
        for (int i = 0; i < n_net; i++) {
            if (nl[i].type == IN) continue;
            if (mc->f[k].varS[i] == 0 && (mc->f[k].is_path[i] || mc->f[k].is_justify[i])) mc->f[k].varS[i] = ++vars;
        }
    }

    // PO観測制約の補助変数：故障ごとに独立（その故障自身のPATH上のPOのみが候補）
    int** po_cand = (int**)malloc(sizeof(int*) * (size_t)K);
    int** po_aux = (int**)malloc(sizeof(int*) * (size_t)K);
    int* n_po_cand = (int*)calloc((size_t)K, sizeof(int));
    for (int k = 0; k < K; k++) {
        po_cand[k] = (int*)malloc(sizeof(int) * (size_t)(n_po > 0 ? n_po : 1));
        po_aux[k] = (int*)malloc(sizeof(int) * (size_t)(n_po > 0 ? n_po : 1));
        for (int i = 0; i < n_po; i++) {
            int po_idx = get_net_index(po[i]->name);
            if (po_idx != -1 && mc->f[k].is_path[po_idx] && varV[po_idx] > 0) {
                po_cand[k][n_po_cand[k]++] = po_idx;
            }
        }
        for (int j = 0; j < n_po_cand[k]; j++) po_aux[k][j] = ++vars;
    }

    int* var_to_net = (int*)malloc(sizeof(int) * (size_t)(vars + 1));
    for (int i = 0; i <= vars; i++) var_to_net[i] = -1;
    for (int i = 0; i < n_net; i++) if (varV[i] > 0) var_to_net[varV[i]] = i;
    for (int k = 0; k < K; k++)
        for (int i = 0; i < n_net; i++)
            if (mc->f[k].varS[i] > 0) var_to_net[mc->f[k].varS[i]] = i;
    *out_var_to_net = var_to_net;
    *out_total_vars = vars;

    FILE* tmp_fp = fopen(opb_file, "wb+");
    if (!tmp_fp) {
        fprintf(stderr, "[ERROR] OPBファイルを開けません: \"%s\" (パス長 %zu)\n", opb_file, strlen(opb_file));
        g_opb_write_failed = 1;
        free(varV);
        for (int k = 0; k < K; k++) { free(po_cand[k]); free(po_aux[k]); }
        free(po_cand); free(po_aux); free(n_po_cand);
        if (out_varV) *out_varV = NULL;
        return;
    }
    static char opb_iobuf[1 << 20];
    setvbuf(tmp_fp, opb_iobuf, _IOFBF, sizeof(opb_iobuf));
    fprintf(tmp_fp, "* #variable= %-12d #constraint= %-12d\n\n", 0, 0);

    // 目的関数：PI確定数の最小化（主目的、PIは共有変数なので1回だけ計上）
    int weight_pi = (K + 1) * n_net + 1;
    fprintf(tmp_fp, "min: ");
    for (int i = 0; i < n_pi; i++) {
        int idx = get_net_index(pi[i]->name);
        if (idx != -1 && varV[idx] > 0) fprintf(tmp_fp, "+%d x%d ", weight_pi, varV[idx]);
    }
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type != IN && varV[i] > 0) fprintf(tmp_fp, "-1 x%d ", varV[i]);
    }
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < n_net; i++) {
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
            char lineV[2048];
            int lenV = snprintf(lineV, sizeof(lineV), "-1 x%d ", varV[i]);
            int has_in = 0;
            for (int j = 0; j < nl[i].n_in; j++) {
                int in_idx = get_net_index(nl[i].in[j]->name);
                if (in_idx != -1 && mc->union_path[in_idx] && varV[in_idx] > 0) {
                    lenV += snprintf(lineV + lenV, sizeof(lineV) - lenV, "+1 x%d ", varV[in_idx]);
                    has_in = 1;
                }
            }
            if (has_in) { fprintf(tmp_fp, "%s>= 0;  * Backward OR(V)\n", lineV); cons_count++; }
        }
    }
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < n_net; i++) {
            if (mc->f[k].is_path[i] && mc->f[k].varS[i] > 0) {
                char lineS[2048];
                int lenS = snprintf(lineS, sizeof(lineS), "-1 x%d ", mc->f[k].varS[i]);
                int has_in = 0;
                for (int j = 0; j < nl[i].n_in; j++) {
                    int in_idx = get_net_index(nl[i].in[j]->name);
                    if (in_idx != -1 && mc->f[k].is_path[in_idx] && mc->f[k].varS[in_idx] > 0) {
                        lenS += snprintf(lineS + lenS, sizeof(lineS) - lenS, "+1 x%d ", mc->f[k].varS[in_idx]);
                        has_in = 1;
                    }
                }
                if (has_in) { fprintf(tmp_fp, "%s>= 0;  * Backward OR(S%d)\n", lineS, k); cons_count++; }
            }
        }
    }

    // 正当化制約：V側1回（全ゲート共通）＋故障ごとにS側1回（K回）。
    // CreateConsEss_StandardGate/SpecialGateは無改造のまま呼ぶ。
    for (int i = 0; i < n_net; i++) {
        if (varV[i] <= 0 || !(mc->union_path[i] || mc->union_justify[i])) continue;
        int c_val = get_controlling_value(nl[i].type);
        if (c_val != -1) {
            int out_c_val = get_out_controlling_value(nl[i].type, c_val);
            CreateConsEss_StandardGate(tmp_fp, &nl[i], c_val, out_c_val, nl[i].value, 0, varV, &cons_count);
        }
        else {
            CreateConsEss_SpecialGate(tmp_fp, &nl[i], varV, &cons_count);
        }
    }
    for (int k = 0; k < K; k++) {
        load_fault_signal(k, mc);
        for (int i = 0; i < n_net; i++) {
            if (mc->f[k].varS[i] <= 0 || !(mc->f[k].is_path[i] || mc->f[k].is_justify[i])) continue;
            if (i == mc->f[k].spec.net_id) continue; // 故障起点のS側は入力由来の正当化を生成しない(hard-fixのみ)
            int c_val = get_controlling_value(nl[i].type);
            if (c_val != -1) {
                int out_c_val = get_out_controlling_value(nl[i].type, c_val);
                CreateConsEss_StandardGate(tmp_fp, &nl[i], c_val, out_c_val, nl[i].signal, 1, mc->f[k].varS, &cons_count);
            }
            else {
                CreateConsEss_SpecialGate(tmp_fp, &nl[i], mc->f[k].varS, &cons_count);
            }
        }
    }

    if (g_emit_var_map) {
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
                i, nl[net_idx].name, (nl[net_idx].type == IN) ? "IN" : "", gate_type_name(nl[net_idx].type), ch);
        }
    }

    fflush(tmp_fp);
    fseek(tmp_fp, 0, SEEK_SET);
    fprintf(tmp_fp, "* #variable= %-12d #constraint= %-12d\n\n", vars, cons_count);
    fclose(tmp_fp);

    *out_cons_count = cons_count;
    if (out_varV) *out_varV = varV; else free(varV);
    for (int k = 0; k < K; k++) { free(po_cand[k]); free(po_aux[k]); }
    free(po_cand); free(po_aux); free(n_po_cand);
}

// out_obj: clasp が出力する目的関数値（"o <値>" 行の最終値）を返す。
//          最適解選択に用いる。不要な場合は NULL を渡してよい。
int run_clasp_and_parse(const char* opb_file, int* var_to_net_idx, int total_vars, int* var_values, long* out_obj) {
    char cmd[1024];
    const char* output_file = "clasp_out.txt";
    if (out_obj) *out_obj = LONG_MAX;

    // 拡張子付きの裸コマンド名だと、Windowsのセキュリティ強化設定
    // （NoDefaultCurrentDirectoryInExePath等）下ではcmd.exeがカレントディレクトリを
    // 検索せず clasp.exe が見つからないことがあるため、パス付き(.\clasp.exe)で呼ぶ。
    // --threads=N (N>=2) のときだけ並列探索にする。1スレッドなら同じ入力で毎回同じ解になる
    char par_opt[32] = "";
    if (g_threads > 1) snprintf(par_opt, sizeof(par_opt), " --parallel-mode %d", g_threads);
    snprintf(cmd, sizeof(cmd), ".\\clasp.exe %s%s --opt-heuristic=model --opt-mode=opt > %s", opb_file, par_opt, output_file);
    // 前のcaseの出力が残っていると、clasp が起動・終了に失敗したときにそれを読んでしまうので先に消す
    remove(output_file);
    clock_t _t0 = clock();
    int ret = system(cmd);
    clock_t _t1 = clock();
    // 直近のclasp実行時間(ms)を記録し、caseの合計時間に加算
    g_last_clasp_ms = (double)(_t1 - _t0) * 1000.0 / CLOCKS_PER_SEC;
    g_case_clasp_ms += g_last_clasp_ms;
    (void)ret;

    FILE* fp = fopen(output_file, "r");
    if (!fp) return -1;

    int status = -1;
    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 's') {
            if (strncmp(line, "s OPTIMUM FOUND", 15) == 0) status = 1;
            else if (strncmp(line, "s SATISFIABLE", 13) == 0 && status != 1) status = 3;
            else if (strncmp(line, "s UNSATISFIABLE", 15) == 0) status = 0;
            else if (strncmp(line, "s UNKNOWN", 9) == 0) status = 2;
        }
        else if (line[0] == 'o' && (line[1] == ' ' || line[1] == '\t')) {
            // clasp は改善のたびに "o <値>" を出力する。最後の行が最適値。
            if (out_obj) *out_obj = strtol(line + 1, NULL, 10);
        }
        else if (line[0] == 'v') {
            char* token = strtok(line + 1, " \r\n");
            while (token) {
                int sign = 1; char* p = token;
                if (*p == '-') { sign = -1; p++; }
                else if (*p == '+') { p++; }
                if (*p == 'x' || *p == 'X') { p++; }
                int val = atoi(p);
                if (val != 0) {
                    val *= sign;
                    int var_id = abs(val);
                    if (var_id <= total_vars) var_values[var_id] = (val > 0) ? 1 : 0;
                }
                token = strtok(NULL, " \r\n");
            }
        }
    }
    fclose(fp);
    return status;
}


// パターン中のドントケア(-, X, x)の数
static int count_x(const char* pat) {
    int c = 0;
    for (int i = 0; pat[i] != '\0'; i++)
        if (pat[i] == '-' || pat[i] == 'X' || pat[i] == 'x') c++;
    return c;
}

// --- 診断用: ゲート種別名 ---
// --varmap の対応表で使う。
static const char* gate_type_name(int type) {
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

// ============================================================================
// 1回解いて検証する
//   OPBを1回生成し clasp で1回解く。得られたパターン(ケアビット以外は "-")を
//   3値故障シミュレーションで検証し、結果を *out_detected に入れる。
//   未検出でも補修・代替はしない。CEGARの排除
//   戻り値: clasp の状態 (1: 最適解, 3: 最適性未証明の解, それ以外: 解なし/エラー)
// ============================================================================
static void extract_pattern_from_solution(const char* pat_str, int* var_to_net, int total_vars, int* var_values, char* out_pat) {
    for (int i = 0; i < n_pi; i++) out_pat[i] = '-';
    out_pat[n_pi] = '\0';
    for (int i = 1; i <= total_vars; i++) {
        int net_idx = var_to_net[i];
        if (net_idx < 0) continue; // PO補助変数（実ネットを持たない）
        if (nl[net_idx].type == IN) {
            int pi_idx = g_pi_of_net[net_idx];
            if (pi_idx != -1 && var_values[i] == 1) out_pat[pi_idx] = pat_str[pi_idx];
        }
    }
}

// 単一故障版（対話モード用）
int run_single_shot(const char* opb_file, const char* pat_str, int fault_net_id, int fault_type,
    int** out_var_to_net, int* out_total_vars, int* out_cons_count, int* var_values, char* final_pat, int* out_detected) {
    int* var_to_net = NULL; int* varV = NULL; int* varS = NULL;
    int total_vars = 0, cons_count = 0;
    *out_detected = 0;
    final_pat[0] = '\0';
    DBG("    [1回求解] OPB生成 ...\n");
    generate_opb_for_single_bundle(opb_file, &var_to_net, &total_vars, &cons_count, fault_net_id, &varV, &varS);
    DBG("    [1回求解] clasp求解 (変数=%d, 制約=%d) ...\n", total_vars, cons_count);
    int status = run_clasp_and_parse(opb_file, var_to_net, total_vars, var_values, NULL);
    free(varV); free(varS);
    *out_var_to_net = var_to_net; *out_total_vars = total_vars; *out_cons_count = cons_count;
    if (status != 1 && status != 3) return status;
    extract_pattern_from_solution(pat_str, var_to_net, total_vars, var_values, final_pat);
    *out_detected = verify_pattern_detects_fault(final_pat, fault_net_id, fault_type);
    DBG("    [1回求解] 検証結果=%s\n", *out_detected ? "Detected" : "NOT-Detected");
    return status;
}

// 複数故障版（バッチモード用）
int run_single_shot_multi(const char* opb_file, const char* pat_str, MULTI_CASE* mc,
    int** out_var_to_net, int* out_total_vars, int* out_cons_count, int* var_values, char* final_pat, int* out_detected) {
    int* var_to_net = NULL; int* varV = NULL;
    int total_vars = 0, cons_count = 0;
    *out_detected = 0;
    final_pat[0] = '\0';
    DBG("    [1回求解 K=%d] OPB生成 ...\n", mc->n_faults);
    generate_opb_for_multi_bundle(opb_file, &var_to_net, &total_vars, &cons_count, mc, &varV);
    DBG("    [1回求解] clasp求解 (変数=%d, 制約=%d) ...\n", total_vars, cons_count);
    int status = run_clasp_and_parse(opb_file, var_to_net, total_vars, var_values, NULL);
    free(varV);
    *out_var_to_net = var_to_net; *out_total_vars = total_vars; *out_cons_count = cons_count;
    if (status != 1 && status != 3) return status;
    extract_pattern_from_solution(pat_str, var_to_net, total_vars, var_values, final_pat);
    // 検証結果は結果ファイルの Verify 列と進捗表示にしか使わないので、どちらも無ければ検証しない
    if (g_write_result || debug_printf) {
        *out_detected = verify_pattern_detects_all_faults(final_pat, mc->f, mc->n_faults);
        DBG("    [1回求解] 検証結果=%s\n", *out_detected ? "Detected" : "NOT-Detected");
    }
    return status;
}


void write_results_to_file(const char* out_file, const char* pat_str, const char* fault_name, int fault_type) {
    FILE* fp = fopen(out_file, "w");
    if (!fp) return;

    fprintf(fp, "入力テストパターン: %s\n", pat_str);
    fprintf(fp, "故障箇所: %s sa%d\n", fault_name, fault_type);
    fprintf(fp, "--------------------------------------------------------\n");
    fprintf(fp, "信号線名\tゲート\t正常値\t故障値\t属性\n");
    fprintf(fp, "--------------------------------------------------------\n");

    for (int i = 0; i < n_net; i++) {
        char role[32] = "None";
        if (nl[i].flag & FLAG_PATH) strcpy(role, "故障伝搬(束)");
        else if (nl[i].flag & FLAG_JUSTIFY) strcpy(role, "正当化経路");

        if ((nl[i].flag & FLAG_PATH) || (nl[i].flag & FLAG_JUSTIFY)) {
            fprintf(fp, "%-10s \t%d \t%d \t%d \t%s\n", nl[i].name, nl[i].type, nl[i].value, nl[i].signal, role);
        }
    }
    fclose(fp);
}

int main(int argc, char* argv[]) {
    // --- オプション引数の切り出し ---
    // "--" で始まる引数はどの位置に置いてもよい。取り除いた残りを
    // 従来どおりの位置引数として扱うので、既存の呼び出し方は変わらない。
    //   --varmap     : OPB末尾に変数番号と信号線名の対応表を出力する
    //   --no-varmap  : 出力しない（既定）
    //   --keep-opb   : バッチ処理のOPBファイルを OPB_Files\<回路>\ に残す
    //   --no-keep-opb: 各caseを解いた後にOPBファイルを削除する（既定）
    //   --batch      : 動作モードを聞かずにバッチ処理を行う / --interactive : 対話モード
    //   --threads=N  : clasp のスレッド数 1..64（既定1）。2以上で並列探索(--parallel-mode N)。
    //                  並列だと同点解の選び方が実行ごとに揺れ、ケアビットの位置が変わることがある
    //   --max-faults=N: 1caseあたりの故障数の上限（既定64）。超える行は ERROR(TooManyFaults:件数) として記録し処理しない
    //   --debug      : 進捗表示を出す（既定）
    //   --no-debug   : 進捗表示を抑制する
    //   -pin <ファイル>: パターンの文字の並び順を pinファイル（外部入力名を1行に1つ）で指定する
    char* pargv[64];
    int pargc = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--varmap") == 0) { g_emit_var_map = 1; continue; }
        if (strcmp(argv[i], "--no-varmap") == 0) { g_emit_var_map = 0; continue; }
        if (strcmp(argv[i], "--keep-opb") == 0) { g_keep_opb = 1; continue; }
        if (strcmp(argv[i], "--batch") == 0) { g_mode_flag = 0; continue; }
        if (strncmp(argv[i], "--threads=", 10) == 0) {
            int v = atoi(argv[i] + 10);
            if (v < 1 || v > 64) { printf("ERROR: --threads=N の N は 1..64 で指定してください (指定値: %s)\n", argv[i] + 10); return -1; }
            g_threads = v; continue;
        }
        if (strcmp(argv[i], "--interactive") == 0) { g_mode_flag = 1; continue; }
        if (strncmp(argv[i], "--max-faults=", 13) == 0) { int v = atoi(argv[i] + 13); if (v > 0) g_max_faults = v; continue; }
        if (strcmp(argv[i], "--no-keep-opb") == 0) { g_keep_opb = 0; continue; }
        if (strcmp(argv[i], "--debug") == 0) { debug_printf = 1; continue; }
        if (strcmp(argv[i], "--no-debug") == 0) { debug_printf = 0; continue; }
        if (strcmp(argv[i], "--no-result") == 0) { g_write_result = 0; continue; }
        if (strcmp(argv[i], "-pin") == 0) {
            if (i + 1 >= argc) { printf("ERROR: -pin の後に pinファイル名を指定してください\n"); return -1; }
            g_pin_file = argv[++i]; continue;
        }
        if (pargc < 64) pargv[pargc++] = argv[i];
    }
    argc = pargc;
    argv = pargv;

    if (argc < 2) return -1;
    DBG("[INIT] ネットリスト読込: %s\n", argv[1]);
    read_nl(argv[1]);

    // 名前→添字／添字→PI番号の対応表を構築（以降の探索を O(1) 化）
    DBG("[INIT] 索引表(ハッシュ/PI対応表)を構築\n");
    build_net_index_table();
    build_pi_of_net_table();
    if (g_pin_file && !load_pin_file(g_pin_file)) return -1;

    printf("回路名：%s\n", argv[1]);
    printf("変数対応表の出力：%s\n", g_emit_var_map ? "あり (--varmap)" : "なし");
    printf("OPBファイル：%s\n", g_keep_opb ? "残す (--keep-opb)" : "各case後に削除 (--keep-opb で残す)");
    printf("1caseの故障数上限：%d (--max-faults=N で変更)\n", g_max_faults);
    printf("claspスレッド数：%d%s\n", g_threads, g_threads > 1 ? " (並列探索。結果のケアビット位置が実行ごとに変わることがある)" : " (--threads=N で変更)");
    printf("進捗表示：%s\n", debug_printf ? "ON (--no-debug で抑制)" : "OFF (--debug で表示)");
    printf("pinファイル：%s\n", g_pin_file ? g_pin_file : "なし (パターンの文字の順 = 回路の input 宣言の順。-pin <ファイル> で指定)");
    printf("結果ファイル：%s\n", g_write_result ? "出力する (--no-result で出力しない)" : "出力しない (--no-result)");

    printf("回路規模：ネット数 %d, 入力 %d, 出力 %d\n", n_net, n_pi, n_po);

    int top_mode = g_mode_flag;
    if (top_mode < 0) {
        printf("動作モードを選択してください (0: バッチ処理, 1: 対話モード): ");
        if (scanf("%d", &top_mode) != 1) return -1;
    }
    else printf("動作モード：%s\n", top_mode == 0 ? "バッチ処理 (--batch)" : "対話モード (--interactive)");

    // ====================================================================
    // バッチ処理モード 
    // ====================================================================
    if (top_mode == 0) {
        // 位置引数: <回路> <故障> <パターン> <結果ファイル> <X抽出後パターン>
        //          --no-result のときは <結果ファイル> を省いた4つでもよい。5つ渡されたら
        //          4つ目(結果ファイル名)は使わず、5つ目を X抽出後パターンにする
        if (argc < (g_write_result ? 6 : 5)) return -1;

        FILE* fp_fault = fopen(argv[2], "r");
        FILE* fp_tp = fopen(argv[3], "r");
        FILE* fp_res = g_write_result ? fopen(argv[4], "w") : NULL;
        FILE* fp_xtp = fopen(argc >= 6 ? argv[5] : argv[4], "w");

        if (!fp_fault || !fp_tp || (g_write_result && !fp_res) || !fp_xtp) return -1;

        // 故障リスト行・パターン行は read_line_dyn で行末まで読み切る（長さ無制限）
        char* fault_line = NULL;
        char* pat_str = NULL;
        char opb_file[1024];
        int case_num = 1;

        system("mkdir OPB_Files 2> nul");
        char mkdir_cmd[1024];
        // 回路のパスの "/" は "\" に置き換えてから作業フォルダ名に使う（"/" のままだと mkdir が失敗する）
        char circ_dir[1024];
        snprintf(circ_dir, sizeof(circ_dir), "%s", argv[1]);
        for (char* c = circ_dir; *c; c++) if (*c == '/') *c = '\\';
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir OPB_Files\\%s 2> nul", circ_dir);
        system(mkdir_cmd);

        // 結果ファイルのヘッダ（--debug の有無にかかわらず常にこの列）
        //   Case     : Case N
        //   Fault    : 対象故障「名前 sa0;名前 sa1;...」
        //   Result   : SAT(OPT) / SAT(FEASIBLE) / UNSAT / ERROR(...)
        //   Verify   : Detected / NOT-Detected / N/A
        //   Xcount   : 出力パターンのドントケア数
        //   Diff     : Xcount - 入力パターンのドントケア数
        //   Time_ms  : 1caseの処理時間(ms)
        //   Clasp_ms : clasp の時間(ms)
        //   Vars     : OPBの変数数
        //   Cons     : OPBの制約数
        if (fp_res) fprintf(fp_res, "Case\tFault\tResult\tVerify\tXcount\tDiff\tTime_ms\tClasp_ms\tVars\tCons\n");
        for (;;) {
            free(fault_line); free(pat_str);
            fault_line = read_line_dyn(fp_fault);
            pat_str = read_line_dyn(fp_tp);
            if (!fault_line || !pat_str) break;
            g_case_clasp_ms = 0.0;              // このcaseの clasp 時間をリセット
            clock_t case_t0 = clock();          // このcaseの処理時間の起点
            DBG("\n[Case %d] 入力行(%d文字)\n", case_num, (int)strlen(fault_line));
            fault_line[strcspn(fault_line, "\r\n")] = '\0';
            pat_str[strcspn(pat_str, "\r\n")] = '\0';
            if (strlen(fault_line) == 0 || strlen(pat_str) == 0) continue;
            // -pin 指定時は、パターンの文字の並び(pinファイルの順)を回路の input 宣言の順に並べ替える
            if (g_pin_perm) {
                char* conv = pin_to_internal(pat_str);
                if (!conv) break;
                free(pat_str); pat_str = conv;
            }

            // [main9_multi] 故障リスト行のパース。"name,type;name,type;..."。
            // セミコロンが無い(=故障1個)行も従来通り解釈できる。
            int n_items = count_fault_items(fault_line);
            if (n_items > g_max_faults) {
                // 上限を超える行は一部だけ処理せず、そのcase全体をエラーとして記録する
                char res[64], fsum[300];
                snprintf(res, sizeof(res), "ERROR(TooManyFaults:%d)", n_items);
                size_t first_len = strcspn(fault_line, ";");
                snprintf(fsum, sizeof(fsum), "%.*s;...(%d faults)", (int)(first_len < 200 ? first_len : 200), fault_line, n_items);
                WRITE_RESULT_ROW(fp_res, case_num, fsum, res, "N/A", count_x(pat_str), 0, case_t0, 0, 0);
                write_xtp(fp_xtp, pat_str); case_num++; continue;
            }
            FAULT_SPEC* specs = (FAULT_SPEC*)malloc(sizeof(FAULT_SPEC) * (size_t)(n_items > 0 ? n_items : 1));
            if (!specs) break;
            int n_faults = parse_fault_line(fault_line, specs, n_items > 0 ? n_items : 1);
            if (n_faults == 0) {
                WRITE_RESULT_ROW(fp_res, case_num, fault_line, "ERROR(Parse)", "N/A", count_x(pat_str), 0, case_t0, 0, 0);
                write_xtp(fp_xtp, pat_str); free(specs); case_num++; continue;
            }
            int bad_net = -1;
            for (int q = 0; q < n_faults; q++) if (specs[q].net_id == -1) { bad_net = q; break; }
            if (bad_net != -1) {
                char nonet[300];
                snprintf(nonet, sizeof(nonet), "ERROR(NoNet:%s)", specs[bad_net].name);
                WRITE_RESULT_ROW(fp_res, case_num, fault_line, nonet, "N/A", count_x(pat_str), 0, case_t0, 0, 0);
                write_xtp(fp_xtp, pat_str); free(specs); case_num++; continue;
            }

            // 故障名を1つの文字列にまとめる（結果ファイルのFault列と進捗表示用。どちらも無ければ作らない）
            char* fault_names_joined = NULL;
            if (g_write_result || debug_printf) {
                size_t joined_cap = strlen(fault_line) + (size_t)n_faults * 8 + 16;
                fault_names_joined = (char*)calloc(joined_cap, 1);
                if (!fault_names_joined) { free(specs); break; }
                for (int q = 0; q < n_faults; q++) {
                    char one[300];
                    snprintf(one, sizeof(one), "%s%s sa%d", q ? ";" : "", specs[q].name, specs[q].type);
                    strncat(fault_names_joined, one, joined_cap - strlen(fault_names_joined) - 1);
                }
            }

            {
                char safe_name[256];
                sanitize_filename(specs[0].name, safe_name, sizeof(safe_name));
                snprintf(opb_file, sizeof(opb_file), "OPB_Files\\%s\\case%d_%s_x%d.opb",
                    circ_dir, case_num, safe_name, n_faults);
                if (strlen(opb_file) > 200) {
                    snprintf(opb_file, sizeof(opb_file), "OPB_Files\\%s\\case%d.opb", circ_dir, case_num);
                }
            }

            g_opb_write_failed = 0;
            DBG("  [Case %d] Faults=%s (%d件) 処理開始\n", case_num, fault_names_joined, n_faults);
            DBG("  [1] 故障ごとの論理・故障シミュレーション(3値)とコーン抽出\n");
            assign_pi_values(pat_str);

            int orig_x = 0;
            for (int i = 0; pat_str[i] != '\0'; i++) if (pat_str[i] == '-' || pat_str[i] == 'X' || pat_str[i] == 'x') orig_x++;

            MULTI_CASE mc = { 0 };
            int all_ok = build_multi_case(&mc, specs, n_faults, pat_str);

            if (all_ok) {
                DBG("  [2] 1回求解+検証 (K=%d故障同時, CEGARなし)\n", n_faults);

                int* var_to_net = NULL;
                int total_vars = 0, cons_count = 0, detected = 0;
                int* var_values = (int*)calloc(MAX_OPB_VARS_MULTI(n_faults), sizeof(int));
                char* final_pat = (char*)malloc((size_t)n_pi + 1);
                if (!final_pat) { free(var_values); free_multi_case(&mc); break; }
                final_pat[0] = '\0';

                int status = run_single_shot_multi(opb_file, pat_str, &mc, &var_to_net, &total_vars, &cons_count, var_values, final_pat, &detected);
                if (!g_keep_opb) remove(opb_file);   // --keep-opb が無ければOPBは残さない
                int solved = (status == 1 || status == 3);
                // 解が無いときは入力パターンをそのまま出力する
                if (!solved) { strncpy(final_pat, pat_str, (size_t)n_pi); final_pat[n_pi] = '\0'; }

                int new_x = 0;
                for (int i = 0; final_pat[i] != '\0'; i++) if (final_pat[i] == '-' || final_pat[i] == 'X' || final_pat[i] == 'x') new_x++;
                int diff = new_x - orig_x;

                const char* verify_str = solved ? (detected ? "Detected" : "NOT-Detected") : "N/A";
                char result_status[64];
                if (status == 1) snprintf(result_status, sizeof(result_status), "SAT(OPT)");
                else if (status == 3) snprintf(result_status, sizeof(result_status), "SAT(FEASIBLE)");
                else if (g_opb_write_failed) snprintf(result_status, sizeof(result_status), "ERROR(OpbWrite)");
                else if (status == 0) snprintf(result_status, sizeof(result_status), "UNSAT");
                // clasp が結果(s 行)を返さなかった（起動失敗・異常終了・UNKNOWN）。UNSAT とは区別する
                else snprintf(result_status, sizeof(result_status), "ERROR(Clasp)");

                DBG("  [完了] %s / Verify=%s / Diff=%+d (時間=%.1fms, clasp=%.1fms)\n",
                    result_status, verify_str, diff, (double)(clock() - case_t0) * 1000.0 / CLOCKS_PER_SEC, g_case_clasp_ms);
                WRITE_RESULT_ROW(fp_res, case_num, fault_names_joined, result_status, verify_str, new_x, diff, case_t0, total_vars, cons_count);
                write_xtp(fp_xtp, final_pat);

                free(final_pat);
                free(var_values);
                if (var_to_net) free(var_to_net);
            }
            else {
                DBG("  -> ERROR: リスト中の故障の一部がこのパターンでは検出されない(入力不整合)\n");
                WRITE_RESULT_ROW(fp_res, case_num, fault_names_joined, "ERROR(FaultNotDetected)", "N/A", orig_x, 0, case_t0, 0, 0);
                write_xtp(fp_xtp, pat_str);
            }
            free_multi_case(&mc);
            free(fault_names_joined);
            free(specs);
            case_num++;
        }

        free(pat_str);
        free(fault_line);
        fclose(fp_fault); fclose(fp_tp); if (fp_res) fclose(fp_res); fclose(fp_xtp);
        remove("clasp_out.txt");
        printf("バッチ処理完了。\n");
        return 0;
    }
    // ====================================================================
    // 対話モード
    // 単一故障。伝搬経路・正当化に関わる信号線の一覧と、解いた OPB を出力する。
    // ====================================================================
    if (top_mode == 1) {
        char fault_name[256], opb_file[1024], out_file[1024];
        int fault_type;
        // パターンは入力数ぶん確保する（固定1024では大規模回路で溢れる）
        size_t pat_buf_size = (size_t)n_pi + 64;
        char* pat_str = (char*)malloc(pat_buf_size);
        if (!pat_str) return -1;
        char pat_fmt[32];
        snprintf(pat_fmt, sizeof(pat_fmt), "%%%zus", pat_buf_size - 1);

        printf("テストパターン: "); scanf(pat_fmt, pat_str);
        printf("故障箇所: "); scanf("%255s", fault_name);
        printf("故障種類(0/1): "); scanf("%d", &fault_type);
        printf("結果テキスト出力: "); scanf("%1023s", out_file);
        printf("OPB出力ファイル: "); scanf("%1023s", opb_file);
        if (g_pin_perm) {   // -pin 指定時は pinファイルの順で入力されたパターンを input 宣言の順へ
            char* conv = pin_to_internal(pat_str);
            if (!conv) { free(pat_str); return -1; }
            free(pat_str); pat_str = conv;
        }

        g_opb_write_failed = 0;
        int fault_net_id = get_net_index(fault_name);
        clear_all_flags();
        assign_pi_values(pat_str);
        run_simulation_logic(fault_net_id, fault_type);

        if (extract_single_bundle_path(fault_net_id, 0)) {
            extract_justification_cone(fault_net_id);
            write_results_to_file(out_file, pat_str, fault_name, fault_type);

            // [main9 バグ修正] n_net+1 は total_vars を下回りうる（上のMAX_OPB_VARS参照）
            int* var_values = (int*)calloc(MAX_OPB_VARS, sizeof(int));
            int* var_to_net = NULL;
            int total_vars = 0, cons_count = 0, detected = 0;
            char* final_pat = (char*)malloc((size_t)n_pi + 1);
            if (!final_pat) { free(var_values); free(pat_str); return -1; }
            final_pat[0] = '\0';

            printf("\n--- 1回求解+検証 (CEGARなし) ---\n");
            int status = run_single_shot(opb_file, pat_str, fault_net_id, fault_type, &var_to_net, &total_vars, &cons_count, var_values, final_pat, &detected);

            if (status == 1 || status == 3) {
                printf("[結果] %s (変数: %d, 制約: %d)\n", status == 1 ? "最適解" : "最適性未証明の解", total_vars, cons_count);
                if (g_pin_perm) {
                    char* o = internal_to_pin(final_pat);
                    printf("X抽出後パターン: %s (pinファイルの順)\n", o ? o : final_pat);
                    free(o);
                }
                else printf("X抽出後パターン: %s\n", final_pat);
                printf("検証結果: %s\n", detected ? "Detected" : "NOT-Detected (未検出として記録)");
            }
            else if (g_opb_write_failed) printf("\n[結果] OPBファイルの書き出しに失敗しました（パス長やディレクトリを確認してください）\n");
            else if (status == 0) printf("\n[結果] 解なし (UNSAT)\n");
            else printf("\n[結果] clasp が結果を返しませんでした（clasp.exe の有無・実行を確認してください）\n");

            free(final_pat);
            free(var_values);
            if (var_to_net) free(var_to_net);
        }
        else printf("故障未検出。\n");

        free(pat_str);
        return 0;
    }
    return 0;
}
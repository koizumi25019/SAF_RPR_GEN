#define _POSIX_C_SOURCE 200809L
#include "fault/equivalence.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* 故障ID = ネット番号 * 2 + 縮退値。Union-Findの根と計算代表は別に管理する。
 * 根は集合統合の効率のために選び、代表は既存の計算順に合わせて選ぶ。 */
typedef struct EquivalenceEntry {
    int parent;
    int group_size;
    FNODE* representative;
    bool output_requested;
    EquivalentFault member;
    const EquivalentFault* members;
} EquivalenceEntry;

static EquivalenceEntry* fault_entries;
static int fault_count;
static FaultEquivalenceStats equivalence_stats;

static int FaultId(const NLIST* net, int type)
{
    return net->n * 2 + (type == SF1);
}

/* 経路圧縮で、次回から集合の根へ直接たどれるようにする。 */
static int FindRoot(int fault_id)
{
    int root = fault_id;
    while (fault_entries[root].parent != root)
        root = fault_entries[root].parent;
    while (fault_id != root) {
        int parent = fault_entries[fault_id].parent;
        fault_entries[fault_id].parent = root;
        fault_id = parent;
    }
    return root;
}

static void MergeFaults(int first_fault, int second_fault)
{
    int first_root = FindRoot(first_fault);
    int second_root = FindRoot(second_fault);
    if (first_root == second_root) return;

    /* 小さい集合を大きい集合につなぎ、木が深くなるのを防ぐ。 */
    if (fault_entries[first_root].group_size < fault_entries[second_root].group_size) {
        int temporary = first_root;
        first_root = second_root;
        second_root = temporary;
    }
    fault_entries[second_root].parent = first_root;
    fault_entries[first_root].group_size += fault_entries[second_root].group_size;
}

static void AddStructuralPair(NLIST* input, int input_type, NLIST* output, int output_type)
{
    MergeFaults(FaultId(input, input_type), FaultId(output, output_type));
    if (input_type == SF0) input->test_sa0 = NO;
    else input->test_sa1 = NO;
}

static void AddStructuralEquivalences(void)
{
    for (int net_index = 0; net_index < n_net; net_index++) {
        NLIST* output = &nl[net_index];
        if (opt.fault_model == FM_TDF) {
            /* LOCの時刻境界・DFF境界は越えず、BUF/INVだけをまとめる。 */
            if (output->type != BUF && output->type != INV) continue;
            if (!output->peer_1t || output->peer_1t->type == IN) continue;
            if (!output->in[0]->peer_1t) continue;
        }

        switch (output->type) {
        case BUF:
        case INV:
            AddStructuralPair(output->in[0], SF0, output, output->type == INV ? SF1 : SF0);
            AddStructuralPair(output->in[0], SF1, output, output->type == INV ? SF0 : SF1);
            break;
        case AND:
        case NAND:
            for (int input_index = 0; input_index < output->n_in; input_index++)
                AddStructuralPair(output->in[input_index], SF0, output,
                                  output->type == NAND ? SF1 : SF0);
            break;
        case OR:
        case NOR:
            for (int input_index = 0; input_index < output->n_in; input_index++)
                AddStructuralPair(output->in[input_index], SF1, output,
                                  output->type == NOR ? SF0 : SF1);
            break;
        default:
            break;
        }
    }
}

void FreeFaultEquivalence(void)
{
    free(fault_entries);
    fault_entries = NULL;
    fault_count = 0;
    /* ログは計算終了後に書くため、統計は次の初期化まで残す。 */
}

bool InitFaultEquivalence(void)
{
    FreeFaultEquivalence();
    equivalence_stats = (FaultEquivalenceStats){0};
    if (n_net <= 0 || n_net > INT_MAX / 2) {
        fprintf(stderr, "ERROR: invalid net count for fault equivalence.\n");
        return false;
    }
    fault_count = n_net * 2;
    fault_entries = calloc((size_t)fault_count, sizeof(*fault_entries));
    if (!fault_entries) {
        fprintf(stderr, "ERROR: failed to allocate fault equivalence groups.\n");
        return false;
    }
    for (int fault_id = 0; fault_id < fault_count; fault_id++) {
        fault_entries[fault_id].parent = fault_id;
        fault_entries[fault_id].group_size = 1;
        fault_entries[fault_id].member.net = &nl[fault_id / 2];
        fault_entries[fault_id].member.type = fault_id % 2 ? SF1 : SF0;
    }
    for (int net_index = 0; net_index < n_net; net_index++) {
        nl[net_index].test_sa0 = YES;
        nl[net_index].test_sa1 = YES;
    }
    /* 明示故障リストだけの既存実行は、各故障を独立に計算する仕様を保つ。 */
    if (!opt.file.input.fault || opt.file.input.equiv) AddStructuralEquivalences();
    return true;
}

static char* TrimWhitespace(char* text)
{
    while (isspace((unsigned char)*text)) text++;
    char* end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

static int CompareNetNames(const void* first, const void* second)
{
    const NLIST* const* first_net = first;
    const NLIST* const* second_net = second;
    return strcmp((*first_net)->name, (*second_net)->name);
}

/* 名前順の索引を使い、ペアごとのネットリスト全走査を避ける。 */
static bool ParseFaultReference(char* text, NLIST** sorted_nets, int* fault_id)
{
    char* reference = TrimWhitespace(text);
    char* suffix = strrchr(reference, '/');
    if (!suffix || suffix == reference ||
        (strcmp(suffix, "/0") != 0 && strcmp(suffix, "/1") != 0)) {
        fprintf(stderr, "expected net_name/0 or net_name/1: '%s'\n", reference);
        return false;
    }
    int type = suffix[1] == '0' ? SF0 : SF1;
    *suffix = '\0';
    NLIST key = {.name = reference};
    NLIST* key_pointer = &key;
    NLIST** match = bsearch(&key_pointer, sorted_nets, (size_t)n_net,
                           sizeof(*sorted_nets), CompareNetNames);
    if (!match) {
        fprintf(stderr, "unknown net name: '%s'\n", reference);
        return false;
    }
    *fault_id = FaultId(*match, type);
    return true;
}

static bool ReadExternalEquivalences(void)
{
    FILE* input = fopen(opt.file.input.equiv, "r");
    if (!input) {
        fprintf(stderr, "FILE ERROR: cannot open equivalence file '%s'\n", opt.file.input.equiv);
        return false;
    }
    NLIST** sorted_nets = malloc((size_t)n_net * sizeof(*sorted_nets));
    if (!sorted_nets) {
        fprintf(stderr, "ERROR: failed to allocate equivalence net-name index.\n");
        fclose(input);
        return false;
    }
    for (int net_index = 0; net_index < n_net; net_index++) sorted_nets[net_index] = &nl[net_index];
    qsort(sorted_nets, (size_t)n_net, sizeof(*sorted_nets), CompareNetNames);

    char* line = NULL;
    size_t capacity = 0;
    size_t line_number = 0;
    bool success = true;
    ssize_t line_length;
    while ((line_length = getline(&line, &capacity, input)) >= 0) {
        line_number++;
        /* 埋め込みNULで行の後半が黙って無視されるのを防ぐ。 */
        if (memchr(line, '\0', (size_t)line_length)) {
            fprintf(stderr, "FILE ERROR: %s:%zu: embedded NUL in equivalence pair\n",
                    opt.file.input.equiv, line_number);
            success = false;
            break;
        }
        char* pair = TrimWhitespace(line);
        if (!*pair || *pair == '#') continue;
        char* comma = strchr(pair, ',');
        if (!comma || strchr(comma + 1, ',')) {
            fprintf(stderr, "FILE ERROR: %s:%zu: expected exactly two comma-separated faults\n",
                    opt.file.input.equiv, line_number);
            success = false;
            break;
        }
        *comma = '\0';
        int first_fault, second_fault;
        if (!ParseFaultReference(pair, sorted_nets, &first_fault) ||
            !ParseFaultReference(comma + 1, sorted_nets, &second_fault)) {
            fprintf(stderr, "FILE ERROR: %s:%zu: invalid equivalence pair\n",
                    opt.file.input.equiv, line_number);
            success = false;
            break;
        }
        MergeFaults(first_fault, second_fault);
        equivalence_stats.input_pairs++;
    }
    if (success && !feof(input)) {
        fprintf(stderr, "FILE ERROR: failed to read equivalence file '%s'\n", opt.file.input.equiv);
        success = false;
    }
    free(line);
    free(sorted_nets);
    if (fclose(input) != 0) success = false;
    return success;
}

/* InitTargetOrderと同じく、レベルが低い故障を優先する。同レベルはハッシュ表の
 * 走査順を維持するので、入力ペアの順番やUnion-Findの根に代表が左右されない。 */
static int ChooseRepresentatives(void)
{
    for (int fault_id = 0; fault_id < fault_count; fault_id++)
        fault_entries[fault_id].representative = NULL;

    int representative_count = 0;
    for (int bucket = 0; bucket < MAXSIZE_HASH; bucket++) {
        for (FNODE* fault = readdata.fault.list[bucket]; fault; fault = fault->nextptr) {
            int root = FindRoot(FaultId(fault->netptr, fault->type));
            FNODE* previous = fault_entries[root].representative;
            if (!previous) representative_count++;
            if (!previous || fault->netptr->level < previous->netptr->level)
                fault_entries[root].representative = fault;
        }
    }
    return representative_count;
}

static void RemoveNonrepresentatives(void)
{
    for (int bucket = 0; bucket < MAXSIZE_HASH; bucket++) {
        /* ノードへのリンクを持ち、リスト先頭も途中も同じ手順で削除する。 */
        FNODE** fault_link = &readdata.fault.list[bucket];
        while (*fault_link) {
            FNODE* fault = *fault_link;
            int root = FindRoot(FaultId(fault->netptr, fault->type));
            if (fault == fault_entries[root].representative) {
                fault_link = &fault->nextptr;
            } else {
                *fault_link = fault->nextptr;
                free(fault->string);
                free(fault->name);
                free(fault);
            }
        }
    }
    readdata.fault.numinit = equivalence_stats.targets_after_external;
    /* baselineのログ項目を保ち、構造由来・外部由来を合わせた除外数を記録する。
     * 明示リストの場合も、CSVへ出力する指定範囲の中で数える。 */
    readdata.fault.numequiv = equivalence_stats.output_faults - readdata.fault.numinit;
}

bool FinalizeFaultEquivalence(void)
{
    /* 出力対象と計算対象を分離する。明示リストでは指定された故障だけを出力し、
     * それ以外の故障も等価関係をつなぐために利用する。 */
    if (opt.file.input.fault) {
        for (int bucket = 0; bucket < MAXSIZE_HASH; bucket++)
            for (FNODE* fault = readdata.fault.list[bucket]; fault; fault = fault->nextptr)
                fault_entries[FaultId(fault->netptr, fault->type)].output_requested = true;
    } else {
        for (int fault_id = 0; fault_id < fault_count; fault_id++) {
            NLIST* net = fault_entries[fault_id].member.net;
            fault_entries[fault_id].output_requested =
                opt.fault_model == FM_SAF || net->peer_1t != NULL;
        }
    }

    equivalence_stats.targets_before_external = ChooseRepresentatives();
    if (opt.file.input.equiv && !ReadExternalEquivalences()) return false;
    equivalence_stats.targets_after_external = ChooseRepresentatives();

    /* 逆順で先頭に追加し、CSVメンバー順をネット番号・縮退値の昇順に固定する。 */
    for (int fault_id = fault_count - 1; fault_id >= 0; fault_id--) {
        EquivalenceEntry* entry = &fault_entries[fault_id];
        if (!entry->output_requested) continue;
        EquivalenceEntry* group = &fault_entries[FindRoot(fault_id)];
        if (!group->representative) {
            fprintf(stderr, "ERROR: no representative for %s %s\n",
                    entry->member.net->name, FaultTypeName(entry->member.type));
            return false;
        }
        entry->member.next = group->members;
        group->members = &entry->member;
        equivalence_stats.output_faults++;
    }
    RemoveNonrepresentatives();
    if (opt.file.input.equiv)
        printf("Equivalence: %zu external pairs, targets %d -> %d, output faults %d\n",
               equivalence_stats.input_pairs, equivalence_stats.targets_before_external,
               equivalence_stats.targets_after_external, equivalence_stats.output_faults);
    return true;
}

const EquivalentFault* GetEquivalentFaults(const FNODE* representative)
{
    int fault_id = FaultId(representative->netptr, representative->type);
    /* 初期化後は書き換えず、forkしたワーカーでも同じメンバー一覧を参照する。 */
    while (fault_entries[fault_id].parent != fault_id)
        fault_id = fault_entries[fault_id].parent;
    return fault_entries[fault_id].members;
}

const FaultEquivalenceStats* GetFaultEquivalenceStats(void)
{
    return &equivalence_stats;
}

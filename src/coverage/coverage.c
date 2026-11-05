/* calc Fの実行部: FDP CSVを読み、CoverageModelで積算し、結果を書く。 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#include "coverage/coverage.h"
#include "coverage/coverage_model.h"

#define RESULT_DIGITS 15  /* F・Eの小数点以下の桁数 */

/** FDP CSVの故障数の内訳 */
typedef struct {
    long total;         /**< Nf: CSVの全故障行 */
    long undetectable;  /**< Nu: fdp = 0 */
    long equivalent;    /**< Neq: cube_cnt が空欄（代表のFDPを共有する等価故障） */
    long incomplete;    /**< complete = 0（列挙打ち切り。FDPは下界） */
} FaultCounts;

/** CSVで使う列の位置 */
typedef struct {
    int fdp;
    int cube_cnt;
    int complete;
} CsvColumns;

//-------------------------------------------------------------------------------------------------------------
//	CSV読み込み
//-------------------------------------------------------------------------------------------------------------

/* line の index 番目（0始まり）の列を field に取り出す。列が無ければ空文字列。 */
static void CsvField(const char* line, int index, char* field, size_t size)
{
    const char* p = line;
    for (int column = 0; column < index; column++) {
        p = strchr(p, ',');
        if (!p) {
            field[0] = '\0';
            return;
        }
        p++;
    }
    size_t length = strcspn(p, ",\r\n");
    if (length >= size) length = size - 1;
    memcpy(field, p, length);
    field[length] = '\0';
}

/* ヘッダー行から列名 name の位置を返す。無ければ -1。 */
static int CsvFindColumn(const char* header, const char* name)
{
    size_t name_length = strlen(name);
    const char* p = header;
    for (int index = 0; p; index++) {
        if (strcspn(p, ",\r\n") == name_length && strncmp(p, name, name_length) == 0) return index;
        p = strchr(p, ',');
        if (p) p++;
    }
    return -1;
}

static bool ReadCsvColumns(const char* header, CsvColumns* columns)
{
    columns->fdp = CsvFindColumn(header, "fdp");
    columns->cube_cnt = CsvFindColumn(header, "cube_cnt");
    columns->complete = CsvFindColumn(header, "complete");
    if (columns->fdp < 0 || columns->cube_cnt < 0 || columns->complete < 0) {
        fprintf(stderr, "ERROR: calc F requires fdp, cube_cnt and complete columns\n");
        return false;
    }
    return true;
}

/* CSVの全故障行をモデルへ加え、内訳を数える。 */
static bool AccumulateFaults(FILE* csv, const char* path, CoverageModel* model, FaultCounts* counts)
{
    char* line = NULL;
    size_t capacity = 0;
    if (getline(&line, &capacity, csv) < 0) {
        fprintf(stderr, "ERROR: calc F: empty FDP CSV: %s\n", path);
        free(line);
        return false;
    }
    CsvColumns columns;
    if (!ReadCsvColumns(line, &columns)) {
        free(line);
        return false;
    }

    bool ok = true;
    mpf_t density;
    mpf_init2(density, model->precision);
    long line_number = 1;
    while (ok && getline(&line, &capacity, csv) >= 0) {
        line_number++;
        char fdp[64], cube_cnt[32], complete[8];
        CsvField(line, columns.fdp, fdp, sizeof(fdp));
        if (fdp[0] == '\0') continue;  /* 空行 */
        CsvField(line, columns.cube_cnt, cube_cnt, sizeof(cube_cnt));
        CsvField(line, columns.complete, complete, sizeof(complete));

        if (mpf_set_str(density, fdp, 10) != 0 || mpf_sgn(density) < 0 || mpf_cmp_ui(density, 1) > 0) {
            fprintf(stderr, "ERROR: calc F: invalid fdp '%s' at %s:%ld\n", fdp, path, line_number);
            ok = false;
            break;
        }
        counts->total++;
        if (cube_cnt[0] == '\0') counts->equivalent++;
        if (strcmp(complete, "0") == 0) counts->incomplete++;
        if (mpf_sgn(density) == 0) {
            counts->undetectable++;  /* 検出確率0。和への寄与も0 */
            continue;
        }
        CoverageModelAddFault(model, density);
    }
    mpf_clear(density);
    free(line);
    if (ok && counts->total == 0) {
        fprintf(stderr, "ERROR: calc F: no fault rows in %s\n", path);
        ok = false;
    }
    return ok;
}

//-------------------------------------------------------------------------------------------------------------
//	結果出力
//-------------------------------------------------------------------------------------------------------------

/* ratio = sum / count（count = 0 なら0） */
static void Ratio(mpf_t ratio, const mpf_t sum, long count)
{
    if (count > 0) mpf_div_ui(ratio, sum, (unsigned long)count);
    else mpf_set_ui(ratio, 0);
}

/* 1つのNpについて F, [F_nr,] E, [E_nr] を separator 区切りで書く。 */
static void WriteResultRow(FILE* fp, const CoverageModel* model, int index,
                           const FaultCounts* counts, const char* separator)
{
    long detectable = counts->total - counts->undetectable;
    mpf_t value;
    mpf_init2(value, model->precision);
    fprintf(fp, "%lu", model->pattern_counts[index]);
    const long denominators[2] = { counts->total, detectable };  /* F, E */
    for (int kind = 0; kind < 2; kind++) {
        Ratio(value, model->sum_with_replacement[index], denominators[kind]);
        gmp_fprintf(fp, "%s%.*Ff", separator, RESULT_DIGITS, value);
        if (model->without_replacement) {
            Ratio(value, model->sum_without_replacement[index], denominators[kind]);
            gmp_fprintf(fp, "%s%.*Ff", separator, RESULT_DIGITS, value);
        }
    }
    fprintf(fp, "\n");
    mpf_clear(value);
}

static void WriteResultHeader(FILE* fp, bool without_replacement, const char* separator)
{
    if (without_replacement) {
        fprintf(fp, "Np%sF with-repl (=Sum/Nf)%sF_nr without-repl (=Sum/Nf)"
                "%sE with-repl (=Sum/Nd)%sE_nr without-repl (=Sum/Nd)\n",
                separator, separator, separator, separator);
    } else {
        fprintf(fp, "Np%sF (=Sum/Nf)%sE (=Sum/Nd)\n", separator, separator);
    }
}

static void WriteCsvReport(FILE* fp, const CoverageModel* model, const FaultCounts* counts,
                           const char* fdp_path, const char* circuit_name, int n_inputs)
{
    fprintf(fp, "[Settings]\n");
    fprintf(fp, "Item,Value\n");
    fprintf(fp, "Circuit,%s\n", circuit_name);
    fprintf(fp, "FDP CSV,%s\n", fdp_path);
    fprintf(fp, "Primary inputs (n_pi),%d\n", n_inputs);
    fprintf(fp, "N total patterns,2^%d\n", n_inputs);
    fprintf(fp, "GMP precision (bits),%lu\n", (unsigned long)model->precision);
    fprintf(fp, "Without replacement,%s\n", model->without_replacement ? "on" : "off");
    fprintf(fp, "Np values,\"");
    for (int i = 0; i < model->n_pattern_counts; i++) {
        fprintf(fp, "%s%lu", i ? " " : "", model->pattern_counts[i]);
    }
    fprintf(fp, "\"\n\n");

    fprintf(fp, "[Fault Statistics]\n");
    fprintf(fp, "Item,Count,Note\n");
    fprintf(fp, "Total faults (Nf),%ld,rows in FDP CSV\n", counts->total);
    fprintf(fp, "Undetectable (Nu),%ld,fdp=0\n", counts->undetectable);
    fprintf(fp, "Detectable (Nd),%ld,Nf-Nu\n", counts->total - counts->undetectable);
    fprintf(fp, "Equivalent (Neq),%ld,cube_cnt blank\n", counts->equivalent);
    fprintf(fp, "Incomplete (Ninc),%ld,complete=0 (fdp is a lower bound)\n", counts->incomplete);
    fprintf(fp, "\n");

    fprintf(fp, "[F/E Results]\n");
    WriteResultHeader(fp, model->without_replacement, ",");
    for (int i = 0; i < model->n_pattern_counts; i++) WriteResultRow(fp, model, i, counts, ",");
}

static void PrintSummary(const CoverageModel* model, const FaultCounts* counts, const char* output)
{
    printf("\n=== calc F ===\n");
    printf("Nf = %ld, Nu = %ld, Nd = %ld, Neq = %ld, Ninc = %ld\n",
           counts->total, counts->undetectable, counts->total - counts->undetectable,
           counts->equivalent, counts->incomplete);
    if (counts->incomplete > 0) printf("Note: incomplete faults exist; F and E are lower bounds.\n");
    WriteResultHeader(stdout, model->without_replacement, "  ");
    for (int i = 0; i < model->n_pattern_counts; i++) WriteResultRow(stdout, model, i, counts, "  ");
    printf("calc F CSV: %s\n", output);
}

//-------------------------------------------------------------------------------------------------------------
//	エントリポイント
//-------------------------------------------------------------------------------------------------------------
bool RunCoverage(const CoverageConfig* config, const char* fdp_path,
                 const char* circuit_name, int n_inputs)
{
    FILE* csv = fopen(fdp_path, "r");
    if (!csv) {
        fprintf(stderr, "ERROR: calc F: cannot open FDP CSV: %s\n", fdp_path);
        return false;
    }
    CoverageModel model;
    if (!CoverageModelInit(&model, config->pattern_counts, config->n_pattern_counts,
                           config->without_replacement, n_inputs)) {
        fclose(csv);
        return false;
    }

    FaultCounts counts = {0};
    bool ok = AccumulateFaults(csv, fdp_path, &model, &counts);
    fclose(csv);

    if (ok) {
        FILE* out = fopen(config->output, "w");
        if (!out) {
            fprintf(stderr, "ERROR: calc F: cannot write: %s\n", config->output);
            ok = false;
        } else {
            WriteCsvReport(out, &model, &counts, fdp_path, circuit_name, n_inputs);
            if (ferror(out)) ok = false;
            if (fclose(out) != 0) ok = false;
            if (!ok) fprintf(stderr, "ERROR: calc F: failed to write: %s\n", config->output);
        }
    }
    if (ok) PrintSummary(&model, &counts, config->output);
    CoverageModelClear(&model);
    return ok;
}

#define _POSIX_C_SOURCE 200809L
/* calc Fのオプション解析。本体のオプション処理からは IsCoverageOption / SetCoverageOption だけを呼ぶ。 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coverage/coverage.h"

#define OPTION_ENABLE         "-calc_f"
#define OPTION_NO_REPLACEMENT "-calc_f_nr"
#define OPTION_OUTPUT         "-calc_f_out"
#define OUTPUT_SUFFIX         "_F"

void InitCoverageConfig(CoverageConfig* config)
{
    memset(config, 0, sizeof(*config));
}

bool IsCoverageOption(const char* name)
{
    return strcmp(name, OPTION_ENABLE) == 0 || strcmp(name, OPTION_NO_REPLACEMENT) == 0 ||
           strcmp(name, OPTION_OUTPUT) == 0;
}

static int CompareCounts(const void* left, const void* right)
{
    unsigned long a = *(const unsigned long*)left;
    unsigned long b = *(const unsigned long*)right;
    return (a > b) - (a < b);
}

/* 1個のNp（正の10進整数）を読む。符号・小数・空要素は受け付けない。 */
static bool ParsePatternCount(const char* text, size_t length, unsigned long* count)
{
    char buffer[32];
    if (length == 0 || length >= sizeof(buffer)) return false;
    for (size_t i = 0; i < length; i++) {
        if (!isdigit((unsigned char)text[i])) return false;
    }
    memcpy(buffer, text, length);
    buffer[length] = '\0';
    errno = 0;
    *count = strtoul(buffer, NULL, 10);
    return errno == 0 && *count > 0;
}

/* "100,1000,10000" を昇順・重複なしのNp配列にする。 */
static bool SetPatternCounts(CoverageConfig* config, const char* value)
{
    unsigned long counts[COVERAGE_MAX_PATTERN_COUNTS];
    int n = 0;
    const char* token = value;
    for (;;) {
        const char* end = strchr(token, ',');
        size_t length = end ? (size_t)(end - token) : strlen(token);
        if (n == COVERAGE_MAX_PATTERN_COUNTS || !ParsePatternCount(token, length, &counts[n])) {
            return false;
        }
        n++;
        if (!end) break;
        token = end + 1;
    }

    qsort(counts, (size_t)n, sizeof(counts[0]), CompareCounts);
    config->n_pattern_counts = 0;
    for (int i = 0; i < n; i++) {
        if (i == 0 || counts[i] != counts[i - 1]) {
            config->pattern_counts[config->n_pattern_counts++] = counts[i];
        }
    }
    config->enabled = true;
    return true;
}

static bool HasValue(const char* value)
{
    return value && *value && *value != '#' && *value != '-';
}

bool SetCoverageOption(CoverageConfig* config, const char* name, const char* value)
{
    if (strcmp(name, OPTION_ENABLE) == 0) {
        if (HasValue(value) && SetPatternCounts(config, value)) return true;
        fprintf(stderr, "COMMAND ERROR: %s requires positive integers separated by commas "
                "(e.g. 100,1000,10000; at most %d values)\n", name, COVERAGE_MAX_PATTERN_COUNTS);
        return false;
    }
    if (strcmp(name, OPTION_NO_REPLACEMENT) == 0) {
        if (value && (strcmp(value, "on") == 0 || strcmp(value, "1") == 0)) {
            config->without_replacement = true;
            return true;
        }
        if (value && (strcmp(value, "off") == 0 || strcmp(value, "0") == 0)) {
            config->without_replacement = false;
            return true;
        }
        fprintf(stderr, "COMMAND ERROR: %s requires on|off\n", name);
        return false;
    }
    /* OPTION_OUTPUT */
    if (!HasValue(value)) {
        fprintf(stderr, "COMMAND ERROR: %s requires a value\n", name);
        return false;
    }
    free(config->output);
    config->output = strdup(value);
    return config->output != NULL;
}

/* "dir/c17a.csv" -> "dir/c17a_F.csv"（拡張子が無ければ末尾に "_F.csv"） */
static char* DefaultOutputPath(const char* fdp_path)
{
    const char* slash = strrchr(fdp_path, '/');
    const char* dot = strrchr(fdp_path, '.');
    if (dot && slash && dot < slash) dot = NULL;
    size_t stem = dot ? (size_t)(dot - fdp_path) : strlen(fdp_path);
    const char* extension = dot ? dot : ".csv";
    size_t size = stem + strlen(OUTPUT_SUFFIX) + strlen(extension) + 1;
    char* path = malloc(size);
    if (path) snprintf(path, size, "%.*s%s%s", (int)stem, fdp_path, OUTPUT_SUFFIX, extension);
    return path;
}

static bool SamePath(const char* a, const char* b)
{
    return a && b && strcmp(a, b) == 0;
}

bool FinalizeCoverageConfig(CoverageConfig* config, const char* fdp_path,
                            const char* log_path, const char* cube_path)
{
    if (!config->enabled) {
        if (config->without_replacement || config->output) {
            fprintf(stderr, "COMMAND ERROR: %s and %s require %s\n",
                    OPTION_NO_REPLACEMENT, OPTION_OUTPUT, OPTION_ENABLE);
            return false;
        }
        return true;
    }
    if (!config->output) config->output = DefaultOutputPath(fdp_path);
    if (!config->output) return false;
    if (SamePath(config->output, fdp_path) || SamePath(config->output, log_path) ||
        SamePath(config->output, cube_path)) {
        fprintf(stderr, "COMMAND ERROR: %s must differ from -fdp, -log and -cube\n", OPTION_OUTPUT);
        return false;
    }
    return true;
}

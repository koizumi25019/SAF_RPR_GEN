#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "io/cube_text.h"
#include "cli/options.h"

static const char marker_name[] = ".fdp-cube-output";
static const char marker_magic[] = "FDP per-fault cube output v2\n";
static const char legacy_marker_magic[] = "FDP per-fault cube output v1\n";

static char* JoinPath(const char* directory, const char* name)
{
    size_t size = strlen(directory) + strlen(name) + 2;
    char* path = malloc(size);
    if (path) snprintf(path, size, "%s/%s", directory, name);
    return path;
}

/* '%'も符号化するので、区切り文字を含む信号名でも別ファイルと衝突しない。 */
static bool IsFilenameChar(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
}

static char* CubeFilename(const FNODE* fault)
{
    char* name = malloc(3 * strlen(fault->name) + 32);
    if (!name) return NULL;
    char* cursor = name;
    for (const unsigned char* c = (const unsigned char*)fault->name; *c; c++) {
        if (IsFilenameChar(*c)) *cursor++ = (char)*c;
        else cursor += sprintf(cursor, "%%%02X", *c);
    }
    sprintf(cursor, "_%s_cube.txt", FaultTypeName(fault->type));
    return name;
}

static bool IsCubeFilename(const char* name)
{
    const char* suffixes[] = {
        "_sa0_cube.txt", "_sa1_cube.txt", "_STR_cube.txt", "_STF_cube.txt",
        "__sa0.txt", "__sa1.txt", "__STR.txt", "__STF.txt" /* 旧命名の出力も入れ替える。 */
    };
    size_t length = strlen(name);
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        size_t suffix_size = strlen(suffixes[i]);
        if (length > suffix_size && strcmp(name + length - suffix_size, suffixes[i]) == 0)
            return true;
    }
    return false;
}

static bool MakeDirectories(const char* directory)
{
    char* path = strdup(directory);
    if (!path) return false;
    bool success = true;
    for (char* cursor = path + 1; ; cursor++) {
        if (*cursor != '/' && *cursor != '\0') continue;
        char saved = *cursor;
        *cursor = '\0';
        if (mkdir(path, 0755) != 0 && errno != EEXIST) success = false;
        *cursor = saved;
        if (!success || !saved) break;
    }
    free(path);
    return success;
}

/* マーカーのない既存の非空ディレクトリは利用しない。
 * マーカーがある場合も削除するのは本機能の予約名の故障ファイルだけ。 */
static bool PrepareDirectory(const CubeOutput* output)
{
    const char* directory = output->directory;
    if (!*directory || !MakeDirectories(directory)) return false;
    DIR* dir = opendir(directory);
    if (!dir) return false;
    char* marker = JoinPath(directory, marker_name);
    bool success = marker != NULL;
    bool owned = false;
    struct stat info;
    if (success && lstat(marker, &info) == 0) {
        char magic[sizeof(marker_magic)] = {0};
        FILE* fp = S_ISREG(info.st_mode) ? fopen(marker, "r") : NULL;
        if (fp) {
            size_t size = fread(magic, 1, strlen(marker_magic), fp);
            owned = size == strlen(marker_magic) &&
                    (memcmp(magic, marker_magic, size) == 0 ||
                     memcmp(magic, legacy_marker_magic, size) == 0);
            fclose(fp);
        }
        success = owned;
    } else if (success && errno != ENOENT) {
        success = false;
    }
    while (success) {
        errno = 0;
        struct dirent* entry = readdir(dir);
        if (!entry) {
            success = errno == 0;
            break;
        }
        const char* name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        if (!owned) {
            fprintf(stderr, "CUBE ERROR: output directory is not empty and has no valid %s: %s\n",
                    marker_name, directory);
            success = false;
        } else if (IsCubeFilename(name)) {
            char* path = JoinPath(directory, name);
            success = path && unlink(path) == 0;
            free(path);
        }
    }
    if (closedir(dir) != 0) success = false;
    if (success) {
        FILE* fp = fopen(marker, "w");
        success = fp != NULL;
        if (fp) {
            success = fputs(marker_magic, fp) >= 0;
            if (success) success = fwrite(output->header, 1, output->header_size, fp) == output->header_size;
            if (fclose(fp) != 0) success = false;
        }
    }
    free(marker);
    return success;
}

bool InitCubeOutput(CubeOutput* output, const char* directory)
{
    *output = (CubeOutput){.directory = directory};
    FILE* fp = open_memstream(&output->header, &output->header_size);
    if (!fp) return false;
    bool success = WriteCubeMetadata(fp);
    if (fclose(fp) != 0) success = false;
    if (success) success = PrepareDirectory(output);
    if (!success) {
        fprintf(stderr, "CUBE ERROR: cannot prepare output directory: %s\n", directory);
        FreeCubeOutput(output);
    }
    return success;
}

void FreeCubeOutput(CubeOutput* output)
{
    free(output->header);
    *output = (CubeOutput){0};
}

bool WriteCubeFile(const CubeOutput* output, const FNODE* fault, const char* text, size_t size)
{
    if (!size) return true;
    char* name = CubeFilename(fault);
    char* path = name ? JoinPath(output->directory, name) : NULL;
    FILE* fp = path ? fopen(path, "w") : NULL;
    bool opened = fp != NULL;
    bool success = opened;
    if (fp) {
        success = fwrite(text, 1, size, fp) == size;
        if (fclose(fp) != 0) success = false;
    }
    if (!success) {
        fprintf(stderr, "CUBE ERROR: cannot write cube file for %s %s\n",
                fault->name, FaultTypeName(fault->type));
        if (opened) unlink(path); /* 部分的なファイルを完成結果として残さない。 */
    }
    free(path);
    free(name);
    return success;
}

bool WriteCubeMetadata(FILE* fp)
{
    fprintf(fp, "# FDP test cube metadata v1\n# model %s\n# limit %d\n# inputs %d\n# pi",
            opt.fault_model == FM_TDF ? "TDF" : "SAF", opt.file.input.limit, n_pi);
    for (int index = 0; index < n_pi; index++) fprintf(fp, " %s", pi[index]->name);
    fprintf(fp, "\n# values 0 1 X (don't care)\n\n");
    fprintf(fp, "# cube_include_incomplete %s\n",
            opt.cube_include_incomplete ? "on" : "off");
    if (opt.cube_fdp_max_text) fprintf(fp, "# cube_fdp_max %s\n\n", opt.cube_fdp_max_text);
    return !ferror(fp);
}

bool WriteFaultCubes(FILE* fp, const FaultResult* result, const CubeSet* cubes)
{
    if (!fp || (!result->complete && !opt.cube_include_incomplete) ||
        mpf_sgn(result->density) <= 0) return true;
    /* 支配流用で複数の集合を取り込むと、completeでも上限を超えることがある。 */
    if (opt.file.input.limit > 0 && cubes->n > opt.file.input.limit) return true;
    if (opt.cube_fdp_max_text && mpf_cmp(result->density, opt.cube_fdp_max) > 0) return true;
    fprintf(fp, "%s\t%s\n%s\n", result->target->name,
            FaultTypeName(result->target->type), result->complete ? "complete" : "incomplete");
    for (int index = 0; index < cubes->n; index++) fprintf(fp, "%s\n", cubes->data[index]);
    return !ferror(fp);
}

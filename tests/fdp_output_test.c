#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fdp/bdd/probability.h"
#include "io/fault_csv.h"
#include "io/cube_text.h"
#include "cli/options.h"

static void check_density(int nvars, CubeSet *cubes, unsigned long num, unsigned long den)
{
    DdManager *dd = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    assert(dd);
    mpf_t result, expected;
    mpf_init2(result, 8192);
    mpf_init2(expected, 8192);
    mpf_set_ui(expected, num);
    mpf_div_ui(expected, expected, den);
    assert(RunBDD(dd, nvars, cubes, result));
    assert(mpf_cmp(result, expected) == 0);
    assert(Cudd_CheckZeroRef(dd) == 0);
    mpf_clear(expected);
    mpf_clear(result);
    Cudd_Quit(dd);
}

// 小さな入力空間を独立に全列挙し、BDD のキューブ和集合と照合する。
static void check_enumeration(void)
{
    unsigned state = 42;
    for (int trial = 0; trial < 50; trial++) {
        char text[5][9];
        char* data[5];
        for (int c = 0; c < 5; c++) {
            data[c] = text[c];
            for (int i = 0; i < 8; i++) {
                state = state * 1664525u + 1013904223u;
                text[c][i] = "01X"[(state >> 16) % 3];
            }
            text[c][8] = '\0';
        }
        unsigned long covered = 0;
        for (int pattern = 0; pattern < 256; pattern++) {
            for (int c = 0; c < 5; c++) {
                bool match = true;
                for (int i = 0; i < 8; i++) {
                    char bit = ((pattern >> i) & 1) ? '1' : '0';
                    if (text[c][i] != 'X' && text[c][i] != bit) match = false;
                }
                if (match) { covered++; break; }
            }
        }
        CubeSet cubes = {.data = data, .n = 5};
        check_density(8, &cubes, covered, 256);
    }
}

static void check_writer(void)
{
    NLIST in = {.name = "input", .type = IN, .test_sa0 = 0};
    NLIST *inputs[] = {&in};
    NLIST net = {.name = "output", .type = BUF, .n_in = 1, .in = inputs};
    FNODE target = {.name = "output", .type = SF0, .netptr = &net};
    FaultResult r = {.target = &target, .cube_cnt = 2, .seeded_cnt = 1, .complete = true};
    mpf_init2(r.density, 8192);
    mpf_set_ui(r.density, 3);
    mpf_div_ui(r.density, r.density, 4);
    FILE *fp = tmpfile(); assert(fp);
    WriteFaultResult(fp, &r);
    r.complete = false;
    WriteFaultResult(fp, &r);
    rewind(fp);
    char text[256] = {0};
    assert(fread(text, 1, sizeof(text)-1, fp) > 0);
    assert(strcmp(text, "output,sa0,2,1,7.5000000000e-01,1\ninput,sa0,,,7.5000000000e-01,\n"
                        "output,sa0,2,0,7.5000000000e-01,1\ninput,sa0,,,7.5000000000e-01,\n") == 0);
    fclose(fp);
    mpf_clear(r.density);
}

static void check_cube_selection(void)
{
    FNODE target = {.name = "output", .type = SF0};
    FaultResult result = {.target = &target, .cube_cnt = 2, .complete = true};
    char* data[] = {"10X", "01X"};
    CubeSet cubes = {.data = data, .n = 2};
    mpf_init2(result.density, 8192);
    mpf_set_ui(result.density, 1);
    FILE* fp = tmpfile(); assert(fp);
    opt.file.input.limit = 1; /* 流用で上限超過・UNSATになった場合も除外する。 */
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0);
    opt.file.input.limit = 2; /* 上限ちょうどの完全被覆は保存する。 */
    result.complete = false;
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0);
    result.complete = true;
    mpf_set_ui(result.density, 0);
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0);
    mpf_set_ui(result.density, 1);
    assert(WriteFaultCubes(fp, &result, &cubes));
    rewind(fp);
    char text[128] = {0};
    assert(fread(text, 1, sizeof(text) - 1, fp) > 0);
    assert(strcmp(text, "output\tsa0\ncomplete\n10X\n01X\n") == 0);
    fclose(fp);
    fp = tmpfile(); assert(fp);
    opt.cube_fdp_max_text = "0.25";
    mpf_init2(opt.cube_fdp_max, 8192);
    mpf_set_ui(opt.cube_fdp_max, 1);
    mpf_div_2exp(opt.cube_fdp_max, opt.cube_fdp_max, 2);
    /* CSVでは同じ0.25に丸められても、上限超過なら保存しない。 */
    mpf_t delta;
    mpf_init2(delta, 8192);
    mpf_set_ui(delta, 1);
    mpf_div_2exp(delta, delta, 100);
    mpf_add(result.density, opt.cube_fdp_max, delta);
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0);
    mpf_set(result.density, opt.cube_fdp_max);
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) > 0); /* 上限値と等しければ保存する。 */
    fclose(fp);
    fp = tmpfile(); assert(fp);
    opt.cube_include_incomplete = true;
    result.complete = false;
    mpf_add(result.density, opt.cube_fdp_max, delta);
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0); /* 未完了を含めてもFDP上限は有効。 */
    mpf_set(result.density, opt.cube_fdp_max);
    opt.file.input.limit = 1;
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0); /* キューブ数の上限も有効。 */
    opt.file.input.limit = 2;
    mpf_set_ui(result.density, 0);
    assert(WriteFaultCubes(fp, &result, &cubes));
    assert(ftell(fp) == 0); /* 冗長故障は引き続き保存しない。 */
    mpf_set(result.density, opt.cube_fdp_max);
    assert(WriteFaultCubes(fp, &result, &cubes));
    rewind(fp);
    memset(text, 0, sizeof(text));
    assert(fread(text, 1, sizeof(text) - 1, fp) > 0);
    assert(strcmp(text, "output\tsa0\nincomplete\n10X\n01X\n") == 0);
    opt.cube_include_incomplete = false;
    mpf_clear(delta);
    mpf_clear(opt.cube_fdp_max);
    opt.cube_fdp_max_text = NULL;
    fclose(fp);
    mpf_clear(result.density);
    opt.file.input.limit = 0;
}

static void check_cube_files(void)
{
    char temporary[] = "/tmp/fdp-cube-files-XXXXXX";
    assert(mkdtemp(temporary));
    char directory[256], path[512], marker[512], notes[512];
    snprintf(directory, sizeof(directory), "%s/nested/cubes", temporary);
    CubeOutput output = {0};
    assert(InitCubeOutput(&output, directory));
    FNODE fault = {.name = "a/b%\\c", .type = SF0};
    const char text[] = "a/b%\\c\tsa0\ncomplete\n1X0\n";
    assert(WriteCubeFile(&output, &fault, text, strlen(text)));
    snprintf(path, sizeof(path), "%s/a%%2Fb%%25%%5Cc_sa0_cube.txt", directory);
    FILE* fp = fopen(path, "r"); assert(fp);
    char contents[1024] = {0};
    assert(fread(contents, 1, sizeof(contents) - 1, fp) > 0);
    assert(strcmp(contents, text) == 0);
    fclose(fp);
    snprintf(notes, sizeof(notes), "%s/notes.md", directory);
    fp = fopen(notes, "w"); assert(fp); fputs("keep", fp); fclose(fp);
    FreeCubeOutput(&output);
    assert(InitCubeOutput(&output, directory));
    assert(access(path, F_OK) != 0); /* 再実行で古い故障ファイルが消える。 */
    assert(access(notes, F_OK) == 0); /* 無関係なファイルは保持する。 */
    assert(WriteCubeFile(NULL, NULL, NULL, 0));
    /* 書き込み途中の失敗も通知し、部分ファイル（ここではリンク）を除く。 */
    assert(symlink("/dev/full", path) == 0);
    assert(!WriteCubeFile(&output, &fault, text, strlen(text)));
    assert(access(path, F_OK) != 0);
    FreeCubeOutput(&output);
    assert(!InitCubeOutput(&output, temporary)); /* マーカーなしの非空フォルダは拒否する。 */
    assert(access(notes, F_OK) == 0);
    unlink(notes);
    snprintf(marker, sizeof(marker), "%s/.fdp-cube-output", directory);
    unlink(marker);
    rmdir(directory);
    snprintf(directory, sizeof(directory), "%s/nested", temporary);
    rmdir(directory);
    rmdir(temporary);
}

int main(void)
{
    CubeSet empty = {0};
    check_density(0, &empty, 0, 1);
    check_density(64, &empty, 0, 1);
    char *zero[] = {""};
    CubeSet scalar = {.data = zero, .n = 1};
    check_density(0, &scalar, 1, 1);
    char *overlap[] = {"1XXXX", "X1XXX", "1XXXX"};
    CubeSet small = {.data = overlap, .n = 3};
    check_density(5, &small, 3, 4);
    const int sizes[] = {31, 32, 63, 64, 65, 257};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        int n = sizes[i];
        char *a = malloc(n+1), *b = malloc(n+1);
        memset(a, 'X', n); a[n] = 0;
        memset(b, 'X', n); b[n] = 0;
        char *data[] = {a, b, a};
        CubeSet cubes = {.data = data, .n = 1};
        check_density(n, &cubes, 1, 1);
        a[0] = '1'; b[1] = '1'; cubes.n = 3;
        check_density(n, &cubes, 3, 4);
        free(a); free(b);
    }
    // 2^30000 は従来の8192バイトの10進文字列バッファに収まらない。
    char *large = malloc(30001); memset(large, 'X', 30000); large[30000] = 0;
    char *data[] = {large}; CubeSet cubes = {.data = data, .n = 1};
    check_density(30000, &cubes, 1, 1); free(large);
    check_enumeration();
    check_writer();
    check_cube_selection();
    check_cube_files();
    puts("BDD probability, multiword import, overlap, large counts, CSV equivalence: PASS");
}

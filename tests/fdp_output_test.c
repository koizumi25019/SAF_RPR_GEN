#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cudd_wrapper.h"
#include "fault_result.h"

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
    puts("BDD probability, multiword import, overlap, large counts, CSV equivalence: PASS");
}

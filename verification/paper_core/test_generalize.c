/* Independent truth-table check: all 256 Boolean functions of three inputs,
   every detecting minterm, every completion and every retained literal.
   Reuses each oracle across queries to catch leaked assumptions. */
#include <stdio.h>
#include <string.h>
#include "paper_core.h"

static int matches(const char* cube, unsigned bits)
{
    for (int i = 0; i < 3; i++)
        if (cube[i] != 'X' && cube[i] - '0' != (int)((bits >> i) & 1)) return 0;
    return 1;
}

static int entails(const char* cube, unsigned fn)
{
    for (unsigned bits = 0; bits < 8; bits++)
        if (matches(cube, bits) && !(fn & (1u << bits))) return 0;
    return 1;
}

int main(void)
{
    const int vars[] = {1, 2, 3};
    int checked = 0;
    for (unsigned fn = 0; fn < 256; fn++) {
        CCaDiCaL* u = ccadical_init();
        ccadical_set_option(u, "factor", 0);
        /* NOT F: forbid each minterm for which F is true. */
        for (unsigned bits = 0; bits < 8; bits++) if (fn & (1u << bits)) {
            for (int i = 0; i < 3; i++)
                ccadical_add(u, bits & (1u << i) ? -vars[i] : vars[i]);
            ccadical_add(u, 0);
        }
        for (unsigned bits = 0; bits < 8; bits++) if (fn & (1u << bits)) {
            char cube[4];
            for (int i = 0; i < 3; i++) cube[i] = bits & (1u << i) ? '1' : '0';
            cube[3] = '\0';
            if (!PaperCoreGeneralize(u, vars, 3, cube, true) || !entails(cube, fn)) return 1;
            for (int i = 0; i < 3; i++) if (cube[i] != 'X') {
                char saved = cube[i]; cube[i] = 'X';
                if (entails(cube, fn)) return 2;
                cube[i] = saved;
            }
            checked++;
        }
        ccadical_release(u);
    }
    /* Empty input domain and empty core (constant true F). */
    CCaDiCaL* u = ccadical_init();
    ccadical_add(u, 0);
    char empty[] = "";
    if (!PaperCoreGeneralize(u, NULL, 0, empty, true)) return 3;
    ccadical_release(u);
    /* A non-detecting pattern must fail without changing the cube. */
    u = ccadical_init();
    char rejected[] = "000";
    if (PaperCoreGeneralize(u, vars, 3, rejected, true) || strcmp(rejected, "000")) return 4;
    ccadical_release(u);
    printf("PASS: %d detecting minterms, 256 truth tables; sound and subset-minimal\n", checked);
    return 0;
}

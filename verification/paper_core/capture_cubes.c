/* Verification-only GNU linker wrappers. Production binaries are unchanged. */
#include "fdp/paper_core.h"

CCaDiCaL* __real_PaperCoreBuildOracle(TARGET*);
bool __real_PaperCoreGeneralize(CCaDiCaL*, const int*, int, char*, bool);
int __real_ccadical_failed(CCaDiCaL*, int);

static char path[4096];
static char* extracted;
static const int* active_vars;
static CCaDiCaL* active_oracle;
static int active_n, failed_index, sequence;

CCaDiCaL* __wrap_PaperCoreBuildOracle(TARGET* target)
{
    const char* dir = getenv("PAPER_CORE_CAPTURE_DIR");
    if (!dir || target->num != 1) exit(1);
    if (snprintf(path, sizeof(path), "%s/%06d.cover", dir, sequence++) >= (int)sizeof(path)) exit(1);
    FILE* fp = fopen(path, "w");
    if (!fp) { perror(path); exit(1); }
    FNODE* fault = target->list[0];
    fprintf(fp, "%s %d %d\n", fault->name, fault->type == SF1, n_pi);
    for (int i = 0; i < n_pi; i++) fprintf(fp, "%s%c", pi[i]->name, i + 1 == n_pi ? '\n' : ' ');
    if (fclose(fp)) exit(1);
    return __real_PaperCoreBuildOracle(target);
}

int __wrap_ccadical_failed(CCaDiCaL* oracle, int lit)
{
    int failed = __real_ccadical_failed(oracle, lit);
    if (active_oracle == oracle) {
        if (failed_index >= active_n) exit(1);
        int expected = extracted[failed_index] == '1' ? active_vars[failed_index] : -active_vars[failed_index];
        if (lit != expected) exit(1);
        if (!failed) extracted[failed_index] = 'X';
        failed_index++;
    }
    return failed;
}

bool __wrap_PaperCoreGeneralize(CCaDiCaL* oracle, const int* vars, int n, char* cube, bool verify)
{
    extracted = malloc((size_t)n + 1);
    if (!extracted) exit(1);
    memcpy(extracted, cube, (size_t)n);
    extracted[n] = '\0';
    active_oracle = oracle;
    active_vars = vars;
    active_n = n;
    failed_index = 0;
    bool ok = __real_PaperCoreGeneralize(oracle, vars, n, cube, verify);
    active_oracle = NULL;
    if (ok) {
        if (failed_index != n) exit(1);
        FILE* fp = fopen(path, "a");
        if (!fp) { perror(path); exit(1); }
        fprintf(fp, "%s %s\n", extracted, cube);
        if (fclose(fp)) exit(1);
    }
    free(extracted);
    extracted = NULL;
    return ok;
}

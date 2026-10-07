/* Verify actual baseline cubes against full original-circuit detection BDDs.
 * No baseline solver, fault CNF, XID, or scope mask enters this oracle. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdp/read.h"
#include "fdp/cube_set.h"
#include "fdp/gt_verify.h"
static void fail(const char *message, const char *name) {
    fprintf(stderr, "[BASELINE_CHECK] %s: %s\n", message, name); exit(1);
}
void CheckBaselineCovers(void) {
    const char *directory = getenv("BASELINE_COVER_DIR");
    for (int h = 0; h < MAXSIZE_HASH; h++) {
        for (FNODE *fault = readdata.fault.list[h]; fault; fault = fault->nextptr) {
            char path[4096], name[1024];
            snprintf(path, sizeof(path), "%s/%s_sa%d.cover", directory,
                     fault->name, fault->type == SF1);
            FILE *fp = fopen(path, "r");
            if (!fp) fail("missing cover", path);
            int stuck, complete, inputs, count;
            if (fscanf(fp, "%1023s %d %d %d %d", name, &stuck, &complete, &inputs, &count) != 5
                || strcmp(name, fault->name) || stuck != (fault->type == SF1)
                || inputs != n_pi || count < 0 || (complete != 0 && complete != 1))
                fail("invalid header", path);
            for (int i = 0; i < n_pi; i++) {
                if (fscanf(fp, "%1023s", name) != 1 || strcmp(name, pi[i]->name))
                    fail("PI order mismatch", path);
            }
            int ch; while ((ch = fgetc(fp)) != '\n' && ch != EOF) {}
            CubeSet cubes; cubeset_init(&cubes, count ? count : 1);
            char *line = NULL; size_t capacity = 0;
            for (int i = 0; i < count; i++) {
                if (getline(&line, &capacity, fp) < 0) fail("missing cube", path);
                line[strcspn(line, "\r\n")] = '\0';
                if (strlen(line) != (size_t)n_pi || strspn(line, "01X") != (size_t)n_pi)
                    fail("invalid cube", path);
                cubeset_push(&cubes, strdup(line));
            }
            free(line); fclose(fp);
            GT_Check(fault, &cubes, !complete);
            cubeset_free(&cubes);
        }
    }
}

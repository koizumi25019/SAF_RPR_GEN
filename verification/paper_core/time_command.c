#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Measure exactly one waited child, including startup/exit and user+system CPU. */
static double now(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) { perror("clock_gettime"); exit(125); }
    return t.tv_sec + t.tv_nsec / 1e9;
}
int main(int argc, char **argv) {
    if (argc < 3) return 125;
    double start = now();
    pid_t child = fork();
    if (child < 0) { perror("fork"); return 125; }
    if (!child) { execvp(argv[2], &argv[2]); perror("execvp"); _exit(127); }
    struct rusage usage;
    int status;
    while (wait4(child, &status, 0, &usage) < 0) {
        if (errno == EINTR) continue;
        perror("wait4"); return 125;
    }
    double wall = now() - start;
    double user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double system = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    FILE *fp = fopen(argv[1], "w");
    if (!fp) { perror("timing output"); return 125; }
    fprintf(fp, "cpu_s,user_s,system_s,wall_s,exit_code\n%.6f,%.6f,%.6f,%.6f,%d\n",
            user + system, user, system, wall, code);
    if (fclose(fp)) return 125;
    return code;
}

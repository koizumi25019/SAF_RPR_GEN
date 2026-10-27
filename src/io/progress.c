#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "io/progress.h"

bool ProgressDisplayDue(ProgressDisplay* display, bool force)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = now.tv_sec - display->last_update.tv_sec +
                     (now.tv_nsec - display->last_update.tv_nsec) / 1e9;
    if (display->started && !force && elapsed < 1.0) return false;
    display->started = true;
    display->last_update = now;
    return true;
}

/* 最終列を空けて自動折り返しを避ける。制御文字や多バイトの途中を出さない。 */
static void write_terminal_line(const char* text, int columns)
{
    int limit = columns > 1 ? columns - 1 : 0;
    size_t length = strlen(text);
    int width = length < (size_t)limit ? (int)length : limit;
    int content = length > (size_t)limit && limit >= 3 ? limit - 3 : width;
    for (int index = 0; index < content; index++) {
        unsigned char ch = (unsigned char)text[index];
        putchar(ch >= 32 && ch < 127 ? ch : '?');
    }
    if (content < width) fputs("...", stdout);
}

void ProgressDisplayWrite(ProgressDisplay* display, const char* header,
                          const char* const* details, int count)
{
    const char* term = getenv("TERM");
    if (!isatty(STDOUT_FILENO) || (term && strcmp(term, "dumb") == 0)) {
        fputs(header, stdout);
        for (int index = 0; index < count; index++) printf(" | %s", details[index]);
        putchar('\n');
        fflush(stdout);
        return;
    }

    struct winsize size = {0};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    int columns = size.ws_col ? size.ws_col : 80;
    int rows = size.ws_row ? size.ws_row : 24;
    int capacity = rows > 1 ? rows - 1 : 1;
    bool paged = count > capacity - 1 && capacity >= 2;
    bool footer = paged && capacity >= 3;
    int slots = capacity - 1 - (footer ? 1 : 0);
    int pages = slots > 0 ? (count + slots - 1) / slots : 1;
    int first = paged ? (int)((display->frames / 5) % (unsigned long)pages) * slots : 0;
    int shown = count - first < slots ? count - first : slots;
    int new_rows = 1 + shown + (footer ? 1 : 0);
    int old_rows = display->rendered_rows < capacity ? display->rendered_rows : capacity;
    if (old_rows) printf("\033[%dA", old_rows);

    char page[96];
    snprintf(page, sizeof(page), "Workers %d-%d/%d (pages rotate every 5 updates)",
             first + 1, first + shown, count);
    int repaint = old_rows > new_rows ? old_rows : new_rows;
    for (int row = 0; row < repaint; row++) {
        fputs("\r\033[2K", stdout);
        if (row < new_rows) {
            const char* line = row == 0 ? header : row <= shown ? details[first + row - 1] : page;
            write_terminal_line(line, columns);
        }
        putchar('\n');
    }
    if (repaint > new_rows) printf("\033[%dA", repaint - new_rows);
    display->rendered_rows = new_rows;
    display->frames++;
    fflush(stdout);
}

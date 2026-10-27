#pragma once
#include <stdbool.h>
#include <time.h>

typedef struct {
    bool started;
    struct timespec last_update;
    int rendered_rows;
    unsigned long frames;
} ProgressDisplay;

/* 全ワーカーで共通の表示間隔。開始と全体の終了だけはforceする。 */
bool ProgressDisplayDue(ProgressDisplay* display, bool force);
/* 端末では幅・高さ内の固定領域を再描画。非端末では改行付きのスナップショット。 */
void ProgressDisplayWrite(ProgressDisplay* display, const char* header,
                          const char* const* details, int count);

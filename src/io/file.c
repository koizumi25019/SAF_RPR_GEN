#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io/file.h"

void fileOpen(
    FILE** fileptr,           /**< ファイルポインタ */
    const char* filename,     /**< ファイル名 */
    const char* mode          /**< モード */
)
{
    // 標準関数 fopen を使用し、戻り値をポインタの参照先に代入する
    *fileptr = fopen(filename, mode);

    // fopen は失敗すると NULL を返すので、それでエラー判定を行う
    if (*fileptr == NULL)
    {
        printf("\n  FILE ERROR: file open error. ");
        printf("%c%s%c cannot open.\n", '"', filename, '"');
        exit(EXIT_FAILURE);
    }

    return;
}

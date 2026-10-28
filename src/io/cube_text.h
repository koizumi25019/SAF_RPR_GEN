#pragma once
#include "io/fault_csv.h"
#include "fdp/cube_set.h"

/** 親プロセス専用。出力先と管理ファイル用の共通情報を保持する。 */
typedef struct {
    const char* directory;
    char* header;
    size_t header_size;
} CubeOutput;

/** 出力ディレクトリを準備し、前回の生成ファイルを取り除く。 */
bool InitCubeOutput(CubeOutput* output, const char* directory);
void FreeCubeOutput(CubeOutput* output);
/** 1故障のテキストを独立したファイルへ保存する。size==0なら何も作らない。 */
bool WriteCubeFile(const CubeOutput* output, const FNODE* fault, const char* text, size_t size);

/** 管理ファイルにPI順序と実験条件を記録する。回路展開・InitGlobalVars後に呼ぶ。 */
bool WriteCubeMetadata(FILE* fp);

/** 完了状態・FDP・キューブ数の保存条件を満たす計算対象故障を書く。等価故障への展開はしない。
 * fp==NULLなら保存せず成功する。cubesの所有権は移さない。 */
bool WriteFaultCubes(FILE* fp, const FaultResult* result, const CubeSet* cubes);

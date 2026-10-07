#pragma once

#include <stdbool.h>
#include <cudd.h>
#include <gmp.h>
#include "./cube_set.h"

/** キューブ和集合の検出確率を計算する。density は初期化済みで渡す。
 *  故障情報や CSV 出力には依存しない。数え上げに失敗した場合は false。 */
bool RunBDD(DdManager* gbm, int nvars, const CubeSet* cubes, mpf_t density);

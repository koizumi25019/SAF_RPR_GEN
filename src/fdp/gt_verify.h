#pragma once
//-------------------------------------------------------------------------------------------------------------
//	gt_verify.h : 独立グラウンドトゥルース検証（env GT_BDD=1）
//	ネットリストから検出関数 D_f = OR_PO(good XOR faulty) を BDD で直接構築し、
//	パイプラインが生成したキューブ和集合と厳密比較する回帰検証ツール。
//	使い方・記録は verification/gt_bdd/SUMMARY.md を参照。
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include "./read.h"
#include "./cube_set.h"

/* 1故障ぶんの厳密検証。RunBDD 直後（キューブ集合が生きている間）に呼ぶ。
   env GT_BDD 未設定なら何もしない。 */
void GT_Check(FNODE* f, CubeSet* cubes, bool limit_hit);

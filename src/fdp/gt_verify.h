#pragma once
//-------------------------------------------------------------------------------------------------------------
//	gt_verify.h : 独立グラウンドトゥルース検証（env GT_BDD=1 / GT_ISOP=1）
//	ネットリストから検出関数 D_f = OR_PO(good XOR faulty) を BDD で直接構築し、
//	パイプラインが生成したキューブ和集合と厳密比較する回帰検証ツール。
//	GT_ISOP は D_f の Minato-Morreale ISOP と現行キューブ数を比較する診断オラクル。
//	使い方・記録は verification/gt_bdd/SUMMARY.md を参照。
//-------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include "./read.h"
#include "./cube_set.h"

/* 1故障ぶんの厳密検証。RunBDD 直後（キューブ集合が生きている間）に呼ぶ。
   env GT_BDD / GT_ISOP の両方が未設定なら何もしない。 */
void GT_Check(FNODE* f, CubeSet* cubes, bool limit_hit);

/* BDD 直接法（env BDD_EXACT=1 のフォールバックから使用）:
   検出関数 D_f を回路から直接構築し、そのミンターム数を10進文字列で返す
   （malloc 済み、呼び出し側で free。失敗時 NULL）。fdp = 返値/2^n_pi が厳密値。 */
char* GT_ExactCountStr(FNODE* f);

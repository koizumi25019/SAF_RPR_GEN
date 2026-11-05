#pragma once
/*
 * calc F: FDP CSVから、ランダムパターンを印加したときの故障検出率Fと故障検出効率Eを求める。
 *
 *   F = (1/Nf) * Σ P_detect(fi, Np)      E = (1/Nd) * Σ P_detect(fi, Np)
 *
 *   復元抽出（既定）   : P_detect = 1 - (1 - D)^Np
 *   非復元抽出（任意） : P_detect = 1 - Π_{k=0..Np-1} (N - Fd - k) / (N - k)
 *                        N = 2^n_pi（全パターン数）, Fd = round(D * N)
 *
 * -calc_f を指定したときだけ、FDP計算でCSVを書き終えた後に1回実行する。
 * 入力は書き出し済みのFDP CSVだけで、FDP計算の内部状態には依存しない。
 */
#include <stdbool.h>

#define COVERAGE_MAX_PATTERN_COUNTS 256  /**< -calc_f に指定できるNpの最大個数 */

/** calc Fの設定。-calc_f* オプションから作る。 */
typedef struct {
    bool enabled;                  /**< -calc_f の指定有無 */
    unsigned long pattern_counts[COVERAGE_MAX_PATTERN_COUNTS]; /**< Np（昇順・重複なし） */
    int n_pattern_counts;
    bool without_replacement;      /**< -calc_f_nr on: 非復元抽出のFも求める */
    char* output;                  /**< 結果CSV。未指定なら -fdp から導出する */
} CoverageConfig;

/** 設定を初期値（無効）にする。 */
void InitCoverageConfig(CoverageConfig* config);

/** name が calc F のオプション（-calc_f, -calc_f_nr, -calc_f_out）か。 */
bool IsCoverageOption(const char* name);

/** CLI・.setで共通のオプション設定。不正値はエラーを表示して false。 */
bool SetCoverageOption(CoverageConfig* config, const char* name, const char* value);

/** 全オプションの読込後に呼ぶ。組合せを確認し、出力先の既定値を決める。 */
bool FinalizeCoverageConfig(CoverageConfig* config, const char* fdp_path,
                            const char* log_path, const char* cube_path);

/** FDP CSVからF・Eを求め、結果CSVと標準出力へ書く。 */
bool RunCoverage(const CoverageConfig* config, const char* fdp_path,
                 const char* circuit_name, int n_inputs);

#include "cadical.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../../../netlist/netlist.h"
#include "../opb.h"

SOLVER_LOG solver;

#define CMD_PATH  "/home/20koizumi/src/cadical/build/cadical" 
#define CNF_FILE  "cnf.txt"
#define LOG_FILE  "cadical_result.txt"

// メモリ確保 (sizeにはSATの最大変数番号 = opb.total.vars を渡す)
void InitSolverMemory(int size) {
    solver.sol = (char**)malloc(sizeof(char*) * 2);
    // 変数番号をそのままインデックスとして使うため (size + 1) 個確保する
    solver.sol[1] = (char*)malloc(sizeof(char) * (size + 2)); 
    
    if (solver.sol[1]) {
        // 未割り当ての変数は 'X' (Don't care) などにしておくと安全
        memset(solver.sol[1], 'X', size + 1);
        solver.sol[1][size + 1] = '\0'; // 文字列の終端
    }
}

// 結果解析
void ParseSolution(FILE* fp, int max_var) {
    char buffer[4096];
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strncmp(buffer, "s SATISFIABLE", 13) == 0) {
            solver.status = CADICAL_SAT;
        }
        else if (strncmp(buffer, "s UNSATISFIABLE", 15) == 0) {
            solver.status = CADICAL_UNSAT;
        }
        else if (buffer[0] == 'v') {
            // "v 1 -2 3..." の行を解析
            char* token = strtok(buffer + 1, " \t\n");
            while (token != NULL) {
                int val = atoi(token);
                if (val == 0) break; // 0は行の終わり

                int var_idx = abs(val); // 変数番号 (絶対値)

                // 範囲チェックをして配列の該当インデックスに直接書き込む
                if (var_idx > 0 && var_idx <= max_var) {
                    if (val > 0) {
                        solver.sol[1][var_idx] = '1';
                    } else {
                        solver.sol[1][var_idx] = '0';
                    }
                }

                token = strtok(NULL, " \t\n");
            }
        }
    }
}

// ★追加：外部入力のみを抽出して取得する関数
void GetPrimaryInputPattern(char* pi_pattern) {
    int idx = 0;

    // ネットリストを走査して外部入力(IN)を探す
    for (int i = 0; i < n_net; i++) {
        if (nl[i].type == IN) { // ゲートタイプが外部入力の場合
            int var_id = nl[i].varsgc; // 外部入力の変数番号を取得
            
            // SATソルバーの解から、該当する変数番号の値をコピー
            pi_pattern[idx] = solver.sol[1][var_id];
            idx++;
        }
    }
    pi_pattern[idx] = '\0'; // 終端文字
    printf("Primary Input Pattern: %s\n", pi_pattern);

    /** for xid */
	FILE* fileptr = fopen("./tp.txt", "w");
    if (fileptr == NULL) {
        fprintf(stderr, "【ERROR】: Cannot open ./tp.txt for writing.\n");
        exit(EXIT_FAILURE);
    }
	fprintf(fileptr, "%s\n", pi_pattern);
	fclose(fileptr);

}

// 実行関数
bool RunCaDiCaL(void) {
    char command[1024];

    solver.status = CADICAL_UNKNOWN;

    // ★ opb.total.vars はこれまでの処理でカウントされた最大変数数
    int max_var = opb.total.vars; 
    
    InitSolverMemory(max_var);

    // コマンド作成
    sprintf(command, "%s %s > %s", CMD_PATH, CNF_FILE, LOG_FILE);
    
    printf("[Exec] %s\n", command);
    int ret = system(command);

    FILE* fp = fopen(LOG_FILE, "r");
    if (fp == NULL) {
        printf("Error: Log file not found. (%s)\n", LOG_FILE);
        exit(EXIT_FAILURE);
    }
    
    ParseSolution(fp, max_var);
    fclose(fp);

    if (solver.status == CADICAL_SAT) {
        // SATなら外部入力の値を抽出する
        char pi_result[1024]; // 外部入力の数に合わせて十分なサイズを用意
        GetPrimaryInputPattern(pi_result);
        return true;
    }

    return false;
}

void FreeSolverMemory(void) {
    if(solver.sol) {
        if(solver.sol[1]) free(solver.sol[1]);
        free(solver.sol);
    }
}
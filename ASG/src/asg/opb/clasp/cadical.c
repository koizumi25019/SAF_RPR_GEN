#include "cadical.h"
#include <string.h>
#include <unistd.h>

SOLVER_LOG solver;

// ★重要: ここをあなたのCaDiCaLのパス(ビルドした場所)に書き換えてください！
// 先ほどの画像を見る限り、おそらく以下のパスです
#define CMD_PATH  "/home/20koizumi/src/cadical/build/cadical" 

#define CNF_FILE  "test.cnf"
#define LOG_FILE  "cadical_result.txt"

// メモリ確保
void InitSolverMemory(int size) {
    solver.sol = (char**)malloc(sizeof(char*) * 2);
    // 十分なサイズを確保(テスト用なので適当に大きめ)
    solver.sol[1] = (char*)malloc(sizeof(char) * (size + 100));
    if (solver.sol[1]) memset(solver.sol[1], 0, size + 100);
}

// 結果解析
void ParseSolution(FILE* fp) {
    char buffer[4096];
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strncmp(buffer, "s SATISFIABLE", 13) == 0) {
            solver.status = CADICAL_SAT;
        }
        else if (strncmp(buffer, "s UNSATISFIABLE", 15) == 0) {
            solver.status = CADICAL_UNSAT;
        }
        else if (buffer[0] == 'v') {
            // "v 1 -2 3..." の行を簡易的に解析
            char* token = strtok(buffer + 1, " \t\n");
            while (token != NULL) {
                int val = atoi(token);
                if (val == 0) break; 

                // 正なら'1', 負なら'0' を連結 (簡易実装)
                if (val > 0) strcat(solver.sol[1], "1");
                else strcat(solver.sol[1], "0");

                token = strtok(NULL, " \t\n");
            }
        }
    }
        printf("Assignment: %s\n", solver.sol[1]);
}

// 実行関数
bool RunCaDiCaL(void) {
    char command[1024];

    solver.status = CADICAL_UNKNOWN;
    if (solver.sol && solver.sol[1]) solver.sol[1][0] = '\0';

    // コマンド作成
    sprintf(command, "%s %s > %s", CMD_PATH, CNF_FILE, LOG_FILE);
    
    printf("[Exec] %s\n", command);
    int ret = system(command);

    FILE* fp = fopen(LOG_FILE, "r");
    if (fp == NULL) {
        printf("Error: Log file not found. (%s)\n", LOG_FILE);
        return false;
    }
    InitSolverMemory(100);
    ParseSolution(fp);
    fclose(fp);

    return (solver.status == CADICAL_SAT);
}

void FreeSolverMemory(void) {
    if(solver.sol) {
        if(solver.sol[1]) free(solver.sol[1]);
        free(solver.sol);
    }
}
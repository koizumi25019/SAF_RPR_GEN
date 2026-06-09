//-------------------------------------------------------------------------------------------------------------
//  essential_assignment.c
//  必須割り当てによる単位節の追加
//  参考: assignment_extraction.c (T.Aono, 2025/03/23)
//  CNF文字列生成 → ccadical_add() の単位節に変換
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "ccadical.h"
#include "./essential_assignment.h"
#include "./init.h"
#include "../lib/lib.h"
#include "../netlist/netlist.h"
#include "./read.h"

//-------------------------------------------------------------------------------------------------------------
// que2: 一意活性化専用キュー（ローカル管理）
//-------------------------------------------------------------------------------------------------------------
static QUE que2;

static void que2_enq(NLIST* netptr)
{
    que2.que[que2.rear++] = netptr;
    que2.num++;
    if (que2.rear >= que2.maxnum) que2.rear = 0;
    if (que2.num >= que2.maxnum) {
        printf("SYSTEM ERROR: que2 overflow in EssentialAssignment\n");
        exit(EXIT_FAILURE);
    }
}

static NLIST* que2_deq(void)
{
    NLIST* netptr = que2.que[que2.front++];
    que2.num--;
    if (que2.front >= que2.maxnum) que2.front = 0;
    return netptr;
}

// que2 内の [left, right] 範囲をレベル昇順にクイックソート
// ラップアラウンドが発生していない前提（発生時は呼び出し元でスキップ）
static void que2_qsort(int left, int right)
{
    if (left >= right) return;
    int l = left, r = right;
    int pivot = que2.que[(left + right) / 2]->level;

    while (l <= r) {
        while (que2.que[l]->level < pivot) l++;
        while (que2.que[r]->level > pivot) r--;
        if (l <= r) {
            NLIST* tmp  = que2.que[l];
            que2.que[l] = que2.que[r];
            que2.que[r] = tmp;
            l++; r--;
        }
    }
    que2_qsort(left, r);
    que2_qsort(l, right);
}

//-------------------------------------------------------------------------------------------------------------
//  assign_ea_value
//  値を確定して単位節をソルバに追加し、含意キュー（que）に積む
//-------------------------------------------------------------------------------------------------------------
static void assign_ea_value(CCaDiCaL* solver, NLIST* netptr, int val)
{
    if (netptr->ea_flag != EA_DOWN) return;

    netptr->ea_flag     = EA_UP;
    netptr->logic_value = val;

    if (val == 1) {
        ccadical_add(solver,  (int)netptr->varsgc);
    } else {
        ccadical_add(solver, -(int)netptr->varsgc);
    }
    ccadical_add(solver, 0);

    if (netptr->type != DFF) {
        queENQ(netptr, QUE_MODE_ONE);
    }
}

//-------------------------------------------------------------------------------------------------------------
//  assign_unique_value
//  唯一感化パス上のゲートの側面入力に非制御値を割り当てる
//  AND/NAND: 他入力に 1、OR/NOR: 他入力に 0
//  参考コードの Flag チェックを ea_flag / unique_flag に分離して移植
//-------------------------------------------------------------------------------------------------------------
static void assign_unique_value(CCaDiCaL* solver, NLIST* t_net, int unique_idx)
{
    if (t_net->n_out == 0) return;

    int non_ctrl_val;
    switch (t_net->type) {
    case AND:
    case NAND:  non_ctrl_val = 1; break;
    case OR:
    case NOR:   non_ctrl_val = 0; break;
    default:    return;
    }

    for (int i = 0; i < t_net->n_in; i++) {
        if (i == unique_idx) continue;
        if (t_net->in[i]->unique_flag != UNIQUE_DOWN) continue;
        if (t_net->in[i]->ea_flag     != EA_DOWN)     continue;
        assign_ea_value(solver, t_net->in[i], non_ctrl_val);
    }
}

//-------------------------------------------------------------------------------------------------------------
//  unique_sentization
//  故障サイトから PO 方向へ BFS で TFO を探索し、
//  キューに 1 要素しか残らない（唯一のパス）ときに側面入力を割り当てる
//  参考コードの que2 操作と uni_Qsort の再帰バグを修正して移植
//-------------------------------------------------------------------------------------------------------------
static void unique_sentization(CCaDiCaL* solver)
{
    NLIST* tmp_net = que2_deq();
    tmp_net->unique_flag = UNIQUE_MIDDLE;

    for (int i = 0; i < tmp_net->n_out; i++) {
        if (tmp_net->out[i]->type == DFF) continue;
        if (tmp_net->out[i]->unique_flag != UNIQUE_DOWN) continue;
        que2_enq(tmp_net->out[i]);
        tmp_net->out[i]->unique_flag = UNIQUE_UP;
    }
    if (que2.num > 1 && que2.front < que2.rear)
        que2_qsort(que2.front, que2.rear - 1);

    while (que2.num > 0) {
        tmp_net = que2_deq();
        tmp_net->unique_flag = UNIQUE_MIDDLE;

        if (que2.num == 0) {
            for (int idx = 0; idx < tmp_net->n_in; idx++) {
                if (tmp_net->in[idx]->unique_flag == UNIQUE_MIDDLE) {
                    assign_unique_value(solver, tmp_net, idx);
                    break;
                }
            }
        }

        for (int i = 0; i < tmp_net->n_out; i++) {
            if (tmp_net->out[i]->type == DFF) continue;
            if (tmp_net->out[i]->unique_flag != UNIQUE_DOWN) continue;
            que2_enq(tmp_net->out[i]);
            tmp_net->out[i]->unique_flag = UNIQUE_UP;
        }
        if (que2.num > 1 && que2.front < que2.rear)
            que2_qsort(que2.front, que2.rear - 1);
    }
}

//-------------------------------------------------------------------------------------------------------------
//  back_implication
//  出力の確定値から入力を推定する（後方含意）
//  参考コードの INV/BUF/FOUT における in[i] → in[0] のバグを修正して移植
//-------------------------------------------------------------------------------------------------------------
static void back_implication(CCaDiCaL* solver, NLIST* t_net)
{
    int val = t_net->logic_value;
    switch (t_net->type) {
    case AND:
        if (val == 1)
            for (int i = 0; i < t_net->n_in; i++)
                assign_ea_value(solver, t_net->in[i], 1);
        break;
    case OR:
        if (val == 0)
            for (int i = 0; i < t_net->n_in; i++)
                assign_ea_value(solver, t_net->in[i], 0);
        break;
    case NAND:
        if (val == 0)
            for (int i = 0; i < t_net->n_in; i++)
                assign_ea_value(solver, t_net->in[i], 1);
        break;
    case NOR:
        if (val == 1)
            for (int i = 0; i < t_net->n_in; i++)
                assign_ea_value(solver, t_net->in[i], 0);
        break;
    case INV:
        assign_ea_value(solver, t_net->in[0], val == 1 ? 0 : 1);
        break;
    case BUF:
    case FOUT:
        assign_ea_value(solver, t_net->in[0], val);
        break;
    default:
        break;
    }
}

//-------------------------------------------------------------------------------------------------------------
//  forward_implication
//  入力の確定値から出力を推定する（前方含意）
//-------------------------------------------------------------------------------------------------------------
static void forward_implication(CCaDiCaL* solver, NLIST* t_net)
{
    for (int i = 0; i < t_net->n_out; i++) {
        NLIST* out = t_net->out[i];
        if (out->ea_flag != EA_DOWN) continue;

        int count = 0;
        switch (out->type) {
        case AND:
            for (int j = 0; j < out->n_in; j++) {
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 0) {
                    assign_ea_value(solver, out, 0);
                    goto next_out;
                }
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 1)
                    count++;
            }
            if (count == out->n_in) assign_ea_value(solver, out, 1);
            break;

        case NAND:
            for (int j = 0; j < out->n_in; j++) {
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 0) {
                    assign_ea_value(solver, out, 1);
                    goto next_out;
                }
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 1)
                    count++;
            }
            if (count == out->n_in) assign_ea_value(solver, out, 0);
            break;

        case OR:
            for (int j = 0; j < out->n_in; j++) {
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 1) {
                    assign_ea_value(solver, out, 1);
                    goto next_out;
                }
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 0)
                    count++;
            }
            if (count == out->n_in) assign_ea_value(solver, out, 0);
            break;

        case NOR:
            for (int j = 0; j < out->n_in; j++) {
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 1) {
                    assign_ea_value(solver, out, 0);
                    goto next_out;
                }
                if (out->in[j]->ea_flag != EA_DOWN && out->in[j]->logic_value == 0)
                    count++;
            }
            if (count == out->n_in) assign_ea_value(solver, out, 1);
            break;

        case INV:
            if (out->in[0]->ea_flag != EA_DOWN)
                assign_ea_value(solver, out, out->in[0]->logic_value == 0 ? 1 : 0);
            break;

        case BUF:
        case FOUT:
            if (out->in[0]->ea_flag != EA_DOWN)
                assign_ea_value(solver, out, out->in[0]->logic_value);
            break;

        default:
            break;
        }
        next_out:;
    }
}

//-------------------------------------------------------------------------------------------------------------
//  EssentialAssignment
//  必須割り当てのエントリポイント
//  1. 唯一感化で側面入力に非制御値を割り当て
//  2. 故障サイトの正常回路値を設定（SA0: gc=1, SA1: gc=0）
//  3. 後方・前方含意ループで伝播
//-------------------------------------------------------------------------------------------------------------
void EssentialAssignment(CCaDiCaL* solver, FNODE* target)
{
    for (int i = 0; i < n_net; i++) {
        nl[i].logic_value = EA_UNKNOWN;
        nl[i].ea_flag     = EA_DOWN;
        nl[i].unique_flag = UNIQUE_DOWN;
    }

    que.front = 0;
    que.num   = 0;
    que.rear  = 0;

    que2.maxnum = n_net + 10;
    que2.front  = 0;
    que2.num    = 0;
    que2.rear   = 0;
    que2.que    = (NLIST**)allocMemory(que2.maxnum, sizeof(NLIST*));

    que2_enq(target->netptr);
    target->netptr->unique_flag = UNIQUE_UP;
    unique_sentization(solver);

    int fault_val = (target->type == SF0) ? 1 : 0;
    assign_ea_value(solver, target->netptr, fault_val);

    while (que.num > 0) {
        NLIST* t_net = queDEQ();
        back_implication(solver, t_net);
        forward_implication(solver, t_net);
    }

    free(que2.que);
    que2.que = NULL;
}

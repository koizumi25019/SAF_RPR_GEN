#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <gmp.h>
#include"../netlist/netlist.h"
#include "./target_fault.h"
#include "./read.h"
#include "../opt/opt.h"


// 等価故障を再帰的に出力する関数
void OutputEquivFaults(
    FILE*  result_fp,
    NLIST* net,
    int    fault_type,
    mpf_t  density
)
{
    int j;

    // TDF の等価は BUF/INV のみ（BUF=同極性、INV=STR↔STF 反転）。
    // DFF 置換 BUF（入力が1時刻目コピー）は時刻境界なので跨がない。
    if (opt.fault_model == FM_TDF)
    {
        if (net->type != BUF && net->type != INV) return;

        NLIST* in0 = net->in[0];
        if (in0->peer_1t == (NLIST*)NULL) return;

        int in_type = (net->type == BUF) ? fault_type
                                         : ((fault_type == SF0) ? SF1 : SF0);
        int flag = (in_type == SF0) ? in0->test_sa0 : in0->test_sa1;
        if (flag == NO)
        {
            gmp_fprintf(result_fp, "%s,%s,,,%.10Fe,\n",
                in0->name, FaultTypeName(in_type), density);
            OutputEquivFaults(result_fp, in0, in_type, density);
        }
        return;
    }

    switch (net->type)
    {
        case AND:
        case NAND:
            if (fault_type == SF0)
            {
                for (j = 0; j < net->n_in; j++)
                {
                    if (net->in[j]->test_sa0 == NO)  // 等価故障としてマークされているもののみ
                    {
                        gmp_fprintf(result_fp, "%s,sa0,,,%.10Fe,\n",
                            net->in[j]->name, density);
                        OutputEquivFaults(result_fp, net->in[j], SF0, density);
                    }
                }
            }
            break;

        case OR:
        case NOR:
            if (fault_type == SF1)
            {
                for (j = 0; j < net->n_in; j++)
                {
                    if (net->in[j]->test_sa1 == NO)  // 追加
                    {
                        gmp_fprintf(result_fp, "%s,sa1,,,%.10Fe,\n",
                            net->in[j]->name, density);
                        OutputEquivFaults(result_fp, net->in[j], SF1, density);
                    }
                }
            }
            break;

        case BUF:
            if (fault_type == SF0 && net->in[0]->test_sa0 == NO) {
                gmp_fprintf(result_fp, "%s,sa0,,,%.10Fe,\n",
                    net->in[0]->name, density);
                OutputEquivFaults(result_fp, net->in[0], SF0, density);
            } else if (fault_type == SF1 && net->in[0]->test_sa1 == NO) {
                gmp_fprintf(result_fp, "%s,sa1,,,%.10Fe,\n",
                    net->in[0]->name, density);
                OutputEquivFaults(result_fp, net->in[0], SF1, density);
            }
            break;

        case INV:
            if (fault_type == SF0 && net->in[0]->test_sa1 == NO) {
                gmp_fprintf(result_fp, "%s,sa1,,,%.10Fe,\n",
                    net->in[0]->name, density);
                OutputEquivFaults(result_fp, net->in[0], SF1, density);
            } else if (fault_type == SF1 && net->in[0]->test_sa0 == NO) {
                gmp_fprintf(result_fp, "%s,sa0,,,%.10Fe,\n",
                    net->in[0]->name, density);
                OutputEquivFaults(result_fp, net->in[0], SF0, density);
            }
            break;

        default:
            break;
    }
}

void calculate_prob_with_gmp(
    const char* numStr,
     int nvars,
     FILE* result_fp,
     FILE* cube_analysis_fp,
     TARGET* target,
     int cube_cnt,
     int seeded_cnt,
     bool limit_hit
    )
    {
    mpf_t num, den, density;
    mpf_set_default_prec(8192);

    mpf_init(num);
    mpf_init(den);
    mpf_init(density);

    if (mpf_set_str(num, numStr, 10) != 0) {
        mpf_set_ui(num, 0);
    }

    mpf_set_ui(den, 1);
    mpf_mul_2exp(den, den, (unsigned long)nvars); // den = 1 * 2^nvars
    mpf_div(density, num, den);

    if (cube_analysis_fp != NULL) {
        gmp_fprintf(cube_analysis_fp, ",%.10Fe", density);
    }

    // 代表故障を出力
    if (result_fp != NULL) {
        gmp_fprintf(result_fp, "%s,%s,%d,%d,%.10Fe,%d\n",
            target->list[0]->name,
            FaultTypeName(target->list[0]->type),
            cube_cnt,
            limit_hit ? 0 : 1,
            density,
            seeded_cnt);
    }


    //等価故障の検出確率は代表故障と同じ
    if (result_fp != NULL) {
        OutputEquivFaults(result_fp,
            target->list[0]->netptr,
            target->list[0]->type,
            density);
    }
    mpf_clear(num);
    mpf_clear(den);
    mpf_clear(density);

    return;
}
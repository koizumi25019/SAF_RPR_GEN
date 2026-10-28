#include "io/fault_csv.h"
#include "cli/options.h"

// 等価故障を再帰的に出力する関数
static void WriteEquivFaults(
    FILE*  result_fp,
    NLIST* net,
    int    fault_type,
    const char* density_str
)
{
    int j;

    // TDF の等価は BUF/INV のみ（BUF=同極性、INV=STR↔STF 反転）。
    // DFF 置換 BUF（入力が1時刻目コピー）は時刻境界なので跨がない。
    if (opt.fault_model == FM_TDF)
    {
        if (net->type != BUF && net->type != INV) return;
        if (net->peer_1t && net->peer_1t->type == IN) return;

        NLIST* in0 = net->in[0];
        if (in0->peer_1t == (NLIST*)NULL) return;

        int in_type = (net->type == BUF) ? fault_type
                                         : ((fault_type == SF0) ? SF1 : SF0);
        int flag = (in_type == SF0) ? in0->test_sa0 : in0->test_sa1;
        if (flag == NO)
        {
            fprintf(result_fp, "%s,%s,,,%s\n",
                in0->name, FaultTypeName(in_type), density_str);
            WriteEquivFaults(result_fp, in0, in_type, density_str);
        }
        return;
    }

    switch (net->type)
    {
        case AND:
        case NAND:
            // NAND は出力が反転するため、入力 sa0 と等価なのは出力 sa1。
            if (fault_type == (net->type == AND ? SF0 : SF1))
            {
                for (j = 0; j < net->n_in; j++)
                {
                    if (net->in[j]->test_sa0 == NO)  // 等価故障としてマークされているもののみ
                    {
                        fprintf(result_fp, "%s,sa0,,,%s\n",
                            net->in[j]->name, density_str);
                        WriteEquivFaults(result_fp, net->in[j], SF0, density_str);
                    }
                }
            }
            break;

        case OR:
        case NOR:
            // NOR の入力 sa1 は、出力 sa0 と等価。
            if (fault_type == (net->type == OR ? SF1 : SF0))
            {
                for (j = 0; j < net->n_in; j++)
                {
                    if (net->in[j]->test_sa1 == NO)  // 等価故障としてマークされているもののみ
                    {
                        fprintf(result_fp, "%s,sa1,,,%s\n",
                            net->in[j]->name, density_str);
                        WriteEquivFaults(result_fp, net->in[j], SF1, density_str);
                    }
                }
            }
            break;

        case BUF:
            if (fault_type == SF0 && net->in[0]->test_sa0 == NO) {
                fprintf(result_fp, "%s,sa0,,,%s\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF0, density_str);
            } else if (fault_type == SF1 && net->in[0]->test_sa1 == NO) {
                fprintf(result_fp, "%s,sa1,,,%s\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF1, density_str);
            }
            break;

        case INV:
            if (fault_type == SF0 && net->in[0]->test_sa1 == NO) {
                fprintf(result_fp, "%s,sa1,,,%s\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF1, density_str);
            } else if (fault_type == SF1 && net->in[0]->test_sa0 == NO) {
                fprintf(result_fp, "%s,sa0,,,%s\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF0, density_str);
            }
            break;

        default:
            break;
    }
}


void WriteFaultResult(FILE* fp, const FaultResult* result)
{
    // 同じ確率を代表故障と全等価故障に使うため、文字列への変換は1回だけ行う。
    // %.10Fe の仮数と指数は、GMP の指数型の最大値でも64文字以内に収まる。
    char density_str[64];
    gmp_snprintf(density_str, sizeof(density_str), "%.10Fe", result->density);

    fprintf(fp, "%s,%s,%d,%d,%s\n",
            result->target->name,
            FaultTypeName(result->target->type),
            result->cube_cnt,
            result->complete ? 1 : 0,
            density_str);
    WriteEquivFaults(fp, result->target->netptr, result->target->type, density_str);
}

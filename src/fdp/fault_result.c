#include "./fault_result.h"
#include "../opt/opt.h"

// 等価故障を再帰的に出力する関数
static void WriteEquivFaults(
    FILE*  result_fp,
    NLIST* net,
    int    fault_type,
    const char* density_str
)
{
    int j;

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
                        fprintf(result_fp, "%s,sa0,,,%s,\n",
                            net->in[j]->name, density_str);
                        WriteEquivFaults(result_fp, net->in[j], SF0, density_str);
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
                    if (net->in[j]->test_sa1 == NO)  // 等価故障としてマークされているもののみ
                    {
                        fprintf(result_fp, "%s,sa1,,,%s,\n",
                            net->in[j]->name, density_str);
                        WriteEquivFaults(result_fp, net->in[j], SF1, density_str);
                    }
                }
            }
            break;

        case BUF:
            if (fault_type == SF0 && net->in[0]->test_sa0 == NO) {
                fprintf(result_fp, "%s,sa0,,,%s,\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF0, density_str);
            } else if (fault_type == SF1 && net->in[0]->test_sa1 == NO) {
                fprintf(result_fp, "%s,sa1,,,%s,\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF1, density_str);
            }
            break;

        case INV:
            if (fault_type == SF0 && net->in[0]->test_sa1 == NO) {
                fprintf(result_fp, "%s,sa1,,,%s,\n",
                    net->in[0]->name, density_str);
                WriteEquivFaults(result_fp, net->in[0], SF1, density_str);
            } else if (fault_type == SF1 && net->in[0]->test_sa0 == NO) {
                fprintf(result_fp, "%s,sa0,,,%s,\n",
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

    fprintf(fp, "%s,%s,%d,%d,%s,%d\n",
            result->target->name,
            (result->target->type == SF0) ? "sa0" : "sa1",
            result->cube_cnt,
            result->complete ? 1 : 0,
            density_str,
            result->seeded_cnt);
    WriteEquivFaults(fp, result->target->netptr, result->target->type, density_str);
}

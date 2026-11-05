#include "io/fault_csv.h"
#include "fault/equivalence.h"

void WriteFaultResult(FILE* fp, const FaultResult* result)
{
    const FNODE* representative = result->target;
    const char* representative_type = FaultTypeName(representative->type);
    /* 同じ数値を全メンバーへ展開するため、GMPからの文字列変換は1回だけ行う。 */
    char density_str[64];
    gmp_snprintf(density_str, sizeof(density_str), "%.10Fe", result->density);

    fprintf(fp, "%s,%s,%d,%d,%s,%s,%s\n",
            representative->name, representative_type, result->cube_cnt,
            result->complete ? 1 : 0, density_str,
            representative->name, representative_type);

    for (const EquivalentFault* member = GetEquivalentFaults(representative);
         member; member = member->next) {
        if (member->net == representative->netptr && member->type == representative->type) continue;
        /* cube_cntとcompleteは実際に計算した代表行だけに記録する。
         * 未完了時は代表の下界を共有し、rep列からその完了状態を確認できる。 */
        fprintf(fp, "%s,%s,,,%s,%s,%s\n", member->net->name, FaultTypeName(member->type),
                density_str, representative->name, representative_type);
    }
}

//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdlib.h>

#include "ccadical.h"
#include "./cnf.h"
#include "./faulty_miter.h"
#include "../../netlist/netlist.h"

int* mt_act  = (int*)NULL;
int* mt_sval = (int*)NULL;
static int* mt_vf  = (int*)NULL;   // faulty value var per net
static int* mt_drv = (int*)NULL;   // faulty gate-output (pre-mux) var per net (0 for inputs)

static int idx_of(NLIST* p) { return (int)(p - nl); }

//*************************************************************************************************************
//	@name		AllocMiterVars
//	@function	全ネットの faulty/act/sval/drv に SAT 変数番号を割り当てる（varsgc の後ろから）
//*************************************************************************************************************
void AllocMiterVars(void)
{
	if (mt_act == (int*)NULL)
	{
		mt_act  = (int*)malloc((size_t)n_net * sizeof(int));
		mt_sval = (int*)malloc((size_t)n_net * sizeof(int));
		mt_vf   = (int*)malloc((size_t)n_net * sizeof(int));
		mt_drv  = (int*)malloc((size_t)n_net * sizeof(int));
	}

	opb.total.vars = opb.constant.vars;   // 正常回路変数の直後から採番
	for (int i = 0; i < n_net; i++)
	{
		mt_act[i]  = ++opb.total.vars;
		mt_sval[i] = ++opb.total.vars;
		mt_vf[i]   = ++opb.total.vars;
		mt_drv[i]  = (nl[i].type == IN || nl[i].type == DFF) ? 0 : ++opb.total.vars;
	}
}

//*************************************************************************************************************
//	@name		emit_gate
//	@function	z = gate(faulty値の入力) の節を投入（CreateConsFC_* と同じ論理）
//*************************************************************************************************************
static void emit_gate(CCaDiCaL* s, NLIST* n, int z)
{
	int ni = n->n_in;
	switch (n->type)
	{
	case AND:
		for (int i = 0; i < ni; i++) ccadical_add(s, -mt_vf[idx_of(n->in[i])]);
		ccadical_add(s, z); ccadical_add(s, 0);
		for (int i = 0; i < ni; i++) { ccadical_add(s, mt_vf[idx_of(n->in[i])]); ccadical_add(s, -z); ccadical_add(s, 0); }
		break;
	case NAND:
		for (int i = 0; i < ni; i++) ccadical_add(s, -mt_vf[idx_of(n->in[i])]);
		ccadical_add(s, -z); ccadical_add(s, 0);
		for (int i = 0; i < ni; i++) { ccadical_add(s, mt_vf[idx_of(n->in[i])]); ccadical_add(s, z); ccadical_add(s, 0); }
		break;
	case OR:
		for (int i = 0; i < ni; i++) ccadical_add(s, mt_vf[idx_of(n->in[i])]);
		ccadical_add(s, -z); ccadical_add(s, 0);
		for (int i = 0; i < ni; i++) { ccadical_add(s, -mt_vf[idx_of(n->in[i])]); ccadical_add(s, z); ccadical_add(s, 0); }
		break;
	case NOR:
		for (int i = 0; i < ni; i++) ccadical_add(s, mt_vf[idx_of(n->in[i])]);
		ccadical_add(s, z); ccadical_add(s, 0);
		for (int i = 0; i < ni; i++) { ccadical_add(s, -mt_vf[idx_of(n->in[i])]); ccadical_add(s, -z); ccadical_add(s, 0); }
		break;
	case BUF:
	case FOUT: {
		int a = mt_vf[idx_of(n->in[0])];
		ccadical_add(s, -a); ccadical_add(s,  z); ccadical_add(s, 0);
		ccadical_add(s,  a); ccadical_add(s, -z); ccadical_add(s, 0);
		break; }
	case INV: {
		int a = mt_vf[idx_of(n->in[0])];
		ccadical_add(s,  a); ccadical_add(s,  z); ccadical_add(s, 0);
		ccadical_add(s, -a); ccadical_add(s, -z); ccadical_add(s, 0);
		break; }
	case EXOR: {
		int a = mt_vf[idx_of(n->in[0])], b = mt_vf[idx_of(n->in[1])];
		ccadical_add(s, -a); ccadical_add(s, -b); ccadical_add(s, -z); ccadical_add(s, 0);
		ccadical_add(s,  a); ccadical_add(s,  b); ccadical_add(s, -z); ccadical_add(s, 0);
		ccadical_add(s,  a); ccadical_add(s, -b); ccadical_add(s,  z); ccadical_add(s, 0);
		ccadical_add(s, -a); ccadical_add(s,  b); ccadical_add(s,  z); ccadical_add(s, 0);
		break; }
	case EXNOR: {
		int a = mt_vf[idx_of(n->in[0])], b = mt_vf[idx_of(n->in[1])];
		ccadical_add(s,  a); ccadical_add(s,  b); ccadical_add(s,  z); ccadical_add(s, 0);
		ccadical_add(s, -a); ccadical_add(s, -b); ccadical_add(s,  z); ccadical_add(s, 0);
		ccadical_add(s, -a); ccadical_add(s,  b); ccadical_add(s, -z); ccadical_add(s, 0);
		ccadical_add(s,  a); ccadical_add(s, -b); ccadical_add(s, -z); ccadical_add(s, 0);
		break; }
	default:
		break;
	}
}

//*************************************************************************************************************
//	@name		emit_mux
//	@function	vf = act ? sval : drv （故障注入スイッチ）の節を投入
//*************************************************************************************************************
static void emit_mux(CCaDiCaL* s, int vf, int act, int sval, int drv)
{
	ccadical_add(s, -act); ccadical_add(s, -sval); ccadical_add(s,  vf); ccadical_add(s, 0);
	ccadical_add(s, -act); ccadical_add(s,  sval); ccadical_add(s, -vf); ccadical_add(s, 0);
	ccadical_add(s,  act); ccadical_add(s, -drv);  ccadical_add(s,  vf); ccadical_add(s, 0);
	ccadical_add(s,  act); ccadical_add(s,  drv);  ccadical_add(s, -vf); ccadical_add(s, 0);
}

//*************************************************************************************************************
//	@name		BuildFaultyMiter
//	@function	故障回路（全ネット）＋注入スイッチ＋検出条件を一度だけ投入する
//*************************************************************************************************************
void BuildFaultyMiter(CCaDiCaL* s)
{
	// 各ネットの faulty 値：vf = act ? sval : driver
	for (int i = 0; i < n_net; i++)
	{
		int drv;
		if (nl[i].type == IN || nl[i].type == DFF)
		{
			drv = (int)nl[i].varsgc;       // 故障なしなら faulty 入力 = 正常入力
		}
		else
		{
			drv = mt_drv[i];
			emit_gate(s, &nl[i], drv);     // drv = gate(faulty inputs)
		}
		emit_mux(s, mt_vf[i], mt_act[i], mt_sval[i], drv);
	}

	// 検出条件：観測点(n_out==0)で少なくとも1つ varsgc ≠ faulty
	int n_obs = 0;
	for (int i = 0; i < n_net; i++) if (nl[i].n_out == 0) n_obs++;
	int* diff = (int*)malloc((size_t)(n_obs ? n_obs : 1) * sizeof(int));
	int m = 0;
	for (int i = 0; i < n_net; i++)
	{
		if (nl[i].n_out != 0) continue;
		int g = (int)nl[i].varsgc;
		int f = mt_vf[i];
		int d = ++opb.total.vars;          // d = g XOR f
		ccadical_add(s, -g); ccadical_add(s, -f); ccadical_add(s, -d); ccadical_add(s, 0);
		ccadical_add(s, -g); ccadical_add(s,  f); ccadical_add(s,  d); ccadical_add(s, 0);
		ccadical_add(s,  g); ccadical_add(s, -f); ccadical_add(s,  d); ccadical_add(s, 0);
		ccadical_add(s,  g); ccadical_add(s,  f); ccadical_add(s, -d); ccadical_add(s, 0);
		diff[m++] = d;
	}
	for (int i = 0; i < m; i++) ccadical_add(s, diff[i]);
	ccadical_add(s, 0);                    // (∨ diff)：少なくとも1観測点で不一致
	free(diff);
}

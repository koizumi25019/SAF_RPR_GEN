//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "ccadical.h" // CaDiCaL API

#include "./cnf.h"
#include "./act_clause.h"
#include "../fault_detection_prob.h"
#include "../read.h"
#include "../../lib/lib.h"

//*************************************************************************************************************
//	@name		：	CreateConsDC
//	@function	：	create the detection-circuit constraint
//*************************************************************************************************************
void CreateConsDC(
	CCaDiCaL* solver,
	FNODE* target
)
{
	/** 各POでの不一致を検出するXORゲート群を作成 */
	CreateConsDC_XOR(solver);

	/** 複数の不一致信号を1つにまとめるORゲートを作成 */
	CreateConsDC_OR(solver);

	/** 故障励起（値を固定）と、最終的な検出条件（z=1）を設定 */
	CreateConsDC_FE(solver, target);

	return;
}

//*************************************************************************************************************
//	@name		：	CreateConsDC_XOR
//	@function	：	各POにおいて「正常値 != 故障値」を判定する変数を導入
//*************************************************************************************************************
void CreateConsDC_XOR(
	CCaDiCaL* solver
)
{
	for (int i = 0; i < n_net; i++)
	{
		// 故障の影響が伝播しうる外部出力(TPO)を対象にする
		if ((nl[i].flag & TPO) == TPO)
		{
			// XORの出力変数を新しく確保（この変数が 1 なら「このPOで不一致」という意味）
			int diff_var = ++opb.total.vars; 
			int gc = nl[i].varsgc;
			int fc = nl[i].varsfc;

			// CNF for XOR: z = x ⊕ y
			// (-x -y -z 0), (-x y z 0), (x -y z 0), (x y -z 0)
			cadd(solver, -gc); cadd(solver, -fc); cadd(solver, -diff_var); cadd(solver, 0);
			cadd(solver, -gc); cadd(solver,  fc); cadd(solver,  diff_var); cadd(solver, 0);
			cadd(solver,  gc); cadd(solver, -fc); cadd(solver,  diff_var); cadd(solver, 0);
			cadd(solver,  gc); cadd(solver,  fc); cadd(solver, -diff_var); cadd(solver, 0);
		}
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsDC_OR
//	@function	：	複数の不一致フラグを1つにまとめる
//*************************************************************************************************************
void CreateConsDC_OR(
	CCaDiCaL* solver
)
{
	// 伝播先POが複数ある場合のみ、それらを束ねるORゲートを作る
	if (numtranpo > 1)
	{
		int z_var = ++opb.total.vars; // 最終的な「検出成功」フラグ
		
		// XORの結果が格納されている変数は、直前に作成された numtranpo 分の変数
		int first_xor_var = z_var - numtranpo;

		// 1. Forward: いずれかの不一致(x)が1なら、出力(z)は1
		for (int x_var = first_xor_var; x_var < z_var; x_var++)
		{
			cadd(solver, -x_var);
			cadd(solver, z_var);
			cadd(solver, 0);
		}

		// 2. Backward: 出力(z)が1なら、どれか少なくとも1つの不一致(x)は1
		for (int x_var = first_xor_var; x_var < z_var; x_var++)
		{
			cadd(solver, x_var);
		}
		cadd(solver, -z_var);
		cadd(solver, 0);
	}
}

//*************************************************************************************************************
//	@name		：	CreateConsDC_FE
//	@function	：	故障箇所の値を固定し、検出フラグを 1 に強制する
//*************************************************************************************************************
void CreateConsDC_FE(
	CCaDiCaL* solver,
	FNODE* fnodeptr
)
{
	int gc = fnodeptr->netptr->varsgc;
	int fc = fnodeptr->netptr->varsfc;

	// 1. 故障励起（Fault Excitation）
	if (fnodeptr->type == SF0) // Stuck-at 0 故障
	{
		// 検出のためには「正常なら1」でなければならない
		cadd(solver,  gc); cadd(solver, 0); // gc = 1
		cadd(solver, -fc); cadd(solver, 0); // fc = 0 (固定値)
	}
	else if (fnodeptr->type == SF1) // Stuck-at 1 故障
	{
		// 検出のためには「正常なら0」でなければならない
		cadd(solver, -gc); cadd(solver, 0); // gc = 0
		cadd(solver,  fc); cadd(solver, 0); // fc = 1 (固定値)
	}

	// 2. 最終出力（検出フラグ）を 1 に固定
	// opb.total.vars は直前の OR または XOR で作成された「最終出力」を指している
	cadd(solver, opb.total.vars);
	cadd(solver, 0);
}
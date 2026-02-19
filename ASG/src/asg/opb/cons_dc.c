//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "./opb.h"
#include "../asg.h"
#include "../read.h"
#include "../../lib/lib.h"


//*************************************************************************************************************
//	@name		�F�@CreateConsDC
//	@function	�F	create the detection-circuit constraint	//�̏ጟ�o��H����
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC(
	FNODE* target,			  /**< target fault */
	int 				  numfault			  /**< target index */
)
{
	char* cons = (char*)NULL;
	cons = (char*)allocMemory(MAXSIZE_CONS, sizeof(char));

	cons[0] = '\0';

	/** create the constraint for connect the pseudo-primary output */ //�^���O���o��
	CreateConsDC_XOR(&cons);


	/** create the constraint for connect the xor output */ //�O���o��
	CreateConsDC_OR(&cons);


	/** create the fault excitation constraint */ //�̏��N����
	CreateConsDC_FE(&cons, target);


	target->netptr->consfc[numfault] = _strdup(cons);

	free(cons);

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsDC_XOR
//	@function	�F	create the constraint for connect transitive-primary output
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC_XOR(
	char** cons				  /**< constraint */
)
{

	for (int i = 0; i < n_net; i++)
	{
		if ((nl[i].flag & TPO) == TPO)
		{

#ifdef FORMAT_CNF
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
			/**                                 ~x   +  ~y   +  ~z	 >=1 */
// 変数番号を進める (XOR出力用)
			opb.total.vars++;

			size_t len = strlen(*cons);
			size_t rem = MAXSIZE_CONS - len;

			snprintf(*cons + len, rem,
				"-%d -%d -%d 0\n"
				"-%d %d %d 0\n"
				"%d -%d %d 0\n"
				"%d %d -%d 0\n",
				nl[i].varsgc, nl[i].varsfc, opb.total.vars,
				nl[i].varsgc, nl[i].varsfc, opb.total.vars,
				nl[i].varsgc, nl[i].varsfc, opb.total.vars,
				nl[i].varsgc, nl[i].varsfc, opb.total.vars
			);
			//PrintDebugMessage("x%d��DCXOR%d\n", opb.total.vars, i);
			/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#endif
		}
	}

	return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsDC_OR
//	@function	�F	create the constraint for connect the xor outputs
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC_OR(
    char** cons               /**< constraint */
)
{
    // 外部出力が複数ある場合のみ、それらを束ねるORゲートを作成する
    // 1つしかない場合は、その変数がそのまま最終出力(z)として扱われるので何もしなくて良い
    if (numtranpo > 1)
    {
        /**********************************************************************
         * DIMACS CNF Format
         * OR Logic Definition:
         * 1. Forward:  Inputs(x) -> Output(z)  =>  -x z 0
         * 2. Backward: Output(z) -> Inputs(x)  =>  x1 x2 ... -z 0
         *********************************************************************/

        // 変数をインクリメントして、ORゲートの出力変数(z)を確保
        int z_var = ++opb.total.vars;

        // 1. Forward Implication (-x z 0)
        // 「入力が1なら、出力は1」
        for (int x_var = z_var - numtranpo; x_var < z_var; x_var++)
        {
            size_t len = strlen(*cons);
            snprintf(*cons + len, MAXSIZE_CONS - len, "-%d %d 0\n",
                x_var,
                z_var);
        }

        // 2. Backward Implication (x1 x2 ... -z 0)
        // 「出力が1なら、入力のどれかは1」
        
        // 入力変数(x1 x2 ...)を列挙
        for (int x_var = z_var - numtranpo; x_var < z_var; x_var++)
        {
            size_t len = strlen(*cons);
            snprintf(*cons + len, MAXSIZE_CONS - len, "%d ",
                x_var);
        }

        // 最後に -z 0 を付けて行を閉じる
        // ※これは「zを0にする」命令ではなく、上のループと合わせて
        //   「x1 ... -z 0」という一つの節を作っている
        size_t len = strlen(*cons);
        snprintf(*cons + len, MAXSIZE_CONS - len, "-%d 0\n",
            z_var);
            
        // ★重要: ここで "z 0" (z=1固定) は出力しません。
        // その役割は FE (CreateConsDC_FE) に移動しました。
    }

    return;
}

//*************************************************************************************************************
//	@name		�F�@CreateConsDC_FE
//	@function	�F	create the constraint for fault excitation
//	@return		�F	(void)
//*************************************************************************************************************
void CreateConsDC_FE(
    char** cons,
    FNODE* fnodeptr
)
{
    size_t len = strlen(*cons);
    int z_output_var = opb.total.vars; // グローバル変数が最終出力を指している前提

    // 故障励起（Fault Excitation）: 故障箇所での値の不一致を強制
    if (fnodeptr->type == SF0)
    {
        // 正常(gc)=1, 故障(fc)=0
        snprintf(*cons + len, MAXSIZE_CONS - len,
            "%d 0\n"    // gc = 1
            "-%d 0\n",  // fc = 0
            fnodeptr->netptr->varsgc,
            fnodeptr->netptr->varsfc
        );
    }
    else if (fnodeptr->type == SF1)
    {
        // 正常(gc)=0, 故障(fc)=1
        snprintf(*cons + len, MAXSIZE_CONS - len,
            "-%d 0\n"   // gc = 0
            "%d 0\n",   // fc = 1
            fnodeptr->netptr->varsgc,
            fnodeptr->netptr->varsfc
        );
    }

    // 故障検出（Fault Detection）: 最終出力が1であることを強制
    len = strlen(*cons);
    snprintf(*cons + len, MAXSIZE_CONS - len,
        "%d 0\n", // z = 1
        z_output_var
    );
}
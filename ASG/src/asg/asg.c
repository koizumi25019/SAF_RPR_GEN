//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cudd.h>
#include <gmp.h>

#include "./createSGmodel.h"
#include "./asg.h"
#include "./init.h"
#include "./read.h"
#include "./fsim.h"
#include "./opb/opb.h"
#include "./opb/clasp/clasp.h"
#include"./MakeBlockingClause.h"

//prototype declaration
void RunBDD(DdManager* gbm,int nvars, int* pattern_list, int list_size, FILE* result_fp, mpf_t* total_prob_sums);

#define MAX_PATTERN_CASES 100

//*************************************************************************************************************
//	@name		�F�@AnalyzeFaultDensity
//	@function	�F	analyze the fault density
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
bool AnalyzeFaultDensity(
	void
)
{
	TARGET  remain;
	TARGET	target;
	FILE* fprpr = (FILE*)NULL;
	FILE* bdd_result = (FILE*)NULL;
	FILE* cube_file = (FILE*)NULL;
	int loop = 0;
	int temp_numrema;
	int count = 0;

	// fault detection probability calculation array
	mpf_t total_prob_sums[100];

	//CUDD initialization
	DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

	//shifting algorithm
	Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	// initialization
	for (int i = 0; i < 100; i++) {
		mpf_init(total_prob_sums[i]);    // initialize
	    mpf_set_ui(total_prob_sums[i], 0); // set to 0
	}

	//result file open
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//BDD result file header
	fprintf(bdd_result, "name,type,cube,rel,var,den");

	// opt file input pattern numbers
	for (int i = 0; i < opt.file.input.list_size; i++) {
		fprintf(bdd_result, ",n=%d", opt.file.input.pattern_num_list[i]);
	}
	fprintf(bdd_result, "\n");
	fclose(bdd_result);


	//initialize global variables
	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

	//read the fault file
	if (ReadFile() != READ_OKAY) return AFD_ERROR;

	//create Good Circuit constraints
	if (CreateConsGC() != TPG_MODEL_OKAY) return AFD_ERROR;

	
	while (readdata.fault.numrema != 0)
	{
		//open cube file
		fileOpen(&cube_file, "./tools/bdd/bdd_cube_file.txt", "w");

		//open BDD result file
		fileOpen(&bdd_result, opt.file.output.result, "a");

		count++;
		temp_numrema = readdata.fault.numrema;

		//fault list set
		SetTarget(&remain, &target, loop++);

		//write TPG model
		if (WriteTPGModel(&target) != W_TPG_MODEL_OKAY) return AFD_ERROR;

		//test generation loop count
		int test_loop = 0;

		//fault name output
		fprintf(bdd_result, "%s,", target.list[0]->name);

		//fault type output
		if (target.list[0]->type == SF0)
		{
			fprintf(bdd_result, "sa0,");
		}
		else{
			fprintf(bdd_result, "sa1,");
		}

		//UNSAT判定時のテスト生成終了判定
		while (1) {
			// SAT判定時
			// それ以外はUNSAT(存在しない) -> テスト終了
			if (CLASP() != CLASP_OKAY) {

				//test generation count output
				fprintf(bdd_result, "%d,", test_loop);

				//close cube file
				fclose(cube_file);

				//BDD running
				RunBDD(
					gbm,                               // CUDD�}�l�[�W���|�C���^
					n_pi,                              // �ϐ���
					opt.file.input.pattern_num_list,   // �����_���p�^�[�������X�g
					opt.file.input.list_size,          // ���X�g�̃T�C�Y(��)
				    bdd_result,                        // ���ʃt�@�C���|�C���^
					total_prob_sums                    // �m���a�z��
				);

				//BDD result file close
				fclose(bdd_result);

				//detected fault list deletion
				DropDeteFault(&target);

				//free memory
				FreeMemory(&remain, &target);

				break;

			}
			// SAT-> テスト生成続行
			else {
				printf("Progress >> %d/%d\n", count,readdata.fault.numinit);
				printf("SAT test generation count:%d\n", test_loop);
				
				//test generation limit reached
				if (test_loop == opt.file.input.limit) {

					//test generation count output
					fprintf(bdd_result, "%d,", test_loop);

					//close cube file
					fclose(cube_file);

					//BDD running
					RunBDD(
						gbm,                               // CUDD�}�l�[�W���|�C���^
						n_pi,                              // �ϐ���
						opt.file.input.pattern_num_list,   // �����_���p�^�[�������X�g
						opt.file.input.list_size,          // ���X�g�̃T�C�Y(��)
						bdd_result,                        // ���ʃt�@�C���|�C���^
						total_prob_sums                    // �m���a�z��
					);

					//BDD result file close
					fclose(bdd_result);

					//detected fault list deletion
					DropDeteFault(&target);

					//free memory
					FreeMemory(&remain, &target);

					break;
				}

				//test generation count increment
				test_loop++;

				//output the xid test pattern
				OutSolution(&target);

				//dont care identification
				CALL_XID_SAF(opt.file.input.net, opt.file.output.pin);

				//tarminal cls
				system("cls");

				//generate blocking clause 
				char* x_pattern = make_blocking_clause(&target);

				//output the blocking clause
				fprintf(cube_file, "%s\n", x_pattern);

				free(x_pattern);
			}
		}
	}

	// result file open
	fileOpen(&bdd_result, opt.file.output.result, "a");

	fprintf(bdd_result, "\n");
	fprintf(bdd_result, "circuit fault coverage,,,,");

	// GMP variables for average calculation
	mpf_t average_val, total_faults_mpf;
	mpf_init(average_val);
	mpf_init(total_faults_mpf);

	mpf_set_ui(total_faults_mpf, readdata.fault.numinit);

	// GMP variables for average calculation
	for (int i = 0; i < opt.file.input.list_size; i++) {
		// average = sum / total
		mpf_div(average_val, total_prob_sums[i], total_faults_mpf);

		fprintf(bdd_result, ",");
		gmp_fprintf(bdd_result, "%.10Fe", average_val);
	}
	fprintf(bdd_result, "\n");

	fclose(bdd_result);

	// CUDD quit
	mpf_clear(average_val);
	mpf_clear(total_faults_mpf);

	for (int i = 0; i < MAX_PATTERN_CASES; i++) {
		mpf_clear(total_prob_sums[i]);
	}

	return AFD_OKAY;
}

//*************************************************************************************************************
//	@name		�F�@OutSolution
//	@function	�F	output the solution
//	@return		�F	(bool) okay, error
//*************************************************************************************************************
void OutSolution(
	TARGET* target		  /**< target fault */
)
{

	/** for xid */
	FILE* fileptr = (FILE*)NULL;
	fileOpen(&fileptr, "./tools/fsim/test.txt", "w");
	fprintf(fileptr, "%s\n", clasp.sol[SOL_TP]);
	fclose(fileptr);

	/** for xid  */
	FILE* filexid = (FILE*)NULL;
	fileOpen(&filexid, "./tools/fsim/xid_fault.txt", "w");
	if (target->list[0]->type == SF0) {
		fprintf(filexid, "SF0 %s\n",target->list[0]->name);
	}
	else {
		fprintf(filexid, "SF1 %s\n", target->list[0]->name);
	}

	fclose(filexid);
	

	return;
}

//*************************************************************************************************************
//	@name		�F�@FreeMemory
//	@function	�F	free the memory
//	@return		�F	(void)
//*************************************************************************************************************
void FreeMemory(
	TARGET* remain,			  /**< remain fault */
	TARGET* target			  /**< target fault */
)
{
	/** free the target fault lists */
	free(remain->list);
	free(target->list);

	/** free the faulty-circuit constraints  */
	for (int i = 0; i < n_net; i++)
	{
		for (int j = 0; j < target->num; j++)
		{
			if (nl[i].consfc[j] != NULL)
			{
				free(nl[i].consfc[j]);

			}
		}

		free(nl[i].consfc);

	}
	return;
}
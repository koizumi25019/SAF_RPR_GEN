//-------------------------------------------------------------------------------------------------------------
//	include
//-------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cudd.h>

#include "./createSGmodel.h"
#include "./asg.h"
#include "./init.h"
#include "./read.h"
#include "./fsim.h"
#include "./opb/opb.h"
#include "./opb/clasp/clasp.h"

#include"./MakeBlockingClause.h"

//�v���g�^�C�v�錾
//void RunBDD(DdManager* gbm,int nvars, int* pattern_list, int list_size, FILE* result_fp/*, mpf_t* total_prob_sums*/);

// �萔
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

	// ��H�ɂ�����̏ጟ�o�m���v�Z�p��mpf_t �̔z���p��
	//mpf_t total_prob_sums[100];

	//CUDD�̏�����
	//DdManager* gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

	//shifting algorithm
	//Cudd_AutodynEnable(gbm, CUDD_REORDER_SIFT);

	// ������
	for (int i = 0; i < 100; i++) {
		//mpf_init(total_prob_sums[i]);    // �������m��
		//mpf_set_ui(total_prob_sums[i], 0); // 0�ŏ�����
	}

	//result file open
	fileOpen(&bdd_result, opt.file.output.result, "w");

	//BDD�����t�@�C���L�q
	fprintf(bdd_result, "name,type,cube,rel,var,den");

	// opt�\���̂̃f�[�^���g���ă��[�v
	for (int i = 0; i < opt.file.input.list_size; i++) {
		fprintf(bdd_result, ",n=%d", opt.file.input.pattern_num_list[i]);
	}
	fprintf(bdd_result, "\n");
	fclose(bdd_result);


	//�ϐ�������
	if (InitGlobalVars() != INIT_OKAY) return AFD_ERROR;

	//read the fault file
	if (ReadFile() != READ_OKAY) return AFD_ERROR;

	//�����H���񎮐���
	if (CreateConsGC() != TPG_MODEL_OKAY) return AFD_ERROR;

	
	while (readdata.fault.numrema != 0)
	{
		//�e�X�g�L���[�u�t�@�C���I�[�v��
		fileOpen(&cube_file, "./tools/bdd/bdd_cube_file.txt", "w");

		//BDD�������ʃt�@�C���I�[�v��
		fileOpen(&bdd_result, opt.file.output.result, "a");

		count++;
		temp_numrema = readdata.fault.numrema;

		//�̏჊�X�g�ǂݍ���
		SetTarget(&remain, &target, loop++);

		//�e�X�g�������f���\�z
		if (WriteTPGModel(&target) != W_TPG_MODEL_OKAY) return AFD_ERROR;

		//�e�X�g�����񐔏�����
		int test_loop = 0;

		//�̏ᖼ�t�@�C���o��
		fprintf(bdd_result, "%s,", target.list[0]->name);

		//�̏�^�C�v�o��
		if (target.list[0]->type == SF0)
		{
			fprintf(bdd_result, "sa0,");
		}
		else{
			fprintf(bdd_result, "sa1,");
		}

		//UNSAT�ɂȂ邩�C���̃e�X�g�����񐔂ɒB����܂ŌJ��Ԃ�
		while (1) {
			// SAT�\���o���s
			// ���ʂ�UNSAT(���Ȃ�) -> �T���I��
			if (CLASP() != CLASP_OKAY) {

				//�L���[�u���o��
				fprintf(bdd_result, "%d,", test_loop);

				//�e�X�g�Ɋ֌W����O�����͐��o��
				fprintf(bdd_result, "%d,", target.list[0]->test_relation_num);

				//�e�X�g�L���[�u�t�@�C���N���[�Y	
				fclose(cube_file);

				//BDD�ɂ��^���l�\���x�v�Z
				/*RunBDD(
					gbm,                               // CUDD�}�l�[�W���|�C���^
					n_pi,                              // �ϐ���
					opt.file.input.pattern_num_list,   // �����_���p�^�[�������X�g
					opt.file.input.list_size,          // ���X�g�̃T�C�Y(��)
				    bdd_result//,                        // ���ʃt�@�C���|�C���^
					//total_prob_sums                    // �m���a�z��
				);*/

				//BDD�������ʃt�@�C���N���[�Y	
				fclose(bdd_result);

				//�����o�̏჊�X�g����폜
				DropDeteFault(&target);

				//�������J��
				FreeMemory(&remain, &target);

				break;

			}
			// SAT(������) -> �e�X�g�����p��
			else {
				printf("Progress >> %d/%d\n", count,readdata.fault.numinit);
				printf("SAT test generation count:%d\n", test_loop);
				
				//�e�X�g�����񐔂�100��ɂȂ�����ł��؂�
				if (test_loop == opt.file.input.limit) {

					//�L���[�u���o��
					fprintf(bdd_result, "%d,", test_loop);

					//�e�X�g�Ɋ֌W����O�����͐��o��
					fprintf(bdd_result, "%d,", target.list[0]->test_relation_num);

					//�e�X�g�L���[�u�t�@�C���N���[�Y
					fclose(cube_file);

					//BDD�ɂ��^���l�\���x�v�Z
					/*RunBDD(
						gbm,                               // CUDD�}�l�[�W���|�C���^
						n_pi,                              // �ϐ���
						opt.file.input.pattern_num_list,   // �����_���p�^�[�������X�g
						opt.file.input.list_size,          // ���X�g�̃T�C�Y(��)
						bdd_result//,                        // ���ʃt�@�C���|�C���^
						//total_prob_sums                    // �m���a�z��
					);*/

					//BDD�������ʃt�@�C���N���[�Y	
					fclose(bdd_result);

					//�����o�̏჊�X�g����폜
					DropDeteFault(&target);

					//�������J��
					FreeMemory(&remain, &target);

					break;
				}

				//�̏�ɑ΂���e�X�g������
				test_loop++;

				//XID�p�e�X�g�p�^�[���o��
				OutSolution(&target);

				//dont care identification
				CALL_XID_SAF(opt.file.input.net, opt.file.output.pin);

				//tarminal cls
				system("cls");

				//generate blocking clause 
				char* x_pattern = make_blocking_clause(&target);

				//�e�X�g�L���[�u���t�@�C���ɏ�������
				fprintf(cube_file, "%s\n", x_pattern);

				free(x_pattern);
			}
		}
	}

	// ��H�S�̂̌̏ጟ�o���o��
	fileOpen(&bdd_result, opt.file.output.result, "a");

	fprintf(bdd_result, "\n");
	fprintf(bdd_result, "circuit fault coverage,,,,");

	// ���όv�Z�p��GMP�ϐ�����
	//mpf_t average_val, total_faults_mpf;
	//mpf_init(average_val);
	//mpf_init(total_faults_mpf);

	//mpf_set_ui(total_faults_mpf, readdata.fault.numinit);

	// �e�����_���p�^�[�������Ƃ̕��όv�Z
	for (int i = 0; i < opt.file.input.list_size; i++) {
		// ���� = ���v / �S�̏ᐔ
		//mpf_div(average_val, total_prob_sums[i], total_faults_mpf);

		fprintf(bdd_result, ",");
		// "%.6Ff" �ŏ����_�ȉ�6���܂ŏo��
		//gmp_fprintf(bdd_result, "%.10Fe", average_val);
	}
	fprintf(bdd_result, "\n");

	fclose(bdd_result);

	// --- ��������� (��n��) ---
	//mpf_clear(average_val);
	//mpf_clear(total_faults_mpf);

	for (int i = 0; i < MAX_PATTERN_CASES; i++) {
		//mpf_clear(total_prob_sums[i]);
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
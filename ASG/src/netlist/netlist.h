//--------------------------------------------------------------------------------------------------------------------
//Netlist ��`, �O���ϐ�, PROTOPYPE
//File name : nelist.h
//Date : 2006/03/26
//Designer : R.Inoue
//Ver : 3.00�i�z�zVer�j
//--------------------------------------------------------------------------------------------------------------------
#pragma once
#include<stdbool.h>

//--------------------------------------------------------------------------------------------------------------------
// ��`
//--------------------------------------------------------------------------------------------------------------------
//�Q�[�g�^�C�v
#define  IN		0
#define  BUF    10
#define  INV	11
#define  AND	12
#define  NAND	13
#define  OR		14
#define  NOR	15
#define  EXOR	16
#define  EXNOR	17
#define  FOUT	18
#define  DFF	19
#define  RDFF	21
#define  DFFS	23
#define  RDFFS	24
#define  GND	25
#define  ACC	26

//�l�b�g���X�g�\���̒�`
typedef struct _Netlist_Format_ {

	char* name;							//���O
	int	n;		           				//ID�i���j�[�N�j
	int	type;	          				//�^�C�v
	int	n_in;	            			//���͐�
	struct _Netlist_Format_** in;		//���̓|�C���^�z��
	int				n_out;	            //�o�͐�
	struct _Netlist_Format_** out;      //�o�̓|�C���^�z��
	char* name_port;					//�[�q�� pin��
	char* name_ins;						//�C���X�^���X��

	int	test_sf0;						//0縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int	test_sf1;						//1縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) 

	int				flag;			    /**< flag */
	unsigned int	varsgc;			    /**< variables for good-circuit  */
	unsigned int	varsfc;				/**< variables for faulty-circuit  */
	char* consgc;						/**< constraints for good-circuit  */
	char** consfc;						/**< constraints for faulty-circuit  */
	unsigned int suf_fc;				//�̏��H�̓Y����
	unsigned int fault_pass;			//�̏ᓞ�B�\�p�X

}NLIST;

//--------------------------------------------------------------------------------------------------------------------
// �O���ϐ�
//--------------------------------------------------------------------------------------------------------------------
//�l�b�g���X�g
NLIST* nl;

//���W���[����
char* module_name;

//�M����
int n_net;

//�O�����͐�, �O������
NLIST** pi;
int n_pi;

//�O���o�͐�, �O���o��
NLIST** po;
int n_po;

//�^���O���o�͐�, �O���o��
NLIST** ppo;
int n_ppo;

//DFF, DFF��
NLIST** dff;
int n_dff;

//RDFF, RDFF��
NLIST** rdff;
int n_rdff;

//DFFS, DFFS��
NLIST** dffs;
int n_dffs;

//RDFFS, RDFFS��
NLIST** rdffs;
int n_rdffs;

//assign
NLIST** assign;
int n_assign;

//--------------------------------------------------------------------------------------------------------------------
// PROTOPYPE
//--------------------------------------------------------------------------------------------------------------------
int		read_nl(char*);
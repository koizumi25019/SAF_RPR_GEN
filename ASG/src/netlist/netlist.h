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


typedef struct EquivNode {
    struct _Netlist_Format_* net;  // 等価故障のネットポインタ
    struct EquivNode*        next;
} EQUIV_NODE;

typedef struct _Netlist_Format_ {
	char* name;							//信号線名
	int	n;		           				//ユニークID
	int	type;	          				//ゲートタイプ
	int	n_in;	            			//
	struct _Netlist_Format_** in;		//
	int				n_out;	            //
	struct _Netlist_Format_** out;      //
	char* name_port;					//
	char* name_ins;						//
	int				flag;			    /**< flag */
	unsigned int	varsgc;			    /**< variables for good-circuit  */
	unsigned int	varsfc;				/**< variables for faulty-circuit  */
	char* consgc;						/**< constraints for good-circuit  */
	char** consfc;						/**< constraints for faulty-circuit  */
	unsigned int suf_fc;				//
	unsigned int fault_pass;			//
	int	test_sa0;						//0縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) }
	int	test_sa1;						//1縮退故障のテスト対象フラグ		{ YES(テスト対象とする), NO(しない) 
    EQUIV_NODE* equiv_sa0;  			// このネットのsa0と等価な故障リスト
    EQUIV_NODE* equiv_sa1;  			// このネットのsa1と等価な故障リスト
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
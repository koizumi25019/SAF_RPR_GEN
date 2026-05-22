//--------------------------------------------------------------------------------------------------------------------
//Netlist
//File name : Netlist.c
//Date : 2010/06/07
//Designer : R.Inoue
//Ver : 3.0�i�z�zVer�j
//--------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include"netlist.h"
#pragma warning(disable:4996)

//--------------------------------------------------------------------------------------------------------------------
// ��`
//--------------------------------------------------------------------------------------------------------------------
#define MAXN 1028      // �l�b�g���X�g�t�@�C�����T�C�Y
#define STR_MAX 1024   //get_token�p
#define SPRINT_MAX 256 //sprintf�p
#define HASH_SIZE 5003 //�n�b�V���\�T�C�Y
#define HASH_MODE
#undef DEBUG_MA
#undef DEBUG_NL

//���W���[���z�񃁃��o�[
typedef struct _Module_Array_MEM_Format_ {
	char* m_name_ins; //�C���X�^���X��
	int m_type;	      //�^�C�v
	int m_n_in;	      //���͐�
	char* m_out_name; //�o�͖�
	char** m_in_name; //���͖��z��
	// exe. <�ʏ�>  A:[0], B:[1],�E�E�E, J:[9]
	//      <DFF >  D:[0]
	//      <RDFF>  D:[0], CD(���Z�b�g):[1]
	//      <DFFS>  D:[0], TI(�X�L�����C��):[1], TE(�X�L�����C�l�[�u��):[2]
	//      <RDFFS> D:[0], TI(�X�L�����C��):[1], TE(�X�L�����C�l�[�u��):[2], CD(���Z�b�g):[3]
	int out_nid;    //�o�͐M���̃l�b�g���X�gID
	int* in_nid;    //���͐M���̃l�b�g���X�gID�z��
} Module_Array_MEM;

//�M�����n�b�V�������o�[
typedef struct _Signal_Name_Hash_Format_ {
	int nid;        //�l�b�g���X�gID
	struct _Signal_Name_Hash_Format_* next;
} Signal_Name_Hash;

//�\����͔z��
int* parser_array[2]; //for_in:[0], for_out[1]

//--------------------------------------------------------------------------------------------------------------------
// �ÓI�ϐ�
//--------------------------------------------------------------------------------------------------------------------
//�[�q���z��i10���͂܂őΉ� 04/03/16�j
char pin_name_array[10] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J' };

//���[�W���[���z��
static Module_Array_MEM** module_array = NULL;

//�M�����n�b�V���e�[�u��
static Signal_Name_Hash** hash_t = NULL;

//���[�W���[����
static int n_module = 0;

//���̐M����
static int nl_num = 0;

//--------------------------------------------------------------------------------------------------------------------
// �v���^�C�v�錾
//--------------------------------------------------------------------------------------------------------------------
static void option(int, char**, char*);
static void file_open(FILE**, char*);
static void init_global(void);
static void read_net(FILE*);
static void read_file(FILE*);
static void read_1st(FILE*);
static void calc_pi_po(FILE*, int);
static void alloc_module_array(void);
static void read_2nd(FILE*);
static void read_n_in(int, char*, int);
static void alloc_in_name(int);
static void read_module_type(FILE*, char*, int);
static void read_ins(FILE*, int);
static void read_pin(FILE*, int);
static void alloc_nl(void);
static void parser(FILE*);
static void alloc_hash_table(void);
static void store_signal(void);
static void store_pi_po(FILE*);
static void read_pi_po(FILE*, int*, int);
static void hash_search_pi_po(char*, int*, int);
static void read_assign(FILE*, int*);
static void analyze_asyntax_error(void);
static int  hash_key(char*);
static void free_hash(void);
static void connection_signal(void);
static void connection_no_fout(int, int);
static void connection_yes_fout(int, int);
static void connection_po_fout(int, int, int);
static void alloc_nl_in_out(void);
static void connection_n_signal(void);
static void set_pi_ins(void);
static void set_ff_array(void);
static int  get_token(FILE*, char*, int, int*);
static void error(int, char*);
static void free_module_array(void);
static void debug_ma(void);
static void debug_nl(void);

//--------------------------------------------------------------------------------------------------------------------
// �O���֐�
//--------------------------------------------------------------------------------------------------------------------
int read_nl(char* v_name)
{
	FILE* r_fp; //�l�b�g���X�g�t�@�C���|�C���^

	//�t�@�C���I�[�v��
	file_open(&r_fp, v_name);

	//�O���ϐ�������
	init_global();

	//�l�b�g���X�g�ǂݍ���
	read_net(r_fp);

	//�l�b�g���X�g�z��f�o�b�O
#ifdef DEBUG_NL
	debug_nl();
#endif

	//�t�@�C���N���[�Y
	fclose(r_fp);

	//�m�F���b�Z�[�W
	//fprintf(stderr, "Completion of Read_Netlist !!\n");

	return 0;
}

//--------------------------------------------------------------------------------------------------------------------
// �����֐�
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
// �R�}���h���C���̏���
//--------------------------------------------------------------------------------------------------------------------
static void option(int argc, char* argv[], char* v_name)
{
	//�g��������
	if (argc != 2) {
		printf("******** The usage of Netlist.exe *******\n");
		printf("Netlist.exe [Verilog-HDL Netlist file]\n");
		printf("*****************************************\n");
		exit(-1);
	}

	//�t�@�C�����擾
	strcpy(v_name, argv[1]);
}

//--------------------------------------------------------------------------------------------------------------------
// �t�@�C���I�[�v��
//--------------------------------------------------------------------------------------------------------------------
static void file_open(FILE** r_fp, char* v_name)
{
	if ((*r_fp = fopen(v_name, "r")) == (FILE*)NULL) {
		fprintf(stderr, "�t�@�C���I�[�v�����s(%s)\n", v_name);
		exit(-1);
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �O���ϐ�������
//--------------------------------------------------------------------------------------------------------------------
static void init_global(void)
{
	n_net = 0;    //�M����
	n_pi = 0;     //�O�����͐�
	n_po = 0;     //�O���o�͐�
	n_dff = 0;    //DFF��
	n_rdff = 0;   //RDFF��
	n_dffs = 0;   //DFFS��
	n_rdffs = 0;  //RDFFS��
	n_assign = 0; //assign��
}

//--------------------------------------------------------------------------------------------------------------------
// �l�b�g���X�g�ǂݍ��ݐ���
//--------------------------------------------------------------------------------------------------------------------
static void read_net(FILE* rfp)
{
	//�l�b�g���X�g�t�@�C���ǂݍ���
	read_file(rfp);

	//�l�b�g���X�g�z��,�O���ϐ�,�\����͔z�� �̈�m��,������
	alloc_nl();

	//�\�����
	parser(rfp);

	//�n�b�V���\���
	free_hash();

	//�M���ڑ�
	connection_signal();

	//���W���[���z��f�o�b�O
#ifdef DEBUG_MA
	debug_ma();
#endif

	//���W���[���z��,�\����͔z�� ���
	free_module_array();
}

//--------------------------------------------------------------------------------------------------------------------
// �l�b�g���X�g�t�@�C�����ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_file(FILE* rfp)
{
	//���W���[����, PI��, PO���Z�o & module���擾
	read_1st(rfp);

	//���W���[���z��̈�m��, ������
	alloc_module_array();

	//�e���W���[���ǂݍ���
	read_2nd(rfp);
}

//--------------------------------------------------------------------------------------------------------------------
// ���W���[����, PI��, PO���Z�o & module���擾
//--------------------------------------------------------------------------------------------------------------------
static void read_1st(FILE* rfp)
{
	char str[STR_MAX];
	int c;

	while (get_token(rfp, str, 1, 0) != EOF) {
		//module���擾
		if (!strcmp(str, "module")) {
			get_token(rfp, str, 1, 0);
			module_name = (char*)malloc(sizeof(char) * (strlen(str) + 1));
			strcpy(module_name, str);
			while ((c = getc(rfp)) != ';'); continue;
		}
		else if (!strcmp(str, "wire")) { while ((c = getc(rfp)) != ';'); continue; }
		//assign���Z�o
		else if (!strcmp(str, "assign")) {
			n_assign++;
			while ((c = getc(rfp)) != ';'); continue;
		}
		else if (!strcmp(str, "endmodule")) { continue; }
		//PI���Z�o
		else if (!strcmp(str, "input")) calc_pi_po(rfp, 0);
		//PO���Z�o
		else if (!strcmp(str, "output")) calc_pi_po(rfp, 1);
		//���W���[�����Z�o
		else {
			n_module++;
			while ((c = getc(rfp)) != ';'); continue;
		}
	}
	rewind(rfp);
}

//--------------------------------------------------------------------------------------------------------------------
// PI��, PO���Z�o
//--------------------------------------------------------------------------------------------------------------------
static void calc_pi_po(FILE* rfp, int mode)
{
	char str[STR_MAX]; //get_token�p
	int bit_1, bit_2;  //�r�b�g��������ꍇ�̃r�b�g���v�Z�p
	int re_no;         //�Ԃ�l
	int num;
	int c;

	//������
	re_no = num = 0;

	c = fgetc(rfp);
	while ((c == ' ') || (c == '\t')) c = fgetc(rfp);
	//bit������
	if (c == '[') {
		get_token(rfp, str, 1, 0); bit_1 = atoi(str);
		get_token(rfp, str, 1, 0); bit_2 = atoi(str);
		re_no = get_token(rfp, str, 0, 0);
		while ((re_no != 1) && (re_no != 2)) {
			num++;
			re_no = get_token(rfp, str, 0, 0);
		}
		if (re_no == 1) num++;
		if (mode == 0) n_pi += (num * (abs(bit_1 - bit_2) + 1));
		else if (mode == 1) n_po += (num * (abs(bit_1 - bit_2) + 1));
	}
	//bit���Ȃ�
	else {
		ungetc(c, rfp);
		re_no = get_token(rfp, str, 0, 0);
		while ((re_no != 1) && (re_no != 2)) {
			//�N���b�N�ǂݔ�΂�
			if (!strcmp(str, "CLK") || !strcmp(str, "clk")) {
				re_no = get_token(rfp, str, 0, 0);
				continue;
			}
			if ((re_no == 0) && (mode == 0)) n_pi++;
			else if ((re_no == 0) && (mode == 1)) n_po++;
			re_no = get_token(rfp, str, 0, 0);
		}
		//�N���b�N�ǂݔ�΂��i�N���b�N�łȂ������ꍇ���s�j
		if (strcmp(str, "CLK") && strcmp(str, "clk")) {
			if ((re_no == 1) && (mode == 0)) n_pi++;
			else if ((re_no == 1) && (mode == 1)) n_po++;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// ���W���[���z��̈�m��, ������
//--------------------------------------------------------------------------------------------------------------------
static void alloc_module_array(void)
{
	int i;

	module_array = (Module_Array_MEM**)malloc(sizeof(Module_Array_MEM*) * n_module);
	for (i = 0; i < n_module; i++)
		module_array[i] = (Module_Array_MEM*)malloc(sizeof(Module_Array_MEM));
}

//--------------------------------------------------------------------------------------------------------------------
// �e���W���[���̓ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_2nd(FILE* rfp)
{
	int in_num; //���͐��ǂݍ��ݗp
	int m_no;   //���ݓǂ�ł��郂�W���[���ԍ�
	char str[STR_MAX];
	int c;

	//������
	in_num = m_no = 0;

	while (get_token(rfp, str, 2, &in_num) != EOF) {
		if (!strcmp(str, "module")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "input")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "output")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "wire")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "assign")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "endmodule")) { continue; }
		else {
			//���͐��̓ǂݍ���
			read_n_in(in_num, str, m_no);
			//���͐M����,���͐M���l�b�g���X�gID�z��̗̈�m��
			alloc_in_name(m_no);
			//���W���[���^�C�v
			read_module_type(rfp, str, m_no);
			//�C���X�^���X���̓ǂݍ���
			read_ins(rfp, m_no);
			//�[�q, �M�����̓ǂݍ���
			read_pin(rfp, m_no);

			in_num = 0;
			m_no++;
			while ((c = getc(rfp)) != ';'); continue;
		}
	}
	rewind(rfp);
}

//--------------------------------------------------------------------------------------------------------------------
// ���͐��̓ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_n_in(int in_num, char* str, int m_no)
{
	if (!strcmp(str, "BUF")) module_array[m_no]->m_n_in = 1;
	else if (!strcmp(str, "INV")) module_array[m_no]->m_n_in = 1;
	else if (!strcmp(str, "DFF")) module_array[m_no]->m_n_in = 1;
	else if (!strcmp(str, "RDFF")) module_array[m_no]->m_n_in = 2;
	else if (!strcmp(str, "DFFS")) module_array[m_no]->m_n_in = 3;
	else if (!strcmp(str, "RDFFS")) module_array[m_no]->m_n_in = 4;
	else module_array[m_no]->m_n_in = in_num;
}

//--------------------------------------------------------------------------------------------------------------------
// ���͐��M����,���̓n�b�V���|�C���^�z��̗̈�m��
//--------------------------------------------------------------------------------------------------------------------
static void alloc_in_name(int m_no)
{
	module_array[m_no]->m_in_name = (char**)malloc(sizeof(char*) * module_array[m_no]->m_n_in);
	module_array[m_no]->in_nid = (int*)malloc(sizeof(int) * module_array[m_no]->m_n_in);
}

//--------------------------------------------------------------------------------------------------------------------
// ���W���[���^�C�v�̓ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_module_type(FILE* rfp, char* str, int m_no)
{
	switch (str[0]) {
	case 'A': module_array[m_no]->m_type = AND; break;
	case 'B': module_array[m_no]->m_type = BUF; break;
	case 'D': if (strlen(str) == 3) { module_array[m_no]->m_type = DFF; n_dff++; }
			else { module_array[m_no]->m_type = DFFS; n_dffs++; }
			break;
	case 'E': if (strlen(str) == 4) module_array[m_no]->m_type = EXOR;
			else module_array[m_no]->m_type = EXNOR;
		//error : 2���͈ȊO��EXOR, EXNOR���������ꍇ
		if (module_array[m_no]->m_n_in != 2) {
			get_token(rfp, str, 0, 0);
			error(1, str);
			exit(-1);
		}
		break;
	case 'I': module_array[m_no]->m_type = INV;	break;
	case 'N': if (strlen(str) == 3) module_array[m_no]->m_type = NOR;
			else module_array[m_no]->m_type = NAND; break;
	case 'O': module_array[m_no]->m_type = OR; break;
	case 'R': if (strlen(str) == 4) { module_array[m_no]->m_type = RDFF; n_rdff++; }
			else { module_array[m_no]->m_type = RDFFS; n_rdffs++; }
			break;
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �C���X�^���X���̓ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_ins(FILE* rfp, int m_no)
{
	char str[STR_MAX]; //get_token�p

	get_token(rfp, str, 0, 0);
	module_array[m_no]->m_name_ins = (char*)malloc(sizeof(char) * (strlen(str) + 1));
	strcpy(module_array[m_no]->m_name_ins, str);
}

//--------------------------------------------------------------------------------------------------------------------
// �[�q,�M�����̓ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_pin(FILE* rfp, int m_no)
{
	char str[STR_MAX]; //get_token�p
	int i, j;

	for (i = 0; i < module_array[m_no]->m_n_in + 1; i++) { //�o�͐���+1
		while (get_token(rfp, str, 0, 0) != 0);
		//�o�͐M��
		if (str[0] == 'Z' || str[0] == 'Q') {
			while (get_token(rfp, str, 0, 0) != 0);
			module_array[m_no]->m_out_name = (char*)malloc(sizeof(char) * (strlen(str) + 1));
			strcpy(module_array[m_no]->m_out_name, str);
		}
		//�eDFF��D�i���́j
		else if (((str[0] == 'D') && (module_array[m_no]->m_type == DFF)) ||
			((str[0] == 'D') && (module_array[m_no]->m_type == RDFF)) ||
			((str[0] == 'D') && (module_array[m_no]->m_type == DFFS)) ||
			((str[0] == 'D') && (module_array[m_no]->m_type == RDFFS))) {
			while (get_token(rfp, str, 0, 0) != 0);
			module_array[m_no]->m_in_name[0] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
			strcpy(module_array[m_no]->m_in_name[0], str);
		}
		//�eDFF��CP�i�N���b�N�j
		else if (!strcmp(str, "CP")) {
			while (get_token(rfp, str, 0, 0) != 0);
			i--;
		}
		//RDFF,RDFFS��CD�i���Z�b�g�j
		else if (!strcmp(str, "CD")) {
			while (get_token(rfp, str, 0, 0) != 0);
			if (module_array[m_no]->m_type == RDFF) {
				module_array[m_no]->m_in_name[1] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
				strcpy(module_array[m_no]->m_in_name[1], str);
			}
			else if (module_array[m_no]->m_type == RDFFS) {
				module_array[m_no]->m_in_name[3] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
				strcpy(module_array[m_no]->m_in_name[3], str);
			}
		}
		//DFFS,RDFFS��TI�i�X�L�����C���j
		else if (!strcmp(str, "TI")) {
			while (get_token(rfp, str, 0, 0) != 0);
			module_array[m_no]->m_in_name[1] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
			strcpy(module_array[m_no]->m_in_name[1], str);
		}
		//DFFS,RDFFS��TE�i�X�L�����C�l�[�u���j
		else if (!strcmp(str, "TE")) {
			while (get_token(rfp, str, 0, 0) != 0);
			module_array[m_no]->m_in_name[2] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
			strcpy(module_array[m_no]->m_in_name[2], str);
		}
		//���̑��̓��͐M��
		else {
			j = 0;
			while (pin_name_array[j] != str[0]) {
				j++;
				if (j > 10) error(2, module_array[m_no]->m_name_ins);
			}
			while (get_token(rfp, str, 0, 0) != 0);
			module_array[m_no]->m_in_name[j] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
			strcpy(module_array[m_no]->m_in_name[j], str);
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �l�b�g���X�g�z��,�O���ϐ�,�\����͔z�� �̈�m��,������
//--------------------------------------------------------------------------------------------------------------------
static void alloc_nl(void)
{
	int i;

	//���̐M������
	nl_num = n_pi + n_po + n_module;
	for (i = 0; i < n_module; i++) nl_num += module_array[i]->m_n_in;

	//�̈�m��
	nl = (NLIST*)malloc(sizeof(NLIST) * nl_num);
	pi = (NLIST**)malloc(sizeof(NLIST*) * n_pi);
	po = (NLIST**)malloc(sizeof(NLIST*) * n_po);
	dff = (NLIST**)malloc(sizeof(NLIST*) * n_dff);
	rdff = (NLIST**)malloc(sizeof(NLIST*) * n_rdff);
	dffs = (NLIST**)malloc(sizeof(NLIST*) * n_dffs);
	rdffs = (NLIST**)malloc(sizeof(NLIST*) * n_rdffs);
	assign = (NLIST**)malloc(sizeof(NLIST*) * n_assign);
	parser_array[0] = (int*)malloc(sizeof(int) * (nl_num));
	parser_array[1] = (int*)malloc(sizeof(int) * (nl_num));

	//������
	for (i = 0; i < nl_num; i++) {
		nl[i].type = -1;
		nl[i].n_in = 0;
		nl[i].in = (NLIST**)NULL;
		nl[i].n_out = 0;
		nl[i].out = (NLIST**)NULL;
		parser_array[0][i] = 0;
		parser_array[1][i] = 0;
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �\�����
//--------------------------------------------------------------------------------------------------------------------
static void parser(FILE* rfp)
{
	//�n�b�V���\�̗̈�m��
	alloc_hash_table();

	//�M�����̊i�[
	store_signal();

	//PI,PO�ǂݍ���
	store_pi_po(rfp);

	//�\���G���[���
	analyze_asyntax_error();
}

//--------------------------------------------------------------------------------------------------------------------
// �n�b�V���\�̗̈�m��
//--------------------------------------------------------------------------------------------------------------------
static void alloc_hash_table(void)
{
	int i;

	hash_t = (Signal_Name_Hash**)malloc(sizeof(Signal_Name_Hash*) * HASH_SIZE);
	for (i = 0; i < HASH_SIZE; i++) hash_t[i] = NULL;
}

//--------------------------------------------------------------------------------------------------------------------
// �M�����̊i�[
//--------------------------------------------------------------------------------------------------------------------
static void store_signal(void)
{
	Signal_Name_Hash* ptr;
	int hash_value; //�n�b�V���l
	int i, j;

	for (i = 0; i < n_module; i++) {
		//����
		for (j = 0; j < module_array[i]->m_n_in; j++) {
			hash_value = hash_key(module_array[i]->m_in_name[j]);
			//�T��
			ptr = hash_t[hash_value];
			while (ptr != NULL) {
				if (!strcmp(nl[ptr->nid].name, module_array[i]->m_in_name[j])) {
					//�\����͔z��C���N�������g
					parser_array[0][ptr->nid] ++;
					//���͐M���l�b�g���X�gID�z��}��
					module_array[i]->in_nid[j] = ptr->nid;
					break;
				}
				ptr = ptr->next;
			}
			//�V�K�쐬
			if (ptr == NULL) {
				//�n�b�V���\�}��
				ptr = (Signal_Name_Hash*)malloc(sizeof(Signal_Name_Hash));
				ptr->next = hash_t[hash_value];
				hash_t[hash_value] = ptr;
				ptr->nid = n_net;
				//�l�b�g���X�g�}��
				nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(module_array[i]->m_in_name[j]) + 1));
				strcpy(nl[n_net].name, module_array[i]->m_in_name[j]);
				nl[n_net].n = n_net; //�l�b�g���X�gID
				//�\����͔z����
				parser_array[0][n_net] = 1;
				//���͐M���l�b�g���X�gID�z��}��
				module_array[i]->in_nid[j] = n_net;

				n_net++;
			}
		}
		//�o��
		hash_value = hash_key(module_array[i]->m_out_name);
		//�T��
		ptr = hash_t[hash_value];
		while (ptr != NULL) {
			if (!strcmp(nl[ptr->nid].name, module_array[i]->m_out_name)) {
				//�\����͔z��C���N�������g
				parser_array[1][ptr->nid] ++;
				//�o�͐M���l�b�g���X�gID�}��
				module_array[i]->out_nid = ptr->nid;
				break;
			}
			ptr = ptr->next;
		}
		//�V�K�쐬
		if (ptr == NULL) {
			//�n�b�V���\�}��
			ptr = (Signal_Name_Hash*)malloc(sizeof(Signal_Name_Hash));
			ptr->next = hash_t[hash_value];
			hash_t[hash_value] = ptr;
			ptr->nid = n_net;
			//�l�b�g���X�g�}��
			nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(module_array[i]->m_out_name) + 1));
			strcpy(nl[n_net].name, module_array[i]->m_out_name);
			nl[n_net].n = n_net; //�l�b�g���X�gID
			//�\����͔z����
			parser_array[1][n_net] = 1;
			//�o�͐M���l�b�g���X�gID�}��
			module_array[i]->out_nid = n_net;

			n_net++;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// PI,PO�ǂݍ��ݐ���
//--------------------------------------------------------------------------------------------------------------------
static void store_pi_po(FILE* rfp)
{
	char str[STR_MAX]; //get_token�p
	int pi_no;
	int po_no;
	int assign_no;
	int c;

	//������
	pi_no = po_no = assign_no = 0;

	while (get_token(rfp, str, 0, 0) != EOF) {
		if (!strcmp(str, "module")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "input")) read_pi_po(rfp, &pi_no, 0);
		else if (!strcmp(str, "output")) read_pi_po(rfp, &po_no, 1);
		else if (!strcmp(str, "assign")) { while ((c = getc(rfp)) != ';'); continue; }//read_assign(rfp, &assign_no);
		else if (!strcmp(str, "wire")) { while ((c = getc(rfp)) != ';'); continue; }
		else if (!strcmp(str, "endmodule")) { continue; }
		else { while ((c = getc(rfp)) != ';'); continue; }
	}
}

//--------------------------------------------------------------------------------------------------------------------
// PI,PO�ǂݍ���
//--------------------------------------------------------------------------------------------------------------------
static void read_pi_po(FILE* rfp, int* no, int mode)
{
	char str[STR_MAX];    //get_token�p
	char tmp[SPRINT_MAX]; //sprint�p
	int bit_1, bit_2;     //�r�b�g��������ꍇ�̃r�b�g���v�Z�p
	int n_bit;            //�r�b�g��
	int start_bit;        //�ŏ��̃r�b�g��
	int re_no;            //�Ԃ�l
	int i, c;

	//������
	re_no = 0;

	c = fgetc(rfp);
	while ((c == ' ') || (c == '\t')) c = fgetc(rfp);
	//bit������
	if (c == '[') {
		get_token(rfp, str, 1, 0); bit_1 = atoi(str);
		get_token(rfp, str, 1, 0); bit_2 = atoi(str);
		n_bit = abs(bit_1 - bit_2) + 1;
		re_no = get_token(rfp, str, 0, 0);
		//�ŏ��̃r�b�g��
		if (bit_1 <= bit_2) start_bit = bit_1;
		else start_bit = bit_2;
		//�����|�[�g�錾
		while ((re_no != 1) && (re_no != 2)) {
			for (i = 0; i < n_bit; i++) {
				sprintf(tmp, "%s[%d]", str, i + start_bit);
				hash_search_pi_po(tmp, no, mode);
			}
			re_no = get_token(rfp, str, 0, 0);
		}
		//�P�ƃ|�[�g�錾
		if (re_no == 1) {
			for (i = 0; i < n_bit; i++) {
				sprintf(tmp, "%s[%d]", str, i + start_bit);
				hash_search_pi_po(tmp, no, mode);
			}
		}
	}
	//bit���Ȃ�
	else {
		ungetc(c, rfp);
		re_no = get_token(rfp, str, 0, 0);
		while ((re_no != 1) && (re_no != 2)) {
			//�N���b�N�ǂݔ�΂�
			if (!strcmp(str, "CLK") || !strcmp(str, "clk")) {
				re_no = get_token(rfp, str, 0, 0);
				continue;
			}
			if (re_no == 0) hash_search_pi_po(str, no, mode);
			re_no = get_token(rfp, str, 0, 0);
		}
		//�N���b�N�ǂݔ�΂��i�N���b�N�łȂ������ꍇ���s�j
		if (strcmp(str, "CLK") && strcmp(str, "clk")) {
			if (re_no == 1) hash_search_pi_po(str, no, mode);
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// PI,PO �n�b�V���T��
//--------------------------------------------------------------------------------------------------------------------
static void hash_search_pi_po(char* word, int* no, int mode)
{
	int hash_value;   //�n�b�V���l
	Signal_Name_Hash* ptr;

	hash_value = hash_key(word);
	ptr = hash_t[hash_value];
	while (ptr != NULL) {
		if (!strcmp(nl[ptr->nid].name, word)) {
			//PI
			if (mode == 0) {
				//TYPE
				nl[ptr->nid].type = IN;
				//PI�z��
				pi[(*no)++] = &nl[ptr->nid];
			}
			//PO
			else if (mode == 1) {
				//PO�z��
				po[(*no)++] = &nl[ptr->nid];
			}
			break;
		}
		ptr = ptr->next;
	}
	//error : �M�����X���[���Ă���ꍇ
	if (ptr == NULL) error(3, word);
}

//--------------------------------------------------------------------------------------------------------------------
// assign�ǂݍ��݁i2006.03.24 ���Ή��j
//--------------------------------------------------------------------------------------------------------------------
static void read_assign(FILE* rfp, int* assign_no)
{
	char str[STR_MAX]; //get_token�p
	int c;

	get_token(rfp, str, 0, 0);
	c = fgetc(rfp);
	while (c != 'b') c = fgetc(rfp);
	while ((c != '0') && (c != '1')) c = fgetc(rfp);
	if ((c == '0') || (c == '1')) {
		nl[n_net].n = n_net;
		nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(str) + 1));
		strcpy(nl[n_net].name, str);
		nl[n_net].n_in = 0;
		nl[n_net].n_out = 0;
		nl[n_net].in = NULL;
		nl[n_net].name_ins = NULL;
		nl[n_net].name_port = NULL;
		nl[n_net].out = NULL;
		if (c == '0') nl[n_net].type = GND;
		else if (c == '1') nl[n_net].type = ACC;
		while (c != ';') c = fgetc(rfp);

		n_net++;
	}
	else error(6, str);
}

//--------------------------------------------------------------------------------------------------------------------
// �\���G���[���
//--------------------------------------------------------------------------------------------------------------------
static void analyze_asyntax_error(void)
{
	int i;

	for (i = 0; i < n_net; i++) {
		//PI�o�b�e�B���O
		if ((parser_array[1][i] >= 1) && (nl[i].type == IN))	error(4, nl[i].name);
		//�����M���o�b�e�B���O
		else if (parser_array[1][i] >= 2) error(5, nl[i].name);;
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �n�b�V���l���Z
//--------------------------------------------------------------------------------------------------------------------
static int hash_key(char* word)
{
	unsigned int key[2] = { 0, 0 };
	int i, slen, no;
	slen = (int)strlen(word);
	for (i = 0; i < slen; i++) {
#ifdef HASH_MODE
		if (i % 2 == 0) {
			key[0] = 65599 * key[1] + word[i];
			no = 0;
		}
		else {
			key[1] = 65599 * key[0] + word[i];
			no = 1;
		}
#endif
#ifndef HASH_MODE
		key[0] += word[i];
		no = 0;
#endif
	}

	return (key[no] % HASH_SIZE);
}

//--------------------------------------------------------------------------------------------------------------------
// �n�b�V���\���
//--------------------------------------------------------------------------------------------------------------------
static void free_hash(void)
{
	Signal_Name_Hash* ptr;
	Signal_Name_Hash* temp;
	int i;

	for (i = 0; i < HASH_SIZE; i++) {
		ptr = hash_t[i];
		while (ptr != NULL) {
			temp = ptr;
			ptr = ptr->next;
			free(temp);
		}
		hash_t[i] = NULL;
	}
	free(hash_t);
}

//--------------------------------------------------------------------------------------------------------------------
// �M���ڑ�����
//--------------------------------------------------------------------------------------------------------------------
static void connection_signal(void)
{
	//�e�M�� n_in,n_out �ǂݍ���,�̈�m��
	alloc_nl_in_out();

	//�e�M���ڑ�
	connection_n_signal();

	//�O�����̓C���X�^���X��,�[�q���ݒ�
	set_pi_ins();

	//FF�z��}��
	set_ff_array();
}

//--------------------------------------------------------------------------------------------------------------------
// �e�M�� n_in,n_out �ǂݍ���,�̈�m��
//--------------------------------------------------------------------------------------------------------------------
static void alloc_nl_in_out(void)
{
	int i, j;

	//�e�M�� n_in �̓ǂݍ���
	for (i = 0; i < n_module; i++)
		nl[module_array[i]->out_nid].n_in = module_array[i]->m_n_in;

	//�e�M�� n_out �̓ǂݍ���
	for (i = 0; i < n_net; i++) nl[i].n_out = parser_array[0][i];
	for (i = 0; i < n_po; i++) //PO��FOUT���Ă���ꍇ
		if (parser_array[0][po[i]->n] >= 1) po[i]->n_out++;

	//�e�M�� in,out �̈�m��,������
	for (i = 0; i < n_net; i++) {
		if (nl[i].n_in != 0) {
			nl[i].in = (NLIST**)malloc(sizeof(NLIST*) * nl[i].n_in);
			for (j = 0; j < nl[i].n_in; j++) nl[i].in[j] = NULL;
		}
		if (nl[i].n_out != 0) {
			nl[i].out = (NLIST**)malloc(sizeof(NLIST*) * nl[i].n_out);
			for (j = 0; j < nl[i].n_out; j++) nl[i].out[j] = NULL;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �M���ڑ�
//--------------------------------------------------------------------------------------------------------------------
static void connection_n_signal(void)
{
	int po_fout_flag; //PO��FOUT�iYES:1 NO:0�j
	int i, j, k;

	//������
	po_fout_flag = 0;

	for (i = 0; i < n_module; i++) {
		for (j = 0; j < module_array[i]->m_n_in; j++) {
			//IN����FOUT���Ă��Ȃ��ꍇ
			if (nl[module_array[i]->in_nid[j]].n_out == 1) {
				connection_no_fout(i, j);
			}
			//IN����FOUT���Ă���ꍇ
			else if (nl[module_array[i]->in_nid[j]].n_out >= 2) {
				connection_yes_fout(i, j);
				//PO��FOUT���Ă��邩�`�F�b�N
				for (k = 0; k < n_po; k++) {
					if (nl[module_array[i]->in_nid[j]].n == po[k]->n) {
						//PO�z�������������
						po[k] = &nl[n_net];
						po_fout_flag = 1; break;
					}
				}
				//PO��FOUT���Ă���ꍇ
				if (po_fout_flag == 1) {
					connection_po_fout(i, j, k);
				}
			}
			po_fout_flag = 0;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// �M���ڑ� IN����FOUT���Ă��Ȃ��ꍇ
//--------------------------------------------------------------------------------------------------------------------
static void connection_no_fout(int m_no, int in_no)
{
	int no;

	//������
	no = 0;

	//IN���̏o�͐M��
	nl[module_array[m_no]->in_nid[in_no]].out[0] = &nl[module_array[m_no]->out_nid];
	//OUT���̓��͐M��
	while (nl[module_array[m_no]->out_nid].in[no] != NULL) no++;
	nl[module_array[m_no]->out_nid].in[no] = &nl[module_array[m_no]->in_nid[in_no]];
	//OUT����TYPE
	nl[module_array[m_no]->out_nid].type = module_array[m_no]->m_type;
	//OUT���̃C���X�^���X��
	nl[module_array[m_no]->out_nid].name_ins
		= (char*)malloc(sizeof(char) * (strlen(module_array[m_no]->m_name_ins) + 1));
	strcpy(nl[module_array[m_no]->out_nid].name_ins, module_array[m_no]->m_name_ins);
	//OUT���̒[�q��
	nl[module_array[m_no]->out_nid].name_port = (char*)malloc(sizeof(char) * (1 + 1));
	if (nl[module_array[m_no]->out_nid].type == DFF || nl[module_array[m_no]->out_nid].type == RDFF ||
		nl[module_array[m_no]->out_nid].type == DFFS || nl[module_array[m_no]->out_nid].type == RDFFS) {
		strcpy(nl[module_array[m_no]->out_nid].name_port, "Q");
	}
	else
		strcpy(nl[module_array[m_no]->out_nid].name_port, "Z");
}

//--------------------------------------------------------------------------------------------------------------------
// �M���ڑ� IN����FOUT���Ă���ꍇ
//--------------------------------------------------------------------------------------------------------------------
static void connection_yes_fout(int m_no, int in_no)
{
	char tmp[SPRINT_MAX]; //sprint�p
	//int no, i;
	int no;

	//������
	no = 0;

	//FOUT�쐬
	sprintf(tmp, "%s_%s_%c", nl[module_array[m_no]->in_nid[in_no]].name
		, nl[module_array[m_no]->out_nid].name
		, pin_name_array[in_no]);
	nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(tmp) + 1));
	strcpy(nl[n_net].name, tmp);
	nl[n_net].n = n_net;
	nl[n_net].type = FOUT;
	nl[n_net].n_in = 1;
	nl[n_net].n_out = 1;
	nl[n_net].in = (NLIST**)malloc(sizeof(NLIST*)); nl[n_net].in[0] = NULL;
	nl[n_net].out = (NLIST**)malloc(sizeof(NLIST*)); nl[n_net].out[0] = NULL;
	//����IN���̏o�͐M��
	while (nl[module_array[m_no]->in_nid[in_no]].out[no] != NULL) no++;
	nl[module_array[m_no]->in_nid[in_no]].out[no] = &nl[n_net];
	no = 0;
	//OUT���̓��͐M��
	while (nl[module_array[m_no]->out_nid].in[no] != NULL) no++;
	nl[module_array[m_no]->out_nid].in[no] = &nl[n_net];
	no = 0;
	//OUT����TYPE
	nl[module_array[m_no]->out_nid].type = module_array[m_no]->m_type;
	//FOUT�̓��͐M��
	nl[n_net].in[0] = &nl[module_array[m_no]->in_nid[in_no]];
	//FOUT�̏o�͐M��
	nl[n_net].out[0] = &nl[module_array[m_no]->out_nid];
	//FOUT�̃C���X�^���X��
	nl[n_net].name_ins
		= (char*)malloc(sizeof(char) * (strlen(module_array[m_no]->m_name_ins) + 1));
	strcpy(nl[n_net].name_ins, module_array[m_no]->m_name_ins);
	//FOUT�̒[�q��
	if (nl[module_array[m_no]->out_nid].type == DFF || nl[module_array[m_no]->out_nid].type == RDFF ||
		nl[module_array[m_no]->out_nid].type == DFFS || nl[module_array[m_no]->out_nid].type == RDFFS) {
		if (in_no == 0) {
			nl[n_net].name_port = (char*)malloc(sizeof(char) * (1 + 1));
			nl[n_net].name_port[0] = 'D'; nl[n_net].name_port[1] = '\0';
		}
		else if (in_no == 1) {
			if (nl[module_array[m_no]->out_nid].type == RDFF) {
				nl[n_net].name_port = (char*)malloc(sizeof(char) * (2 + 1));
				strcpy(nl[n_net].name_port, "CD");
			}
			else if (nl[module_array[m_no]->out_nid].type == DFFS || nl[module_array[m_no]->out_nid].type == RDFFS) {
				nl[n_net].name_port = (char*)malloc(sizeof(char) * (2 + 1));
				strcpy(nl[n_net].name_port, "TI");
			}
		}
		else if (in_no == 2) {
			nl[n_net].name_port = (char*)malloc(sizeof(char) * (2 + 1));
			strcpy(nl[n_net].name_port, "TE");
		}
		else if (in_no == 3) {
			nl[n_net].name_port = (char*)malloc(sizeof(char) * (2 + 1));
			strcpy(nl[n_net].name_port, "CD");
		}
	}
	else {
		nl[n_net].name_port = (char*)malloc(sizeof(char) * (1 + 1));
		nl[n_net].name_port[0] = pin_name_array[in_no];	nl[n_net].name_port[1] = '\0';
	}
	//OUT���̃C���X�^���X��
	nl[module_array[m_no]->out_nid].name_ins
		= (char*)malloc(sizeof(char) * (strlen(module_array[m_no]->m_name_ins) + 1));
	strcpy(nl[module_array[m_no]->out_nid].name_ins, module_array[m_no]->m_name_ins);
	//OUT���̒[�q��
	nl[module_array[m_no]->out_nid].name_port = (char*)malloc(sizeof(char) * (1 + 1));
	if (nl[module_array[m_no]->out_nid].type == DFF || nl[module_array[m_no]->out_nid].type == RDFF ||
		nl[module_array[m_no]->out_nid].type == DFFS || nl[module_array[m_no]->out_nid].type == RDFFS) {
		strcpy(nl[module_array[m_no]->out_nid].name_port, "Q");
	}
	else
		strcpy(nl[module_array[m_no]->out_nid].name_port, "Z");

	n_net++;
}

//--------------------------------------------------------------------------------------------------------------------
// �M���ڑ� PO��FOUT���Ă���ꍇ 2010.09.07�ύX
//--------------------------------------------------------------------------------------------------------------------
static void connection_po_fout(int m_no, int in_no, int po_no)
{
	char tmp[SPRINT_MAX]; //sprint�p
	int no;

	//������
	no = 0;

	//FOUT�쐬
	nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(nl[module_array[m_no]->in_nid[in_no]].name) + 1));
	strcpy(nl[n_net].name, nl[module_array[m_no]->in_nid[in_no]].name);
	nl[n_net].n = n_net;
	nl[n_net].type = FOUT;
	nl[n_net].n_in = 1;
	nl[n_net].in = (NLIST**)malloc(sizeof(NLIST*));
	nl[n_net].in[0] = NULL;
	nl[n_net].n_out = 0;
	//����IN���̏o�͐M��
	while (nl[module_array[m_no]->in_nid[in_no]].out[no] != NULL) no++;
	nl[module_array[m_no]->in_nid[in_no]].out[no] = &nl[n_net];
	free(nl[module_array[m_no]->in_nid[in_no]].name);
	sprintf(tmp, "%s_stem", nl[n_net].name);
	nl[module_array[m_no]->in_nid[in_no]].name = (char*)malloc(sizeof(char) * (strlen(tmp) + 1));
	strcpy(nl[module_array[m_no]->in_nid[in_no]].name, tmp);
	//FOUT�̓��͐M��
	nl[n_net].in[0] = &nl[module_array[m_no]->in_nid[in_no]];
	//FOUT�̃C���X�^���X��,�[�q��
	nl[n_net].name_ins
		= (char*)malloc(sizeof(char) * (strlen(nl[n_net].name) + 1));
	strcpy(nl[n_net].name_ins, nl[n_net].name);
	nl[n_net].name_port = NULL;

	n_net++;
}

//--------------------------------------------------------------------------------------------------------------------
// �O�����̓C���X�^���X��,�[�q���ݒ�
//--------------------------------------------------------------------------------------------------------------------
static void set_pi_ins(void)
{
	int i;

	for (i = 0; i < n_pi; i++) {
		pi[i]->name_ins = (char*)malloc(sizeof(char) * (strlen(pi[i]->name) + 1));
		strcpy(pi[i]->name_ins, pi[i]->name);
		pi[i]->name_port = NULL;
	}
}

//--------------------------------------------------------------------------------------------------------------------
// FF�z��}��
//--------------------------------------------------------------------------------------------------------------------
static void set_ff_array(void)
{
	int dff_no, dffs_no, rdff_no, rdffs_no;
	int i;

	//������
	dff_no = dffs_no = rdff_no = rdffs_no = 0;

	for (i = 0; i < n_net; i++) {
		switch (nl[i].type) {
		case DFF: dff[dff_no++] = &nl[i]; break;
		case DFFS: dffs[dffs_no++] = &nl[i]; break;
		case RDFF: rdff[rdff_no++] = &nl[i]; break;
		case RDFFS:	rdffs[rdffs_no++] = &nl[i]; break;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// 1�P��ǂ�+��
//--------------------------------------------------------------------------------------------------------------------
static int get_token(FILE* rfp, char* s, int mode, int* in_num)
{
	int aha_flag;     //isalpha���ʗp
	char c_in_num[3]; //���͐��ǂݍ��ݗp
	int i, c;

	//������
	i = aha_flag = 0;

	//�ꕶ���ǂ�
	c = fgetc(rfp);
	while ((c == ' ') || (c == '\t') || (c == '\n')) c = fgetc(rfp);
	if (c == EOF) return EOF;              //EOF�̏ꍇ

	//��؂蕶���܂œǂݑ����P����쐬
	if (mode == 0) {
		while ((c != '\t') && (c != '\n') && (c != ' ') && (c != ';') &&
			(c != '(') && (c != ')') && (c != ',') && (c != ':') && (c != '=') && (c != '.')) {
			s[i] = c;
			c = fgetc(rfp);
			i++;
		}
	}
	else if (mode == 1) {
		while ((c != '\t') && (c != '\n') && (c != ' ') && (c != ';') && (c != '[') && (c != ']') &&
			(c != '(') && (c != ')') && (c != ',') && (c != ':') && (c != '=') && (c != '.')) {
			s[i] = c;
			c = fgetc(rfp);
			i++;
		}
	}
	else if (mode == 2) {
		while ((c != '\t') && (c != '\n') && (c != ' ') && (c != ';') && (aha_flag = isalpha(c)) &&
			(c != '(') && (c != ')') && (c != ',') && (c != ':') && (c != '=') && (c != '.')) {
			s[i] = c;
			c = fgetc(rfp);
			i++;
		}
		if (aha_flag == 0) {
			c_in_num[0] = c;
			c = fgetc(rfp);
			if ((c != ' ') && (c != '\t')) { c_in_num[1] = c; c_in_num[2] = '\0'; }
			else c_in_num[1] = '\0';
			*in_num = atoi(c_in_num);
		}
	}
	s[i] = '\0';

	//�ǂ̋�؂蕶���܂œǂ񂾂���Ԃ�
	if ((c == ';') && (i != 0)) return 1;      //�Ō�̕������u;�v�̏ꍇ
	else if ((c == ';') && (i == 0)) return 2; //�Ō�̕������u;�v�ł��P�Ƃ̏ꍇ
	else if (s[0] == '\0') return -2;      //�����񂪍��Ă��Ȃ��ꍇ
	else return 0;                        //�ʏ�
}

//--------------------------------------------------------------------------------------------------------------------
// �G���[����
//--------------------------------------------------------------------------------------------------------------------
static void error(int code, char* s)
{
	fprintf(stderr, "ERROR : ");
	switch (code) {
	case 1:
		fprintf(stderr, "2���͈ȊO��EXOR,EXNOR�����݂��܂�. instans_name : %s\n", s);
		break;
	case 2:
		fprintf(stderr, "11���͈ȏ�͑Ή����Ă��܂���. �����ɂ�葦�Ή��H instans_name : %s\n", s);
		break;
	case 3:
		fprintf(stderr, "�M�� \"%s\" ���X���[���Ă��܂�. �o�b�t�@��}�����Ă�������.\n", s);
		fprintf(stderr, "        ��������, �M�� \"%s\" �̋L�q���ɉ��s�������Ă���\n", s);
		break;
	case 4:
		fprintf(stderr, "�O�����͐M�� \"%s\" ���o�b�e�B���O���Ă��܂�. �����ꂽ��H��ǂݍ��܂��Ȃ��ŉ������i�{�j\n", s);
		break;
	case 5:
		fprintf(stderr, "�M�� \"%s\" ���o�b�e�B���O���܂�. �����ꂽ��H��ǂݍ��܂��Ȃ��ŉ������i�{�j\n", s);
		break;
	case 6:
		fprintf(stderr, "assign����ACC,GND�̂ݑΉ����Ă��܂�. �M�� \"%s\" ���O�����͐錾�����铙�őΉ����Ă�������.\n", s);
		break;
	}

	exit(1);
}

//--------------------------------------------------------------------------------------------------------------------
// ���W���[���z��,�\����͔z�� ���
//--------------------------------------------------------------------------------------------------------------------
static void free_module_array(void)
{
	int i, j;

	//���W���[���z��
	for (i = 0; i < n_module; i++) {
		free(module_array[i]->m_out_name);
		free(module_array[i]->m_name_ins);
		free(module_array[i]->in_nid);
		for (j = 0; j < module_array[i]->m_n_in; j++)
			free(module_array[i]->m_in_name[j]);
		free(module_array[i]);
	}
	free(module_array);

	//�\����͔z��
	free(parser_array[0]);
	free(parser_array[1]);
}

#ifdef DEBUG_MA
//--------------------------------------------------------------------------------------------------------------------
// ���W���[���z��f�o�b�O
//--------------------------------------------------------------------------------------------------------------------
static void debug_ma(void)
{
	int i, j;

	printf("n_module : %d\n", n_module);
	for (i = 0; i < n_module; i++) {
		printf("module_no:%d ", i);
		printf("instans_name:%s ", module_array[i]->m_name_ins);
		printf("type:%d ", module_array[i]->m_type);
		printf("out_name:%s ", module_array[i]->m_out_name);
		printf("n_in:%d\n", module_array[i]->m_n_in);
		printf("in_name : ");
		for (j = 0; j < module_array[i]->m_n_in; j++) {
			printf("[%d]:%s ", j, module_array[i]->m_in_name[j]);
		}
		printf("\n");
	}
	printf("\n");
}
#endif

#ifdef DEBUG_NL
//--------------------------------------------------------------------------------------------------------------------
// �l�b�g���X�g�z��f�o�b�O
//--------------------------------------------------------------------------------------------------------------------
static void debug_nl(void)
{
	int i, j;

	printf("n_net : %d\n", n_net);
	printf("n_pi : %d ", n_pi);
	for (i = 0; i < n_pi; i++) {
		printf("pi[%d]:%s ", i, pi[i]->name);
	}
	printf("\n");
	printf("n_po : %d ", n_po);
	for (i = 0; i < n_po; i++) {
		printf("po[%d]:%s ", i, po[i]->name);
	}
	printf("\n");
	printf("n_dff : %d ", n_dff);
	for (i = 0; i < n_dff; i++) {
		printf("dff[%d]:%s ", i, dff[i]->name);
	}
	printf("\n");
	printf("n_rdff : %d ", n_rdff);
	for (i = 0; i < n_rdff; i++) {
		printf("rdff[%d]:%s ", i, rdff[i]->name);
	}
	printf("\n");
	printf("n_dffs : %d ", n_dffs);
	for (i = 0; i < n_dffs; i++) {
		printf("dffs[%d]:%s ", i, dffs[i]->name);
	}
	printf("\n");
	printf("n_rdffs : %d ", n_rdffs);
	for (i = 0; i < n_rdffs; i++) {
		printf("rdffs[%d]:%s ", i, rdffs[i]->name);
	}
	printf("\n\n");

	for (i = 0; i < n_net; i++) {
		printf("nl[%d] name : %s\n", nl[i].n, nl[i].name);
		printf("type : %d ins_name : %s port_name : %s\n", nl[i].type, nl[i].name_ins, nl[i].name_port);
		printf("n_in : %d ", nl[i].n_in);
		for (j = 0; j < nl[i].n_in; j++) {
			printf("in[%d]:%s ", j, nl[i].in[j]->name);
		}
		printf("\n");
		printf("n_out : %d ", nl[i].n_out);
		for (j = 0; j < nl[i].n_out; j++) {
			printf("out[%d]:%s ", j, nl[i].out[j]->name);
		}
		printf("\n");
	}
	printf("\n");
}
#endif
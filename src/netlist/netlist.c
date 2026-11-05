//--------------------------------------------------------------------------------------------------------------------
//ネットリスト
//ファイル名 : Netlist.c
//日付 : 2010/06/07
//設計者 : R.Inoue
//Ver : 3.0（以前のVer）
//--------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include"netlist.h"

//--------------------------------------------------------------------------------------------------------------------
// 構造体
//--------------------------------------------------------------------------------------------------------------------
#define STR_MAX 1024   //get_token用
#define HASH_SIZE 5003 //ハッシュサイズ
#define N_PIN_NAME 10  //通常ゲートの入力ピン名（A〜J）の数

//モジュール配列メンバー
typedef struct _Module_Array_MEM_Format_ {
	char* m_name_ins; //インスタンス名
	int m_type;	      //タイプ
	int m_n_in;	      //入力数
	char* m_out_name; //出力名
	char** m_in_name; //入力名配列
	// exe. <通常>  A:[0], B:[1],…, J:[9]
	//      <DFF >  D:[0]
	//      <RDFF>  D:[0], CD(リセット):[1]
	//      <DFFS>  D:[0], TI(スキャンイン):[1], TE(スキャンイネーブル):[2]
	//      <RDFFS> D:[0], TI(スキャンイン):[1], TE(スキャンイネーブル):[2], CD(リセット):[3]
	int out_nid;    //出力信号のネットリストID
	int* in_nid;    //入力信号のネットリストID配列
} Module_Array_MEM;

//信号名ハッシュメンバー
typedef struct _Signal_Name_Hash_Format_ {
	int nid;        //ネットリストID
	struct _Signal_Name_Hash_Format_* next;
} Signal_Name_Hash;

//--------------------------------------------------------------------------------------------------------------------
// 静的変数
//--------------------------------------------------------------------------------------------------------------------
//パーサ配列
static int* parser_array[2]; //for_in:[0], for_out[1]

//ピン名配列（10ピンまで対応 04/03/16）
static const char pin_name_array[N_PIN_NAME] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J' };

//モジュール配列
static Module_Array_MEM** module_array = NULL;

//信号名ハッシュテーブル
static Signal_Name_Hash** hash_t = NULL;

//モジュール数
static int n_module = 0;

//総信号線数
static int nl_num = 0;

//--------------------------------------------------------------------------------------------------------------------
// プロトタイプ宣言
//--------------------------------------------------------------------------------------------------------------------
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
static void read_module_type(FILE*, char*, size_t, int);
static void read_ins(FILE*, int);
static void read_pin(FILE*, int);
static void alloc_nl(void);
static void parser(FILE*);
static void alloc_hash_table(void);
static void store_signal(void);
static void store_pi_po(FILE*);
static void read_pi_po(FILE*, int*, int);
static void hash_search_pi_po(char*, int*, int);
static void analyze_asyntax_error(void);
static int  hash_key(const char*);
static void free_hash(void);
static void connection_signal(void);
static void connection_no_fout(int, int);
static void connection_yes_fout(int, int);
static void connection_po_fout(int, int);
static void alloc_nl_in_out(void);
static void connection_n_signal(void);
static char* format_signal_name(const char*, ...);
static int  get_token(FILE*, char*, size_t, int, int*);
static int  statement_token(FILE*, char*, size_t, int);
static void skip_statement(FILE*);
static void error(int, const char*);
static void free_module_array(void);

//--------------------------------------------------------------------------------------------------------------------
// グローバル関数
//--------------------------------------------------------------------------------------------------------------------
int read_nl(char* v_name)
{
	FILE* r_fp; //ネットリストファイルポインタ

	//ファイルオープン
	file_open(&r_fp, v_name);

	//グローバル変数初期化
	init_global();

	//ネットリスト読み込み
	read_net(r_fp);

	//ファイルクローズ
	fclose(r_fp);

	return 0;
}

//--------------------------------------------------------------------------------------------------------------------
// ローカル関数
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
// ファイルオープン
//--------------------------------------------------------------------------------------------------------------------
static void file_open(FILE** r_fp, char* v_name)
{
	if ((*r_fp = fopen(v_name, "r")) == (FILE*)NULL) {
		fprintf(stderr, "ファイルオープン失敗(%s)\n", v_name);
		exit(EXIT_FAILURE);
	}
}

//--------------------------------------------------------------------------------------------------------------------
// 文の終わり（;）まで読み飛ばす。; の前にEOFになった場合は途中で切れたネットリストとしてエラー
//--------------------------------------------------------------------------------------------------------------------
static void skip_statement(FILE* rfp)
{
	int c;
	while ((c = getc(rfp)) != ';') {
		if (c == EOF) error(7, "");
	}
}

//--------------------------------------------------------------------------------------------------------------------
// 文の途中のトークンを読む。文の途中でEOFになった場合はエラー（get_tokenはEOFを繰り返し返すため無限ループになる）
//--------------------------------------------------------------------------------------------------------------------
static int statement_token(FILE* rfp, char* s, size_t capacity, int mode)
{
	int re_no = get_token(rfp, s, capacity, mode, 0);
	if (re_no == EOF) error(7, "");
	return re_no;
}

//--------------------------------------------------------------------------------------------------------------------
// グローバル変数初期化
//--------------------------------------------------------------------------------------------------------------------
static void init_global(void)
{
	n_net = 0;    //信号線数
	n_pi = 0;     //外部入力数
	n_po = 0;     //外部出力数
	n_dff = 0;    //DFF数
	n_rdff = 0;   //RDFF数
	n_dffs = 0;   //DFFS数
	n_rdffs = 0;  //RDFFS数
    n_ppi = n_ppo = 0;
    ppi = ppo = NULL;
}

//--------------------------------------------------------------------------------------------------------------------
// ネットリスト読み込み処理
//--------------------------------------------------------------------------------------------------------------------
static void read_net(FILE* rfp)
{
	//ネットリストファイル読み込み
	read_file(rfp);

	//ネットリスト配列、グローバル変数、構造体入力配列 領域確保、初期化
	alloc_nl();

	//パーサ実行
	parser(rfp);

	//ハッシュ構造体解放
	free_hash();

	//信号接続
	connection_signal();

	//モジュール配列、構造体入力配列 解放
	free_module_array();
}

//--------------------------------------------------------------------------------------------------------------------
// ネットリストファイルの読み込み
//--------------------------------------------------------------------------------------------------------------------
static void read_file(FILE* rfp)
{
	//モジュール数, PI数, PO数算出 & module情報取得
	read_1st(rfp);

	//モジュール配列領域確保, 初期化
	alloc_module_array();

	//各モジュール読み込み
	read_2nd(rfp);
}

//--------------------------------------------------------------------------------------------------------------------
// モジュール数, PI数, PO数算出 & module情報取得
//--------------------------------------------------------------------------------------------------------------------
static void read_1st(FILE* rfp)
{
	char str[STR_MAX];

	while (get_token(rfp, str, sizeof(str), 1, 0) != EOF) {
		if (!strcmp(str, "module") || !strcmp(str, "wire")) skip_statement(rfp);
		//assign は未対応。読み飛ばすと信号が未駆動のまま解析されるためエラーにする
		else if (!strcmp(str, "assign")) error(9, "");
		else if (!strcmp(str, "endmodule")) { continue; }
		//PI数算出
		else if (!strcmp(str, "input")) calc_pi_po(rfp, 0);
		//PO数算出
		else if (!strcmp(str, "output")) calc_pi_po(rfp, 1);
		//モジュール数算出
		else {
			n_module++;
			skip_statement(rfp);
		}
	}
	rewind(rfp);
}

//--------------------------------------------------------------------------------------------------------------------
// PI数, PO数算出
//--------------------------------------------------------------------------------------------------------------------
static void calc_pi_po(FILE* rfp, int mode)
{
	char str[STR_MAX]; //get_token用
	int bit_1, bit_2;  //ビット指定がある場合のビット数計算用
	int re_no;         //戻り値
	int num;
	int c;

	//初期化
	re_no = num = 0;

	c = fgetc(rfp);
	while ((c == ' ') || (c == '\t')) c = fgetc(rfp);
	//bit指定あり
	if (c == '[') {
		statement_token(rfp, str, sizeof(str), 1); bit_1 = atoi(str);
		statement_token(rfp, str, sizeof(str), 1); bit_2 = atoi(str);
		re_no = statement_token(rfp, str, sizeof(str), 0);
		while ((re_no != 1) && (re_no != 2)) {
			num++;
			re_no = statement_token(rfp, str, sizeof(str), 0);
		}
		if (re_no == 1) num++;
		if (mode == 0) n_pi += (num * (abs(bit_1 - bit_2) + 1));
		else if (mode == 1) n_po += (num * (abs(bit_1 - bit_2) + 1));
	}
	//bitなし
	else {
		ungetc(c, rfp);
		re_no = statement_token(rfp, str, sizeof(str), 0);
		while ((re_no != 1) && (re_no != 2)) {
				//CLK読み飛ばし
			if (!strcmp(str, "CLK") || !strcmp(str, "clk")) {
				re_no = statement_token(rfp, str, sizeof(str), 0);
				continue;
			}
			if ((re_no == 0) && (mode == 0)) n_pi++;
			else if ((re_no == 0) && (mode == 1)) n_po++;
			re_no = statement_token(rfp, str, sizeof(str), 0);
		}
		//CLK読み飛ばし（CLKでなかった場合実行）
		if (strcmp(str, "CLK") && strcmp(str, "clk")) {
			if ((re_no == 1) && (mode == 0)) n_pi++;
			else if ((re_no == 1) && (mode == 1)) n_po++;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// モジュール配列領域確保, 初期化
//--------------------------------------------------------------------------------------------------------------------
static void alloc_module_array(void)
{
	int i;

	module_array = (Module_Array_MEM**)malloc(sizeof(Module_Array_MEM*) * n_module);
	for (i = 0; i < n_module; i++)
		module_array[i] = (Module_Array_MEM*)calloc(1, sizeof(Module_Array_MEM)); //未接続ピンをNULLで検出する
}

//--------------------------------------------------------------------------------------------------------------------
// 各モジュールの読み込み
//--------------------------------------------------------------------------------------------------------------------
static void read_2nd(FILE* rfp)
{
	int in_num; //入力数読み込み用
	int m_no;   //現在読んでいるモジュール番号
	char str[STR_MAX];

	//初期化
	in_num = m_no = 0;

	while (get_token(rfp, str, sizeof(str), 2, &in_num) != EOF) {
		if (!strcmp(str, "module") || !strcmp(str, "input") ||
			!strcmp(str, "output") || !strcmp(str, "wire")) skip_statement(rfp);
		else if (!strcmp(str, "endmodule")) { continue; }
		else {
				//入力数の読み込み
			read_n_in(in_num, str, m_no);
				//入力信号名、入力信号ネットリストID配列の領域確保
			alloc_in_name(m_no);
				//モジュールタイプ
			read_module_type(rfp, str, sizeof(str), m_no);
				//インスタンス名の読み込み
			read_ins(rfp, m_no);
				//端子、信号名の読み込み
			read_pin(rfp, m_no);

			in_num = 0;
			m_no++;
			skip_statement(rfp);
		}
	}
	rewind(rfp);
}

//--------------------------------------------------------------------------------------------------------------------
// 入力数の読み込み
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
// 入力信号名,入力ハッシュポインタ配列の領域確保
//--------------------------------------------------------------------------------------------------------------------
static void alloc_in_name(int m_no)
{
	module_array[m_no]->m_in_name = (char**)calloc((size_t)module_array[m_no]->m_n_in, sizeof(char*));
	module_array[m_no]->in_nid = (int*)malloc(sizeof(int) * module_array[m_no]->m_n_in);
}

//--------------------------------------------------------------------------------------------------------------------
// モジュールタイプの読み込み
//--------------------------------------------------------------------------------------------------------------------
static void read_module_type(FILE* rfp, char* str, size_t capacity, int m_no)
{
	//ゲート名（末尾の入力数を除いた英字部分）を完全一致で判定する
	static const struct { const char* name; int type; } gate_types[] = {
		{ "AND", AND }, { "NAND", NAND }, { "OR", OR }, { "NOR", NOR },
		{ "BUF", BUF }, { "INV", INV }, { "EXOR", EXOR }, { "EXNOR", EXNOR },
		{ "DFF", DFF }, { "RDFF", RDFF }, { "DFFS", DFFS }, { "RDFFS", RDFFS }
	};
	size_t k;
	for (k = 0; k < sizeof(gate_types) / sizeof(gate_types[0]); k++) {
		if (!strcmp(str, gate_types[k].name)) break;
	}
	//error : 未対応のゲート種別
	if (k == sizeof(gate_types) / sizeof(gate_types[0])) error(8, str);

	module_array[m_no]->m_type = gate_types[k].type;
	switch (module_array[m_no]->m_type) {
	case DFF:   n_dff++;   break;
	case RDFF:  n_rdff++;  break;
	case DFFS:  n_dffs++;  break;
	case RDFFS: n_rdffs++; break;
	case EXOR:
	case EXNOR:
		//error : 2入力以外のEXOR, EXNORが存在する場合
		if (module_array[m_no]->m_n_in != 2) {
			statement_token(rfp, str, capacity, 0);
			error(1, str);
		}
		break;
	}
}

//--------------------------------------------------------------------------------------------------------------------
// インスタンス名の読み込み
//--------------------------------------------------------------------------------------------------------------------
static void read_ins(FILE* rfp, int m_no)
{
	char str[STR_MAX]; //get_token用

	statement_token(rfp, str, sizeof(str), 0);
	module_array[m_no]->m_name_ins = (char*)malloc(sizeof(char) * (strlen(str) + 1));
	strcpy(module_array[m_no]->m_name_ins, str);
}

//--------------------------------------------------------------------------------------------------------------------
// 端子,信号名の読み込み
//--------------------------------------------------------------------------------------------------------------------
/* 入力ピンへ信号名を設定する。範囲外・重複したピンはエラー（配列外書き込みを防ぐ）。 */
static void set_input_pin(int m_no, int index, const char* signal)
{
	Module_Array_MEM* module = module_array[m_no];
	if (index < 0 || index >= module->m_n_in || module->m_in_name[index] != NULL)
		error(2, module->m_name_ins);
	module->m_in_name[index] = format_signal_name("%s", signal);
}

static void read_pin(FILE* rfp, int m_no)
{
	char str[STR_MAX]; //get_token用
	int i, j;
	int type = module_array[m_no]->m_type;

	for (i = 0; i < module_array[m_no]->m_n_in + 1; i++) { //出力数分+1
		while (statement_token(rfp, str, sizeof(str), 0) != 0);
		//出力信号
		if (str[0] == 'Z' || str[0] == 'Q') {
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			if (module_array[m_no]->m_out_name != NULL) error(2, module_array[m_no]->m_name_ins);
			module_array[m_no]->m_out_name = format_signal_name("%s", str);
		}
		//各DFFのD（入力）
		else if (str[0] == 'D' && (type == DFF || type == RDFF || type == DFFS || type == RDFFS)) {
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			set_input_pin(m_no, 0, str);
		}
		//各DFFのCP（クロック）
		else if (!strcmp(str, "CP")) {
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			i--;
		}
		//RDFF,RDFFSのCD（リセット）
		else if (!strcmp(str, "CD")) {
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			set_input_pin(m_no, type == RDFF ? 1 : (type == RDFFS ? 3 : -1), str);
		}
		//DFFS,RDFFSのTI（スキャンイン）
		else if (!strcmp(str, "TI")) {
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			set_input_pin(m_no, (type == DFFS || type == RDFFS) ? 1 : -1, str);
		}
		//DFFS,RDFFSのTE（スキャンイネーブル）
		else if (!strcmp(str, "TE")) {
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			set_input_pin(m_no, (type == DFFS || type == RDFFS) ? 2 : -1, str);
		}
		//その他の入力信号（A〜J）
		else {
			for (j = 0; j < N_PIN_NAME && pin_name_array[j] != str[0]; j++);
			while (statement_token(rfp, str, sizeof(str), 0) != 0);
			set_input_pin(m_no, j < N_PIN_NAME ? j : -1, str);
		}
	}

	//error : 出力または入力ピンが接続されていない
	if (module_array[m_no]->m_out_name == NULL) error(2, module_array[m_no]->m_name_ins);
	for (j = 0; j < module_array[m_no]->m_n_in; j++) {
		if (module_array[m_no]->m_in_name[j] == NULL) error(2, module_array[m_no]->m_name_ins);
	}
}

//--------------------------------------------------------------------------------------------------------------------
// ネットリスト配列、グローバル変数、構造体入力配列 領域確保、初期化
//--------------------------------------------------------------------------------------------------------------------
static void alloc_nl(void)
{
	int i;

	//総信号線数計算
	nl_num = n_pi + n_po + n_module;
	for (i = 0; i < n_module; i++) nl_num += module_array[i]->m_n_in;

	//領域確保
	nl = (NLIST*)malloc(sizeof(NLIST) * nl_num);
	pi = (NLIST**)malloc(sizeof(NLIST*) * n_pi);
	po = (NLIST**)malloc(sizeof(NLIST*) * n_po);
	parser_array[0] = (int*)malloc(sizeof(int) * (nl_num));
	parser_array[1] = (int*)malloc(sizeof(int) * (nl_num));

	//初期化
	for (i = 0; i < nl_num; i++) {
		nl[i].type = -1;
		nl[i].n_in = 0;
		nl[i].in = (NLIST**)NULL;
		nl[i].n_out = 0;
		nl[i].out = (NLIST**)NULL;
        nl[i].peer_1t = NULL;
		parser_array[0][i] = 0;
		parser_array[1][i] = 0;
	}
}

//--------------------------------------------------------------------------------------------------------------------
// パーサ実行
//--------------------------------------------------------------------------------------------------------------------
static void parser(FILE* rfp)
{
	//ハッシュ構造体領域確保
	alloc_hash_table();

	//信号名の格納
	store_signal();

	//PI,PO読み込み
	store_pi_po(rfp);

	//パーサエラー確認
	analyze_asyntax_error();
}

//--------------------------------------------------------------------------------------------------------------------
// ハッシュ構造体領域確保
//--------------------------------------------------------------------------------------------------------------------
static void alloc_hash_table(void)
{
	int i;

	hash_t = (Signal_Name_Hash**)malloc(sizeof(Signal_Name_Hash*) * HASH_SIZE);
	for (i = 0; i < HASH_SIZE; i++) hash_t[i] = NULL;
}

//--------------------------------------------------------------------------------------------------------------------
// 信号名の格納
//--------------------------------------------------------------------------------------------------------------------
static void store_signal(void)
{
	Signal_Name_Hash* ptr;
	int hash_value; //ハッシュ値
	int i, j;

	for (i = 0; i < n_module; i++) {
		//入力
		for (j = 0; j < module_array[i]->m_n_in; j++) {
			hash_value = hash_key(module_array[i]->m_in_name[j]);
			//探索
			ptr = hash_t[hash_value];
			while (ptr != NULL) {
				if (!strcmp(nl[ptr->nid].name, module_array[i]->m_in_name[j])) {
						//パーサ配列インクリメント
					parser_array[0][ptr->nid] ++;
						//入力信号ネットリストID配列挿入
					module_array[i]->in_nid[j] = ptr->nid;
					break;
				}
				ptr = ptr->next;
			}
			//新規作成
			if (ptr == NULL) {
					//ハッシュ構造体挿入
				ptr = (Signal_Name_Hash*)malloc(sizeof(Signal_Name_Hash));
				ptr->next = hash_t[hash_value];
				hash_t[hash_value] = ptr;
				ptr->nid = n_net;
					//ネットリスト挿入
				nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(module_array[i]->m_in_name[j]) + 1));
				strcpy(nl[n_net].name, module_array[i]->m_in_name[j]);
					nl[n_net].n = n_net; //ネットリストID
					//パーサ配列セット
				parser_array[0][n_net] = 1;
					//入力信号ネットリストID配列挿入
				module_array[i]->in_nid[j] = n_net;

				n_net++;
			}
		}
		//出力
		hash_value = hash_key(module_array[i]->m_out_name);
		//探索
		ptr = hash_t[hash_value];
		while (ptr != NULL) {
			if (!strcmp(nl[ptr->nid].name, module_array[i]->m_out_name)) {
					//パーサ配列インクリメント
				parser_array[1][ptr->nid] ++;
					//出力信号ネットリストID挿入
				module_array[i]->out_nid = ptr->nid;
				break;
			}
			ptr = ptr->next;
		}
		//新規作成
		if (ptr == NULL) {
			//ハッシュ構造体挿入
			ptr = (Signal_Name_Hash*)malloc(sizeof(Signal_Name_Hash));
			ptr->next = hash_t[hash_value];
			hash_t[hash_value] = ptr;
			ptr->nid = n_net;
			//ネットリスト挿入
			nl[n_net].name = (char*)malloc(sizeof(char) * (strlen(module_array[i]->m_out_name) + 1));
			strcpy(nl[n_net].name, module_array[i]->m_out_name);
			nl[n_net].n = n_net; //ネットリストID
			//パーサ配列セット
			parser_array[1][n_net] = 1;
			//出力信号ネットリストID挿入
			module_array[i]->out_nid = n_net;

			n_net++;
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// PI,PO読み込み処理
//--------------------------------------------------------------------------------------------------------------------
static void store_pi_po(FILE* rfp)
{
	char str[STR_MAX]; //get_token用
	int pi_no;
	int po_no;

	//初期化
	pi_no = po_no = 0;

	while (get_token(rfp, str, sizeof(str), 0, 0) != EOF) {
		if (!strcmp(str, "input")) read_pi_po(rfp, &pi_no, 0);
		else if (!strcmp(str, "output")) read_pi_po(rfp, &po_no, 1);
		else if (!strcmp(str, "endmodule")) { continue; }
		else skip_statement(rfp); //module, wire, ゲート
	}
}

//--------------------------------------------------------------------------------------------------------------------
// PI,PO読み込み
//--------------------------------------------------------------------------------------------------------------------
static void read_pi_po(FILE* rfp, int* no, int mode)
{
	char str[STR_MAX];    //get_token用
	int bit_1, bit_2;     //ビット指定がある場合のビット数計算用
	int n_bit;            //ビット数
	int start_bit;        //最初のビット番号
	int re_no;            //戻り値
	int i, c;

	//初期化
	re_no = 0;

	c = fgetc(rfp);
	while ((c == ' ') || (c == '\t')) c = fgetc(rfp);
	//bit指定あり
	if (c == '[') {
		statement_token(rfp, str, sizeof(str), 1); bit_1 = atoi(str);
		statement_token(rfp, str, sizeof(str), 1); bit_2 = atoi(str);
		n_bit = abs(bit_1 - bit_2) + 1;
		re_no = statement_token(rfp, str, sizeof(str), 0);
		//最初のビット番号
		if (bit_1 <= bit_2) start_bit = bit_1;
		else start_bit = bit_2;
		//複数ポート宣言
		while ((re_no != 1) && (re_no != 2)) {
			for (i = 0; i < n_bit; i++) {
				char* signal_name = format_signal_name("%s[%d]", str, i + start_bit);
				hash_search_pi_po(signal_name, no, mode);
				free(signal_name);
			}
			re_no = statement_token(rfp, str, sizeof(str), 0);
		}
		//単一ポート宣言
		if (re_no == 1) {
			for (i = 0; i < n_bit; i++) {
				char* signal_name = format_signal_name("%s[%d]", str, i + start_bit);
				hash_search_pi_po(signal_name, no, mode);
				free(signal_name);
			}
		}
	}
	//bitなし
	else {
		ungetc(c, rfp);
		re_no = statement_token(rfp, str, sizeof(str), 0);
		while ((re_no != 1) && (re_no != 2)) {
				//CLK読み飛ばし
			if (!strcmp(str, "CLK") || !strcmp(str, "clk")) {
				re_no = statement_token(rfp, str, sizeof(str), 0);
				continue;
			}
			if (re_no == 0) hash_search_pi_po(str, no, mode);
			re_no = statement_token(rfp, str, sizeof(str), 0);
		}
		//CLK読み飛ばし（CLKでなかった場合実行）
		if (strcmp(str, "CLK") && strcmp(str, "clk")) {
			if (re_no == 1) hash_search_pi_po(str, no, mode);
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// PI,PO ハッシュ探索
//--------------------------------------------------------------------------------------------------------------------
static void hash_search_pi_po(char* word, int* no, int mode)
{
	int hash_value;   //ハッシュ値
	Signal_Name_Hash* ptr;

	hash_value = hash_key(word);
	ptr = hash_t[hash_value];
	while (ptr != NULL) {
		if (!strcmp(nl[ptr->nid].name, word)) {
			//PI
			if (mode == 0) {
				//TYPE
				nl[ptr->nid].type = IN;
					//PI配列
				pi[(*no)++] = &nl[ptr->nid];
			}
			//PO
			else if (mode == 1) {
					//PO配列
				po[(*no)++] = &nl[ptr->nid];
			}
			break;
		}
		ptr = ptr->next;
	}
	//error : 信号名スルーしている場合
	if (ptr == NULL) error(3, word);
}

//--------------------------------------------------------------------------------------------------------------------
// パーサエラー確認
//--------------------------------------------------------------------------------------------------------------------
static void analyze_asyntax_error(void)
{
	int i;

	for (i = 0; i < n_net; i++) {
		//PIバッティング
		if ((parser_array[1][i] >= 1) && (nl[i].type == IN))	error(4, nl[i].name);
		//複数信号バッティング
		else if (parser_array[1][i] >= 2) error(5, nl[i].name);
	}
}

//--------------------------------------------------------------------------------------------------------------------
// ハッシュ値演算
//--------------------------------------------------------------------------------------------------------------------
static int hash_key(const char* word)
{
	unsigned int key[2] = { 0, 0 };
	int no = 0;
	for (int i = 0; word[i] != '\0'; i++) {
		no = i % 2;
		key[no] = 65599 * key[no ^ 1] + word[i];
	}

	return (int)(key[no] % HASH_SIZE);
}

//--------------------------------------------------------------------------------------------------------------------
// ハッシュ構造体解放
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
// 信号接続処理
//--------------------------------------------------------------------------------------------------------------------
static void connection_signal(void)
{
	//各信号 n_in,n_out 読み込み,領域確保
	alloc_nl_in_out();

	//各信号接続
	connection_n_signal();
}

//--------------------------------------------------------------------------------------------------------------------
// 各信号 n_in,n_out 読み込み,領域確保
//--------------------------------------------------------------------------------------------------------------------
static void alloc_nl_in_out(void)
{
	int i, j;

	//各信号 n_in の読み込み
	for (i = 0; i < n_module; i++)
		nl[module_array[i]->out_nid].n_in = module_array[i]->m_n_in;

	//各信号 n_out の読み込み
	for (i = 0; i < n_net; i++) nl[i].n_out = parser_array[0][i];
	for (i = 0; i < n_po; i++) //POにFOUTしている場合
		if (parser_array[0][po[i]->n] >= 1) po[i]->n_out++;

	//各信号 in,out 領域確保、初期化
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
// 信号接続
//--------------------------------------------------------------------------------------------------------------------
static void connection_n_signal(void)
{
	int i, j, k;

	for (i = 0; i < n_module; i++) {
		for (j = 0; j < module_array[i]->m_n_in; j++) {
				//INからFOUTしていない場合
			if (nl[module_array[i]->in_nid[j]].n_out == 1) {
				connection_no_fout(i, j);
			}
				//INからFOUTしている場合
			else if (nl[module_array[i]->in_nid[j]].n_out >= 2) {
				connection_yes_fout(i, j);
					//POにFOUTしているかチェック
				for (k = 0; k < n_po; k++) {
					if (nl[module_array[i]->in_nid[j]].n == po[k]->n) {
									//PO配列を書き換え
						po[k] = &nl[n_net];
						connection_po_fout(i, j);
						break;
					}
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------------------
// 信号接続 INからFOUTしていない場合
//--------------------------------------------------------------------------------------------------------------------
static void connection_no_fout(int m_no, int in_no)
{
	int no;

	//初期化
	no = 0;

	//IN側の出力信号
	nl[module_array[m_no]->in_nid[in_no]].out[0] = &nl[module_array[m_no]->out_nid];
	//OUT側の入力信号
	while (nl[module_array[m_no]->out_nid].in[no] != NULL) no++;
	nl[module_array[m_no]->out_nid].in[no] = &nl[module_array[m_no]->in_nid[in_no]];
	//OUT側のTYPE
	nl[module_array[m_no]->out_nid].type = module_array[m_no]->m_type;
}

//--------------------------------------------------------------------------------------------------------------------
// 信号接続 INからFOUTしている場合
//--------------------------------------------------------------------------------------------------------------------
static void connection_yes_fout(int m_no, int in_no)
{
	int no;

	//初期化
	no = 0;

	//FOUT作成
	nl[n_net].name = format_signal_name("%s_%s_%c", nl[module_array[m_no]->in_nid[in_no]].name
		, nl[module_array[m_no]->out_nid].name
		, pin_name_array[in_no]);
	nl[n_net].n = n_net;
	nl[n_net].type = FOUT;
	nl[n_net].n_in = 1;
	nl[n_net].n_out = 1;
	nl[n_net].in = (NLIST**)malloc(sizeof(NLIST*)); nl[n_net].in[0] = NULL;
	nl[n_net].out = (NLIST**)malloc(sizeof(NLIST*)); nl[n_net].out[0] = NULL;
	//元のIN側の出力信号
	while (nl[module_array[m_no]->in_nid[in_no]].out[no] != NULL) no++;
	nl[module_array[m_no]->in_nid[in_no]].out[no] = &nl[n_net];
	no = 0;
	//OUT側の入力信号
	while (nl[module_array[m_no]->out_nid].in[no] != NULL) no++;
	nl[module_array[m_no]->out_nid].in[no] = &nl[n_net];
	//OUT側のTYPE
	nl[module_array[m_no]->out_nid].type = module_array[m_no]->m_type;
	//FOUTの入力信号
	nl[n_net].in[0] = &nl[module_array[m_no]->in_nid[in_no]];
	//FOUTの出力信号
	nl[n_net].out[0] = &nl[module_array[m_no]->out_nid];

	n_net++;
}

//--------------------------------------------------------------------------------------------------------------------
// 信号接続 POにFOUTしている場合 2010.09.07変更
//--------------------------------------------------------------------------------------------------------------------
static void connection_po_fout(int m_no, int in_no)
{
	int no;

	//初期化
	no = 0;

	//FOUT作成
	nl[n_net].name = format_signal_name("%s", nl[module_array[m_no]->in_nid[in_no]].name);
	nl[n_net].n = n_net;
	nl[n_net].type = FOUT;
	nl[n_net].n_in = 1;
	nl[n_net].in = (NLIST**)malloc(sizeof(NLIST*));
	nl[n_net].in[0] = NULL;
	nl[n_net].n_out = 0;
	//元のIN側の出力信号
	while (nl[module_array[m_no]->in_nid[in_no]].out[no] != NULL) no++;
	nl[module_array[m_no]->in_nid[in_no]].out[no] = &nl[n_net];
	free(nl[module_array[m_no]->in_nid[in_no]].name);
	nl[module_array[m_no]->in_nid[in_no]].name = format_signal_name("%s_stem", nl[n_net].name);
	//FOUTの入力信号
	nl[n_net].in[0] = &nl[module_array[m_no]->in_nid[in_no]];

	n_net++;
}

/* 生成名は入力名より長くなるため、固定長の作業配列を使わず必要量を確保する。 */
static char* format_signal_name(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (length < 0) {
		fprintf(stderr, "信号名の生成に失敗しました.\n");
		exit(EXIT_FAILURE);
	}

	char* name = (char*)malloc((size_t)length + 1);
	if (name == NULL) {
		fprintf(stderr, "信号名のメモリ確保に失敗しました.\n");
		exit(EXIT_FAILURE);
	}
	va_start(args, format);
	int written = vsnprintf(name, (size_t)length + 1, format, args);
	va_end(args);
	if (written != length) {
		free(name);
		fprintf(stderr, "信号名の生成に失敗しました.\n");
		exit(EXIT_FAILURE);
	}
	return name;
}

//--------------------------------------------------------------------------------------------------------------------
// 1単語読み+α（終端文字の分も残して書き込む）
//--------------------------------------------------------------------------------------------------------------------
static int get_token(FILE* rfp, char* s, size_t capacity, int mode, int* in_num)
{
	int aha_flag;     //isalpha結果用
	char c_in_num[3]; //入力数読み込み用
	int i, c;

	//初期化
	i = aha_flag = 0;

	//一文字読む
	c = fgetc(rfp);
	while ((c == ' ') || (c == '\t') || (c == '\n')) c = fgetc(rfp);
	if (c == EOF) return EOF;              //EOFの場合

	//区切り文字まで読み進み単語を作成
	if (mode == 0) {
		while ((c != EOF) && (c != '\t') && (c != '\n') && (c != ' ') && (c != ';') &&
			(c != '(') && (c != ')') && (c != ',') && (c != ':') && (c != '=') && (c != '.')) {
			if ((size_t)i + 1 >= capacity) {
				fprintf(stderr, "ネットリストのトークンが長すぎます（最大%zuバイト）.\n", capacity - 1);
				exit(EXIT_FAILURE);
			}
			s[i] = c;
			c = fgetc(rfp);
			i++;
		}
	}
	else if (mode == 1) {
		while ((c != EOF) && (c != '\t') && (c != '\n') && (c != ' ') && (c != ';') && (c != '[') && (c != ']') &&
			(c != '(') && (c != ')') && (c != ',') && (c != ':') && (c != '=') && (c != '.')) {
			if ((size_t)i + 1 >= capacity) {
				fprintf(stderr, "ネットリストのトークンが長すぎます（最大%zuバイト）.\n", capacity - 1);
				exit(EXIT_FAILURE);
			}
			s[i] = c;
			c = fgetc(rfp);
			i++;
		}
	}
	else if (mode == 2) {
		while ((c != EOF) && (c != '\t') && (c != '\n') && (c != ' ') && (c != ';') && (aha_flag = isalpha(c)) &&
			(c != '(') && (c != ')') && (c != ',') && (c != ':') && (c != '=') && (c != '.')) {
			if ((size_t)i + 1 >= capacity) {
				fprintf(stderr, "ネットリストのトークンが長すぎます（最大%zuバイト）.\n", capacity - 1);
				exit(EXIT_FAILURE);
			}
			s[i] = c;
			c = fgetc(rfp);
			i++;
		}
		if (aha_flag == 0) {
			c_in_num[0] = c;
			c = fgetc(rfp);
			if ((c != EOF) && (c != ' ') && (c != '\t')) { c_in_num[1] = c; c_in_num[2] = '\0'; }
			else c_in_num[1] = '\0';
			*in_num = atoi(c_in_num);
		}
	}
	s[i] = '\0';

	//どの区切り文字まで読んだかを返す
	if ((c == ';') && (i != 0)) return 1;      //最後の文字が「;」の場合
	else if ((c == ';') && (i == 0)) return 2; //最後の文字が「;」で単一の場合
	else if (s[0] == '\0') return -2;      //文字列が来ていない場合
	else return 0;                        //通常
}

//--------------------------------------------------------------------------------------------------------------------
// エラー処理
//--------------------------------------------------------------------------------------------------------------------
static void error(int code, const char* s)
{
	fprintf(stderr, "ERROR : ");
	switch (code) {
	case 1:
		fprintf(stderr, "2入力以外のEXOR,EXNORが存在します. instans_name : %s\n", s);
		break;
	case 2:
		fprintf(stderr, "入力ピンが不正です（A〜Jかつ入力数以内・重複なし・全ピン接続が必要）. instans_name : %s\n", s);
		break;
	case 3:
		fprintf(stderr, "信号 \"%s\" がスルーしています. バッファを挿入してください.\n", s);
		fprintf(stderr, "        また, 信号 \"%s\" の記述前に改行がされている\n", s);
		break;
	case 4:
		fprintf(stderr, "外部入力信号 \"%s\" がバッティングしています. 削除されたか？読み込まれないで以上です（本）\n", s);
		break;
	case 5:
		fprintf(stderr, "信号 \"%s\" がバッティングします. 削除されたか？読み込まれないで以上です（本）\n", s);
		break;
	case 7:
		fprintf(stderr, "ネットリストが途中で終わっています（文末の ';' がありません）.%s\n", s);
		break;
	case 8:
		fprintf(stderr, "未対応のゲート種別です. gate : %s\n", s);
		break;
	case 9:
		fprintf(stderr, "assign文には対応していません. 信号をBUFなどのゲートで記述してください.%s\n", s);
		break;
	}

	exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------------------------------------------------
// モジュール配列、構造体入力配列 解放
//--------------------------------------------------------------------------------------------------------------------
static void free_module_array(void)
{
	int i, j;

	//モジュール配列
	for (i = 0; i < n_module; i++) {
		free(module_array[i]->m_out_name);
		free(module_array[i]->m_name_ins);
		free(module_array[i]->in_nid);
		for (j = 0; j < module_array[i]->m_n_in; j++)
			free(module_array[i]->m_in_name[j]);
		free(module_array[i]->m_in_name);
		free(module_array[i]);
	}
	free(module_array);

	//パーサ配列
	free(parser_array[0]);
	free(parser_array[1]);
}

//--------------------------------------------------------------------------------------------------------------------
// ネットリスト配列解放
//--------------------------------------------------------------------------------------------------------------------
void free_netlist(void)
{
	int i;

	for (i = 0; i < n_net; i++) {
		free(nl[i].name);
		free(nl[i].in);
		free(nl[i].out);
		free(nl[i].consgc);  //正常回路のCNF（CreateConsGCで確保）
	}
	free(nl);
	free(pi);
	free(po);
	free(ppi);
	free(ppo);
}

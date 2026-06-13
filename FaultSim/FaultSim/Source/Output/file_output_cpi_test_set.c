//------------------------------------------------------------------------
//File name : file_output_cpi_test_set.c
//Date : 2013/11/19
//Designer : H.Yamazaki
//Ver : 0.01
//
//memo:
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>

#include	"../StandardHead.h"
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../option.h"
#include	"../XID/Xidentification.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void		file_outpuf_cpi_txt		(FILE*);
void		file_outpuf_cpi_stil	(FILE*, FILE*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
//#pragma warning ( disable : 4996 )

#define	DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------

//----------------------------------------------------------------------
// 静的変数
//----------------------------------------------------------------------

//--------------------------------------------------------------------
//関数名：file_outpuf_cpi_stil
//機能　：【CPI_XID用：stil形式】X抽出後テスト集合{0,1,X}のファイル出力
//引数　：st(出力用stilファイルポインタ), in(初期テスト集合.stil)
//戻り値：なし
//※増加したPINを拡張して出力
//--------------------------------------------------------------------
void	file_outpuf_cpi_stil(FILE* st, FILE* in){
	
	int		i,j;
	int		nval;
	char	str[1000000];
	char	*temp;

	//=======================================================
	// 初期テスト集合.stilのファイルポインタを先頭に戻す
	//=======================================================
	rewind(in);	

	//=======================================================
	// テストパターンのところまでinの内容を出力ファイル(st)にコピー
	//=======================================================
	while(fgets(str,1000000,in) !=NULL){

		//Ann {* fast_sequential *}まで来たらbreak
		if(strstr(str,"Ann {* fast_sequential *}")!=NULL){
			break;				
		}
		//Ann {* fast_sequential *}がある場所までコピペ
		else{
			fprintf(st,"%s",str);
		}
	}
	
	//=======================================================
	// 【テストパターン出力】1パターン目のみ特殊処理
	//=======================================================
	fprintf(st,"Ann {* fast_sequential *}\n");
	fprintf(st,"\"pattern 1\": Call \"load_unload\" { \n");

	//2行進む
	fgets(str,1000000,in);
	fgets(str,1000000,in);

	//pattern 0のtest_soの値を切り出す
	temp = strtok(str,";");

	//test_soをファイル書き込み
	fprintf(st,"%s; \"test_si\"=", temp);
	
	//-----------------------------------------------
	//1パターン目のPPI値を出力
	//-----------------------------------------------
	for(i=0; i<n_scan_ff; i++){
		if(ppi_tp[i].nl_id != -1){
			//X抽出後の正常値取得
			nval = Get_NBit(ppi_tp[i].sinput->xid_nval_t1, 0);

			//正常値==0
			if(nval == 0){
				fprintf(st, "0");
			}

			//正常値==1
			else if(nval == 1){
				fprintf(st, "1");
			}

			//正常値==X
			else if(nval == 3){
				fprintf(st, "N");
			}

			//その他(バグ)
			else{
				printf("ERROR: file_outpuf_saf_stil() \n");
				exit(-1);
			}
		}
	}
	fprintf(st,"; }\n");
	
	//-----------------------------------------------
	//_piをファイル書き込み
	//-----------------------------------------------
	fprintf(st,"   V { \"_pi\"=");

	//PI値出力
	for(i=0; i<n_se_input; i++){
		if(pi_tp[i].nl_id != -1){
			//X抽出後の正常値取得
			nval = Get_NBit(pi_tp[i].sinput->xid_nval_t1, 0);

			//正常値==0
			if(nval == 0){
				fprintf(st, "0");
			}

			//正常値==1
			else if(nval == 1){
				fprintf(st, "1");
			}

			//正常値==X
			else if(nval == 3){
				fprintf(st, "N");
			}

			//その他(バグ)
			else{
				printf("ERROR: file_outpuf_saf_stil() \n");
				exit(-1);
			}
		}
		//それ以外の場合
		else{
			fprintf(st, "0");
		}
	}

	fprintf(st,"; }\n");
	
	//-----------------------------------------------
	//1回目Call
	//-----------------------------------------------
	fprintf(st,"   Call \"capture_CLK\" { \n");
	fprintf(st,"      \"_pi\"=");
	//PI値出力
	for(i=0; i<n_se_input; i++){
		if(pi_tp[i].nl_id != -1){
			//X抽出後の正常値取得
			nval = Get_NBit(pi_tp[i].sinput->xid_nval_t1, 0);

			//正常値==0
			if(nval == 0){
				fprintf(st, "0");
			}

			//正常値==1
			else if(nval == 1){
				fprintf(st, "1");
			}

			//正常値==X
			else if(nval == 3){
				fprintf(st, "N");
			}

			//その他(バグ)
			else{
				printf("ERROR: file_outpuf_saf_stil() \n");
				exit(-1);
			}
		}
		//それ以外の場合
		else{
			fprintf(st, "0");
		}
	}
	fprintf(st,"; }\n");
	
	//-----------------------------------------------
	//2回目Call
	//-----------------------------------------------
	fprintf(st,"   Call \"capture_CLK\" { \n");
	fprintf(st,"      \"_pi\"=");
	//PI値出力
	for(i=0; i<n_se_input; i++){
		if(pi_tp[i].nl_id != -1){
			//X抽出後の正常値取得
			nval = Get_NBit(pi_tp[i].sinput->xid_nval_t1, 0);

			//正常値==0
			if(nval == 0){
				fprintf(st, "0");
			}

			//正常値==1
			else if(nval == 1){
				fprintf(st, "1");
			}

			//正常値==X
			else if(nval == 3){
				fprintf(st, "N");
			}

			//その他(バグ)
			else{
				printf("ERROR: file_outpuf_saf_stil() \n");
				exit(-1);
			}
		}
		//それ以外の場合
		else{
			fprintf(st, "0");
		}
	}
	//-----------------------------------------------
	//POの期待値(マスク)
	//-----------------------------------------------
	fprintf(st,"; \"_po\"=");
	for(j=0; j<(n_spo+1); j++){
		fprintf(st,"X");
	}
	fprintf(st,"; }\n");


	//=======================================================
	// 【テストパターン出力】2パターン目以降
	//=======================================================
	for(i=1; i<n_tp; i++){

		fprintf(st,"   Ann {* fast_sequential *}\n");
		fprintf(st,"   \"pattern %d\": Call \"load_unload\" { \n", i+1);
		
		//-----------------------------------------------
		//1パターン前のtest_soを出力
		//-----------------------------------------------
		fprintf(st,"      \"test_so\"=");

		for(j=0; j<n_scan_ff; j++){
			if(ppi_tp[j].nl_id != -1){
				//X抽出後のPPO出力期待値
				nval = Get_NBit(ppi_tp[j].sinput->in[0]->xid_nval_t2, i-1);

				//正常値==0
				if(nval == 0){
					fprintf(st, "L");
				}

				//正常値==1
				else if(nval == 1){
					fprintf(st, "H");
				}

				//正常値==X
				else if(nval == 3){
					fprintf(st, "N");
				}

				//その他(バグ)
				else{
					printf("ERROR: file_outpuf_saf_stil() \n");
					exit(-1);
				}
			}
		}
		//-----------------------------------------------
		//現在パターンのtest_siを出力
		//-----------------------------------------------
		fprintf(st,"; \"test_si\"=");

		for(j=0; j<n_scan_ff; j++){
			if(ppi_tp[j].nl_id != -1){
				//X抽出後のPPI出力期待値
				nval = Get_NBit(ppi_tp[j].sinput->xid_nval_t1, i);

				//正常値==0
				if(nval == 0){
					fprintf(st, "0");
				}

				//正常値==1
				else if(nval == 1){
					fprintf(st, "1");
				}

				//正常値==X
				else if(nval == 3){
					fprintf(st, "N");
				}

				//その他(バグ)
				else{
					printf("ERROR: file_outpuf_saf_stil() \n");
					exit(-1);
				}
			}
		}
		fprintf(st,"; }\n");
		
		//-----------------------------------------------
		//現在パターンの_piを出力
		//-----------------------------------------------
		fprintf(st,"   V { \"_pi\"=");
		//PI値出力
		for(j=0; j<n_se_input; j++){
			if(pi_tp[j].nl_id != -1){
				//X抽出後の正常値取得
				nval = Get_NBit(pi_tp[j].sinput->xid_nval_t1, i);

				//正常値==0
				if(nval == 0){
					fprintf(st, "0");
				}

				//正常値==1
				else if(nval == 1){
					fprintf(st, "1");
				}

				//正常値==X
				else if(nval == 3){
					fprintf(st, "N");
				}

				//その他(バグ)
				else{
					printf("ERROR: file_outpuf_saf_stil() \n");
					exit(-1);
				}
			}
			//それ以外の場合
			else{
				fprintf(st, "0");
			}
		}
		fprintf(st,"; }\n");

		
		//-----------------------------------------------
		//1回目Call
		//-----------------------------------------------
		fprintf(st,"   Call \"capture_CLK\" { \n");
		fprintf(st,"      \"_pi\"=");
		//PI値出力
		for(j=0; j<n_se_input; j++){
			if(pi_tp[j].nl_id != -1){
				//X抽出後の正常値取得
				nval = Get_NBit(pi_tp[j].sinput->xid_nval_t1, i);

				//正常値==0
				if(nval == 0){
					fprintf(st, "0");
				}

				//正常値==1
				else if(nval == 1){
					fprintf(st, "1");
				}

				//正常値==X
				else if(nval == 3){
					fprintf(st, "N");
				}

				//その他(バグ)
				else{
					printf("ERROR: file_outpuf_saf_stil() \n");
					exit(-1);
				}
			}
			//それ以外の場合
			else{
				fprintf(st, "0");
			}
		}
		fprintf(st,"; }\n");

		
		//-----------------------------------------------
		//2回目Call
		//-----------------------------------------------
		fprintf(st,"   Call \"capture_CLK\" { \n");
		fprintf(st,"      \"_pi\"=");
		//PI値出力
		for(j=0; j<n_se_input; j++){
			if(pi_tp[j].nl_id != -1){
				//X抽出後の正常値取得
				nval = Get_NBit(pi_tp[j].sinput->xid_nval_t1, i);

				//正常値==0
				if(nval == 0){
					fprintf(st, "0");
				}

				//正常値==1
				else if(nval == 1){
					fprintf(st, "1");
				}

				//正常値==X
				else if(nval == 3){
					fprintf(st, "N");
				}

				//その他(バグ)
				else{
					printf("ERROR: file_outpuf_saf_stil() \n");
					exit(-1);
				}
			}
			//それ以外の場合
			else{
				fprintf(st, "0");
			}
		}
		
		//-----------------------------------------------
		//POの期待値(マスク)
		//-----------------------------------------------
		fprintf(st,"; \"_po\"=");
		for(j=0; j<(n_spo+1); j++){
			fprintf(st,"X");
		}
		fprintf(st,"; }\n");
	}
	
	//=======================================================
	// 最終パターンのキャプチャ処理
	//=======================================================
	fprintf(st,"   Ann {* fast_sequential *}\n");
	fprintf(st,"   \"end %d unload\": Call \"load_unload\" { \n", i);
	fprintf(st,"      \"test_so\"=");

	for(j=0; j<n_scan_ff; j++){
		if(ppi_tp[j].nl_id != -1){
			//X抽出後のPPO出力期待値
			nval = Get_NBit(ppi_tp[j].sinput->in[0]->xid_nval_t2, i-1);

			//正常値==0
			if(nval == 0){
				fprintf(st, "L");
			}

			//正常値==1
			else if(nval == 1){
				fprintf(st, "H");
			}

			//正常値==X
			else if(nval == 3){
				fprintf(st, "N");
			}

			//その他(バグ)
			else{
				printf("ERROR: file_outpuf_saf_stil() \n");
				exit(-1);
			}
		}
	}
	fprintf(st,"; }\n");
	fprintf(st,"}");
}
//------------------------------------------------------------------------
//File name : file_outpuf_xid_test_set.c
//Date : 2013/7/05
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
void		file_outpuf_saf_txt		(FILE*);
void		file_outpuf_saf_stil	(FILE*, FILE*);
void		file_outpuf_tdf_txt		(FILE*);
void		file_outpuf_tdf_stil	(FILE*, FILE*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

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
//関数名：file_outpuf_xid_test_set
//機能　：X抽出後テスト集合{0,1,X}のファイル出力
//引数　：tx(出力用txtファイルポインタ), st(出力用stilファイルポインタ), in_st(初期テスト集合.stil)
//戻り値：なし
//--------------------------------------------------------------------
void file_outpuf_xid_test_set(FILE* tx, FILE* st, FILE* in_st){
	
	//==================================================================
	// 縮退故障ドントケア抽出
	//==================================================================
	if(opt.fmodel == SAF){
		//-------------------------------------------
		// .txt形式でファイル出力
		//-------------------------------------------
		if(opt.xtp_tx == YES){
			file_outpuf_saf_txt(tx);
			fclose(tx);
		}
		//-------------------------------------------
		// .stil形式でファイル出力
		//-------------------------------------------
		if(opt.xtp_st == YES){
			if(opt.tp_stil == YES){
				file_outpuf_saf_stil(st, in_st);
				fclose(st);
			}
			else{
				printf("Warning: STILファイル形式で入力されていないので，STILファイル形式での出力ができません。\n");
			}
		}
	}
	
	//==================================================================
	// 遷移故障ドントケア抽出
	//==================================================================
	else if(opt.fmodel == TDF){
		//-------------------------------------------
		// .txt形式でファイル出力
		//-------------------------------------------
		if(opt.xtp_tx == YES){
			file_outpuf_tdf_txt(tx);
			fclose(tx);
		}
		//-------------------------------------------
		// .stil形式でファイル出力
		//-------------------------------------------
		if(opt.xtp_st == YES){
			file_outpuf_tdf_stil(st, in_st);
			fclose(st);
		}
	}
}


//--------------------------------------------------------------------
//関数名：file_outpuf_saf_txt
//機能　：【縮退故障用：txt形式】X抽出後テスト集合{0,1,X}のファイル出力
//引数　：tx(出力用txtファイルポインタ)
//戻り値：なし
//--------------------------------------------------------------------
void	file_outpuf_saf_txt(FILE* tx){

	int			tp_id, pi_id;
	int			nval;

	//=================================================
	// txt形式ファイル出力 (piポインタの順に出力)
	//=================================================
	for(tp_id=0; tp_id<n_tp; tp_id++){

		for(pi_id=0; pi_id<n_pi; pi_id++){
			
			//X抽出後の値を得る
			nval = Get_NBit(pi[pi_id]->xid_nval, tp_id);

			//0出力
			if(nval == 0){
				fprintf(tx, "0");
			}

			//1出力
			else if(nval == 1){
				fprintf(tx, "1");
			}

			//X出力
			else if(nval == 3){
				fprintf(tx, "X");
			}
		}
		//テストパターンごとに改行
		fprintf(tx, "\n");
	}

}


//--------------------------------------------------------------------
//関数名：file_outpuf_saf_stil
//機能　：【縮退故障用：stil形式】X抽出後テスト集合{0,1,X}のファイル出力
//引数　：st(出力用stilファイルポインタ), in(初期テスト集合.stil)
//戻り値：なし
//--------------------------------------------------------------------
void	file_outpuf_saf_stil(FILE* st, FILE* in){

	int		i,j;
	int		nval;
	char	str[1000000];

	//=======================================================
	// 初期テスト集合.stilのファイルポインタを先頭に戻す
	//=======================================================
	rewind(in);	

	//=======================================================
	// テストパターンのところまでinの内容を出力ファイル(st)にコピー
	//=======================================================
	while(fgets(str,1000000,in) !=NULL){

		//Callまで来たらbreak
		if(strstr(str,"Call")!=NULL){
			break;				
		}
		//Callがある場所までコピペ
		else{
			fprintf(st,"%s",str);
		}
	}
	
	//=======================================================
	// X抽出後テストパターンと出力期待値をファイル出力
	//=======================================================
	for(i=0; i<n_tp; i++){

		//-----------------------------------------------
		// テストパターン出力
		//-----------------------------------------------
		fprintf(st,"   \"pattern %d\": Call \"capture\" { \n",i);
		fprintf(st,"      \"_pi\"=");

		for(j=0; j<n_pi; j++){

			//X抽出後の正常値取得
			nval = Get_NBit(pi[j]->xid_nval, i);

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
		
		//-----------------------------------------------
		// 出力期待値出力
		//-----------------------------------------------
		fprintf(st,"; \"_po\"=");
		for(j=0; j<n_po; j++){

			//X抽出後の正常値取得
			nval = Get_NBit(po[j]->xid_nval, i);

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
				fprintf(st, "X");
			}

			//その他(バグ)
			else{
				printf("ERROR: file_outpuf_saf_stil() \n");
				exit(-1);
			}
		}

		fprintf(st,"}\n");
	}


}


//--------------------------------------------------------------------
//関数名：file_outpuf_tdf_txt
//機能　：【遷移故障用：txt形式】X抽出後テスト集合{0,1,X}のファイル出力
//引数　：tx(出力用txtファイルポインタ)
//戻り値：なし
//--------------------------------------------------------------------
void	file_outpuf_tdf_txt(FILE* tx){
	
	int			tp_id, pi_id;
	int			nval;

	//=================================================
	// txt形式ファイルで入力の場合
	//=================================================
	if(opt.tp_txt == YES){
		for(tp_id=0; tp_id<n_tp; tp_id++){
		
			//----------------------------------
			// PIN順序テーブルに従っての1時刻目値を出力
			//----------------------------------
			for(pi_id=0; pi_id<(n_spi+n_ppi); pi_id++){

				//X抽出後の値を得る
				nval = Get_NBit(pin_tbl[pi_id]->sinput->xid_nval_t1, tp_id);

				//0出力
				if(nval == 0){
					fprintf(tx, "0");
				}

				//1出力
				else if(nval == 1){
					fprintf(tx, "1");
				}

				//X出力
				else if(nval == 3){
					fprintf(tx, "X");
				}
			}

			//テストパターンごとに改行
			fprintf(tx, "\n");
		}
	}

	//=================================================
	// STIL形式ファイルで入力の場合
	//=================================================
	if(opt.tp_stil == YES){
		for(tp_id=0; tp_id<n_tp; tp_id++){
		
			//----------------------------------
			//PI出力
			//----------------------------------
			for(pi_id=0; pi_id<n_spi; pi_id++){
				//X抽出後の値を得る
				nval = Get_NBit(spi[pi_id]->xid_nval_t1, tp_id);
				
				//0出力
				if(nval == 0){
					fprintf(tx, "0");
				}

				//1出力
				else if(nval == 1){
					fprintf(tx, "1");
				}

				//X出力
				else if(nval == 3){
					fprintf(tx, "X");
				}
			}
			
			//----------------------------------
			//PPI出力
			//----------------------------------
			for(pi_id=0; pi_id<n_ppi; pi_id++){
				//X抽出後の値を得る
				nval = Get_NBit(ppi[pi_id]->xid_nval_t1, tp_id);

				//0出力
				if(nval == 0){
					fprintf(tx, "0");
				}

				//1出力
				else if(nval == 1){
					fprintf(tx, "1");
				}

				//X出力
				else if(nval == 3){
					fprintf(tx, "X");
				}
			}
			//テストパターンごとに改行
			fprintf(tx, "\n");
		}
	}
}


//--------------------------------------------------------------------
//関数名：file_outpuf_tdf_stil
//機能　：【遷移故障用：stil形式】X抽出後テスト集合{0,1,X}のファイル出力
//引数　：st(出力用stilファイルポインタ), in(初期テスト集合.stil)
//戻り値：なし
//--------------------------------------------------------------------
void	file_outpuf_tdf_stil(FILE* st, FILE* in){
	
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
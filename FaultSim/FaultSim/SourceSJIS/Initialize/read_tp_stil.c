//------------------------------------------------------------------------
//テストパターンファイル読み込み(.stil)
//File name : read_tp_stil.c
//Date : 2011/6/16
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../StandardHead.h"
#include	"../option.h"
#include	"../Lib/alloc.h"

//------------------------------------------------------------------------
// プロタイプ宣言
//------------------------------------------------------------------------
void	read_tp_stil_saf(FILE*);
void	read_tp_stil_tdf(FILE*);
void	read_tp_stil_tdf_cpi(FILE*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

	//#define TP_DEBUG

//-----------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//------------------------------------------------------------------------
//  関数名 : read_tp_stil
//  機  能 : テストパターンファイル読み込み(.stil)
//  戻り値 : なし
//  引  数 : テストパターンファイルポインタ
//------------------------------------------------------------------------
void read_tp_stil(FILE *fp){

	
	//==================================
	// 縮退故障の場合
	//==================================
	if(opt.fmodel == SAF){
		read_tp_stil_saf(fp);
		n_tp_int = pi_tp[0].tp->int_num;	//unsigned int配列数保存
	}
	//==================================
	// 遷移故障の場合
	//==================================
	else if(opt.fmodel == TDF){
		
		//CPI_XIDモードの場合のみ別関数
		if(opt.cpi_xid == YES){
			read_tp_stil_tdf_cpi(fp);
		}
		//通常モード
		else{
			read_tp_stil_tdf(fp);
		}

		n_tp_int = pi_tp[0].tp->int_num;	//unsigned int配列数保存
	}


}//END
//**********************************************************************************
//**********************************************************************************
//**********************************************************************************
//------------------------------------------------------------------------
//  関数名 : read_tp_stil_saf
//  機  能 : 縮退故障用：テストパターンファイル読み込み(.stil)
//  戻り値 : なし
//  引  数 : テストパターンファイルポインタ
//------------------------------------------------------------------------
void read_tp_stil_saf(FILE *fp){

	int				i,j,k;
	char			temp[MAXN];
	char			temp2[MAXN];
	int				get_temp;
	int				nbit=0;		//格納先テストパターンビット
	
	//===========================================================================
	// テストパターン数カウント
	//===========================================================================
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"#internal")!=NULL){//SignalGroupsがある場所までポインタを移動
			break;
		}
	}

	get_temp=fgetc(fp);

	do{
		if(get_temp=='1'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=1;
		}else if(get_temp=='2'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=2;
		}else if(get_temp=='3'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=3;
		}else if(get_temp=='4'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=4;
		}else if(get_temp=='5'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=5;
		}else if(get_temp=='6'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=6;
		}else if(get_temp=='7'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=7;
		}else if(get_temp=='8'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=8;
		}else if(get_temp=='9'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=9;
		}else if(get_temp=='0'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
		}

		get_temp=fgetc(fp);

	}while(get_temp!='}');

#ifdef TP_DEBUG
	printf("入力テストパターン数: %d\n", n_tp);
#endif
	//===========================================================================
	// メモリ確保
	//===========================================================================
	//外部入力(PI)テストパターン構造体
	pi_tp = (TPH*)malloc(n_pi * sizeof(TPH));
	
	
	//===========================================================================
	// 初期化
	//===========================================================================
	//テストパターン構造体
	for(i=0; i<n_pi; i++){
		pi_tp[i].tp = Alloc_Bit_TP(n_tp);	//テストパターン数分ビット確保
		pi_tp[i].flag = 0;
		pi_tp[i].nl_id = -1;
	}

	//ネットリスト構造体
	for(i=0; i<n_net; i++){nl[i].nval = Alloc_Bit_TP(n_tp);}
	
	//=================================================
	// 外部入力(PI)読み込み
	//=================================================
	//-----------------------
	//STIL読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"SignalGroups")!=NULL){//SignalGroupsがある場所までポインタを移動
			break;	
		}
	}

	i=0;
	j=0;

	get_temp=fgetc(fp);

	do{
		if(get_temp=='\''){

			get_temp=fgetc(fp);

			do{

				if(get_temp=='"'){

					get_temp=fgetc(fp);

					do{
						temp2[i]=get_temp;
//							printf("get_temp:%c \n",get_temp);
						i++;
						get_temp=fgetc(fp);

					}while(get_temp!='"');
					temp2[i]='\0';
					strcpy(pi_tp[j].pin_name,temp2);

//						pi_tp[j].pin_name=temp2;
//						printf("haed[%d] pin_name=%s\n",j,pi_tp[j].pin_name);
					j++;
					i=0;
						
				}

				get_temp=fgetc(fp);

			}while(get_temp!='\'');

			break;

		}
			
		get_temp=fgetc(fp);

	}while(get_temp!=EOF);

	//=================================================
	// テストパターン読み込み
	//=================================================
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Call")!=NULL){//Callがある場所までポインタを移動
			break;	
		}
	}

	get_temp=fgetc(fp);
#ifdef TP_DEBUG
	printf("テストパターン読み込み開始->");
#endif

	do{
//		printf("%c \n",get_temp);
		if(get_temp=='='){
				
			get_temp=fgetc(fp);

			for(i=0;i<n_pi;i++){
					
//				printf("%c",get_temp);
				//---------------------------------------------
				if(get_temp=='0'){
#ifdef TP_DEBUG
					printf("0");
#endif
					Set_NBit_Zero(pi_tp[i].tp, nbit);
				}
				//---------------------------------------------
				else if(get_temp=='1'){
#ifdef TP_DEBUG
					printf("1");
#endif
					Set_NBit_One(pi_tp[i].tp, nbit);
				}
				//---------------------------------------------
				else if(get_temp=='N'){
#ifdef TP_DEBUG
					printf("X");
#endif
					Set_NBit_X(pi_tp[i].tp, nbit);
					n_before_x++;	//テストパターン集合中の全X数更新
				}
				//---------------------------------------------
				else if(get_temp='\n'){
					i--;
				}
				//---------------------------------------------
				else{
					printf("Read Test Pattern Errof\n");
					exit(1);
				}
				get_temp=fgetc(fp);

			}
				
			nbit++;	//テストパターン格納ビット目更新

//				printf("pattern=%d \n",pattern);
//				printf("\n");


			do{
				if(get_temp=='='){
					break;
				}
//				printf("%c",get_temp);
				get_temp=fgetc(fp);
			}while(get_temp!=EOF);
					
		}

		get_temp=fgetc(fp);

	}while(get_temp!=EOF);


	//===========================================================================
	// Netlistとの連結
	//===========================================================================
	for(i=0; i<n_pi; i++){

		//PI信号線
		for(j=0; j<n_pi; j++){
			if(strcmp(pi[i]->name, pi_tp[j].pin_name) == 0){
				pi_tp[j].nl_id = pi[i]->n;
				pi_tp[j].input = pi[i];

				//テストパターンをNetlistの正常値(nval)に代入
				for(k=0; k<(int)pi[i]->nval->int_num; k++){
					pi[i]->nval->x_buf[k] = pi_tp[j].tp->x_buf[k];
					pi[i]->nval->p_buf[k] = pi_tp[j].tp->p_buf[k];
				}

				break;
			}
		}
	}
	
	//===========================================================================
	// DEBUG
	//===========================================================================
#ifdef TP_DEBUG
	printf("\n\n//------------------------\n");
	printf("// テストパターン格納DEBUG\n");
	printf("//------------------------\n");
	for(i=0; i<n_pi; i++){
		printf("PI[%d] name:%s nl_id:%d \n", i, pi_tp[i].pin_name, pi_tp[i].nl_id);
	}
	printf("\n");

	for(i=0; i<n_tp; i++){
		//PIのテストパターン表示
		for(j=0; j<n_pi; j++){
			if(pi_tp[j].nl_id != -1){
				if( (Get_NBit(pi_tp[j].tp,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(pi_tp[j].tp,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(pi_tp[j].tp,i)) == 3){
					printf("X");
				}
			}
		}
		printf("\n");
	}
#endif

}//END

//**********************************************************************************
//**********************************************************************************
//**********************************************************************************

//------------------------------------------------------------------------
//  関数名 : read_tp_stil_tdf
//  機  能 : 遷移故障用：テストパターンファイル読み込み(.stil)
//  戻り値 : なし
//  引  数 : テストパターンファイルポインタ
//------------------------------------------------------------------------
void read_tp_stil_tdf(FILE *fp){

	int				i,j;
	unsigned int	k;
	char			temp[MAXN];
	char			temp2[MAXN];
	char			get_temp;
	int				nbit=0;		//格納先テストパターンビット
	
	//===========================================================================
	// テストパターン数カウント
	//===========================================================================
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"#basic_scan patterns")!=NULL){//#basic_scan patternsがある場所までポインタを移動
			break;
		}
	}

	get_temp=fgetc(fp);

	do{
		if(get_temp=='1'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=1;
		}else if(get_temp=='2'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=2;
		}else if(get_temp=='3'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=3;
		}else if(get_temp=='4'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=4;
		}else if(get_temp=='5'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=5;
		}else if(get_temp=='6'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=6;
		}else if(get_temp=='7'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=7;
		}else if(get_temp=='8'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=8;
		}else if(get_temp=='9'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=9;
		}else if(get_temp=='0'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
		}
		get_temp=fgetc(fp);

	}while(get_temp!='}');

#ifdef TP_DEBUG
	printf("入力テストパターン数: %d\n", n_tp);
#endif
	
	
	//===========================================================================
	// 正常値格納変数のメモリ確保
	//===========================================================================
	for(i=0; i<n_snet; i++){
		s_nl[i].nval_t1 = Alloc_Bit_TP(n_tp);
		s_nl[i].nval_t2 = Alloc_Bit_TP(n_tp);
	}

	
	//===========================================================================
	// 外部入力(PI)読み込み
	//===========================================================================
	//-----------------------
	//STIL読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"SignalGroups")!=NULL){//SignalGroupsがある場所までポインタを移動
			break;
		}
	}

	i=0;
	j=0;

	get_temp=fgetc(fp);
	do{
		if(get_temp=='#'){
			break;
		}
		get_temp=fgetc(fp);
	}while(get_temp!=EOF);

	get_temp=fgetc(fp);
	do{
		if(get_temp=='='){
			break;
		}
		get_temp=fgetc(fp);
	}while(get_temp!=EOF);

	n_se_input=0;
	do{
		if(get_temp=='1'){
			n_se_input*=10;
			n_se_input+=1;
		}else if(get_temp=='2'){
			n_se_input*=10;
			n_se_input+=2;
		}else if(get_temp=='3'){
			n_se_input*=10;
			n_se_input+=3;
		}else if(get_temp=='4'){
			n_se_input*=10;
			n_se_input+=4;
		}else if(get_temp=='5'){
			n_se_input*=10;
			n_se_input+=5;
		}else if(get_temp=='6'){
			n_se_input*=10;
			n_se_input+=6;
		}else if(get_temp=='7'){
			n_se_input*=10;
			n_se_input+=7;
		}else if(get_temp=='8'){
			n_se_input*=10;
			n_se_input+=8;
		}else if(get_temp=='9'){
			n_se_input*=10;
			n_se_input+=9;
		}else if(get_temp=='0'){
			n_se_input*=10;
		}
		get_temp=fgetc(fp);
	}while(get_temp!='\n');
	
	//-----------------------
	//メモリ確保(pi_tp)
	//-----------------------
	pi_tp = (TPH*)malloc(n_se_input * sizeof(TPH));
	
	//-----------------------
	//初期化(pi_tp)
	//-----------------------
	for(i=0; i<n_se_input; i++){
		pi_tp[i].tp = Alloc_Bit_TP(n_tp);	//テストパターン数分ビット確保
		pi_tp[i].flag = 0;
		pi_tp[i].nl_id = -1;
	}
	
	//-----------------------
	//PI信号線名読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"#")!=NULL){//_inがある場所までポインタを移動
			break;	
		}
	}
	i=0;
	j=0;
	do{
		if(get_temp=='\''){

			get_temp=fgetc(fp);

			do{

				if(get_temp=='"'){

					get_temp=fgetc(fp);

					do{
						temp2[i]=get_temp;
#ifdef TP_DEBUG
					printf("get_temp:%c \n",get_temp);
#endif
						i++;
						get_temp=fgetc(fp);

					}while(get_temp!='"');
					temp2[i]='\0';
					strcpy(pi_tp[j].pin_name,temp2);
#ifdef TP_DEBUG
					printf("pi_tp[%d].pin_name = %s\n",j,pi_tp[j].pin_name);
#endif
					j++;
					i=0;
						
				}

				get_temp=fgetc(fp);

			}while(get_temp != '\'');
			break;
		}
		get_temp=fgetc(fp);

	}while(get_temp!=EOF);
	
	
	//===========================================================================
	// 疑似外部入力(PPI)読み込み　[※CLKとかも読む]
	//===========================================================================
	//-----------------------
	//STIL読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanStructures")!=NULL){//ScanStructuresがある場所までポインタを移動
			fgets(temp,MAXN,fp);
			break;	
		}
	}

	i=0;
	j=0;

	get_temp=fgetc(fp);

	do{
		get_temp=fgetc(fp);

		if(get_temp=='1'){
			n_scan_ff*=10;
			n_scan_ff+=1;
		}else if(get_temp=='2'){
			n_scan_ff*=10;
			n_scan_ff+=2;
		}else if(get_temp=='3'){
			n_scan_ff*=10;
			n_scan_ff+=3;
		}else if(get_temp=='4'){
			n_scan_ff*=10;
			n_scan_ff+=4;
		}else if(get_temp=='5'){
			n_scan_ff*=10;
			n_scan_ff+=5;
		}else if(get_temp=='6'){
			n_scan_ff*=10;
			n_scan_ff+=6;
		}else if(get_temp=='7'){
			n_scan_ff*=10;
			n_scan_ff+=7;
		}else if(get_temp=='8'){
			n_scan_ff*=10;
			n_scan_ff+=8;
		}else if(get_temp=='9'){
			n_scan_ff*=10;
			n_scan_ff+=9;
		}else if(get_temp=='0'){
			n_scan_ff*=10;
		}
	}while(get_temp!=';');

	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanInversion")!=NULL){//ScanInversionがある場所までポインタを移動
			break;		
		}
	}

	//-----------------------
	//スキャンFF数確認
	//-----------------------
	if(n_scan_ff != n_sdff){
		printf("\n//-------------------------------------------------------\n");
		printf("/ WARNING: read_tp_stil.c\n");
		printf("//-------------------------------------------------------\n");
		printf("%d個のFFがスキャン設計されてない可能性があります！\n", n_sdff-n_scan_ff);
	}

	//-----------------------
	//メモリ確保(ppi_tp)
	//-----------------------
	ppi_tp = (TPH*)malloc(n_scan_ff * sizeof(TPH));


	//-----------------------
	//初期化(ppi_tp)
	//-----------------------
	for(i=0; i<n_scan_ff; i++){
		ppi_tp[i].tp = Alloc_Bit_TP(n_tp);	//テストパターン数分ビット確保
		ppi_tp[i].flag = 0;
		ppi_tp[i].nl_id = -1;
	}


	//-----------------------
	//PPI信号線名読み込み(逆から入れてる)
	//-----------------------
	j=n_scan_ff-1;
	do{
		i=0;
		get_temp=fgetc(fp);
		if(get_temp=='.'){
			get_temp=fgetc(fp);
			while(get_temp!='.'){	
				temp2[i]=get_temp;
				i++;
#ifdef TP_DEBUG
				printf("get_temp:%c\n",get_temp);
#endif
				get_temp=fgetc(fp);
			}
			temp2[i]='\0';
			strcpy(ppi_tp[j].pin_name,temp2);
#ifdef TP_DEBUG
			printf("ppi_tp[%d].pin_name = %s\n",j,ppi_tp[j].pin_name);
#endif
			j--;
			i=0;
		}

	}while(get_temp!=';');

	
	//===========================================================================
	// テストパターン読み込み
	//===========================================================================
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Ann {* fast_sequential *}")!=NULL){	//Ann {* fast_sequential *}がある場所までポインタを移動
			break;
		}
	}
	get_temp=fgetc(fp);

	//---------------------------------------------------------------
	//pattern1からのテストパターンを保存(※pattern0は読まない！)
	//---------------------------------------------------------------
	do{
		get_temp=fgetc(fp);
	}while(get_temp!=':');
	

	do{
#ifdef TP_DEBUG
		printf("%c",get_temp);
#endif
		//全部格納したら終了
		if(nbit == n_tp){
			break;
		}
		if(get_temp=='='){
			get_temp=fgetc(fp);
			do{
				get_temp=fgetc(fp);
#ifdef TP_DEBUG
				printf("%c",get_temp);
#endif
			}while(get_temp!='=');

			get_temp=fgetc(fp);

			//PPIにテストパターン格納
			for(i=0;i<n_scan_ff;i++){
#ifdef TP_DEBUG
				//printf("%c",get_temp);
				printf("ppi_tp[%d].pin_name = %s = ",i, ppi_tp[i].pin_name);
#endif
				//-----------------------------------------
				if(get_temp=='0'){
#ifdef TP_DEBUG
					printf("0\n");
#endif
					Set_NBit_Zero(ppi_tp[i].tp, nbit);
				}
				//-----------------------------------------
				else if(get_temp=='1'){
#ifdef TP_DEBUG
					printf("1\n");
#endif
					Set_NBit_One(ppi_tp[i].tp, nbit);
				}
				//-----------------------------------------
				else if(get_temp=='N'){
#ifdef TP_DEBUG
					printf("get_temp=N \n");
#endif
					Set_NBit_X(ppi_tp[i].tp, nbit);
					n_before_x++;	//テストパターン集合中の全X数更新
				}
				//-----------------------------------------
				else if(get_temp='\n'){
					i--;
				}
				//-----------------------------------------
				else{
					printf("Read Test Pattern Errof\n");
					exit(1);
				}
				//-----------------------------------------
				get_temp=fgetc(fp);
			}
			
			//PIにテストパターン格納
			do{
				get_temp=fgetc(fp);
				//printf("%c",get_temp);
				
				if(get_temp=='='){
					get_temp=fgetc(fp);

					for(i=0;i<n_se_input;i++){
#ifdef TP_DEBUG
						//printf("%c",get_temp);
						printf("pi_tp[%d].pin_name = %s = ",i, pi_tp[i].pin_name);
#endif
						//-----------------------------------------
						if(get_temp=='0'){
#ifdef TP_DEBUG
							printf("0\n");
#endif
							Set_NBit_Zero(pi_tp[i].tp, nbit);
						}
						//-----------------------------------------
						else if(get_temp=='1'){
#ifdef TP_DEBUG
							printf("1\n");
#endif
							Set_NBit_One(pi_tp[i].tp, nbit);
						}
						//-----------------------------------------
						else if(get_temp=='N'){
#ifdef TP_DEBUG
							printf("get_temp=N \n");
#endif
							Set_NBit_X(pi_tp[i].tp, nbit);
							n_before_x++;	//テストパターン集合中の全X数更新
						}
						//-----------------------------------------
						else if(get_temp='\n'){
							i--;
						}
						//-----------------------------------------
						else{
							printf("Read Test Pattern Errof\n");
							exit(1);
						}
						//-----------------------------------------
						get_temp=fgetc(fp);

					}
					nbit++;	//テストパターン格納ビット目更新
					break;
				}
			}while(get_temp!=EOF);


			do{
				if(get_temp==':'){
					break;
				}

				get_temp=fgetc(fp);
#ifdef TP_DEBUG
				//printf("%c",get_temp);
#endif
			}while(get_temp!=EOF);
					
		}

		get_temp=fgetc(fp);

	}while(get_temp!=EOF);

	
	//===========================================================================
	// Netlistとの連結
	//===========================================================================
	//-----------------------------------------------
	// PI信号線のテストパターン印加
	//-----------------------------------------------
	for(i=0; i<n_spi; i++){
		for(j=0; j<n_se_input; j++){
			if(strcmp(spi[i]->name, pi_tp[j].pin_name) == 0){

				//テストパターンをNetlistの正常値(nval)に代入
				for(k=0; k<spi[i]->nval_t1->int_num; k++){
					//1次刻目PI印加
					spi[i]->nval_t1->x_buf[k] = pi_tp[j].tp->x_buf[k];
					spi[i]->nval_t1->p_buf[k] = pi_tp[j].tp->p_buf[k];

					//2時刻目PI印加(ブロードサイド方式のため)
					spi[i]->nval_t2->x_buf[k] = pi_tp[j].tp->x_buf[k];
					spi[i]->nval_t2->p_buf[k] = pi_tp[j].tp->p_buf[k];
				}

				//Netlist情報格納
				pi_tp[j].nl_id = spi[i]->n;
				pi_tp[j].sinput = spi[i];
				break;
			}
		}
	}
	
	//-----------------------------------------------
	// PPI信号線のテストパターン印加
	//-----------------------------------------------
	for(i=0; i<n_ppi; i++){
		for(j=0;j<n_scan_ff;j++){
			//★PPIはインスタンス名と比較★
			if(strcmp(ppi[i]->name_ins, ppi_tp[j].pin_name) == 0){

				//テストパターンをNetlistの正常値(nval)に代入
				for(k=0; k<ppi[i]->nval_t1->int_num; k++){
					//1次刻目PPI印加
					ppi[i]->nval_t1->x_buf[k] = ppi_tp[j].tp->x_buf[k];
					ppi[i]->nval_t1->p_buf[k] = ppi_tp[j].tp->p_buf[k];
				}

				//Netlist情報格納
				ppi_tp[j].nl_id = ppi[i]->n;
				ppi_tp[j].sinput = ppi[i];
				break;
			}
		}
	}


	//===========================================================================
	// DEBUG
	//===========================================================================
#ifdef TP_DEBUG
	printf("\n\n//------------------------\n");
	printf("// テストパターン格納DEBUG\n");
	printf("//------------------------\n");
	for(i=0; i<n_se_input; i++){
		printf("PI[%d] name:%s nl_id:%d \n", i, pi_tp[i].pin_name, pi_tp[i].nl_id);
	}
	for(i=0;i<n_scan_ff;i++){
		printf("PPI[%d] name:%s nl_id:%d \n", i, ppi_tp[i].pin_name, ppi_tp[i].nl_id);
	}

	printf("\n\n");

	printf("-テストパターン構造体の中身-\n");
	for(i=0; i<n_tp; i++){
		//PIのテストパターン表示
		for(j=0; j<n_se_input; j++){
			if(pi_tp[j].nl_id != -1){
				if( (Get_NBit(pi_tp[j].tp,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(pi_tp[j].tp,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(pi_tp[j].tp,i)) == 3){
					printf("X");
				}
			}
		}
		//PPIのテストパターン表示
		for(j=n_scan_ff-1; 0<=j; j--){
			if(ppi_tp[j].nl_id != -1){
				if( (Get_NBit(ppi_tp[j].tp,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(ppi_tp[j].tp,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(ppi_tp[j].tp,i)) == 3){
					printf("X");
				}
			}
		}
		printf("\n");
	}

	printf("\n\n");


	printf("ピン順番\n");
	for(j=0; j<n_se_input; j++){
		if(pi_tp[j].nl_id != -1){
			printf("%s ", pi_tp[j].pin_name);
		}
	}
	for(j=n_scan_ff-1; 0<=j; j--){
		printf("%s ", ppi_tp[j].pin_name);
	}

	printf("\n");

	printf("-Netlist構造体の中身-\n");
	for(i=0; i<n_tp; i++){
		//PIのテストパターン表示
		for(j=0; j<n_se_input; j++){
			if(pi_tp[j].nl_id != -1){
				if( (Get_NBit(pi_tp[j].sinput->nval_t1,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(pi_tp[j].sinput->nval_t1,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(pi_tp[j].sinput->nval_t1,i)) == 3){
					printf("X");
				}
			}
		}
		//PPIのテストパターン表示
		for(j=n_scan_ff-1; 0<=j; j--){
			if(ppi_tp[j].nl_id != -1){
				if( (Get_NBit(ppi_tp[j].sinput->nval_t1,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(ppi_tp[j].sinput->nval_t1,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(ppi_tp[j].sinput->nval_t1,i)) == 3){
					printf("X");
				}
			}
		}
		printf("\n");
	}

#endif


}//END

//**********************************************************************************
//**********************************************************************************
//**********************************************************************************

//------------------------------------------------------------------------
//  関数名 : read_tp_stil_tdf_cpi
//  機  能 : 【CPI_XID専用】遷移故障用：テストパターンファイル読み込み(.stil)
//  戻り値 : なし
//  引  数 : テストパターンファイルポインタ
//------------------------------------------------------------------------
void read_tp_stil_tdf_cpi(FILE *fp){

	int				i,j,count;
	unsigned int	k;
	char			temp[MAXN];
	char			temp2[MAXN];
	char			get_temp;
	int				nbit=0;		//格納先テストパターンビット
	
	//===========================================================================
	// テストパターン数カウント
	//===========================================================================
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"#basic_scan patterns")!=NULL){//#basic_scan patternsがある場所までポインタを移動
			break;
		}
	}

	get_temp=fgetc(fp);

	do{
		if(get_temp=='1'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=1;
		}else if(get_temp=='2'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=2;
		}else if(get_temp=='3'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=3;
		}else if(get_temp=='4'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=4;
		}else if(get_temp=='5'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=5;
		}else if(get_temp=='6'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=6;
		}else if(get_temp=='7'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=7;
		}else if(get_temp=='8'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=8;
		}else if(get_temp=='9'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
			n_tp+=9;
		}else if(get_temp=='0'){
//				printf("get_temp:%c\n",get_temp);
			n_tp*=10;
		}
		get_temp=fgetc(fp);

	}while(get_temp!='}');

#ifdef TP_DEBUG
	printf("入力テストパターン数: %d\n", n_tp);
#endif
	
	
	//===========================================================================
	// 正常値格納変数のメモリ確保
	//===========================================================================
	for(i=0; i<n_snet; i++){
		s_nl[i].nval_t1 = Alloc_Bit_TP(n_tp);
		s_nl[i].nval_t2 = Alloc_Bit_TP(n_tp);
	}

	
	//===========================================================================
	// 外部入力(PI)読み込み
	//===========================================================================
	//-----------------------
	//STIL読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"SignalGroups")!=NULL){//SignalGroupsがある場所までポインタを移動
			break;
		}
	}

	i=0;
	j=0;

	get_temp=fgetc(fp);
	do{
		if(get_temp=='#'){
			break;
		}
		get_temp=fgetc(fp);
	}while(get_temp!=EOF);

	get_temp=fgetc(fp);
	do{
		if(get_temp=='='){
			break;
		}
		get_temp=fgetc(fp);
	}while(get_temp!=EOF);

	n_se_input=0;
	do{
		if(get_temp=='1'){
			n_se_input*=10;
			n_se_input+=1;
		}else if(get_temp=='2'){
			n_se_input*=10;
			n_se_input+=2;
		}else if(get_temp=='3'){
			n_se_input*=10;
			n_se_input+=3;
		}else if(get_temp=='4'){
			n_se_input*=10;
			n_se_input+=4;
		}else if(get_temp=='5'){
			n_se_input*=10;
			n_se_input+=5;
		}else if(get_temp=='6'){
			n_se_input*=10;
			n_se_input+=6;
		}else if(get_temp=='7'){
			n_se_input*=10;
			n_se_input+=7;
		}else if(get_temp=='8'){
			n_se_input*=10;
			n_se_input+=8;
		}else if(get_temp=='9'){
			n_se_input*=10;
			n_se_input+=9;
		}else if(get_temp=='0'){
			n_se_input*=10;
		}
		get_temp=fgetc(fp);
	}while(get_temp!='\n');
	
	//-----------------------
	//★メモリ確保(pi_tp)
	//-----------------------
	//※制御ポイント挿入数分増加
	pi_tp = (TPH*)malloc((n_se_input+(n_sdff/2)) * sizeof(TPH));
	
	//-----------------------
	//★初期化(pi_tp)
	//-----------------------
	for(i=0; i<(n_se_input+(n_sdff/2)); i++){
		pi_tp[i].tp = Alloc_Bit_TP(n_tp);	//テストパターン数分ビット確保
		pi_tp[i].flag = 0;
		pi_tp[i].nl_id = -1;
	}
	
	//-----------------------
	//PI信号線名読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"#")!=NULL){//_inがある場所までポインタを移動
			break;	
		}
	}
	i=0;
	j=0;
	do{
		if(get_temp=='\''){

			get_temp=fgetc(fp);

			do{

				if(get_temp=='"'){

					get_temp=fgetc(fp);

					do{
						temp2[i]=get_temp;
#ifdef TP_DEBUG
					printf("get_temp:%c \n",get_temp);
#endif
						i++;
						get_temp=fgetc(fp);

					}while(get_temp!='"');
					temp2[i]='\0';
					strcpy(pi_tp[j].pin_name,temp2);
#ifdef TP_DEBUG
					printf("pi_tp[%d].pin_name = %s\n",j,pi_tp[j].pin_name);
#endif
					j++;
					i=0;
						
				}

				get_temp=fgetc(fp);

			}while(get_temp != '\'');
			break;
		}
		get_temp=fgetc(fp);

	}while(get_temp!=EOF);
	
	//===========================================================================
	// 疑似外部入力(PPI)読み込み　[※CLKとかも読む]
	//===========================================================================
	//-----------------------
	//STIL読み込み
	//-----------------------
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanStructures")!=NULL){//ScanStructuresがある場所までポインタを移動
			fgets(temp,MAXN,fp);
			break;	
		}
	}

	i=0;
	j=0;

	get_temp=fgetc(fp);

	do{
		get_temp=fgetc(fp);

		if(get_temp=='1'){
			n_scan_ff*=10;
			n_scan_ff+=1;
		}else if(get_temp=='2'){
			n_scan_ff*=10;
			n_scan_ff+=2;
		}else if(get_temp=='3'){
			n_scan_ff*=10;
			n_scan_ff+=3;
		}else if(get_temp=='4'){
			n_scan_ff*=10;
			n_scan_ff+=4;
		}else if(get_temp=='5'){
			n_scan_ff*=10;
			n_scan_ff+=5;
		}else if(get_temp=='6'){
			n_scan_ff*=10;
			n_scan_ff+=6;
		}else if(get_temp=='7'){
			n_scan_ff*=10;
			n_scan_ff+=7;
		}else if(get_temp=='8'){
			n_scan_ff*=10;
			n_scan_ff+=8;
		}else if(get_temp=='9'){
			n_scan_ff*=10;
			n_scan_ff+=9;
		}else if(get_temp=='0'){
			n_scan_ff*=10;
		}
	}while(get_temp!=';');

	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"ScanInversion")!=NULL){//ScanInversionがある場所までポインタを移動
			break;		
		}
	}

	//-----------------------
	//★スキャンFF数確認
	//-----------------------
	//※制御ポイント挿入してるので2倍メモリ確保
	if(n_scan_ff != (n_sdff/2) ){
		printf("\n//-------------------------------------------------------\n");
		printf("/ WARNING: read_tp_stil.c\n");
		printf("//-------------------------------------------------------\n");
		printf("%d個のFFがスキャン設計されてない可能性があります！\n", (n_sdff*2)-n_scan_ff );
	}

	//-----------------------
	//★メモリ確保(ppi_tp)
	//-----------------------
	//※制御ポイント挿入してるので余分にメモリ確保
	ppi_tp = (TPH*)malloc( (n_scan_ff+(n_sdff/2)) * sizeof(TPH));


	//-----------------------
	//★初期化(ppi_tp)
	//-----------------------
	for(i=0; i<(n_scan_ff+(n_sdff/2)); i++){
		ppi_tp[i].tp = Alloc_Bit_TP(n_tp);	//テストパターン数分ビット確保
		ppi_tp[i].flag = 0;
		ppi_tp[i].nl_id = -1;
	}


	//-----------------------
	//PPI信号線名読み込み(逆から入れてる)
	//-----------------------
	j=n_scan_ff-1;
	do{
		i=0;
		get_temp=fgetc(fp);
		if(get_temp=='.'){
			get_temp=fgetc(fp);
			while(get_temp!='.'){	
				temp2[i]=get_temp;
				i++;
#ifdef TP_DEBUG
				printf("get_temp:%c\n",get_temp);
#endif
				get_temp=fgetc(fp);
			}
			temp2[i]='\0';
			strcpy(ppi_tp[j].pin_name,temp2);
#ifdef TP_DEBUG
			printf("ppi_tp[%d].pin_name = %s\n",j,ppi_tp[j].pin_name);
#endif
			j--;
			i=0;
		}

	}while(get_temp!=';');

	
	//===========================================================================
	// テストパターン読み込み
	//===========================================================================
	while(fgets(temp,MAXN,fp) !=NULL){
		if(strstr(temp,"Ann {* fast_sequential *}")!=NULL){	//Ann {* fast_sequential *}がある場所までポインタを移動
			break;
		}
	}
	get_temp=fgetc(fp);

	//---------------------------------------------------------------
	//pattern1からのテストパターンを保存(※pattern0は読まない！)
	//---------------------------------------------------------------
	do{
		get_temp=fgetc(fp);
	}while(get_temp!=':');
	

	do{
#ifdef TP_DEBUG
		printf("%c",get_temp);
#endif
		//全部格納したら終了
		if(nbit == n_tp){
			break;
		}
		if(get_temp=='='){
			get_temp=fgetc(fp);
			do{
				get_temp=fgetc(fp);
#ifdef TP_DEBUG
				printf("%c",get_temp);
#endif
			}while(get_temp!='=');

			get_temp=fgetc(fp);

			//PPIにテストパターン格納
			for(i=0;i<n_scan_ff;i++){
#ifdef TP_DEBUG
				//printf("%c",get_temp);
				printf("ppi_tp[%d].pin_name = %s = ",i, ppi_tp[i].pin_name);
#endif
				//-----------------------------------------
				if(get_temp=='0'){
#ifdef TP_DEBUG
					printf("0\n");
#endif
					Set_NBit_Zero(ppi_tp[i].tp, nbit);
				}
				//-----------------------------------------
				else if(get_temp=='1'){
#ifdef TP_DEBUG
					printf("1\n");
#endif
					Set_NBit_One(ppi_tp[i].tp, nbit);
				}
				//-----------------------------------------
				else if(get_temp=='N'){
#ifdef TP_DEBUG
					printf("get_temp=N \n");
#endif
					Set_NBit_X(ppi_tp[i].tp, nbit);
					n_before_x++;	//テストパターン集合中の全X数更新
				}
				//-----------------------------------------
				else if(get_temp='\n'){
					i--;
				}
				//-----------------------------------------
				else{
					printf("Read Test Pattern Errof\n");
					exit(1);
				}
				//-----------------------------------------
				get_temp=fgetc(fp);
			}
			
			//PIにテストパターン格納
			do{
				get_temp=fgetc(fp);
				//printf("%c",get_temp);
				
				if(get_temp=='='){
					get_temp=fgetc(fp);

					for(i=0;i<n_se_input;i++){
#ifdef TP_DEBUG
						//printf("%c",get_temp);
						printf("pi_tp[%d].pin_name = %s = ",i, pi_tp[i].pin_name);
#endif
						//-----------------------------------------
						if(get_temp=='0'){
#ifdef TP_DEBUG
							printf("0\n");
#endif
							Set_NBit_Zero(pi_tp[i].tp, nbit);
						}
						//-----------------------------------------
						else if(get_temp=='1'){
#ifdef TP_DEBUG
							printf("1\n");
#endif
							Set_NBit_One(pi_tp[i].tp, nbit);
						}
						//-----------------------------------------
						else if(get_temp=='N'){
#ifdef TP_DEBUG
							printf("get_temp=N \n");
#endif
							Set_NBit_X(pi_tp[i].tp, nbit);
							n_before_x++;	//テストパターン集合中の全X数更新
						}
						//-----------------------------------------
						else if(get_temp='\n'){
							i--;
						}
						//-----------------------------------------
						else{
							printf("Read Test Pattern Errof\n");
							exit(1);
						}
						//-----------------------------------------
						get_temp=fgetc(fp);

					}
					nbit++;	//テストパターン格納ビット目更新
					break;
				}
			}while(get_temp!=EOF);


			do{
				if(get_temp==':'){
					break;
				}

				get_temp=fgetc(fp);
#ifdef TP_DEBUG
				//printf("%c",get_temp);
#endif
			}while(get_temp!=EOF);
					
		}

		get_temp=fgetc(fp);

	}while(get_temp!=EOF);

	
	//===========================================================================
	// Netlistとの連結
	//===========================================================================
	//-----------------------------------------------
	// ★PI信号線のテストパターン印加
	//-----------------------------------------------]
	//付加PIの保存先インデックス初期化
	count = n_se_input;

	for(i=0; i<n_spi; i++){

		//-------------------------------------
		// STILファイル内にあったPI信号線を接続
		//-------------------------------------
		for(j=0; j<(n_se_input+(n_sdff/2)); j++){
			if(strcmp(spi[i]->name, pi_tp[j].pin_name) == 0){

				//テストパターンをNetlistの正常値(nval)に代入
				for(k=0; k<spi[i]->nval_t1->int_num; k++){
					//1次刻目PI印加
					spi[i]->nval_t1->x_buf[k] = pi_tp[j].tp->x_buf[k];
					spi[i]->nval_t1->p_buf[k] = pi_tp[j].tp->p_buf[k];

					//2時刻目PI印加(ブロードサイド方式のため)
					spi[i]->nval_t2->x_buf[k] = pi_tp[j].tp->x_buf[k];
					spi[i]->nval_t2->p_buf[k] = pi_tp[j].tp->p_buf[k];
				}

				//Netlist情報格納
				pi_tp[j].nl_id = spi[i]->n;
				pi_tp[j].sinput = spi[i];
				break;
			}
		}

		//-------------------------------------
		// ★制御ポイント挿入した付加PI(CP)の場合
		//-------------------------------------
		if(j == (n_se_input+(n_sdff/2))){

			//付加PI名保存
			strcpy(pi_tp[count].pin_name, spi[i]->name);

			//ポインタ接続
			pi_tp[count].sinput = spi[i];

			//Netlist ID保存
			pi_tp[count].nl_id = spi[i]->n;

			//CPの値はオールXで初期化(2値モードなら1になるので)
			for(k=0; k<pi_tp[count].tp->int_num; k++){
				pi_tp[count].tp->x_buf[k] = 0xFFFFFFFF;		//オール1
				pi_tp[count].tp->p_buf[k] = 0xFFFFFFFF;		//オール1
			}

			//印加
			for(k=0; k<spi[i]->nval_t1->int_num; k++){
				//1次刻目PPI印加
				spi[i]->nval_t1->x_buf[k] = pi_tp[count].tp->x_buf[k];
				spi[i]->nval_t1->p_buf[k] = pi_tp[count].tp->p_buf[k];
			}

			//付加PI保存先インデックス更新
			count++;
		}
	}
	
	//-----------------------------------------------
	// PPI信号線のテストパターン印加
	//-----------------------------------------------
	//付加PPIの保存先インデックス初期化
	count = n_scan_ff;

	for(i=0; i<n_ppi; i++){

		//-------------------------------------
		// STILファイル内にあったPPI信号線を接続
		//-------------------------------------
		for(j=0;j<(n_scan_ff+(n_sdff/2));j++){
			//★PPIはインスタンス名と比較★
			if(strcmp(ppi[i]->name_ins, ppi_tp[j].pin_name) == 0){

				//テストパターンをNetlistの正常値(nval)に代入
				for(k=0; k<ppi[i]->nval_t1->int_num; k++){
					//1次刻目PPI印加
					ppi[i]->nval_t1->x_buf[k] = ppi_tp[j].tp->x_buf[k];
					ppi[i]->nval_t1->p_buf[k] = ppi_tp[j].tp->p_buf[k];
				}

				//Netlist情報格納
				ppi_tp[j].nl_id = ppi[i]->n;
				ppi_tp[j].sinput = ppi[i];
				break;
			}
		}

		//-------------------------------------
		// ★制御ポイント挿入した付加PPI(FFC)の場合
		//-------------------------------------
		if(j == (n_scan_ff+(n_sdff/2))){

			//※付加PPIのインスタンス名を保存
			strcpy(ppi_tp[count].pin_name, ppi[i]->name_ins);

			//ポインタ接続
			ppi_tp[count].sinput = ppi[i];

			//Netlist ID保存
			ppi_tp[count].nl_id = ppi[i]->n;

			//FFCの値はオール0で初期化
			for(k=0; k<ppi_tp[count].tp->int_num; k++){
				ppi_tp[count].tp->x_buf[k] = 0;				//オール0
				ppi_tp[count].tp->p_buf[k] = 0xFFFFFFFF;	//オール1
			}

			//印加
			for(k=0; k<ppi[i]->nval_t1->int_num; k++){
				//1次刻目PPI印加
				ppi[i]->nval_t1->x_buf[k] = ppi_tp[count].tp->x_buf[k];
				ppi[i]->nval_t1->p_buf[k] = ppi_tp[count].tp->p_buf[k];
			}

			//付加PI保存先インデックス更新
			count++;
		}
	}


	//===========================================================================
	// DEBUG
	//===========================================================================
#ifdef TP_DEBUG
	printf("\n\n//------------------------\n");
	printf("// テストパターン格納DEBUG\n");
	printf("//------------------------\n");
	for(i=0; i<(n_se_input+(n_sdff/2)); i++)printf("PI[%d] name:%s nl_id:%d \n", i, pi_tp[i].pin_name, pi_tp[i].nl_id);
	for(i=0;i<(n_scan_ff+(n_sdff/2));i++)printf("PPI[%d] name:%s nl_id:%d \n", i, ppi_tp[i].pin_name, ppi_tp[i].nl_id);

	printf("\n\n");

	printf("-テストパターン構造体の中身-\n");
	for(i=0; i<n_tp; i++){
		//PIのテストパターン表示
		for(j=0; j<(n_se_input+(n_sdff/2)); j++){
			if(pi_tp[j].nl_id != -1){
				if( (Get_NBit(pi_tp[j].tp,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(pi_tp[j].tp,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(pi_tp[j].tp,i)) == 3){
					printf("X");
				}
			}
		}
		//PPIのテストパターン表示
		for(j=(n_scan_ff+(n_sdff/2))-1; 0<=j; j--){
			if(ppi_tp[j].nl_id != -1){
				if( (Get_NBit(ppi_tp[j].tp,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(ppi_tp[j].tp,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(ppi_tp[j].tp,i)) == 3){
					printf("X");
				}
			}
		}
		printf("\n");
	}

	printf("\n\n");


	printf("ピン順番\n");
	for(j=0; j<(n_se_input+(n_sdff/2)); j++){
		if(pi_tp[j].nl_id != -1){
			printf("%s ", pi_tp[j].pin_name);
		}
	}
	for(j=(n_scan_ff+(n_sdff/2))-1; 0<=j; j--){
		printf("%s ", ppi_tp[j].pin_name);
	}

	printf("\n");

	printf("-Netlist構造体の中身-\n");
	for(i=0; i<n_tp; i++){
		//PIのテストパターン表示
		for(j=0; j<(n_se_input+(n_sdff/2)); j++){
			if(pi_tp[j].nl_id != -1){
				if( (Get_NBit(pi_tp[j].sinput->nval_t1,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(pi_tp[j].sinput->nval_t1,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(pi_tp[j].sinput->nval_t1,i)) == 3){
					printf("X");
				}
			}
		}
		//PPIのテストパターン表示
		for(j=(n_scan_ff+(n_sdff/2))-1; 0<=j; j--){
			if(ppi_tp[j].nl_id != -1){
				if( (Get_NBit(ppi_tp[j].sinput->nval_t1,i)) == 0){
					printf("0");
				}
				else if( (Get_NBit(ppi_tp[j].sinput->nval_t1,i)) == 1){
					printf("1");
				}
				else if( (Get_NBit(ppi_tp[j].sinput->nval_t1,i)) == 3){
					printf("X");
				}
			}
		}
		printf("\n");
	}

#endif


}//END
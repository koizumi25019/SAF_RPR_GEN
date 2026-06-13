/*

//------------------------------------------------------------------------
//File name : search_ffr_combinational.c
//Date : 2011/12/31
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Netlist/s_netlist.h"
#include	"../Lib/alloc.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void Make_FFR(NLIST*, NLIST* ,int);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		FFR_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : search_ffr_combinational
//  機  能 : FFR解析[組合せ回路用]
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	search_ffr_combinational(){

	int		i,j;
	

	//============================================================
	// メモリ確保
	//============================================================
	//FFR数計算
	n_ffr = n_fstem+n_po;

	//メモリ確保
	ffr = (struct FoutFreeRegion*)malloc(sizeof(struct FoutFreeRegion) * n_ffr);
	
	
	//============================================================
	//FFR構造体格納【PO対象】
	//============================================================
	for(i=0; i<n_po; i++){

		ffr[i].ffr_id		= i;					//FFR ID
		ffr[i].FoutStem		= po[i];				//PO信号線へのポインタ
		ffr[i].n_out		= 0;					//POなので出力数0
		ffr[i].n_in			= 0;					//0で初期化
		ffr[i].n_pi			= 0;					//PI数初期化
		ffr[i].n_in_net		= 0;					//FFRの入力数を初期化
		ffr[i].Reconv		= NO;					//再収斂しているか？(POは絶対NO！)
		ffr[i].det_tp		= Alloc_Bit_INT(n_tp);	//フラグのメモリ確保
		ffr[i].out			=(FFR**)NULL;			//POは出力FFRが絶対無い！
		ffr[i].flag			= 0;					//フラグ初期化
		ffr[i].n_fault		= 0;					//FFR内故障数初期化
		ffr[i].n_det_fault	= 0;					//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_detect		= 0;					//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_xid_detect	= 0;					//FFR内検出故障数初期化(X抽出後テスト集合)

		//FFRの作成
		Make_FFR(ffr[i].FoutStem, ffr[i].FoutStem, i);

		//入力FFRポインタメモリ確保
		if(ffr[i].n_in == 0){
			ffr[i].in = (FFR**)NULL;
		}
		else{
			ffr[i].in = (FFR**)malloc(sizeof(FFR*) * ffr[i].n_in);
		}

	}
	//============================================================
	// FFR構造体格納【FOUT_STEM対象】
	//============================================================
	j=0;
	for(i=n_po; i<n_ffr; i++){
		
		ffr[i].ffr_id		= i;						//FFR ID
		ffr[i].FoutStem		= fstem[j];					//FOUT_STEM信号線へのポインタ
		ffr[i].n_out		= ffr[i].FoutStem->n_out;	//ブランチ数が他FFRへの出力数
		ffr[i].n_in			= 0;						//0で初期化
		ffr[i].n_pi			= 0;						//PI数初期化
		ffr[i].n_in_net		= 0;						//FFRの入力数を初期化
		ffr[i].det_tp		= Alloc_Bit_INT(n_tp);		//フラグのメモリ確保
		ffr[i].flag			= 0;						//フラグ初期化
		ffr[i].n_fault		= 0;						//FFR内故障数初期化
		ffr[i].n_det_fault	= 0;						//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_detect		= 0;						//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_xid_detect	= 0;						//FFR内検出故障数初期化(X抽出後テスト集合)

		//FFR作成
		Make_FFR(ffr[i].FoutStem, ffr[i].FoutStem, i);

		//入力FFRポインタメモリ確保
		if(ffr[i].n_in == 0){
			ffr[i].in = (FFR**)NULL;
		}
		else{
			ffr[i].in = (FFR**)malloc(sizeof(FFR*) * ffr[i].n_in);
		}

		//fstem番号の更新
		j++;
	}
	
	
#ifdef FFR_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG: FFR確認【組合せ回路】\n");
	printf("//------------------------------\n");
	for(i=0; i<n_fstem+n_po; i++){
		printf("FFR[%d] - STEM:%s  faults:%d\n", i, ffr[i].FoutStem->name, ffr[i].n_fault);
		for(j=0; j<n_net; j++){
			if(nl[j].ffr_id == i){
				printf(" name:%s\n", nl[j].name);
			}
		}
		printf("\n");
	}
#endif

	
	//============================================================
	// 【入力・出力FFR】他FFRと接続(グラフ化)
	//============================================================
	for(i=n_po; i<n_ffr; i++){
		
		//---------------------------------------
		// メモリ確保
		//---------------------------------------
		//出力数分メモリ確保
		ffr[i].out = (FFR**)malloc(sizeof(FFR*) * ffr[i].n_out);

		
		//---------------------------------------
		// FFRポインタ接続
		//---------------------------------------
		for(j=0; j<ffr[i].n_out; j++){
			//出力接続
			ffr[i].out[j] = &ffr[ffr[i].FoutStem->out[j]->ffr_id];

			//入力接続
			if(ffr[i].out[j]->n_in != 0){
				ffr[i].out[j]->in[ffr[i].out[j]->flag] = &ffr[i];
				ffr[i].out[j]->flag++;
			}
		}
	}



#ifdef FFR_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG: FFRグラフ確認【組合せ回路】\n");
	printf("//------------------------------\n");
	for(i=n_ffr-1; i>=0; i--){
		printf("FFR-ID:%d STEM:%s in-net:%d個", ffr[i].ffr_id, ffr[i].FoutStem->name, ffr[i].n_in_net);
		for(j=0; j<ffr[i].n_in; j++){
			printf(" in[%d]:%d", j, ffr[i].in[j]->ffr_id);
		}
		for(j=0; j<ffr[i].n_out; j++){
			printf(" out[%d]:%d", j, ffr[i].out[j]->ffr_id);
		}
		printf("\n");
	}

	printf("\n");
#endif

}//END


//----------------------------------------------
//  関数名 : Make_FFR
//  機  能 : FFR作成(組合せ回路用)
//  戻り値 : なし
//  引  数 : FFRのステム信号線ポインタ，FFRのステム信号線ポインタ，FFR-ID
//----------------------------------------------
void Make_FFR(NLIST *target_line, NLIST *fout_stem, int ffr_id){

	int i;
	
	//FFR内の全信号線に対して値設置
	target_line->fout_stem	= fout_stem;	//FOUT_STEMへのポインタ
	target_line->ffr_id		= ffr_id;		//FFR-ID

	//FFR内の故障数カウント
	if(target_line->test_sf0 == YES){
		ffr[ffr_id].n_fault++;
	}
	if(target_line->test_sf1 == YES){
		ffr[ffr_id].n_fault++;
	}

	//FFRの入力信号線数カウント(PIかFOUTブランチなら入力)
	if(target_line->type==IN || target_line->type==FOUT){
		ffr[ffr_id].n_in_net++;
	}

	//FFRの入力数カウント(FOUTブランチならFFR入力)
	if(target_line->type == FOUT){
		ffr[ffr_id].n_in++;
	}

	//FFR内のPI数カウント
	if(target_line->type == IN){
		ffr[ffr_id].n_pi++;
	}

	//FFRの先頭(ステム)から入力信号線側へ向かって再帰処理
	if(target_line->type != FOUT){
		for(i=0; i<target_line->n_in; i++){
			Make_FFR(target_line->in[i], fout_stem, ffr_id);
		}
	}

}

*/


//------------------------------------------------------------------------
//File name : search_ffr_combinational.c
//Date : 2013/10/13
//Designer : H.Yamazaki
//Ver : 0.03
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/netlist.h"
#include	"../Lib/alloc.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void Make_FFR(NLIST*, NLIST* ,int, unsigned int, int*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		FFR_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : search_ffr_combinational
//  機  能 : FFR解析[組合せ回路用]
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void	search_ffr_combinational(){

	
	int				i,j,k;
	unsigned int	ffr_flag=0;		//同一FFRの二重カウント防止フラグ(入力用)
	int				*ffr_id_temp;	//同一FFRの二重カウント防止フラグ(出力用)
	int				count;
	

	//============================================================
	// メモリ確保
	//============================================================
	//FFR数計算
	n_ffr = n_fstem+n_po;

	//メモリ確保
	ffr = (struct FoutFreeRegion*)malloc(sizeof(struct FoutFreeRegion) * n_ffr);
	ffr_id_temp = INT_alloc(n_ffr);
	
	//============================================================
	//FFR構造体初期化
	//============================================================
	//-----------------------------------------------
	// PO対象
	//-----------------------------------------------
	for(i=0; i<n_po; i++){
		
		//POのみ先にFFR-IDを代入
		po[i]->ffr_id = i;

		//構造体初期化
		ffr[i].ffr_id		= i;					//FFR ID
		ffr[i].FoutStem		= po[i];				//PO信号線へのポインタ
		ffr[i].n_out		= 0;					//POなので出力数0
		ffr[i].n_in			= 0;					//0で初期化
		ffr[i].n_pi			= 0;					//PI数初期化
		ffr[i].n_in_net		= 0;					//FFRの入力数を初期化
		ffr[i].Reconv		= NO;					//再収斂しているか？(POは絶対NO！)
		ffr[i].det_tp		= Alloc_Bit_INT(n_tp);	//フラグのメモリ確保
		ffr[i].out			=(FFR**)NULL;			//POは出力FFRが絶対無い！
		ffr[i].flag			= 0;					//フラグ初期化
		ffr[i].n_fault		= 0;					//FFR内故障数初期化
		ffr[i].n_det_fault	= 0;					//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_detect		= 0;					//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_xid_detect	= 0;					//FFR内検出故障数初期化(X抽出後テスト集合)
	}


	//-----------------------------------------------
	// FOUT-STEM対象
	//-----------------------------------------------
	j=0;
	for(i=n_po; i<n_ffr; i++){
		
		//STEMのみ先にFFR-IDを代入
		fstem[j]->ffr_id = i;

		//構造体初期化
		ffr[i].ffr_id		= i;						//FFR ID
		ffr[i].FoutStem		= fstem[j];					//FOUT_STEM信号線へのポインタ
		ffr[i].n_out		= 0;						//0で初期化
		ffr[i].n_in			= 0;						//0で初期化
		ffr[i].n_pi			= 0;						//PI数初期化
		ffr[i].n_in_net		= 0;						//FFRの入力数を初期化
		ffr[i].det_tp		= Alloc_Bit_INT(n_tp);		//フラグのメモリ確保
		ffr[i].flag			= 0;						//フラグ初期化
		ffr[i].n_fault		= 0;						//FFR内故障数初期化
		ffr[i].n_det_fault	= 0;						//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_detect		= 0;						//FFR内検出故障数初期化(初期テスト集合)
		ffr[i].n_xid_detect	= 0;						//FFR内検出故障数初期化(X抽出後テスト集合)
		
		//fstem番号の更新
		j++;
	}

	
	//============================================================
	//入力FFRグラフ作成
	//============================================================
	//-----------------------------------------------
	/// PO対象
	//-----------------------------------------------
	for(i=0; i<n_po; i++){
		
		//FFRフラグ更新
		ffr_flag++;
		
#ifdef FFR_DEBUG
		printf("\nFFR[%d] %s (ffr_flag:%d)\n", i, ffr[i].FoutStem->name, ffr_flag);
#endif
		//FFRの作成
		Make_FFR(ffr[i].FoutStem, ffr[i].FoutStem, i, ffr_flag, ffr_id_temp);

		//入力FFRポインタメモリ確保
		if(ffr[i].n_in == 0){
			ffr[i].in = (FFR**)NULL;
		}
		else{
			//メモリ確保
			ffr[i].in = (FFR**)malloc(sizeof(FFR*) * ffr[i].n_in);

			//入力FFRをポインタ接続
			for(k=0; k<ffr[i].n_in; k++){
				ffr[i].in[k] = &ffr[ffr_id_temp[k]];
			}
		}

	}

	//-----------------------------------------------
	// FOUT-STEM対象
	//-----------------------------------------------
	for(i=n_po; i<n_ffr; i++){
		
		//FFRフラグ更新
		ffr_flag++;
			
#ifdef FFR_DEBUG
		printf("\nFFR[%d] %s (ffr_flag:%d)\n", i, ffr[i].FoutStem->name, ffr_flag);
#endif
		//FFR作成
		Make_FFR(ffr[i].FoutStem, ffr[i].FoutStem, i, ffr_flag, ffr_id_temp);

		//入力FFRポインタメモリ確保
		if(ffr[i].n_in == 0){
			ffr[i].in = (FFR**)NULL;
		}
		else{
			//メモリ確保
			ffr[i].in = (FFR**)malloc(sizeof(FFR*) * ffr[i].n_in);
			
			//入力FFRをポインタ接続
			for(k=0; k<ffr[i].n_in; k++){
				ffr[i].in[k] = &ffr[ffr_id_temp[k]];
			}
		}
	}
	
	
#ifdef FFR_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG: FFR確認【組合せ回路】\n");
	printf("//------------------------------\n");
	for(i=0; i<n_fstem+n_po; i++){
		printf("FFR[%d] - STEM:%s  faults:%d\n", i, ffr[i].FoutStem->name, ffr[i].n_fault);
		for(j=0; j<n_net; j++){
			if(nl[j].ffr_id == i){
				printf(" name:%s\n", nl[j].name);
			}
		}
		printf("\n");
	}
#endif

	//============================================================
	// 出力FFRポインタ接続
	//============================================================
	for(i=n_po; i<n_ffr; i++){
		
		//カウンタ初期化
		count=0;

		//FFR出力数の計算
		for(j=0; j<ffr[i].FoutStem->n_out; j++){
			//計算済みFFR調査
			for(k=0; k<count; k++){
				if(ffr[i].FoutStem->out[j]->ffr_id == ffr_id_temp[k]){
					break;
				}
			}
			//FFR出力数更新
			if(k == count){
				ffr[i].n_out++;
				ffr_id_temp[count] = ffr[i].FoutStem->out[j]->ffr_id;
				count++;
			}
		}
		
		//--------------------------------------
		//出力数分メモリ確保
		//--------------------------------------
		ffr[i].out = (FFR**)malloc(sizeof(FFR*) * ffr[i].n_out);
				
		//--------------------------------------
		//出力FFRをポインタ接続
		//--------------------------------------
		for(k=0; k<count; k++){
			ffr[i].out[k] = &ffr[ffr_id_temp[k]];
		}
	}

	


#ifdef FFR_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG: FFRグラフ確認【組合せ回路】\n");
	printf("//------------------------------\n");
	for(i=n_ffr-1; i>=0; i--){
		printf("FFR-ID:%d STEM:%s in-net:%d個", ffr[i].ffr_id, ffr[i].FoutStem->name, ffr[i].n_in_net);
		for(j=0; j<ffr[i].n_in; j++){
			printf(" in[%d]:%d", j, ffr[i].in[j]->ffr_id);
		}
		for(j=0; j<ffr[i].n_out; j++){
			printf(" out[%d]:%d", j, ffr[i].out[j]->ffr_id);
		}
		printf("\n");
	}

	printf("\n");
#endif

}//END


//----------------------------------------------
//  関数名 : Make_FFR
//  機  能 : FFR作成(組合せ回路用)
//  戻り値 : なし
//  引  数 : FFRのステム信号線ポインタ，FFRのステム信号線ポインタ，FFR-ID, 同一FFR二重カウント防止フラグ, 入力FFRのFFR_ID
//----------------------------------------------
void Make_FFR(NLIST *target_line, NLIST *fout_stem, int ffr_id, unsigned int ffr_flag, int	*ffr_id_temp){

	int i;
	
	//FFR内の全信号線に対して値設置
	target_line->fout_stem	= fout_stem;	//FOUT_STEMへのポインタ
	target_line->ffr_id		= ffr_id;		//FFR-ID

	//FFR内の故障数カウント
	if(target_line->test_sf0 == YES){
		ffr[ffr_id].n_fault++;
	}
	if(target_line->test_sf1 == YES){
		ffr[ffr_id].n_fault++;
	}

	//FFRの入力信号線数カウント(PIかFOUTブランチなら入力)
	if(target_line->type==IN || target_line->type==FOUT){
		ffr[ffr_id].n_in_net++;
	}

	//FFRの入力数カウント(FOUTブランチならFFR入力)
	if(target_line->type == FOUT){
		if(target_line->in[0]->flag != ffr_flag){
			
			//入力FFRのIDを保存
			ffr_id_temp[ffr[ffr_id].n_in] = target_line->in[0]->ffr_id;

			//入力FFR数更新
			ffr[ffr_id].n_in++;					//入力FFR数更新
			target_line->in[0]->flag = ffr_flag;	//入力FFRにフラグ設置
#ifdef FFR_DEBUG
			printf("入力FFRステム：%s (FFR_ID:%d)\n", target_line->in[0]->name, target_line->in[0]->ffr_id);
#endif
		}
	}

	//FFR内のPI数カウント
	if(target_line->type == IN){
		ffr[ffr_id].n_pi++;
	}

	//FFRの先頭(ステム)から入力信号線側へ向かって再帰処理
	if(target_line->type != FOUT){
		for(i=0; i<target_line->n_in; i++){
			Make_FFR(target_line->in[i], fout_stem, ffr_id, ffr_flag, ffr_id_temp);
		}
	}

}
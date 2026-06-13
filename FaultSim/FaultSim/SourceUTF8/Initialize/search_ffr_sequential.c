//------------------------------------------------------------------------
//File name : search_ffr_sequential.c
//Date : 2013/10/13
//Designer : H.Yamazaki
//Ver : 0.03
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	"../Netlist/s_netlist.h"
#include	"../Lib/alloc.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
void Make_SFFR(S_NLIST*, S_NLIST* ,int, unsigned int, int*);

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
	//#define		FFR_DEBUG

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : search_ffr_sequential
//  機  能 : FFR解析[順序回路用]
//  戻り値 : なし
//  引  数 : なし
//----------------------------------------------
void search_ffr_sequential(){

	int				i,j,k;
	unsigned int	ffr_flag=0;		//同一FFRの二重カウント防止フラグ(入力用)
	int				*ffr_id_temp;	//同一FFRの二重カウント防止フラグ(出力用)
	int				count;

	//============================================================
	// メモリ確保
	//============================================================
	//FFR数計算
	n_ffr = n_sfstem + n_spo + n_ppo;
	
	//メモリ確保
	sffr = (struct SFoutFreeRegion*)malloc(sizeof(struct SFoutFreeRegion) * n_ffr);
	ffr_id_temp = INT_alloc(n_ffr);

	//============================================================
	//FFR構造体初期化
	//============================================================
	//-----------------------------------------------
	// PO対象
	//-----------------------------------------------
	for(i=0; i<n_spo; i++){

		//POのみ先にFFR-IDを代入
		spo[i]->ffr_id = i;

		//構造体初期化
		sffr[i].ffr_id		= i;					//FFR ID
		sffr[i].FoutStem	= spo[i];				//PO信号線へのポインタ
		sffr[i].n_out		= 0;					//POなので出力数0
		sffr[i].n_in		= 0;					//0で初期化
		sffr[i].n_pi		= 0;					//PI数初期化
		sffr[i].n_ppi		= 0;					//PPI数初期化
		sffr[i].n_in_net	= 0;					//FFRの入力数を初期化
		sffr[i].Reconv		= NO;					//再収斂しているか？(POは絶対NO！)
		sffr[i].det_tp		= Alloc_Bit_INT(n_tp);	//フラグのメモリ確保
		sffr[i].out			=(SFFR**)NULL;			//POは出力FFRが絶対無い！
		sffr[i].flag		= 0;					//フラグ初期化
		sffr[i].n_fault		= 0;					//FFR内故障数初期化
		sffr[i].n_det_fault	= 0;					//FFR内検出故障数初期化(初期テスト集合)
		sffr[i].n_detect	= 0;					//FFR内検出故障数初期化(初期テスト集合)
		sffr[i].n_xid_detect= 0;					//FFR内検出故障数初期化(X抽出後テスト集合)
	}

	//-----------------------------------------------
	// PPO対象
	//-----------------------------------------------
	j=0;
	for(i; i<(n_spo + n_ppo); i++){

		//PPOのみ先にFFR-IDを代入
		ppo[j]->ffr_id = i;

		//構造体初期化
		sffr[i].ffr_id		= i;						//FFR ID
		sffr[i].FoutStem	= ppo[j];					//PPO信号線へのポインタ
		sffr[i].n_out		= 0;						//0で初期化
		sffr[i].n_in		= 0;						//0で初期化
		sffr[i].n_pi		= 0;						//PI数初期化
		sffr[i].n_ppi		= 0;						//PPI数初期化
		sffr[i].n_in_net	= 0;						//FFRの入力数を初期化
		sffr[i].Reconv		= NO;						//再収斂しているか？(POは絶対NO！)
		sffr[i].det_tp		= Alloc_Bit_INT(n_tp);		//フラグのメモリ確保
		sffr[i].flag		= 0;						//フラグ初期化
		sffr[i].n_fault		= 0;						//FFR内故障数初期化
		sffr[i].n_det_fault	= 0;						//FFR内検出故障数初期化(初期テスト集合)
		sffr[i].n_detect	= 0;						//FFR内検出故障数初期化(初期テスト集合)
		sffr[i].n_xid_detect= 0;						//FFR内検出故障数初期化(X抽出後テスト集合)
		
		//PPO番号の更新
		j++;
	}

	//-----------------------------------------------
	// FOUT-STEM対象
	//-----------------------------------------------
	j=0;
	for(i; i<n_ffr; i++){

		//STEMのみ先にFFR-IDを代入
		sfstem[j]->ffr_id = i;

		//構造体初期化
		sffr[i].ffr_id		= i;						//FFR ID
		sffr[i].FoutStem	= sfstem[j];				//FOUT_STEM信号線へのポインタ
		sffr[i].n_out		= 0;						//0で初期化
		sffr[i].n_in		= 0;						//0で初期化
		sffr[i].n_pi		= 0;						//PI数初期化
		sffr[i].n_ppi		= 0;						//PPI数初期化
		sffr[i].n_in_net	= 0;						//FFRの入力数を初期化
		sffr[i].det_tp		= Alloc_Bit_INT(n_tp);		//フラグのメモリ確保
		sffr[i].flag		= 0;						//フラグ初期化
		sffr[i].n_fault		= 0;						//FFR内故障数初期化
		sffr[i].n_det_fault	= 0;						//FFR内検出故障数初期化(初期テスト集合)
		sffr[i].n_detect	= 0;						//FFR内検出故障数初期化(初期テスト集合)
		sffr[i].n_xid_detect= 0;						//FFR内検出故障数初期化(X抽出後テスト集合)
		
		//fstem番号の更新
		j++;
	}
	
	//============================================================
	//入力FFRグラフ作成
	//============================================================
	//-----------------------------------------------
	/// PO対象
	//-----------------------------------------------
	for(i=0; i<n_spo; i++){

		//FFRフラグ更新
		ffr_flag++;
		
#ifdef FFR_DEBUG
		printf("\nFFR[%d] %s (ffr_flag:%d)\n", i, sffr[i].FoutStem->name, ffr_flag);
#endif
		//FFRの作成
		Make_SFFR(sffr[i].FoutStem, sffr[i].FoutStem, i, ffr_flag, ffr_id_temp);

		//入力FFRポインタメモリ確保
		if(sffr[i].n_in == 0){
			sffr[i].in = (SFFR**)NULL;
		}
		else{
			//メモリ確保
			sffr[i].in = (SFFR**)malloc(sizeof(SFFR*) * sffr[i].n_in);
			
			//入力FFRをポインタ接続
			for(k=0; k<sffr[i].n_in; k++){
				sffr[i].in[k] = &sffr[ffr_id_temp[k]];
			}
		}
	}

	//-----------------------------------------------
	// PPO対象
	//-----------------------------------------------
	for(i; i<(n_spo + n_ppo); i++){
		
		//FFRフラグ更新
		ffr_flag++;		
		
#ifdef FFR_DEBUG
		printf("\nFFR[%d] %s (ffr_flag:%d)\n", i, sffr[i].FoutStem->name, ffr_flag);
#endif		
		//FFRの作成
		Make_SFFR(sffr[i].FoutStem, sffr[i].FoutStem, i, ffr_flag, ffr_id_temp);

		//入力FFRポインタメモリ確保
		if(sffr[i].n_in == 0){
			sffr[i].in = (SFFR**)NULL;
		}
		else{
			//メモリ確保
			sffr[i].in = (SFFR**)malloc(sizeof(SFFR*) * sffr[i].n_in);

			//入力FFRをポインタ接続
			for(k=0; k<sffr[i].n_in; k++){
				sffr[i].in[k] = &sffr[ffr_id_temp[k]];
			}
		}
	}
	
	//-----------------------------------------------
	// FOUT-STEM対象
	//-----------------------------------------------
	for(i; i<n_ffr; i++){
		
		//FFRフラグ更新
		ffr_flag++;

#ifdef FFR_DEBUG
		printf("\nFFR[%d] %s (ffr_flag:%d)\n", i, sffr[i].FoutStem->name, ffr_flag);
#endif
		//FFRの作成
		Make_SFFR(sffr[i].FoutStem, sffr[i].FoutStem, i, ffr_flag, ffr_id_temp);

		//入力FFRポインタメモリ確保
		if(sffr[i].n_in == 0){
			sffr[i].in = (SFFR**)NULL;
		}
		else{
			//メモリ確保
			sffr[i].in = (SFFR**)malloc(sizeof(SFFR*) * sffr[i].n_in);

			//入力FFRをポインタ接続
			for(k=0; k<sffr[i].n_in; k++){
				sffr[i].in[k] = &sffr[ffr_id_temp[k]];
			}
		}
	}	

	
	//============================================================
	// 出力FFRポインタ接続
	//============================================================
	for(i=n_spo; i<n_ffr; i++){
		
		//--------------------------------------
		//カウンタ初期化
		//--------------------------------------
		count=0;
		
		//--------------------------------------
		//FFR出力数の計算
		//--------------------------------------
		for(j=0; j<sffr[i].FoutStem->n_out; j++){
			//計算済みFFR調査
			for(k=0; k<count; k++){
				if(sffr[i].FoutStem->out[j]->ffr_id == ffr_id_temp[k]){
					break;
				}
			}
			//FFR出力数更新
			if(k == count){
				sffr[i].n_out++;
				ffr_id_temp[count] = sffr[i].FoutStem->out[j]->ffr_id;
				count++;
			}
		}
		
		//--------------------------------------
		//出力数分メモリ確保
		//--------------------------------------
		sffr[i].out = (SFFR**)malloc(sizeof(SFFR*) * sffr[i].n_out);
				
		//--------------------------------------
		//出力FFRをポインタ接続
		//--------------------------------------
		for(k=0; k<count; k++){
			sffr[i].out[k] = &sffr[ffr_id_temp[k]];
		}
	}

	
#ifdef FFR_DEBUG
	printf("\n//------------------------------\n");
	printf("// DEBUG: FFR確認【順序回路】\n");
	printf("//------------------------------\n");
	for(i=0; i<n_ffr; i++){
		printf("FFR[%d] - STEM:%s  faults:%d\n", i, sffr[i].FoutStem->name, sffr[i].n_fault);
		printf("PI数:%d  PPI数:%d\n", sffr[i].n_pi, sffr[i].n_ppi);
		for(j=0; j<n_snet; j++){
			if(s_nl[j].ffr_id == i){
				printf(" name:%s\n", s_nl[j].name);
			}
		}
		printf("\n");
	}
	printf("\n");
	
	printf("グラフ確認\n");
	for(i=0; i<n_ffr; i++){
		printf("FFR[%d] - STEM:%s  in:%d  out:%d\n", i, sffr[i].FoutStem->name, sffr[i].n_in, sffr[i].n_out);
		printf("・入力\n");
		for(j=0; j<sffr[i].n_in; j++){
			printf("  FFR[%d]\n", sffr[i].in[j]->ffr_id);
		}
		printf("・出力\n");
		for(j=0; j<sffr[i].n_out; j++){
			printf("  FFR[%d]\n", sffr[i].out[j]->ffr_id);
		}
		printf("\n");
	}
	printf("\n");
#endif

}//END


//----------------------------------------------
//  関数名 : Make_SFFR
//  機  能 : FFR作成(順序回路用)
//  戻り値 : なし
//  引  数 : FFRのステム信号線ポインタ，FFRのステム信号線ポインタ，FFR-ID, 同一FFR二重カウント防止フラグ, 入力FFRのFFR_ID
//----------------------------------------------
void Make_SFFR(S_NLIST *target_line, S_NLIST *fout_stem, int ffr_id, unsigned int ffr_flag, int	*ffr_id_temp){

	int i;
	
	//FFR内の全信号線に対して値設置
	target_line->fout_stem	= fout_stem;	//FOUT_STEMへのポインタ
	target_line->ffr_id		= ffr_id;		//FFR-ID

	//FFR内の故障数カウント(※テスト可能信号線の場合)
	if(target_line->test_str==YES && target_line->testable_net==YES){
		sffr[ffr_id].n_fault++;
	}
	if(target_line->test_stf==YES && target_line->testable_net==YES){
		sffr[ffr_id].n_fault++;
	}

	//FFRの入力信号線数カウント(PIかFOUTブランチかDFF(PPI)なら入力)
	if(target_line->type==IN || target_line->type==FOUT || target_line->type==DFF){
		sffr[ffr_id].n_in_net++;
	}

	//他FFRからの入力数カウント(FOUTかDFFならFFR入力)
	if(target_line->type==FOUT || target_line->type==DFF){
		if(target_line->in[0]->flag != ffr_flag){

			//入力FFRのIDを保存
			ffr_id_temp[sffr[ffr_id].n_in] = target_line->in[0]->ffr_id;

			//入力FFR数更新
			sffr[ffr_id].n_in++;					//入力FFR数更新
			target_line->in[0]->flag = ffr_flag;	//入力FFRにフラグ設置
#ifdef FFR_DEBUG
			printf("入力FFRステム：%s (FFR_ID:%d)\n", target_line->in[0]->name, target_line->in[0]->ffr_id);
#endif
		}
	}

	//FFR内のPI数カウント
	if(target_line->type == IN){
		sffr[ffr_id].n_pi++;
	}
	
	//FFR内のPPI数カウント
	if(target_line->type == DFF){
		sffr[ffr_id].n_ppi++;
	}

	//FFRの先頭(ステム)から入力信号線側へ向かって再帰処理
	if(target_line->type!=FOUT && target_line->type!=DFF){	//※FOUTとDFF(PPI)の場合は遡らない
		for(i=0; i<target_line->n_in; i++){
			Make_SFFR(target_line->in[i], fout_stem, ffr_id, ffr_flag, ffr_id_temp);
		}
	}
}
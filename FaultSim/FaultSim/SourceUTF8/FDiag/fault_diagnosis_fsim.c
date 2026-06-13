//------------------------------------------------------------------------
//File name : fault_diagnosis_fsim.c
//Date : 2012/8/4
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include	<stdio.h>
#include    <stdlib.h>
#include	<string.h>
#include	"../Netlist/netlist.h"
#include	"../Lib/bit_int.h"
#include	"../StandardHead.h"
#include	"../option.h"

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
int		SAF_2v_PPSFP_Diag		(int, NLIST*, unsigned int, int);
void	LSIM_2v_combinational	();

//---------------------------------------------------------------------
// 定義
//---------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE
#pragma warning ( disable : 4996 )

typedef struct _Fault_LSI_{
	char			*lsi_name;		//故障LSI名
	char			*f_net;			//故障信号線名
	int				ftype;			//故障種類 {SAF0, SAF1}
	int				net_id;			//故障信号線のID
	int				n_ftp;			//フェイルテストパターン数
}FLSI;


//================================
//|  ・故障LSIリスト	         |
//|  (LSI名 信号線名 故障タイプ) |
//================================
//|  LSI1 a SF0                  |
//|  LSI2 b SF1                  |
//|  LSI3 c SF0                  |
//================================

//------------------------------------------------------------------------
//  外部関数
//------------------------------------------------------------------------
//----------------------------------------------
//  関数名 : fault_diagnosis_fsim
//  機  能 : 【縮退故障】故障診断用故障シミュレーション(疑似テスター)
//  戻り値 : なし
//  引  数 : 故障LSIファイルポインタ(fp), 診断用FSIM結果書き込みファイルポインタ(fpo)
//----------------------------------------------
void	fault_diagnosis_fsim(FILE* fp, FILE* fpo){
	
	int			i,j,k,m;
	int			ed_flag=0;
	char		str[MAXN];
	int			n_lsi=0;		//故障LSI数
	FLSI		*LSI;			//故障LSIリスト
	char		*name;			//LSI名
	char		*net;			//故障信号線名
	char		*flt;			//故障タイプ
	int			first_flag;		//ファイル出力用のフラグ
	int			count;
	
	//=======================================================
	//初期化	
	//=======================================================
	// 全信号線のフラグ初期化
	for(i=0; i<n_net; i++)nl[i].flag = 0;

	//1故障分の故障辞書【fdic_po:故障検出したテストパターンと検出したPO情報】
	fdic_po = (BIT_INT **)malloc(sizeof(BIT_INT *) * n_tp);		//タテ
	for(i=0;i<n_tp;i++)fdic_po[i] = Alloc_Bit_INT(n_po);		//ヨコ

	//PO-ID保存
	for(i=0; i<n_po; i++)po[i]->po_id = i;

	//i初期化
	i=0;

	//=======================================================
	// 故障LSI情報読み込み
	//=======================================================
	//故障LSI数確認
	while(fgets(str,MAXN,fp)!=NULL)n_lsi++;

	//ファイルポインタを先頭に戻す
	rewind(fp);

	//故障LSI情報格納先メモリ確保
	LSI = (FLSI*)malloc(n_lsi * sizeof(FLSI));
	
	//------------------------------
	//故障LSI情報読み込み
	//------------------------------
	while(fgets(str,MAXN,fp)!=NULL){
		//故障LSI名読込み
		name = strtok(str, " \t");
		
		//信号線名読込み
		net = strtok('\0'," \t");

		//故障タイプ読込み
		flt = strtok('\0'," \t\n");

		//------------------------------
		//故障LSI情報保存
		//------------------------------
		//故障LSI名保存
		LSI[i].lsi_name = (char*)malloc(sizeof(char) * (strlen(name)));
		strcpy(LSI[i].lsi_name, name);

		//故障信号線名保存
		LSI[i].f_net = (char*)malloc(sizeof(char) * (strlen(net)));
		strcpy(LSI[i].f_net, net);
		
		//故障種類保存
		if(!strcmp(flt,"SF0")){
			LSI[i].ftype = SAF0;	//0縮退故障
		}
		else if(!strcmp(flt,"SF1")){
			LSI[i].ftype = SAF1;	//1縮退故障
		}
		else{
			fprintf(stderr,"Fault list wrong!\n");
			exit(-1);
		}

		//------------------------------
		//故障信号線探索
		//------------------------------
		for(j=0; j<n_net; j++){
			if(!strcmp(net,nl[j].name)){
				if(LSI[i].ftype == SAF0){
					nl[j].test_sf0 = YES;	//故障フラグ設定
					LSI[i].net_id = j;		//信号線ID保存
					break;
				}
				else if(LSI[i].ftype == SAF1){
					nl[j].test_sf1 = YES;	//故障フラグ設定
					LSI[i].net_id = j;		//信号線ID保存
					break;
				}
			}
		}
		//テスト対象とする信号線がない
		if(j==n_net){
			fprintf(stderr,"Not found signal : %s\n",net);
		}

		//LSI番号更新
		i++;
	}


	//=======================================================
	// 故障シミュレーション(疑似テスター)
	//=======================================================
	//-------------------------------------
	// 2値シミュレーション(0,1)
	//-------------------------------------
	if(n_before_x == 0){
		//論理シミュレーション
		LSIM_2v_combinational();
		
		//故障シミュレーション(PPSFP)
		for(i=0; i<n_lsi; i++){
			//----------------------------------------
			// 初期化
			//----------------------------------------
			LSI[i].n_ftp = 0;		//フェイルテストパターン数初期化
			for(k=0; k<n_tp; k++)All_INT_Zero(fdic_po[k]);	//故障検出PO辞書初期化

			//----------------------------------------
			// FSIM開始
			//----------------------------------------
			for(j=0; j<(int)n_tp_int; j++){	//確保したunsigned int数分ループ

				//故障値挿入
				nl[LSI[i].net_id].x_fault = ~nl[LSI[i].net_id].nval->x_buf[j];	//x_fault に x_buffの反転値を代入
			
				//イベントドリブン用フラグ設定
				ed_flag+=2;										//計算済みflag値を決定(故障伝搬確定)
				nl[LSI[i].net_id].flag = ed_flag;				//故障信号線にflag値を代入
			
				//PPSFPイベントドリブン開始
				LSI[i].n_ftp += SAF_2v_PPSFP_Diag(i, &nl[LSI[i].net_id], ed_flag, j);	//2値PPSFPイベントドリブン

			}

			//----------------------------------------
			// FSIM情報ファイル出力
			//----------------------------------------
			//LSI名　フェイルパターン数　ファイル出力
			fprintf(fpo, "%s %d", LSI[i].lsi_name, LSI[i].n_ftp);
			count = LSI[i].n_ftp;

			//フェイルパターンと検出PO　ファイル出力
			for(k=0; k<n_tp; k++){
				first_flag = 0;	//フラグ初期化
				for(m=0; m<n_po; m++){
					if(Get_NBit_INT(fdic_po[k], m)==1){
						//初めて出力
						if(first_flag == 0){
							fprintf(fpo, " (t%d, po%d", k, m);
							first_flag=1;
							count--;
						}
						//2回目以降
						else{
							fprintf(fpo, ", po%d", m);
						}
					}
				}
				if(first_flag != 0){
					fprintf(fpo, ")");
				}				
			}
			fprintf(fpo, "\n");
		}
	}
	//-------------------------------------
	// 3値シミュレーション(0,1,X)
	//-------------------------------------
	else{
		printf("\n//==========================\n");
		printf("// ERROR\n");
		printf("//==========================\n");
		printf("3値{0,1,X}テストパターンは未対応!\n\n");
	}

}
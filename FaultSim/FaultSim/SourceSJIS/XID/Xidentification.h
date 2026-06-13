//------------------------------------------------------------------------
//File name : Xidentificaion.h
//Date : 2012/08/11
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------
#include <stdio.h>
#include "../Netlist/netlist.h"
#include "../Netlist/s_netlist.h"

//========================================================================
// 定義
//========================================================================

//------------------
//    X-buf   P-buf|
//-----------------|
// 0 |   0  |  1   | 
// 1 |   1  |  0   |
// X |   1  |  1   |
// -----------------
//========================================================================
// プロトタイプ宣言
//========================================================================

//========================================================================
// 構造体
//========================================================================
//---------------------------------------------------
//テストパターン毎のビット情報 (n_tp数分保持)
//---------------------------------------------------
typedef struct TestPattern_Bit{
	int				tp_id;					//テストパターン番号
	int				ui_num;					//対象テストパターンがunsigned intの何個目か(tp_infoと同じ)
	int				bit_num;				//対象テストパターンがunsigned intの何bit目か(tp_infoと同じ)
	int				n_cbit;					//対象テストパターンが保持しているケアビット数(0と1)
	int				n_0bit;					//対象テストパターンが保持している0数
	int				n_1bit;					//対象テストパターンが保持している1数
}TPBIT;


//---------------------------------------------------
//外部入力毎のビット情報 (n_pi数分保持)
//---------------------------------------------------
typedef struct PrimaryInput_Bit{
	int				pi_id;					//(疑似)外部入力番号
	int				n_cbit;					//対象外部入力が保持しているケアビット数(0と1)
	int				n_0bit;					//対象外部入力が保持している0数
	int				n_1bit;					//対象外部入力が保持している1数
}PIBIT;


//========================================================================
// 外部変数
//========================================================================
//テストパターン毎のBIT情報
TPBIT			*tp_bit;			//X抽出時に内部データ値が更新

//外部入力毎のBIT情報
PIBIT			*pi_bit;			//X抽出時に内部データ値が更新
PIBIT			*ppi_bit;			//X抽出時に内部データ値が更新

//関数ポインタ変数
//縮退故障用
void(*func_path[20])(NLIST*, int, int, unsigned int, unsigned int);			//故障伝搬経路用関数ポインタ
void(*func_jus[20])(NLIST*, int, unsigned int);								//正当化(限定含意・限定正当化)用関数ポインタ
void(*func_exjus[20])(NLIST*, int, int, unsigned int);						//正当化(拡張含意・拡張正当化)用関数ポインタ(※見逃し故障用)
void(*func_miss_jus[20])(NLIST*, int, unsigned int, unsigned int, int);		//見逃し故障専用正当化用関数ポインタ
void(*func_sppfp[20])(NLIST*, int, unsigned int);							//X抽出用の(2値or3値)SPPFP関数ポインタ
void(*func_3v_sppfp[20])(NLIST*, int, unsigned int);						//X抽出後テストパターン用の3値SPPFPポインタ
void(*func_3v_ppsfp[20])(NLIST*, int, unsigned int);						//X抽出後テスト集合用の3値PPSFPポインタ

//遷移故障用
void(*sfunc_path[20])(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int);	//故障伝搬経路用関数ポインタ
void(*sfunc_jus[20])(S_NLIST*, int, unsigned int, unsigned int);						//正当化(限定含意・限定正当化)用関数ポインタ
void(*sfunc_exjus[20])(S_NLIST*, int, int, unsigned int);								//正当化(拡張含意・拡張正当化)用関数ポインタ(※見逃し故障用)
void(*sfunc_miss_jus[20])(S_NLIST*, int, unsigned int, unsigned int, int);				//見逃し故障専用正当化用関数ポインタ
void(*sfunc_sppfp[20])(S_NLIST*, int, unsigned int);									//X抽出用の(2値or3値)SPPFP関数ポインタ
void(*sfunc_3v_sppfp[20])(S_NLIST*, int, unsigned int);									//X抽出後テストパターン用の3値SPPFPポインタ
void(*sfunc_3v_ppsfp[20])(S_NLIST*, int, unsigned int);									//X抽出後テスト集合用の3値PPSFPポインタ

//優先パスXID用(遷移故障)
void(*sfunc_pripath[20])(S_NLIST*, int, int, unsigned int, unsigned int, unsigned int, unsigned int);	//優先パス故障伝搬経路用関数ポインタ
void(*sfunc_prijus[20])(S_NLIST*, int, unsigned int, unsigned int, unsigned int);						//優先パス正当化(限定含意・限定正当化)用関数ポインタ
//------------------------------------------------------------------------
//File name : mt19937ar.h
//Date : 2010/9/17
//Designer : H.Yamazaki
//Mersenne Twister
//------------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
/* initializes mt[N] with a seed */
// seed初期化
void init_genrand(unsigned long s);

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned long init_key[], int key_length);

// 32ビットの整数乱数 0x0以上 0xffffffff以下
unsigned long genrand_int32(void);

// 31ビットの整数乱数 0x0以上 0x7fffffff以下
long genrand_int31(void);

/* These real versions are due to Isaku Wada, 2002/01/09 added */
// 0以上 1以下の実数乱数
double genrand_real1(void);

// 0以上 1未満の実数乱数
double genrand_real2(void);

// 0より大きく 1未満の実数乱数
double genrand_real3(void);

// 0以上 1未満の実数乱数
double genrand_res53(void);

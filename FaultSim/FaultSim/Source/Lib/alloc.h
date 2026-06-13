//------------------------------------------------------------------------
//File name : alloc.c
//Date : 2011/6/13
//Designer : H.Yamazaki
//Ver : 0.01
//------------------------------------------------------------------------

//---------------------------------------------------------------------
// プロトタイプ宣言
//---------------------------------------------------------------------
//----------------------------
// alloc.c
//----------------------------
void *mem_alloc(int, int);


//---------------------------------------------------------------------
//  マクロ
//---------------------------------------------------------------------
#define		 CHAR_alloc(n)		((char     *)mem_alloc(n, sizeof(char)))
#define		PCHAR_alloc(n)		((char    **)mem_alloc(n, sizeof(char *)))
#define		  INT_alloc(n)		((int      *)mem_alloc(n, sizeof(int)))
#define		PVOID_alloc(n)		((void    **)mem_alloc(n, sizeof(void *)))
#define		   NL_alloc(n)		((NLIST    *)mem_alloc(n, sizeof(NLIST)))
#define		  PNL_alloc(n)		((NLIST   **)mem_alloc(n, sizeof(NLIST *)))
#define		  SPNL_alloc(n)		((S_NLIST   **)mem_alloc(n, sizeof(S_NLIST *)))
#define  UNSIGNED_alloc(n)      ((unsigned int *)mem_alloc(n,sizeof(unsigned int)))
#define	 PATHGRAPH_alloc(n)		((PATHGRAPH *)mem_alloc(n, sizeof(PATHGRAPH)))
#define	P_PATHNODE_alloc(n)		((PATHNODE  **)mem_alloc(n, sizeof(PATHNODE *)))
#define	  PATHNODE_alloc(n)		((PATHNODE  *)mem_alloc(n, sizeof(PATHNODE)))


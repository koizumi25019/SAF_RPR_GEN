#pragma once

#include "fdp/xid/xid_adapter.h"

typedef enum {
    XID_ZERO = 0,
    XID_ONE,
    XID_X
} XID_VALUE;

/* xid_fsim のイベント状態（ed_tag）。 */
typedef enum {
    XID_UNTAGGED = 0,  /* 故障シミュレーションのイベントが届いていない（故障の影響を受けない） */
    XID_STACKED  = 1,  /* レベルスタックに積まれ、評価待ち */
    XID_COMPUTED = 2   /* 故障値を評価済み */
} XID_ED_TAG;

/* xid_tag のビット。そのネットの正常値・故障値の3値要求が決まっていることを表す。 */
typedef enum {
    XID_FLAG_NONE   = 0,
    XID_FLAG_NORMAL = 1,
    XID_FLAG_FAULT  = 2,
    XID_FLAG_BOTH   = 3
} XID_TAG_FLAG;

typedef struct XID_VAR_INFO {
    int    normal_2value;
    int    fault_2value;
    int    normal_3value;
    int    fault_3value;
    size_t ed_tag;
    size_t xid_tag;
} XID_VAR_INFO;

typedef struct DETECT_PO {
    size_t  n_det_po;
    size_t* po_id;
} DETECT_PO;

/* 外部入力(PI)のドントケア埋め（外部XIDプロセス呼び出しの代替）。
   malloc 済みの char[n_pi+1] を返す：PI ごとに '0'/'1'/'X' + 終端 '\0'。呼び出し側が free する。
   SATモデルの故障シミュレーションで検出する観測点が無い（SATとの不整合）ときは NULL を返す。
   exc_net: TDFの1時刻目の励起ネット。SAFではNULL。 */
extern char* InlineXID(CCaDiCaL* solver, NLIST* fault_net, NLIST* exc_net);

extern void xid_fsim(size_t fsigID, XID_VAR_INFO* var_info, DETECT_PO* detect_po);

/* fsig_id: 故障サイトの net id。サイトの故障側3値は「縮退の公理」であって
   駆動側から正当化される値ではないため、サイトを通る故障側の後方含意は
   遮断する（正常側=活性化要求の逆伝搬は正常回路の含意なので正当・維持）。 */
extern void xid_backward_imp(Queue_t* bwd_q, Queue_t* jus_q, NLIST_t* t_net, XID_VAR_INFO* var_info, size_t fsig_id);
extern void xid_forward_imp(Queue_t* fwd_q, NLIST_t* t_net, XID_VAR_INFO* var_info);
extern void xid_backward_imp_limited(Queue_t* bwd_q, NLIST_t* t_net, XID_VAR_INFO* var_info, size_t fsig_id);

extern void init_xid_fpath_table(void);
extern void init_xid_bimp_table(void);
extern void init_xid_fimp_table(void);
extern void init_xid_bimp_limited_table(void);

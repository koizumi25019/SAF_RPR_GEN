/* Compile the good circuit once into compact topological instructions.
 * SAT supplies required PI values. Only gates inside the fault's normal-CNF scope
 * are simulated: the scope is closed under fan-in, XID never reads outside it,
 * and outside PIs become X in the cube. The scope-local gate/net lists are rebuilt
 * once per fault (when NormalScopeBuild runs), not once per cube.
 */
#include "fdp/cnf/normal_scope.h"
#include "fdp/xid/XID.h"
typedef struct { int type,output,offset,count; } Gate;
static Gate *gates;
static int *inputs,ngates;
static unsigned char *values;
static int *scope_gates, nscope_gates;   /* gates[] の添字。範囲内のゲートだけをレベル順に並べる */
static int *scope_nets, nscope_nets;     /* 範囲内のネット（外部入力を含む） */
static unsigned long long scope_generation;
static int compare_level(const void *a,const void *b) {
 const NLIST *x=*(NLIST *const*)a,*y=*(NLIST *const*)b;
 if(x->level!=y->level) return x->level<y->level ? -1:1;
 return x->n<y->n ? -1:x->n>y->n;
}
static void compile(void) {
 NLIST **order=malloc((size_t)n_net*sizeof(*order));
 gates=malloc((size_t)n_net*sizeof(*gates));
 values=calloc((size_t)n_net,1);
 size_t edges=0;
 if(!order || !gates || !values) { perror("simulation allocation"); exit(1); }
 for(int i=0;i<n_net;i++) if(nl[i].type!=IN) { order[ngates++]=nl+i; edges+=nl[i].n_in; }
 inputs=malloc((edges ? edges:1)*sizeof(*inputs));
 if(!inputs) { perror("simulation inputs"); exit(1); }
 qsort(order,ngates,sizeof(*order),compare_level);
 int offset=0;
 for(int i=0;i<ngates;i++) {
  NLIST *g=order[i];
  gates[i]=(Gate){g->type,g->n,offset,g->n_in};
  for(int j=0;j<g->n_in;j++) {
   if(g->in[j]->level>=g->level) { fprintf(stderr,"Invalid topological levels\n"); exit(1); }
   inputs[offset++]=g->in[j]->n;
  }
 }
 free(order);
}
/* 故障が変わったら範囲内のゲート・ネットの一覧を作り直し、作業領域を全体で一度だけ初期化する。
 * 以降のキューブでは範囲外のネットに誰も触れない（XIDの前方含意・故障シミュレーションも範囲内に限定）。 */
static void prepare_scope(XID_VAR_INFO *v) {
 if(!scope_gates) {
  scope_gates=malloc((size_t)(ngates?ngates:1)*sizeof(*scope_gates));
  scope_nets=malloc((size_t)n_net*sizeof(*scope_nets));
  if(!scope_gates || !scope_nets) { perror("simulation scope"); exit(1); }
 }
 nscope_gates=nscope_nets=0;
 for(int i=0;i<ngates;i++) if(NormalScopeRequiredNet(gates[i].output)) scope_gates[nscope_gates++]=i;
 for(int i=0;i<n_net;i++) {
  if(NormalScopeRequiredNet(i)) scope_nets[nscope_nets++]=i;
  v[i].ed_tag=v[i].xid_tag=0;
  v[i].normal_3value=v[i].fault_3value=XID_X;
  v[i].normal_2value=v[i].fault_2value=0;
 }
 scope_generation=NormalScopeGeneration();
}

void NormalScopeModelValues(CCaDiCaL *solver,XID_VAR_INFO *v) {
 if(!gates) compile();
 if(scope_generation!=NormalScopeGeneration()) prepare_scope(v);
 /* 範囲内のPIはSATの値を使う。範囲外のPIはどのゲートからも参照されない。 */
 for(int i=0;i<n_pi;i++) if(NormalScopeRequiredNet(pi[i]->n))
  values[pi[i]->n]=ccadical_val(solver,(int)pi[i]->varsgc)>0;
 for(int k=0;k<nscope_gates;k++) {
  const Gate *g=gates+scope_gates[k];
  const int *in=inputs+g->offset;
  int value=0;
  switch(g->type) {
   case AND: case NAND:
    value=1;
    for(int j=0;j<g->count;j++) value &= values[in[j]];
    if(g->type==NAND) value=!value;
    break;
   case OR: case NOR:
    for(int j=0;j<g->count;j++) value |= values[in[j]];
    if(g->type==NOR) value=!value;
    break;
   case BUF: case FOUT: value=values[in[0]]; break;
   case INV: value=!values[in[0]]; break;
   case EXOR: case EXNOR:
    value=values[in[0]] ^ values[in[1]];
    if(g->type==EXNOR) value=!value;
    break;
   default: fprintf(stderr,"Unsupported simulated gate %d\n",g->type); exit(1);
  }
  values[g->output]=value;
 }
 for(int k=0;k<nscope_nets;k++) {
  int i=scope_nets[k];
  v[i].ed_tag=v[i].xid_tag=0;
  v[i].normal_3value=v[i].fault_3value=XID_X;
  v[i].normal_2value=v[i].fault_2value=values[i];
 }
 if(getenv("FDP_NORMAL_SCOPE_VALIDATE")) for(int k=0;k<nscope_nets;k++) {
  int i=scope_nets[k];
  int value=ccadical_val(solver,(int)nl[i].varsgc);
  if(value && (value>0)!=v[i].normal_2value) {
   fprintf(stderr,"SAT/simulation mismatch %s\n",nl[i].name); exit(1);
  }
 }
}

void NormalScopeModelRelease(void) {
 free(gates); free(inputs); free(values); free(scope_gates); free(scope_nets);
 gates=NULL; inputs=NULL; values=NULL; ngates=0;
 scope_gates=scope_nets=NULL; nscope_gates=nscope_nets=0; scope_generation=0;
}

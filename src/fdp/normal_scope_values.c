/* Compile the good circuit once into compact topological instructions.
 * SAT supplies required PI values; outside PIs are completed with zero.
 * The full gate model is then unique; downstream XID uses that model.
 */
#include "normal_scope.h"
#include "xid/XID.h"
typedef struct { int type,output,offset,count; } Gate;
static Gate *gates;
static int *inputs,ngates;
static unsigned char *values;
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
 for(int i=0;i<n_net;i++) if(nl[i].type!=IN && nl[i].type!=DFF) { order[ngates++]=nl+i; edges+=nl[i].n_in; }
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
void NormalScopeModelValues(CCaDiCaL *solver,XID_VAR_INFO *v) {
 if(!gates) compile();
 /* Outside-scope PIs have no detection dependency. Complete them explicitly
  * with zero, without relying on CaDiCaL's value for an absent variable. */
 for(int i=0;i<n_pi;i++) values[pi[i]->n]=NormalScopeRequiredNet(pi[i]->n)
     ? ccadical_val(solver,(int)pi[i]->varsgc)>0 : 0;
 for(int i=0;i<ngates;i++) {
  const Gate *g=gates+i;
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
 for(int i=0;i<n_net;i++) {
  v[i].ed_tag=v[i].edx_tag=v[i].xid_tag=0;
  v[i].normal_3value=v[i].fault_3value=XID_X;
  v[i].normal_2value=v[i].fault_2value=values[i];
 }
 if(getenv("FDP_NORMAL_SCOPE_VALIDATE")) for(int i=0;i<n_net;i++) {
  if(!NormalScopeRequiredNet(i)) continue; /* omitted gates are unconstrained SAT variables */
  int value=ccadical_val(solver,(int)nl[i].varsgc);
  if(value && (value>0)!=v[i].normal_2value) {
   fprintf(stderr,"SAT/simulation mismatch %s\n",nl[i].name); exit(1);
  }
 }
}

void NormalScopeModelRelease(void) {
 free(gates); free(inputs); free(values);
 gates=NULL; inputs=NULL; values=NULL; ngates=0;
}

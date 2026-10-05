/* Recover the unique good-circuit gate values from the SAT PI assignment.
 * No DC decision changes: fsim and X-filling consume the same Boolean model.
 */
#include "fdp/xid/XID.h"
static NLIST **order;
static int ngates;
static int compare_level(const void *a,const void *b) {
 const NLIST *x=*(NLIST *const*)a,*y=*(NLIST *const*)b;
 if(x->level!=y->level) return x->level<y->level ? -1:1;
 return x->n<y->n ? -1:x->n>y->n;
}
void ProfileXidValues(CCaDiCaL *solver,XID_VAR_INFO *v) {
 if(!order) {
  order=malloc((size_t)n_net*sizeof(*order));
  if(!order) { perror("xid order"); exit(1); }
  for(int i=0;i<n_net;i++) if(nl[i].type!=IN && nl[i].type!=DFF) order[ngates++]=nl+i;
  qsort(order,ngates,sizeof(*order),compare_level);
  for(int i=0;i<ngates;i++) for(int j=0;j<order[i]->n_in;j++)
   if(order[i]->in[j]->level>=order[i]->level) { fprintf(stderr,"Invalid topological levels\n"); exit(1); }
 }
 for(int i=0;i<n_net;i++) {
  v[i].ed_tag=v[i].edx_tag=v[i].xid_tag=0;
  v[i].normal_3value=v[i].fault_3value=XID_X;
  v[i].normal_2value=v[i].fault_2value=XID_X;
 }
 for(int i=0;i<n_pi;i++) {
  int k=pi[i]->n;
  v[k].normal_2value=v[k].fault_2value=ccadical_val(solver,(int)pi[i]->varsgc)>0;
 }
 for(int i=0;i<ngates;i++) {
  NLIST *g=order[i];
  int value=0;
  switch(g->type) {
   case AND: case NAND:
    value=1;
    for(int j=0;j<g->n_in;j++) value &= v[g->in[j]->n].normal_2value;
    if(g->type==NAND) value=!value;
    break;
   case OR: case NOR:
    for(int j=0;j<g->n_in;j++) value |= v[g->in[j]->n].normal_2value;
    if(g->type==NOR) value=!value;
    break;
   case BUF: case FOUT: value=v[g->in[0]->n].normal_2value; break;
   case INV: value=!v[g->in[0]->n].normal_2value; break;
   case EXOR: case EXNOR:
    value=v[g->in[0]->n].normal_2value ^ v[g->in[1]->n].normal_2value;
    if(g->type==EXNOR) value=!value;
    break;
   default: fprintf(stderr,"Unsupported simulated gate %d\n",g->type); exit(1);
  }
  if(value!=0 && value!=1) { fprintf(stderr,"Unassigned simulation input\n"); exit(1); }
  v[g->n].normal_2value=v[g->n].fault_2value=value;
 }
 if(getenv("PROFILE_XID_VALIDATE")) for(int i=0;i<n_net;i++) {
  int value=ccadical_val(solver,(int)nl[i].varsgc);
  if(value && (value>0)!=v[i].normal_2value) {
   fprintf(stderr,"SAT/simulation mismatch %s\n",nl[i].name); exit(1);
  }
 }
}

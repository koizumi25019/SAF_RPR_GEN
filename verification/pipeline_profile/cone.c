/* Normal CNF projection: retain definitions of all good variables referenced
 * by the fault/propagation/detection/EA clauses and their transitive fanins.
 * Every omitted deterministic gate has a unique extension for any PI model.
 */
#include "fdp/create_TPG_model.h"
#include "fdp/cnf/cnf.h"
#include "fdp/cnf_dump.h"
static NLIST **byvar,**cone_stack;
static unsigned char *needed;
static int collecting,top;
static unsigned long long full_gates,kept_gates,instances;
void ProfileConeBegin(void) {
 if(!getenv("PROFILE_CONE")) return;
 if(!needed) {
  needed=calloc((size_t)n_net,1);
  cone_stack=malloc((size_t)n_net*sizeof(*cone_stack));
  byvar=calloc((size_t)cnf.constant.vars+1,sizeof(*byvar));
  if(!needed || !cone_stack || !byvar) { perror("cone scratch"); exit(1); }
  for(int i=0;i<n_net;i++) byvar[nl[i].varsgc]=nl+i;
 }
 memset(needed,0,(size_t)n_net);
 top=0;
 collecting=1;
}
void ProfileConeLiteral(int lit) {
 if(!collecting || !lit) return;
 int var=lit<0 ? -lit:lit;
 if(var>cnf.constant.vars) return;
 NLIST *g=byvar[var];
 if(g && !needed[g->n]) { needed[g->n]=1; cone_stack[top++]=g; }
}
void ProfileConeLoad(CCaDiCaL *solver) {
 collecting=0;
 while(top) {
  NLIST *g=cone_stack[--top];
  if(g->type==IN || g->type==DFF) continue;
  for(int j=0;j<g->n_in;j++) {
   NLIST *in=g->in[j];
   if(!needed[in->n]) { needed[in->n]=1; cone_stack[top++]=in; }
  }
 }
 for(int i=0;i<n_net;i++) {
  if(!nl[i].consgc) continue;
  full_gates++;
  if(!needed[i]) continue;
  kept_gates++;
  for(int j=0;j<nl[i].consgc_len;j++) CNF_ADD(solver,nl[i].consgc[j]);
 }
 instances++;
}
void ProfileConeReport(void) {
 if(instances) fprintf(stderr,"[CONE] instances=%llu kept_gates=%llu full_gates=%llu\n",instances,kept_gates,full_gates);
}
int ProfileConeRequiredNet(int index) {
 return !getenv("PROFILE_CONE") || !needed || needed[index];
}

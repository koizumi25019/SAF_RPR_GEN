#include "fdp/target_fault.h"
#include "fdp/init.h"
#include "fdp/essential_assignment.h"
static int *touched,*ea_touched,ntouched,nea;
static unsigned char *ea_seen;
static int cmp_int(const void *a,const void *b) { int x=*(const int*)a,y=*(const int*)b;return (x>y)-(x<y); }
void SparseResetTFO(void) {
 if(!touched) {
  touched=malloc((size_t)n_net*sizeof(*touched));
  if(!touched) { perror("sparse TFO");exit(1); }
  for(int i=0;i<n_net;i++) { nl[i].flag=RESET;nl[i].varsfc=nl[i].varsgc; }
 } else for(int k=0;k<ntouched;k++) { int i=touched[k];nl[i].flag=RESET;nl[i].varsfc=nl[i].varsgc; }
 ntouched=0;
}
void SparseRecord(int i) { touched[ntouched++]=i; }
void SparseFinish(void) { qsort(touched,ntouched,sizeof(*touched),cmp_int); }
int SparseCount(void) { return ntouched; }
int SparseNet(int k) { return touched[k]; }
void SparseEAReset(void) {
 if(!ea_touched) {
  ea_touched=malloc((size_t)n_net*sizeof(*ea_touched));
  ea_seen=calloc((size_t)n_net,1);
  if(!ea_touched || !ea_seen) { perror("sparse EA");exit(1); }
  for(int i=0;i<n_net;i++) { nl[i].logic_value=EA_UNKNOWN;nl[i].ea_flag=EA_DOWN;nl[i].unique_flag=UNIQUE_DOWN; }
 } else for(int k=0;k<nea;k++) { int i=ea_touched[k];nl[i].logic_value=EA_UNKNOWN;nl[i].ea_flag=EA_DOWN;nl[i].unique_flag=UNIQUE_DOWN;ea_seen[i]=0; }
 nea=0;
}
void SparseEAMark(NLIST *net) {
 int i=net->n;
 if(!ea_seen[i]) { ea_seen[i]=1;ea_touched[nea++]=i; }
}

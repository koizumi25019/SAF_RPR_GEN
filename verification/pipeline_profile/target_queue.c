/* One immutable ReadFault population per process. Preserve original tie order. */
#include "fdp/target_fault.h"
typedef struct { FNODE *fault; size_t order; } Entry;
static Entry *entries;
static size_t length, cursor;
static int initialized;
static int cmp(const void *a,const void *b) {
 const Entry *x=a,*y=b;
 int lx=x->fault->netptr->level,ly=y->fault->netptr->level;
 if(lx!=ly) return lx<ly ? -1:1;
 return x->order<y->order ? -1 : x->order>y->order;
}
extern bool OriginalSetTarget(TARGET *);
bool SetTarget(TARGET *target) {
 if(!getenv("PROFILE_QUEUE")) return OriginalSetTarget(target);
 if(!initialized) {
  for(int i=0;i<MAXSIZE_HASH;i++) for(FNODE *p=readdata.fault.list[i];p;p=p->nextptr) if(p->detect==UNDETECTED) length++;
  entries=malloc((length ? length:1)*sizeof(*entries));
  if(!entries) { perror("target queue"); exit(1); }
  size_t j=0;
  for(int i=0;i<MAXSIZE_HASH;i++) for(FNODE *p=readdata.fault.list[i];p;p=p->nextptr)
   if(p->detect==UNDETECTED) { entries[j]=(Entry){p,j}; j++; }
  qsort(entries,length,sizeof(*entries),cmp);
  initialized=1;
 }
 while(cursor<length && entries[cursor].fault->detect!=UNDETECTED) cursor++;
 if(cursor==length) return TARGET_ERROR;
 target->num=1;
 target->list=allocMemory(1,sizeof(FNODE*));
 target->list[0]=entries[cursor++].fault;
 if(cursor==length) { free(entries); entries=NULL; }
 return TARGET_OKAY;
}

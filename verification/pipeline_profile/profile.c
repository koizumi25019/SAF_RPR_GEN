#include "profile.h"
double profile_seconds[PF_N];
unsigned long profile_calls[PF_N];
void profile_report(void) {
 const char *names[] = {"read_net", "read_fault", "target", "model", "good_load", "search_tfo", "propagation", "detection", "essential", "solver_init", "solver_release", "xid_model_values", "xid_fsim", "xid_filling"};
 for(int i=0;i<PF_N;i++) fprintf(stderr,"[PROFILE] %s %.9f %lu\n",names[i],profile_seconds[i],profile_calls[i]);
}

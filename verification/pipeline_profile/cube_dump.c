#include "fdp/target_fault.h"
#include "fdp/cube_set.h"
void ProfileDumpCubes(int index,FNODE *fault,CubeSet *cubes,int complete) {
 const char *dir=getenv("PROFILE_CUBE_DIR");
 if(!dir) return;
 char path[4096];
 snprintf(path,sizeof(path),"%s/%06d.cover",dir,index);
 FILE *fp=fopen(path,"w");
 if(!fp) { perror(path); exit(1); }
 fprintf(fp,"%s %d %d %d %d\n",fault->name,fault->type==SF1,complete,n_pi,cubes->n);
 for(int i=0;i<n_pi;i++) fprintf(fp,"%s%c",pi[i]->name,i+1==n_pi?'\n':' ');
 for(int i=0;i<cubes->n;i++) fprintf(fp,"%s\n",cubes->data[i]);
 fclose(fp);
}

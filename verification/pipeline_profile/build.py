"""Instrument isolated source copies; never rewrite src/ or production builds."""
import os
import pathlib
import shutil
import subprocess
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
BUILD=HERE/'build'/os.environ.get('PIPELINE_BUILD_VARIANT','instrumented')
TREE=BUILD/'source'
TREE.mkdir(parents=True,exist_ok=True)
shutil.copytree(ROOT/'src',TREE/'src',dirs_exist_ok=True)
def edit(name,old,new):
 p=TREE/'src'/name
 s=p.read_text()
 assert s.count(old)==1,(name,old,s.count(old))
 p.write_text(s.replace(old,new))
edit('main.c','//set the option','atexit(profile_report); atexit(ProfileConeReport);\n\t//set the option')
edit('main.c','read_nl(opt.file.input.net);','PM(PF_READNET, read_nl(opt.file.input.net));')
f='fdp/fault_detection_prob.c'
edit(f,'if (ReadFault() != READ_OKAY) return READ_ERROR;', 'PM(PF_READFAULT, if (ReadFault() != READ_OKAY) return READ_ERROR);')
edit(f,'if (CreateConsGC() != true) return AFD_ERROR;',r'''
 if (getenv("PROFILE_TARGET_ONLY")) {
  unsigned long long digest=14695981039346656037ULL;
  while(readdata.fault.numrema) {
   PM(PF_TARGET, SetTarget(&target));
   FNODE *f=target.list[0];
   for(const unsigned char *p=(const unsigned char*)f->name;*p;p++) digest=(digest^*p)*1099511628211ULL;
   digest=(digest^(unsigned)f->type)*1099511628211ULL;
   DropDeteFault(&target); FreeMemory(&target);
  }
  fprintf(stderr,"[TARGET_ORDER] %llu\n",digest);
  *out_time_read=time_read;
  return AFD_OKAY;
 }
 if (CreateConsGC() != true) return AFD_ERROR;''')
edit(f,'bool AnalyzeFaultDensity(', 'void ProfileDumpCubes(int,FNODE*,CubeSet*,int);\nbool AnalyzeFaultDensity(')
edit(f,'dom_total_cubes  += cubes.n;', 'ProfileDumpCubes(count,f,&cubes,!limit_hit);\n                dom_total_cubes  += cubes.n;')
edit(f,'CCaDiCaL* solver = ccadical_init();','CCaDiCaL* solver; PM(PF_INIT, solver = ccadical_init());')
edit(f,'\n\t\tSetTarget(&target);','\n\t\tPM(PF_TARGET, SetTarget(&target));')
edit(f,'if (WriteTPGModel(solver, &target) != true) return AFD_ERROR;', 'PM(PF_MODEL, if (WriteTPGModel(solver, &target) != true) return AFD_ERROR);')
edit(f,'\n\t\tccadical_release(solver);\n\t}', '\n\t\tPM(PF_RELEASE, ccadical_release(solver));\n\t}')
edit('fdp/create_TPG_model.c','LoadModelToSolver(solver, target);','PM(PF_GOODLOAD, LoadModelToSolver(solver, target));')
f='fdp/cnf/faulty_circuit.c'
for call,key in [('SearchTFO(fault)','SEARCH'),('CreateConsProp(solver, fault)','PROP'),('CreateConsDC(solver, fault)','DC'),('EssentialAssignment(solver, fault)','EA')]:
 edit(f,call+';',f'PM(PF_{key}, {call});')
f='fdp/xid/XID.c'
edit(f,'    /* reset per-call state:', '    clock_t values_start=clock();\n    /* reset per-call state:')
edit(f,'    /* 2-value fault simulation','    profile_seconds[PF_XID_VALUES]+=(double)(clock()-values_start)/CLOCKS_PER_SEC;\n    profile_calls[PF_XID_VALUES]++;\n    /* 2-value fault simulation')
edit(f,'xid_fsim(fsigID, var_info, &detect_po);','PM(PF_XID_FSIM, xid_fsim(fsigID, var_info, &detect_po));')
edit(f,'Xfilling(fsigID, var_info, po, 0, excID);','PM(PF_XID_FILL, Xfilling(fsigID, var_info, po, 0, excID));')
p=TREE/'src/fdp/xid/XID.c'
s=p.read_text()
start=s.index('    for (int i = 0; i < n_net; ++i) {',s.index('char* InlineXID('))
end=s.index('    profile_seconds[PF_XID_VALUES]',start)
s=s[:start]+'    if(getenv("PROFILE_XID_SIM") || getenv("PROFILE_CONE")) ProfileXidValues(solver,var_info);\n    else {\n'+s[start:end]+'    }\n'+s[end:]
s='#include "fdp/xid/XID.h"\nvoid ProfileXidValues(CCaDiCaL*,XID_VAR_INFO*);\n'+s
p.write_text(s)
edit('fdp/create_TPG_model.c','void LoadModelToSolver(CCaDiCaL *solver, TARGET* target) {',
     'void LoadModelToSolver(CCaDiCaL *solver, TARGET* target) {\n if(getenv("PROFILE_CONE")) { ProfileConeLoad(solver); return; }')
edit('fdp/create_TPG_model.c','if (CreateTPGmodel(solver, target) != true) return false;',
     'ProfileConeBegin();\n if (CreateTPGmodel(solver, target) != true) return false;')
edit('fdp/cnf_dump.c','if (!body) return;\n    if (lit == 0)',
     'ProfileConeLiteral(lit);\n    if (!body) return;\n    if (lit == 0)')
edit('fdp/target_fault.c' ,'bool SetTarget(TARGET* target)','bool OriginalSetTarget(TARGET* target)')
if os.environ.get('PIPELINE_SPARSE_BUILD'):
 from sparse_patch import patch
 patch(TREE)
s=(ROOT/'CMakeLists.txt').read_text().replace('${CMAKE_SOURCE_DIR}/external',str(ROOT/'external'))
simulation='xid_values_pointer.c' if os.environ.get('PIPELINE_SIM_POINTER') else 'xid_values.c'
s+=f'\ntarget_sources(main_release PRIVATE {HERE}/profile.c {HERE}/target_queue.c {HERE}/cone.c {HERE}/cube_dump.c {HERE}/sparse.c {HERE}/{simulation})\ntarget_compile_options(main_release PRIVATE -include {HERE}/profile.h)\n'
(TREE/'CMakeLists.txt').write_text(s)
subprocess.run(['cmake','-S',str(TREE),'-B',str(BUILD/'bin')],check=True)
subprocess.run(['cmake','--build',str(BUILD/'bin'),'--target','main_release','-j4'],check=True)

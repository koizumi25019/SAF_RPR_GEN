import csv,random
from run import run,HERE,RUNS,ROOT
f=RUNS/'b18_32.faults'
f.write_text((HERE/'cases/b18_32.faults').read_text())
binary=HERE/'build/compact/bin/main_release'
run('b18_sample_base','b18_C',True,faults=f,binary=binary)
run('b18_sample_sim','b18_C',True,faults=f,binary=binary,extra_env={'PROFILE_XID_SIM':'1'})
run('b18_target_queue','b18_C',True,target_only=True,binary=binary)

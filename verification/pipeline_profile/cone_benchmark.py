from run import run,HERE,RUNS
binary=HERE/'build/cone/bin/main_release'
for name,net,faults in [('s5378_cone','s5378_C',None),('s38584_sample_cone','s38584_C',HERE/'cases/s38584_128.faults'),('b18_sample_cone','b18_C',HERE/'cases/b18_32.faults')]:
 run(name,net,True,faults=faults,binary=binary,extra_env={'PROFILE_CONE':'1'})

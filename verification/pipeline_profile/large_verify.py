"""Independent validation of the fixed large-circuit samples."""
from run import run,HERE,RUNS
from verify_cubes import verify
binary=HERE/'build/verify/bin/main_release'
run('s38584_cone_gt','s38584_C',True,gt=True,faults=HERE/'cases/s38584_128.faults',binary=binary,
    extra_env={'PROFILE_CONE':'1','PROFILE_XID_VALIDATE':'1'})
directory=RUNS/'b18_cone_dump'
directory.mkdir(exist_ok=True)
run('b18_cone_dump','b18_C',True,faults=HERE/'cases/b18_32.faults',binary=binary,
    extra_env={'PROFILE_CONE':'1','PROFILE_XID_VALIDATE':'1','PROFILE_CUBE_DIR':str(directory)})
verify(directory,'b18_C')

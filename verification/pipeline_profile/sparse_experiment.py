import csv,json
from decimal import Decimal
from run import run,HERE,RUNS,ROOT
binary=HERE/'build/sparse/bin/main_release'
results=[]
for net in ['c17a','s27_C','s208_C','s298_C','s344_C','s510_C','s641_C','s713_C','s1494_C','s5378_C']:
 r=run(net+'_sparse_gt',net,True,gt=True,limit=30 if net=='s5378_C' else 0,binary=binary,extra_env={'PROFILE_CONE':'1'})
 assert 'ALL VERIFIED' in (RUNS/(net+'_sparse_gt')/'stderr.log').read_text()
 results.append(r)
def vals(p): return {(r['net_name'],r['f_type']):Decimal(r['fdp']) for r in csv.DictReader(open(p))}
assert vals(ROOT/'expected/c17a_result.csv')==vals(RUNS/'c17a_sparse_gt/fdp.csv')
for stem,net,faults in [('s5378','s5378_C',None),('s38584_sample','s38584_C',HERE/'cases/s38584_128.faults'),('b18_sample','b18_C',HERE/'cases/b18_32.faults')]:
 r=run(stem+'_sparse',net,True,faults=faults,binary=binary,extra_env={'PROFILE_CONE':'1'})
 assert (RUNS/(stem+'_sparse')/'fdp.csv').read_bytes()==(RUNS/(stem+'_cone')/'fdp.csv').read_bytes(),stem
 results.append(r)
(HERE/'results/sparse_experiment.json').write_text(json.dumps({'all_verified':True,'csv_identical':True,'runs':results},indent=2))

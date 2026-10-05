import csv,json
from decimal import Decimal
from run import run,HERE,ROOT,RUNS
binary=HERE/'build/cone/bin/main_release'
results=[]
for net in ['c17a','s27_C','s208_C','s298_C','s344_C','s510_C','s641_C','s713_C','s1494_C','s5378_C']:
 result=run(net+'_cone_regression',net,True,gt=True,limit=30 if net=='s5378_C' else 0,binary=binary,extra_env={'PROFILE_CONE':'1'})
 stderr=(RUNS/(net+'_cone_regression')/'stderr.log').read_text()
 assert 'ALL VERIFIED' in stderr and 'UNSOUND=0' in stderr
 results.append(result)
def vals(path):
 return {(r['net_name'],r['f_type']):Decimal(r['fdp']) for r in csv.DictReader(open(path))}
assert vals(ROOT/'expected/c17a_result.csv')==vals(RUNS/'c17a_cone_regression/fdp.csv')
(HERE/'results/cone_regression.json').write_text(json.dumps({'all_verified':True,'c17_golden':True,'runs':results},indent=2))

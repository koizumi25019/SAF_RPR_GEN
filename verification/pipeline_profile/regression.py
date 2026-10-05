"""Production-regression cases with isolated output paths and independent GT."""
import csv,json
from decimal import Decimal
from run import run,HERE,ROOT
binary=HERE/'build/compact/bin/main_release'
results=[]
for circuit in ['c17a','s27_C','s208_C','s298_C','s344_C','s510_C','s641_C','s713_C','s1494_C','s5378_C']:
 result=run(circuit+'_regression',circuit,True,gt=True,limit=30 if circuit=='s5378_C' else 0,binary=binary,
            extra_env={'PROFILE_XID_SIM':'1','PROFILE_XID_VALIDATE':'1'})
 stderr=(HERE/'runs'/(circuit+'_regression')/'stderr.log').read_text()
 assert 'ALL VERIFIED' in stderr and 'UNSOUND=0' in stderr
 results.append(result)
def vals(path):
 return {(r['net_name'],r['f_type']):Decimal(r['fdp']) for r in csv.DictReader(open(path))}
assert vals(ROOT/'expected/c17a_result.csv')==vals(HERE/'runs/c17a_regression/fdp.csv')
for case in ['s5378','s38584_sample']:
 a=(HERE/'runs'/(case+'_base')/'fdp.csv').read_bytes()
 for variant in ['queue','sim','compact']:
  assert a==(HERE/'runs'/(case+'_'+variant)/'fdp.csv').read_bytes(),(case,variant)
(HERE/'results/regression.json').write_text(json.dumps({'all_verified':True,'c17_golden':True,'byte_identical_comparisons':6,'runs':results},indent=2))

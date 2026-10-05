"""Comparable serial runs, paths and all outputs isolated under verification."""
import csv,json,os,pathlib,random,re,subprocess
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
RUNS=HERE/'runs'
RUNS.mkdir(exist_ok=True)
BINARY=HERE/'build/instrumented/bin/main_release'

def run(name,net,queue=False,target_only=False,faults=None,gt=False,limit=30,binary=None,extra_env=None):
 d=RUNS/name
 d.mkdir(exist_ok=True)
 options=f'-net {ROOT}/input/circuit/{net}.v\n-fdp {d}/fdp.csv\n-log {d}/timing.log\n'
 if limit: options+=f'-limit {limit}\n'
 if faults: options+=f'-fault {faults}\n'
 (d/'run.set').write_text(options)
 env={k:v for k,v in os.environ.items() if not k.startswith(('MAXDC','GT_','MDC_','PROFILE_')) and k not in ['PCOUNT','BDD_EXACT','DUAL','SPLIT','MAXHAM','XID_EXTERNAL','DUMP_CNF','AIG_DUMP','AIG_DUMP_DIR','CUBE_TREND','CUBE_TREND_CSV','XSTAT']}
 env['NO_DISCORD']='1'
 if queue: env['PROFILE_QUEUE']='1'
 if target_only: env['PROFILE_TARGET_ONLY']='1'
 if gt: env['GT_BDD']='1'
 if extra_env: env.update(extra_env)
 with open(d/'stdout.log','w') as out,open(d/'stderr.log','w') as err:
  subprocess.run([str(binary or BINARY),'-set',str(d/'run.set')],cwd=d,env=env,stdout=out,stderr=err,check=True,timeout=600)
 result={'name':name,'queue':queue,'target_only':target_only,'net':net}
 result['profile']={k:{'seconds':float(t),'calls':int(n)} for k,t,n in re.findall(r'\[PROFILE\] (\w+) ([\d.]+) (\d+)',(d/'stderr.log').read_text())}
 result['timing']={k.strip():float(v) for k,v in re.findall(r'//\s+(.*?)\s+:\s+([\d.]+) sec',(d/'timing.log').read_text())}
 result['order']=re.findall(r'\[TARGET_ORDER\] (\d+)',(d/'stderr.log').read_text())
 (d/'result.json').write_text(json.dumps(result,indent=2))
 print(json.dumps(result),flush=True)
 return result

if __name__=='__main__':
 flist=RUNS/'s38584_128.faults'
 flist.write_text((HERE/'cases/s38584_128.faults').read_text())
 for queue in (False,True):
  tag='queue' if queue else 'base'
  run('s5378_'+tag,'s5378_C',queue)
  run('s38584_sample_'+tag,'s38584_C',queue,faults=flist)
  run('s38584_target_'+tag,'s38584_C',queue,target_only=True)

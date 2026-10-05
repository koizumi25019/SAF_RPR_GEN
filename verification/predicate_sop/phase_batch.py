"""Run the existing factored SOP pipeline with bounded phase assignment only."""
import importlib.util,json,pathlib,sys,time,resource
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
sys.path.insert(0,str(HERE.parent/'factored_sop'))
from fast_basis import FastNet
sys.path.insert(0,str(HERE))
source=(HERE.parent/'factored_sop/factor.py').read_text()
def replace_once(old,new):
 global source
 assert source.count(old)==1,(old,source.count(old))
 source=source.replace(old,new)
replace_once('from experiment import shape_key, run_fault','from experiment import shape_key\nfrom phase import run_fault')
replace_once("self.regions += result['cubes']","self.regions += result.get('cost_cubes', result['cubes'])")
replace_once("'complete': result['complete'], 'regions': result['cubes']}","'complete': result['complete'], 'regions': result['cubes'], 'negated': result.get('_cover_negated', False)}")
replace_once("'negated': inverted,","'negated': inverted ^ prior.get('negated',False),")
build=HERE/'build';build.mkdir(exist_ok=True)
p=build/'phase_factor.py';p.write_text(source)
spec=importlib.util.spec_from_file_location('phase_factor',p);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
from expand_connections import expand

def run(circuit='s5378_C',faults=None,outdir=None,seconds=2):
 outdir=pathlib.Path(outdir or HERE/'runs/s5378_phase');outdir.mkdir(parents=True,exist_ok=True)
 start=time.monotonic()
 net=FastNet(expand(ROOT/'input/circuit'/f'{circuit}.v',outdir/'expanded.v'))
 engine=module.Engine(outdir,width=3,seconds=seconds,difference=True,complement=True,exact_only=True,union_timeout=10)
 faults=pathlib.Path(faults or HERE.parent/'linear_scaling/cases/s5378.faults')
 results=[]
 with open(outdir/'faults.jsonl','w') as fp:
  for i,line in enumerate(faults.read_text().splitlines()):
   fault,kind=line.split();r=engine.fault(net,fault,int(kind[-1]),i)
   results.append(r);fp.write(json.dumps(r)+'\n');fp.flush()
   if (i+1)%100==0:print(json.dumps({'done':i+1,'complete':sum(r['complete'] for r in results),'seconds':time.monotonic()-start}),flush=True)
 phases=[json.loads(p.read_text()) for p in outdir.glob('leaf*.phase.json')]
 summary={'faults':len(results),'complete':sum(r['complete'] for r in results),'wall_seconds':time.monotonic()-start,
          'negative_selected':sum(p['negative_selected'] for p in phases),'trials':sum(len(p['trials']) for p in phases),
          'sat_calls':engine.sat_calls,'regions_including_probes':engine.regions,'new_leaves':engine.solved,'reused_leaves':engine.reused}
 (outdir/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary),flush=True)
 return summary
if __name__=='__main__':run()

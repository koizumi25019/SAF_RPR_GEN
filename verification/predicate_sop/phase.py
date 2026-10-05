"""Bounded output-phase probing; only complement a completed non-detection cover."""
import json,pathlib,time
from fractions import Fraction
from experiment import run_fault as base_run

def run_fault(net,fault,stuck,prefix,**kwargs):
 prefix=pathlib.Path(prefix);start=time.monotonic();budget=kwargs['seconds']
 if budget<.1:return base_run(net,fault,stuck,prefix,**kwargs)
 trials=[]
 def attempt(label,seconds,negative):
  args=dict(kwargs);args['seconds']=seconds;args['count_partial']=False
  prepared=list(args['prepared'])
  if negative:prepared[1]^=1
  args['prepared']=tuple(prepared)
  target=prefix.with_name(prefix.name+'_'+label)
  result=base_run(net,fault,stuck,target,**args)
  trials.append((target,result,negative))
  return result
 attempt('positive_probe',min(.01,budget),False)
 if not trials[-1][1]['complete']:
  remaining=budget-(time.monotonic()-start)
  if remaining>.01:attempt('negative_probe',min(.05,remaining),True)
 if not trials[-1][1]['complete']:
  remaining=budget-(time.monotonic()-start)
  if remaining>.001:attempt('positive_finish',remaining,False)
 # Do not use an incomplete negative cover as a positive lower bound.
 target,result,negative=next(((p,r,n) for p,r,n in reversed(trials) if r['complete'] or not n),trials[0])
 for suffix in ['.cnf','.metadata.json','.result.json','.cubes.json','.regions.json','.union.json','.sim']:
  source=target.with_suffix(suffix)
  if source.exists():prefix.with_suffix(suffix).write_bytes(source.read_bytes())
 result=dict(result)
 result['_cover_negated']=negative
 if negative:
  assert result['complete'] and result['union'] is not None
  result['union']=dict(result['union'])
  result['union']['probability']=str(1-Fraction(result['union']['probability']))
 for key in ['sat_calls','seconds','init_seconds']:
  result[key]=sum(r.get(key,0) for _,r,_ in trials)
 result['cost_cubes']=sum(r['cubes'] for _,r,_ in trials)
 result['phase_trials']=len(trials)
 prefix.with_suffix('.phase.json').write_text(json.dumps({'negative_selected':negative,'trials':[
  {'prefix':str(p),'complete':r['complete'],'negative':n,'seconds':r['seconds'],'cubes':r['cubes']} for p,r,n in trials]},indent=2))
 return result

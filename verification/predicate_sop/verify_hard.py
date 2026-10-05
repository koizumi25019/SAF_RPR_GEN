"""Independent certificate even when counting the complete cover timed out."""
import importlib.util,json,pathlib,sys
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import predicates
prefix=HERE/'runs/s38584_C_g16349_1_learned512'
netpath=predicates.ROOT/'input/circuit/s38584_C.v'
net=predicates.FastNet(netpath)
ex,out,rows=predicates.prepare(net,'g16349',1)
data=json.loads(prefix.with_suffix('.regions.json').read_text())
result=json.loads(prefix.with_suffix('.result.json').read_text())
assert result['complete'] and len(rows)==data['nvars']
meta={'fault':'g16349','stuck':1,'npi':ex.n,'basis_rows':[hex(r) for r in rows],'pi_order':net.pi,'groups':data['groups'],
      'status':'enumeration_complete_count_pending','width':3}
prefix.with_suffix('.metadata.json').write_text(json.dumps(meta,indent=2))
spec=importlib.util.spec_from_file_location('independent_scaling',HERE.parent/'linear_scaling/verify.py')
verify=importlib.util.module_from_spec(spec);spec.loader.exec_module(verify)
# Restrict only to structurally affected outputs; gate relations remain raw.
import collections
rawnet=predicates.probe.Netlist(netpath);fanout=collections.defaultdict(list)
for output,(_,ins) in rawnet.g.items():
 for source in ins:fanout[source].append(output)
affected={'g16349'};pending=['g16349']
while pending:
 for output in fanout[pending.pop()]:
  if output not in affected:affected.add(output);pending.append(output)
rawnet.po=[o for o in rawnet.po if o in affected]
r=verify.certify(rawnet,'g16349',1,rows,[],prefix,groups=data['groups'],regions=data['regions'],complete=True)
(HERE/'results/g16349_certificate.json').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True)

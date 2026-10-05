"""Prove dumped production-XID covers against independent original-gate CNF."""
import collections,importlib.util,json,pathlib,sys,time
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
sys.path.insert(0,str(HERE.parent/'linear_scaling'))
from expand_connections import expand
from experiment import write_cnf
from probe import Netlist,SAT
spec=importlib.util.spec_from_file_location('raw',HERE.parent/'linear_coordinates/verify.py')
raw=importlib.util.module_from_spec(spec);spec.loader.exec_module(raw)

def verify(directory,circuit):
 directory=pathlib.Path(directory)
 net=Netlist(expand(ROOT/'input/circuit'/f'{circuit}.v',directory/'expanded.v'))
 original_po=list(net.po)
 fanout=collections.defaultdict(list)
 for output,(_,inputs) in net.g.items():
  for source in inputs: fanout[source].append(output)
 results=[]
 for path in sorted(directory.glob('*.cover')):
  lines=path.read_text().splitlines()
  fault,stuck,complete,n,ncubes=lines[0].split()
  names=lines[1].split();n=int(n);stuck=int(stuck);complete=bool(int(complete));ncubes=int(ncubes)
  assert n==len(names)==len(net.pi) and len(lines)==ncubes+2
  assert set(names)==set(net.pi)
  pos={p:i for i,p in enumerate(net.pi)}
  rows=[1<<pos[p] for p in names]
  # Unaffected original outputs cannot detect this fault. No logical rewriting.
  affected={fault};pending=[fault]
  while pending:
   for nxt in fanout[pending.pop()]:
    if nxt not in affected: affected.add(nxt);pending.append(nxt)
  net.po=[p for p in original_po if p in affected]
  clauses,nv,det=raw.raw_cnf(net,fault,stuck,rows)
  cubes=[]
  for text in lines[2:]:
   assert len(text)==n and set(text)<=set('01X')
   cube=[i+1 if c=='1' else -i-1 for i,c in enumerate(text) if c!='X']
   nv+=1;z=nv;cubes.append(z)
   clauses.extend([[-z,l] for l in cube]);clauses.append([z]+[-l for l in cube])
  nv+=1;covered=nv
  clauses.extend([[-z,covered] for z in cubes]);clauses.append([-covered]+cubes)
  cnf=path.with_suffix('.raw.cnf');write_cnf(cnf,clauses,nv)
  solver=SAT(cnf,0);start=time.monotonic()
  try:
   sound,_=solver.solve([-det,covered]);assert sound==20,(fault,'unsound')
   remainder,_=solver.solve([det,-covered]);assert not complete or remainder==20,(fault,'incomplete')
  finally: solver.close()
  result={'fault':fault,'stuck':stuck,'cubes':ncubes,'claimed_complete':complete,'sound':True,'exact':remainder==20,'seconds':time.monotonic()-start}
  path.with_suffix('.verified.json').write_text(json.dumps(result,indent=2))
  results.append(result);print(json.dumps(result),flush=True)
 (HERE/'results'/f'{circuit}_raw_cover_verification.json').write_text(json.dumps(results,indent=2))
 return results
if __name__=='__main__': verify(sys.argv[1],sys.argv[2])

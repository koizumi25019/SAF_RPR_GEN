"""Independent raw gate-CNF and exhaustive tests for new SOP representations."""
import csv,importlib.util,itertools,json,pathlib,sys
from fractions import Fraction
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import blocks,predicates
sys.path.insert(0,str(HERE.parent/'factored_sop'))
spec=importlib.util.spec_from_file_location('independent_factored',HERE.parent/'factored_sop/verify.py')
rawverify=importlib.util.module_from_spec(spec);spec.loader.exec_module(rawverify)
from expand_connections import expand

def certificate(netpath,prefix,negative=False):
 prefix=pathlib.Path(prefix)
 meta=json.loads(prefix.with_suffix('.metadata.json').read_text());r=json.loads(prefix.with_suffix('.result.json').read_text())
 assert r['complete'],'Certificate helper checks completed covers'
 p=Fraction(json.loads(prefix.with_suffix('.union.json').read_text())['probability'])
 n=len(meta['basis_rows'])
 data={'fault':meta['fault'],'stuck':meta['stuck'],'ncoords':n,'basis_rows':meta['basis_rows'],
       'tree':{'kind':'cover','prefix':str(prefix),'map':{i:i for i in range(1,n+1)},'negated':negative,
               'probability':str(1-p if negative else p),'complete':True}}
 manifest=prefix.with_suffix('.cover.json');manifest.write_text(json.dumps(data))
 return rawverify.verify(str(netpath),manifest)

if __name__=='__main__':
 netpath=expand(blocks.ROOT/'input/circuit/c17a.v',HERE/'runs/c17/expanded.v')
 net=blocks.FastNet(netpath)
 gold={(r['net_name'],int(r['f_type'][-1])):r['fdp'] for r in csv.DictReader(open(blocks.ROOT/'expected/c17a_result.csv'))}
 # Include every golden fault, including explicit fanout branches.
 results=[]
 for (fault,stuck),fdp in gold.items():
  prepared=predicates.prepare(net,fault,stuck)
  ex,out,rows=prepared
  for mode in ['overlap','predicates','negative']:
   prefix=HERE/'runs/c17'/f'{fault}_{stuck}_{mode}'
   if mode=='predicates':r=predicates.run(net,fault,stuck,prefix,3,3,prepared)
   else:r=blocks.run(net,fault,stuck,prefix,'overlap' if mode=='overlap' else 'partition',3,3,(ex,out^(mode=='negative'),rows,[],0,set()),False)
   assert r['complete']
   prob=Fraction(r['union']['probability']);prob=1-prob if mode=='negative' else prob
   assert f'{float(prob):.10e}'==fdp,(fault,mode,prob,fdp)
   result=certificate(netpath,prefix,mode=='negative')
   result['mode']=mode;results.append(result)
 (HERE/'results/c17_verification.json').write_text(json.dumps(results,indent=2))
 print(json.dumps({'c17_cases':len(results),'all_raw_verified':True,'golden_match':True}),flush=True)
 # A dependent family of local OR conditions demonstrates the meaning of overlap.
 n=14;names=[f'x{i}' for i in range(n)]
 path=HERE/'cases/overlap_chain.v'
 lines=['module overlap_chain('+', '.join(names+['z'])+');','input '+', '.join(names)+';','output z;','wire '+', '.join(f'a{i}' for i in range(n-1))+';']
 lines += [f'OR2 g{i} (.A(x{i}), .B(x{i+1}), .Z(a{i}));' for i in range(n-1)]
 lines += ['AND'+str(n-1)+' last ('+', '.join([f'.{chr(65+i)}(a{i})' for i in range(n-1)]+['.Z(z)'])+');','endmodule']
 path.write_text('\n'.join(lines)+'\n')
 net=blocks.FastNet(path);ex,out,rows=predicates.prepare(net,'z',0)
 prepared=(ex,out,rows,[],0,set());artificial=[]
 expected=Fraction(sum(all((value>>i&3)!=0 for i in range(n-1)) for value in range(1<<n)),1<<n)
 for mode in ['partition','overlap','predicates']:
  prefix=HERE/'runs'/('chain_'+mode)
  r=predicates.run(net,'z',0,prefix,3,5,(ex,out,rows)) if mode=='predicates' else blocks.run(net,'z',0,prefix,mode,3,5,prepared,False)
  assert r['complete'] and Fraction(r['union']['probability'])==expected
  r['verified']=certificate(path,prefix);artificial.append(r)
 (HERE/'results/chain.json').write_text(json.dumps(artificial,indent=2))
 print(json.dumps({'chain_expected':str(expected),'results':[(r['mode'],r['cubes']) for r in artificial]}),flush=True)

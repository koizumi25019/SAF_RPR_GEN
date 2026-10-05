"""Independent biased signals combined by parity: a nonlinear decomposition test."""
import importlib.util
import json
from fractions import Fraction
from weighted import HERE,FastNet,base,prepare,run

spec=importlib.util.spec_from_file_location('weighted_certificate',HERE/'verify.py')
verify=importlib.util.module_from_spec(spec);spec.loader.exec_module(verify)

results=[]
for blocks in [4,10]:
    inputs=[f'x{i}' for i in range(4*blocks)]
    ands=[f'a{i}' for i in range(blocks)]
    partial=[f'p{i}' for i in range(blocks-2)]
    text=['module nonlinear ('+', '.join(inputs+['z'])+');',
          'input '+', '.join(inputs)+';', 'output z;', 'wire '+', '.join(ands+partial)+';']
    for i in range(blocks):
        text.append(f'AND4 g{i} (.A(x{4*i}), .B(x{4*i+1}), .C(x{4*i+2}), .D(x{4*i+3}), .Z(a{i}));')
    acc=ands[0]
    for i in range(1,blocks):
        out='z' if i==blocks-1 else partial[i-1]
        text.append(f'XOR2 e{i} (.A({acc}), .B(a{i}), .Z({out}));');acc=out
    text.append('endmodule')
    path=HERE/'runs/synthetic'/f'nonlinear{blocks}.v';path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text('\n'.join(text)+'\n');net=FastNet(path)
    prepared=prepare(net,'z',0)
    expected=(1-Fraction(7,8)**blocks)/2
    prefix=path.with_suffix('').with_name(f'weighted{blocks}')
    r=run(net,'z',0,prefix,prepared,max_inputs=4,seconds=3)
    assert r['complete'] and Fraction(r['probability'])==expected
    r['certificate']=verify.certificate(path,prefix.with_suffix('.cover.json'))
    r.update(blocks=blocks,approach='weighted',expected=str(expected))
    results.append(r);print(json.dumps(r),flush=True)
    plain=base.run_fault(net,'z',0,path.with_suffix('').with_name(f'baseline{blocks}'),prepared=prepared,
                         width=3,simulate=True,seconds=3,count_partial=False,union_order='large')
    if plain['complete']:assert Fraction(plain['union']['probability'])==expected
    plain.update(blocks=blocks,approach='baseline',expected=str(expected))
    results.append(plain);print(json.dumps(plain),flush=True)
    (HERE/'results/synthetic.json').write_text(json.dumps(results,indent=2))

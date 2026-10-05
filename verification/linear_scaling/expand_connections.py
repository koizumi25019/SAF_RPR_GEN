"""Expose fanout branches with the repository parser's SAF signal names."""
import collections
import pathlib
import sys

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent.parent/'linear_coordinates'))
from probe import Netlist

def expand(source,target):
    net=Netlist(source)
    uses=collections.Counter(i for _,ins in net.g.values() for i in ins)
    fanout={k:n+int(k in net.po) for k,n in uses.items()}
    names={k:k for k in net.pi+list(net.g)}
    branches=[];po_branches=[];newg={}
    for output,(kind,inputs) in net.g.items():
        new_inputs=[]
        for pin,source in enumerate(inputs):
            if fanout[source]>=2:
                label=names[source]+'_'+names[output]+'_'+chr(65+pin)
                branches.append((label,source));new_inputs.append(('branch',label))
                if source in net.po and names[source]==source:
                    po_branches.append((source,source));names[source]=source+'_stem'
            else:new_inputs.append(('net',source))
        newg[output]=(kind,new_inputs)
    pi=[names[p] for p in net.pi]
    gates=[]
    for output,(kind,inputs) in newg.items():
        gates.append((kind,[names[n] if t=='net' else n for t,n in inputs],names[output]))
    gates.extend(('BUF',[names[src]],label) for label,src in branches+po_branches)
    allnets=set(pi)|set(net.po)|{out for _,_,out in gates}
    lines=['module expanded('+', '.join(pi+net.po)+');','input '+', '.join(pi)+';','output '+', '.join(net.po)+';',
           'wire '+', '.join(sorted(allnets-set(pi)-set(net.po)))+';']
    for j,(kind,ins,out) in enumerate(gates):
        body=[f'.{chr(65+i)}({n})' for i,n in enumerate(ins)]+[f'.Z({out})']
        lines.append(f'{kind}{len(ins) if len(ins)>1 else ""} gate{j} ('+', '.join(body)+');')
    lines.append('endmodule')
    target=pathlib.Path(target);target.parent.mkdir(parents=True,exist_ok=True);target.write_text('\n'.join(lines)+'\n')
    return target

if __name__=='__main__':expand(sys.argv[1],sys.argv[2])

"""Disjoint nonlinear signal abstraction, without building any circuit BDD.

Select internal signals whose input supports are disjoint and have no path to
the top output bypassing that signal. These functions can become independent,
usually biased, Boolean coordinates. The test is structural and sufficient;
it is not the complete BDD-based DSD algorithm from Bertacco/Damiani.
"""
import functools
import pathlib
import sys

HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parents[1]
sys.path.insert(0,str(HERE.parent/'factored_sop'))
from fast_basis import FastNet,build_fast,compact,probe,bits


def prepare(net,fault,stuck):
    old,out,rows,_,npar,_=build_fast(net,fault,stuck,difference=True)
    ex,out,active=compact(old,out)
    return ex,out,[rows[i-1] for i in active],[],npar,set()


def select(ex,out,max_inputs=12):
    reach=sorted(ex.reachable(out))
    support={0:0}
    for k in reach:
        if k<=ex.n:
            support[k]=1<<(k-1)
        else:
            s=0
            for a in ex.nodes[k][1]:s|=support[a//2]
            support[k]=s
    candidates=[k for k in reach if k>ex.n and k!=out//2 and 2<=support[k].bit_count()<=max_inputs]
    candidates.sort(key=lambda k:(-support[k].bit_count(),-k))
    selected=[];used=0;tested=0
    for k in candidates:
        s=support[k]
        if s & used:continue
        # Reject if any source PI of k remains reachable with k cut away.
        seen=set();stack=[out//2];bypass=False
        while stack:
            node=stack.pop()
            if node==k or node in seen or not support[node]&s:continue
            seen.add(node)
            if node<=ex.n:
                bypass=True;break
            stack.extend(a//2 for a in ex.nodes[node][1])
        tested+=1
        if bypass:continue
        selected.append(k);used|=s
    # Cut signals and remaining original PIs are pairwise independent.
    residual=[k for k in reach if k<=ex.n and not used>>(k-1)&1]
    sources=[{'kind':'pi','node':k,'support':[k]} for k in residual]
    sources += [{'kind':'cut','node':k,'support':[i+1 for i in bits(support[k])]} for k in selected]
    sources.sort(key=lambda s:min(s['support']))
    top=probe.Expr(len(sources))
    memo={0:0,**{s['node']:2*(i+1) for i,s in enumerate(sources)}}
    def visit(lit):
        k=lit//2
        if k not in memo:
            assert k>ex.n, 'A cut input leaked into the outer function'
            kind,children=ex.nodes[k]
            args=[visit(a) for a in children]
            memo[k]=top.and_(*args) if kind=='a' else top.xor(*args)
        return memo[k]^(lit&1)
    root=visit(out)
    assert len({v for s in sources for v in s['support']})==sum(len(s['support']) for s in sources)
    assert {v for s in sources for v in s['support']}=={k for k in reach if k<=ex.n}
    return top,root,sources,{'candidates':len(candidates),'tested':tested,'selected':len(selected),
                             'original_inputs':ex.n,'top_inputs':top.n}


if __name__=='__main__':
    import json,time
    nets={}
    for circuit,fault,stuck in [('s5378_C','n673gat',0),('s5378_C','n1592gat',0),('s5378_C','n291gat',1),
                               ('s38584_C','g16349',1),('b19_C','P2_P1_P1_U3002',0)]:
        if circuit not in nets:nets[circuit]=FastNet(ROOT/'input/circuit'/f'{circuit}.v')
        ex,out,*_=prepare(nets[circuit],fault,stuck)
        for width in [6,12,24]:
            t=time.monotonic();_,_,sources,stats=select(ex,out,width)
            print(json.dumps({'fault':fault,'width':width,**stats,'seconds':time.monotonic()-t,
                              'cuts':[len(s['support']) for s in sources if s['kind']=='cut']}),flush=True)

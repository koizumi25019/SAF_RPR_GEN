"""Reusable exact affine extraction plus fault-local GF(2) bases.
All truth tables/polynomial rewrites are local Boolean algebra; no BDD circuit build.
"""
import collections
import functools
import importlib.util
import json
import pathlib
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
OLD = HERE.parent.parent/'linear_coordinates'
sys.path.insert(0, str(OLD))
import probe

class CachedNet(probe.Netlist):
    def __init__(self, path, tt_limit=6, anf_limit=128):
        start=time.monotonic()
        super().__init__(path)
        self.tt_limit=tt_limit;self.anf_limit=anf_limit
        self.fanout=collections.defaultdict(list)
        for k,(_,ins) in self.g.items():
            for i in ins:self.fanout[i].append(k)
        self.descendants=functools.cache(self._descendants)
        self.poly=functools.cache(self._poly)
        self.affine=functools.cache(self._affine_full)
        for k in self.pi+list(self.g):self.affine(k)
        self.preprocessing_seconds=time.monotonic()-start
    def _descendants(self,k):
        acc={k};stack=[k]
        while stack:
            for n in self.fanout[stack.pop()]:
                if n not in acc:acc.add(n);stack.append(n)
        return frozenset(acc)
    def _poly(self,k):
        if k in self.pidx:return frozenset([1<<self.pidx[k]])
        t,ins=self.g[k];ps=[self.poly(i) for i in ins]
        if any(p is None for p in ps):return None
        def product(a,b):
            if len(a)*len(b)>self.anf_limit*8:return None
            ret=set()
            for x in a:
                for y in b:
                    m=x|y
                    if m in ret:ret.remove(m)
                    else:ret.add(m)
            return ret if len(ret)<=self.anf_limit else None
        if t in ('BUF','INV'):res=set(ps[0])
        elif t in ('XOR','XNOR','EXOR','EXNOR'):
            res=set()
            for p in ps:res.symmetric_difference_update(p)
        elif t in ('AND','NAND','OR','NOR'):
            isor=t in ('OR','NOR');res={0}
            for p in ps:
                pp=set(p)
                if isor:pp.symmetric_difference_update({0})
                res=product(res,pp)
                if res is None:return None
            if isor:res.symmetric_difference_update({0})
        else:raise ValueError(t)
        if t in ('INV','NAND','NOR','XNOR','EXNOR'):res.symmetric_difference_update({0})
        return frozenset(res) if len(res)<=self.anf_limit else None
    def _affine_full(self,k):
        poly=self.poly(k)
        if poly is not None and all(m.bit_count()<=1 for m in poly):
            return functools.reduce(int.__or__,poly,0),int(0 in poly)
        ss=sorted(self.support(k))
        if len(ss)>self.tt_limit:return None
        vals={n:sum(((a>>i)&1)<<a for a in range(1<<len(ss))) for i,n in enumerate(ss)}
        mask=(1<<(1<<len(ss)))-1
        def ev(n):
            if n not in vals:
                t,ins=self.g[n];vals[n]=probe.eval_gate(t,[ev(i) for i in ins],mask)
            return vals[n]
        tt=ev(k);const=tt&1;amask=0
        for i,n in enumerate(ss):
            if ((tt>>(1<<i))&1)^const:amask|=1<<self.pidx[n]
        predicted=mask if const else 0
        for n in ss:
            if amask>>self.pidx[n]&1:predicted^=vals[n]
        return (amask,const) if predicted==tt else None

@functools.lru_cache(maxsize=256)
def complete_basis(n,candidates):
    rows=[];echelon={}
    def insert(row):
        v=row;combo=1<<len(rows)
        for pivot,(r,c) in sorted(echelon.items(),reverse=True):
            if v>>pivot&1:v^=r;combo^=c
        if not v:return False
        echelon[v.bit_length()-1]=(v,combo);rows.append(row);return True
    for r in candidates:insert(r)
    npar=len(rows)
    for i in range(n):insert(1<<i)
    inverse=[]
    for i in range(n):
        v=1<<i;combo=0
        for pivot,(r,c) in sorted(echelon.items(),reverse=True):
            if v>>pivot&1:v^=r;combo^=c
        assert v==0;inverse.append(combo)
    return rows,inverse,npar

def build_cached(net,fault,stuck,variant='full',candidates=None):
    if fault not in net.pidx and fault not in net.g:raise ValueError(fault)
    downstream=net.descendants(fault)
    ends=[o for o in net.po if o in downstream]
    relevant=set();stack=ends[:]
    while stack:
        k=stack.pop()
        if k in relevant:continue
        relevant.add(k)
        if k in net.g:stack.extend(net.g[k][1])
    if candidates is None:
        counts=collections.Counter()
        for k in sorted(relevant):
            ac=net.affine(k)
            if ac and ac[0].bit_count()>1:counts[ac[0]]+=1
        if variant=='binary':candidates=[]
        elif variant=='old':
            candidates=[r for r,c in sorted(counts.items(),key=lambda x:(-x[1],x[0])) if r.bit_count()<=3]
        elif variant=='short':candidates=[r for r,c in sorted(counts.items(),key=lambda x:(x[0].bit_count(),-x[1],x[0]))]
        elif variant=='long':candidates=[r for r,c in sorted(counts.items(),key=lambda x:(-x[0].bit_count(),-x[1],x[0]))]
        else:candidates=[r for r,c in sorted(counts.items(),key=lambda x:(-x[1],x[0]))]
    rows,inverse,npar=complete_basis(len(net.pi),tuple(candidates))
    ex=probe.Expr(len(net.pi))
    @functools.cache
    def original_affine(mask,const):
        tr=0
        for i in range(len(net.pi)):
            if mask>>i&1:tr^=inverse[i]
        return ex.xor(*(2*(j+1) for j in range(len(net.pi)) if tr>>j&1),const)
    @functools.cache
    def rec(k,bad):
        if bad and k==fault:return stuck
        if k in net.pidx:return original_affine(1<<net.pidx[k],0)
        if bad and k not in downstream:return rec(k,False)
        ac=net.affine(k)
        if ac is not None and (not bad or fault in net.pidx):
            mask,const=ac
            if bad and fault in net.pidx and mask>>net.pidx[fault]&1:
                mask^=1<<net.pidx[fault];const^=stuck
            return original_affine(mask,const)
        t,ins=net.g[k]
        return ex.gate(t,[rec(i,bad) for i in ins])
    out=ex.or_(*(ex.xor(rec(o,False),rec(o,True)) for o in ends))
    return ex,out,rows,inverse,npar,relevant

if __name__=='__main__':
    net=CachedNet(probe.ROOT/'input/circuit/s5378_C.v')
    counts=collections.Counter(ac[0].bit_count() for k in net.g if (ac:=net.affine(k)))
    print(json.dumps({'preprocessing_seconds':net.preprocessing_seconds,'affine_lengths':counts}))
    for f in ('n673gat','n2141gat','n1312gat'):
        for v in ('old','full','short','long'):
            start=time.monotonic();ex,out,rows,inv,npar,rel=build_cached(net,f,0,v)
            reach=ex.reachable(out)
            print(json.dumps({'fault':f,'variant':v,'npar':npar,'support':sum(i<=ex.n for i in reach),'gates':sum(i>ex.n for i in reach),'build_seconds':time.monotonic()-start,'parities':[[net.pi[j] for j in range(ex.n) if r>>j&1] for r in rows[:npar]]}))

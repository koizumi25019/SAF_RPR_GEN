import argparse
import collections
import functools
import hashlib
import itertools
import json
import pathlib
import random
import re
import subprocess
import time
from fractions import Fraction

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
TMP = HERE/'runs'

class Netlist:
    def __init__(self, path):
        s = pathlib.Path(path).read_text()
        def decl(k):
            return re.findall(r'\w+', re.search(r'\b'+k+r'\s+(.*?);', s, re.S).group(1))
        self.pi, self.po = decl('input'), decl('output')
        self.pidx = {x:i for i,x in enumerate(self.pi)}
        self.g = {}
        for t,b in re.findall(r'\b([A-Z]+\d*)\s+\w+\s*\((.*?)\)\s*;', s, re.S):
            pins = re.findall(r'\.([A-Z]+)\(\s*(\w+)\s*\)', b)
            out = next(n for p,n in pins if p=='Z')
            self.g[out] = (re.sub(r'\d+$','',t), [n for p,n in pins if p!='Z'])
        self.support = functools.cache(self._support)
        self.affine = functools.cache(self._affine)
    def _support(self, n):
        return frozenset([n]) if n in self.pidx else frozenset().union(*(self.support(i) for i in self.g[n][1]))
    def _affine(self, n):
        ss = sorted(self.support(n))
        if len(ss)>3: return None
        vals = {n: sum(((a>>i)&1)<<a for a in range(1<<len(ss))) for i,n in enumerate(ss)}
        mask = (1<<(1<<len(ss)))-1
        def ev(k):
            if k not in vals:
                t,ins = self.g[k]; vals[k] = eval_gate(t, [ev(i) for i in ins], mask)
            return vals[k]
        tt = ev(n); const=tt&1; amask=0
        for i,k in enumerate(ss):
            if ((tt>>(1<<i))&1)^const: amask |= 1<<self.pidx[k]
        for a in range(1<<len(ss)):
            expected=const
            for i,k in enumerate(ss):
                if amask&(1<<self.pidx[k]): expected ^= (a>>i)&1
            if ((tt>>a)&1) != expected: return None
        return amask,const
    def simulate(self, bits, fault, stuck):
        good = dict(zip(self.pi,bits)); bad=good.copy()
        if fault in bad: bad[fault]=stuck
        def ev(n, v, faulty):
            if n not in v:
                if faulty and n==fault:v[n]=stuck
                else:
                    t,ins=self.g[n];v[n]=eval_gate(t,[ev(i,v,faulty) for i in ins],1)
            return v[n]
        return any(ev(n,good,False)!=ev(n,bad,True) for n in self.po)

def eval_gate(t, vs, mask):
    if t in ('BUF','INV'):r=vs[0]
    elif t in ('AND','NAND'):r=functools.reduce(int.__and__,vs)
    elif t in ('OR','NOR'):r=functools.reduce(int.__or__,vs)
    elif t in ('XOR','XNOR','EXOR','EXNOR'):r=functools.reduce(int.__xor__,vs)
    else:raise ValueError(t)
    return r^mask if t in ('INV','NAND','NOR','XNOR','EXNOR') else r

class Expr:
    def __init__(self, n):
        self.n=n; self.nodes=[None]*(n+1); self.unique={}
    def node(self,t,args):
        key=t,tuple(sorted(args))
        if key not in self.unique:
            self.unique[key]=len(self.nodes)*2; self.nodes.append(key)
        return self.unique[key]
    def and_(self,*args):
        acc=set()
        for a in args:
            if a==0:return 0
            if a==1:continue
            nd=self.nodes[a//2]
            if a%2==0 and nd and nd[0]=='a':acc.update(nd[1])
            else:acc.add(a)
        if any(a^1 in acc for a in acc):return 0
        if not acc:return 1
        if len(acc)==1:return next(iter(acc))
        return self.node('a',acc)
    def or_(self,*args):return self.and_(*(a^1 for a in args))^1
    def xor(self,*args):
        neg=0; acc=set()
        for a in args:
            neg^=a&1; a&=~1
            if a==0:continue
            nd=self.nodes[a//2]
            children=nd[1] if nd and nd[0]=='x' else [a]
            for b in children:
                if b in acc:acc.remove(b)
                else:acc.add(b)
        if not acc:return neg
        if len(acc)==1:return next(iter(acc))^neg
        if len(acc)==2:
            a,b=acc
            def factors(v):
                nd=self.nodes[v//2]
                return set(nd[1]) if nd and nd[0]=='a' else {v}
            fa,fb=factors(a),factors(b); common=fa&fb
            if common:return self.and_(*common,self.xor(self.and_(*(fa-common)),self.and_(*(fb-common))))^neg
        return self.node('x',acc)^neg
    def gate(self,t,vs):
        if t in ('BUF','INV'):r=vs[0]
        elif t in ('AND','NAND'):r=self.and_(*vs)
        elif t in ('OR','NOR'):r=self.or_(*vs)
        elif t in ('XOR','XNOR','EXOR','EXNOR'):r=self.xor(*vs)
        else:raise ValueError(t)
        return r^int(t in ('INV','NAND','NOR','XNOR','EXNOR'))
    def reachable(self,out):
        seen=set(); stack=[out//2]
        while stack:
            i=stack.pop()
            if i in seen or i==0:continue
            seen.add(i)
            if self.nodes[i]:stack.extend(a//2 for a in self.nodes[i][1])
        return seen
    def evaluate(self, out, bits):
        memo={i+1:b for i,b in enumerate(bits)}; memo[0]=0
        def ev(a):
            k=a//2
            if k not in memo:
                t,ins=self.nodes[k]; vs=[ev(i) for i in ins]
                memo[k]=int(all(vs)) if t=='a' else functools.reduce(int.__xor__,vs)
            return memo[k]^(a&1)
        return ev(out)
    def cnf(self,out):
        reachable=self.reachable(out); clauses=[]; nv=len(self.nodes)-1
        def lit(a):return -(a//2) if a&1 else a//2
        def xor_clauses(z,a,b):
            clauses.extend([[a,b,-z],[-a,-b,-z],[a,-b,z],[-a,b,z]])
        for k in sorted(reachable):
            nd=self.nodes[k]
            if nd is None:continue
            t,ins=nd; ins=[lit(i) for i in ins]
            if t=='a':
                clauses.extend([[-k,a] for a in ins]);clauses.append([k]+[-a for a in ins])
            else:
                acc=ins[0]
                for i,b in enumerate(ins[1:]):
                    if i==len(ins)-2:z=k
                    else:nv+=1;z=nv
                    xor_clauses(z,acc,b);acc=z
        if out in (0,1):
            nv+=1; ol=nv;clauses.append([ol if out else -ol])
        else:ol=lit(out)
        return clauses,nv,ol

def make_basis(net,relevant, mode):
    n=len(net.pi); rows=[]; echelon={}
    def insert(row):
        v=row; combo=1<<len(rows)
        for pivot,(r,c) in sorted(echelon.items(),reverse=True):
            if v&(1<<pivot):v^=r;combo^=c
        if not v:return False
        echelon[v.bit_length()-1]=(v,combo);rows.append(row);return True
    if mode=='linear':
        counts=collections.Counter()
        for k in sorted(relevant):
            ac=net.affine(k)
            if ac and ac[0].bit_count()>1:counts[ac[0]]+=1
        for row,count in sorted(counts.items(), key=lambda x:(-x[1],x[0])):insert(row)
    parities=len(rows)
    for i in range(n):insert(1<<i)
    assert len(rows)==n
    inverse=[]
    for i in range(n):
        v=1<<i; combo=0
        for pivot,(r,c) in sorted(echelon.items(),reverse=True):
            if v&(1<<pivot):v^=r;combo^=c
        assert v==0;inverse.append(combo)
    for i,inv in enumerate(inverse):
        assert functools.reduce(int.__xor__,(rows[j] for j in range(n) if inv>>j&1),0)==1<<i
    return rows,inverse,parities

def build(net,fault,stuck,mode):
    if fault not in net.pidx and fault not in net.g:
        raise ValueError('This prototype supports named net stem faults only: '+fault)
    ends=[o for o in net.po if fault in net.support(o)] if fault in net.pidx else net.po
    relevant=set(); stack=ends[:]
    while stack:
        k=stack.pop()
        if k in relevant:continue
        relevant.add(k)
        if k in net.g:stack.extend(net.g[k][1])
    rows,inverse,parities=make_basis(net,relevant,mode)
    ex=Expr(len(net.pi)); maps={}
    def original_affine(mask,const):
        tr=0
        for i in range(len(net.pi)):
            if mask>>i&1:tr^=inverse[i]
        return ex.xor(*(2*(j+1) for j in range(len(net.pi)) if tr>>j&1),const)
    @functools.cache
    def rec(k,bad):
        if bad and k==fault:return stuck
        if k in net.pidx:return original_affine(1<<net.pidx[k],0)
        ac=net.affine(k)
        if ac is not None and (not bad or fault in net.pidx or fault not in relevant):
            mask,const=ac
            if bad and fault in net.pidx and mask>>net.pidx[fault]&1:
                mask^=1<<net.pidx[fault];const^=stuck
            return original_affine(mask,const)
        t,ins=net.g[k]
        return ex.gate(t,[rec(i,bad) for i in ins])
    out=ex.or_(*(ex.xor(rec(o,False),rec(o,True)) for o in ends))
    return ex,out,rows,inverse,parities,relevant

class SAT:
    def __init__(self,path,n):
        self.p=subprocess.Popen([str(HERE/'build'/'sat_bridge'),str(path),str(n)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True,bufsize=1)
        self.calls=0
    def add(self, lits):self.p.stdin.write('a '+' '.join(map(str,lits))+' 0\n')
    def solve(self,lits=()):
        self.calls+=1;self.p.stdin.write('s '+' '.join(map(str,lits))+' 0\n');self.p.stdin.flush()
        line=self.p.stdout.readline()
        if not line:raise RuntimeError('solver died')
        ans=list(map(int,line.split()));assert ans[0] in (10,20),ans
        return ans[0],ans[1:]
    def close(self):
        if self.p.poll() is None:
            self.p.stdin.write('q\n');self.p.stdin.flush();self.p.wait()

def enumerate_cover(ex,out,prefix,limit,disjoint,seconds,minimize=False):
    cls,nv,ol=ex.cnf(out); path=TMP/(prefix+'.cnf')
    path.write_text('p cnf '+str(nv)+' '+str(len(cls))+'\n'+''.join(' '.join(map(str,c))+' 0\n' for c in cls))
    det,off=SAT(path,ex.n),SAT(path,ex.n); det.add([ol]); bad=-ol
    support=sorted(i for i in ex.reachable(out) if i<=ex.n)
    start=time.monotonic();total=0; hist=collections.Counter();complete=False;cubes=[]
    try:
        for idx in range(limit):
            rc,model=det.solve()
            if rc==20:complete=True;break
            fixed=[model[i-1] for i in support]
            rc,core=off.solve([bad]+fixed);assert rc==20
            fixed=[l for l in fixed if l in core]
            if minimize:
                for candidate in fixed[:]:
                    if candidate not in fixed:continue
                    trial=[l for l in fixed if l!=candidate]
                    rc,core=off.solve([bad]+trial)
                    if rc==20:fixed=[l for l in trial if l in core]
            # Iterated cores often suffice; certify the resulting implicant explicitly.
            rc,core=off.solve([bad]+fixed);assert rc==20
            fixed=[l for l in fixed if l in core]
            hist[len(fixed)]+=1;total+=1<<(ex.n-len(fixed));cubes.append(fixed)
            det.add([-l for l in fixed])
            if disjoint:
                # bad_new <=> bad_old OR conjunction(fixed), using fresh Tseitin variables.
                nv+=1;c=nv
                for l in fixed:off.add([-c,l])
                off.add([c]+[-l for l in fixed])
                nv+=1;b=nv
                off.add([-bad,b]);off.add([-c,b]);off.add([-b,bad,c]);bad=b
            if (idx+1)%1000==0:
                print(json.dumps({'progress':idx+1,'seconds':time.monotonic()-start,'mass_sum':float(Fraction(total,1<<ex.n))}),flush=True)
            if time.monotonic()-start>seconds:break
        else:
            rc,_=det.solve();complete=rc==20
        result={'cubes':len(cubes),'complete':complete,'seconds':time.monotonic()-start,'sat_calls':det.calls+off.calls,'disjoint':disjoint,'care_hist':dict(sorted(hist.items()))}
        if disjoint:result.update(probability=str(Fraction(total,1<<ex.n)),fdp=float(Fraction(total,1<<ex.n)))
        (TMP/(prefix+'.cubes.json')).write_text(json.dumps(cubes))
        return result
    finally:det.close();off.close()

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--net',default='input/circuit/s5378_C.v')
    p.add_argument('--fault',default='n673gat')
    p.add_argument('--stuck',type=int,default=0)
    p.add_argument('--mode',choices=['binary','linear'],default='linear')
    p.add_argument('--limit',type=int,default=10000)
    p.add_argument('--seconds',type=int,default=45)
    p.add_argument('--disjoint',action='store_true')
    p.add_argument('--minimize',action='store_true')
    p.add_argument('--structural-only',action='store_true')
    p.add_argument('--prefix',help='Output stem within runs/; use distinct stems for separate conditions')
    a=p.parse_args()
    task_start=time.monotonic()
    net=Netlist(ROOT/a.net);ex,out,rows,inverse,npar,rel=build(net,a.fault,a.stuck,a.mode)
    reachable=ex.reachable(out);support=sorted(i for i in reachable if i<=ex.n)
    info={'mode':a.mode,'npi':ex.n,'parity_basis_rows':npar,'detection_support':len(support),'reachable_gates':len(reachable)-len(support),'parities':[[net.pi[j] for j in range(ex.n) if row>>j&1] for row in rows[:npar]]}
    rng=random.Random(1)
    for _ in range(300):
        xb=rng.getrandbits(ex.n);bits=[xb>>i&1 for i in range(ex.n)];yb=[(xb&r).bit_count()&1 for r in rows]
        assert ex.evaluate(out,yb)==net.simulate(bits,a.fault,a.stuck)
    info['random_simulation_checks']=300
    print(json.dumps(info),flush=True)
    if not a.structural_only:
        prefix=a.prefix or a.fault+'_'+a.mode+('_disjoint' if a.disjoint else '_overlap')+('_prime' if a.minimize else '')
        result=enumerate_cover(ex,out,prefix,a.limit,a.disjoint,a.seconds,a.minimize)
        metadata={'arguments':vars(a),'netlist_sha256':hashlib.sha256((ROOT/a.net).read_bytes()).hexdigest(),
                  'original_pi_order':net.pi,'basis_rows_hex':[hex(r) for r in rows],
                  'total_seconds':time.monotonic()-task_start}
        (TMP/(prefix+'.result.json')).write_text(json.dumps({'info':info,'result':result,'metadata':metadata},indent=2))
        print(json.dumps(result),flush=True)

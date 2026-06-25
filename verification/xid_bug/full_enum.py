#!/usr/bin/env python3
# サポートPIが小さい故障について、サポート上で全パターンを総当たりし、
# 厳密なFDP(=検出パターン数/2^k)を求める。モンテカルロでなく完全列挙。
# 対象はゲート出力 stuck 故障(実ネット)。分岐故障はゲート入力ピンを stuck にする。
import re, sys
exec(open("verification/xid_bug/check.py").read().split("# --- ビット並列")[0])  # パーサ

PIset=set(PIs)
fanin = {g[3]: list(g[2]) for g in gates}
fanout = {}
for g in gates:
    for i in g[2]:
        fanout.setdefault(i, []).append(g[3])

def tfo(seed):
    seen=set([seed]); st=[seed]
    while st:
        x=st.pop()
        for o in fanout.get(x,[]):
            if o not in seen: seen.add(o); st.append(o)
    return seen

def tfi_pis(nets):
    seen=set(nets); st=list(nets); pis=set()
    while st:
        x=st.pop()
        if x in PIset: pis.add(x); continue
        for i in fanin.get(x,[]):
            if i not in seen: seen.add(i); st.append(i)
    return pis

W=4096; MASK=(1<<W)-1
LOWBITS=min(12, 1)  # placeholder; W=4096=2^12
LB=12
LOWMASK=[0]*LB
for j in range(LB):
    m=0
    for s in range(W):
        if (s>>j)&1: m|=(1<<s)
    LOWMASK[j]=m

def base_type(t): return re.match(r"[A-Z]+",t).group(0)
def bit_eval(gtype, vals):
    bt=base_type(gtype)
    if bt=="INV": return (~vals[0])&MASK
    if bt=="BUF": return vals[0]
    if bt=="NOR":
        a=0
        for x in vals: a|=x
        return (~a)&MASK
    if bt=="OR":
        a=0
        for x in vals: a|=x
        return a
    if bt=="NAND":
        a=MASK
        for x in vals: a&=x
        return (~a)&MASK
    if bt=="AND":
        a=MASK
        for x in vals: a&=x
        return a
    if bt in ("XOR","XNOR"):
        a=0
        for x in vals: a^=x
        return (~a)&MASK if bt=="XNOR" else a
    raise SystemExit("gate "+gtype)

def parse_fault(name):
    # 実ネットならゲート出力故障。'_gat_' を含み実ネットでなければ分岐故障。
    if name in fanout or name in fanin or name in PIset:
        return ("node", name, None)   # ゲート出力 or PI の stuck
    m=re.match(r"(n\d+gat)_(n\d+gat)_([A-Z])$", name)
    if m:
        stem, drivennet, pin = m.group(1), m.group(2), m.group(3)
        return ("branch", drivennet, (stem, pin))
    raise SystemExit("unknown fault name "+name)

def run(name, stuck):
    kind, fsite, extra = parse_fault(name)
    # 観測POとサポート
    fo = tfo(fsite)
    obs_po=[p for p in POs if p in fo]
    supp = sorted(tfi_pis(set(obs_po)) | ({fsite}&PIset))
    k=len(supp)
    if k>26:
        print("%-26s サポート%d -> 大きすぎ(skip)"%(name,k)); return
    # 分岐: drivennet ゲートの該当入力index
    pin_idx=None
    if kind=="branch":
        g=[x for x in gates if x[3]==fsite][0]
        # ピン文字 A,B,C,.. を入力順に対応(.Aが0)
        pin_idx=ord(extra[1])-ord('A')
    suppset=set(supp)
    fixed0 = {net:0 for net in PIs if net not in suppset}  # 非サポートPIは0固定(影響なし)
    total=1<<k
    nchunks=(total + W -1)//W
    det_total=0
    for c in range(nchunks):
        pim=dict(fixed0)
        for jb,net in enumerate(supp):
            if jb<LB:
                pim[net]=LOWMASK[jb]
            else:
                pim[net]= MASK if ((c*W)>>jb)&1 else 0
        # 有効ビット数(最終チャンクが端数の場合)
        nvalid = min(W, total - c*W)
        vbmask = MASK if nvalid==W else ((1<<nvalid)-1)
        # good
        vg=dict(pim)
        for gid,gtype,ins,out in order:
            vg[out]=bit_eval(gtype,[vg[i] for i in ins])
        # faulty
        vf=dict(pim)
        for gid,gtype,ins,out in order:
            iv=[vf[i] for i in ins]
            if kind=="node" and out==fsite:
                vf[out]= MASK if stuck else 0
            elif kind=="branch" and out==fsite:
                iv=list(iv); iv[pin_idx]= MASK if stuck else 0
                vf[out]=bit_eval(gtype,iv)
            else:
                vf[out]=bit_eval(gtype,iv)
        d=0
        for o in POs:
            d|=(vg[o]^vf[o])
        det_total += bin(d & vbmask).count("1")
    fdp = det_total/total
    print("%-26s sa%d  サポートPI=%2d  検出パターン=%d/%d  厳密FDP=%.10f"%(
        name, stuck, k, det_total, total, fdp))
    return fdp

if __name__=="__main__":
    # 残存ミスマッチのうちゲート出力故障(=実ネット)を厳密評価
    for name,stuck in [("n2058gat",0),("n2150gat",0),("n2512gat",0),("n2512gat",1)]:
        run(name,stuck)

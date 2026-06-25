#!/usr/bin/env python3
# ある故障の「検出関数のサポートPI」(=観測PO群の入力錐に入るPI)を求め、総当たり可能性を測る。
import re, sys
exec(open("verification/xid_bug/check.py").read().split("# --- ビット並列")[0])  # パーサ部のみ流用

# fanin/fanout 隣接
fanin = {g[3]: list(g[2]) for g in gates}   # out -> inputs
fanout = {}
for g in gates:
    for i in g[2]:
        fanout.setdefault(i, []).append(g[3])

def tfo(seed):  # 前方到達集合
    seen=set([seed]); st=[seed]
    while st:
        x=st.pop()
        for o in fanout.get(x,[]):
            if o not in seen: seen.add(o); st.append(o)
    return seen

def tfi_pis(nets):  # net群の入力錐に含まれるPI
    seen=set(nets); st=list(nets); pis=set()
    while st:
        x=st.pop()
        if x in PIset: pis.add(x); continue
        for i in fanin.get(x,[]):
            if i not in seen: seen.add(i); st.append(i)
    return pis

PIset=set(PIs)

# 故障サイト = ネット名(分岐故障も netlist 上のネット名)
faults = sys.argv[1:] or ["n1006gat_n1175gat_B"]
for fsite in faults:
    fo = tfo(fsite)
    obs_po = [p for p in POs if p in fo]
    supp = tfi_pis(set(obs_po)) | ({fsite} & PIset)
    print("%-26s 観測PO=%2d  サポートPI=%3d  -> 2^%d 総当たり %s" %
          (fsite, len(obs_po), len(supp), len(supp),
           "可能" if len(supp)<=24 else "不可(大きすぎ)"))

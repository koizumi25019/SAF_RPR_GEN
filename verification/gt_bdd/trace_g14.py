#!/usr/bin/env python3
# s298_C / G14 sa1 / 不健全キューブの検出経路解析。
# キューブの care ビットを固定し、G0=0/1 それぞれで X を全列挙 or 代表値で
# good/faulty を評価し、どの PO が検出に使われ、G0=1 でどこで伝搬が死ぬかを見る。
import re, itertools, sys

NET = "../../data/circuit/s298_C.v"
txt = open(NET).read()
ID = r"[A-Za-z_][A-Za-z0-9_]*"

def decl(kw):
    m = re.search(r"^\s*" + kw + r"\b(.*?);", txt, re.S | re.M)
    return re.findall(ID, m.group(1))
PIs, POs = decl("input"), decl("output")

gates = []
for m in re.finditer(r"([A-Z][A-Z0-9]*)\s+(gate\d+)\s*\((.*?)\)\s*;", txt, re.S):
    gtype, gid, body = m.group(1), m.group(2), m.group(3)
    pins = re.findall(r"\.([A-Z])\(\s*(" + ID + r")\s*\)", body)
    ins  = [n for p, n in pins if p != "Z"]
    out  = [n for p, n in pins if p == "Z"][0]
    gates.append((gid, re.match(r"[A-Z]+?(?=\d|$)", gtype).group(0), ins, out))

def topo(gates):
    done = set(PIs); order = []
    rest = gates[:]
    while rest:
        nxt = [g for g in rest if all(i in done for i in g[2])]
        assert nxt, "cycle?"
        for g in nxt: order.append(g); done.add(g[3])
        rest = [g for g in rest if g not in nxt]
    return order
order = topo(gates)

def ev(t, vals):
    if t == "INV":  return vals[0] ^ 1
    if t == "BUF":  return vals[0]
    if t == "AND":  return int(all(vals))
    if t == "NAND": return int(not all(vals))
    if t == "OR":   return int(any(vals))
    if t == "NOR":  return int(not any(vals))
    if t in ("XOR","EXOR"):  return sum(vals) & 1
    if t in ("XNOR","EXNOR"): return (sum(vals) & 1) ^ 1
    raise Exception(t)

def sim(piv, fault=None):  # fault=(net,val)
    v = dict(piv)
    if fault and fault[0] in v: v[fault[0]] = fault[1]
    for gid, t, ins, out in order:
        v[out] = ev(t, [v[i] for i in ins])
        if fault and out == fault[0]: v[out] = fault[1]
    return v

PIORDER = "G23 G22 G21 G20 G2 G19 G18 G17 G16 G15 G14 G13 G12 G11 G10 G1 G0".split()
cube = sys.argv[1] if len(sys.argv) > 1 else "XXXXXXXXXX01001XX"

xpos = [i for i, c in enumerate(cube) if c == "X"]
print(f"cube={cube}  care={[(PIORDER[i],c) for i,c in enumerate(cube) if c!='X']}  nX={len(xpos)}")

# X を全列挙して、G0 別に検出率と検出POを集計
from collections import Counter
det_po = {0: Counter(), 1: Counter()}
det_cnt = {0: [0,0], 1: [0,0]}   # [detected, total]
for bits in itertools.product([0,1], repeat=len(xpos)):
    piv = {}
    for i, c in enumerate(cube):
        piv[PIORDER[i]] = int(c) if c != "X" else bits[xpos.index(i)]
    g = sim(piv); f = sim(piv, ("G14", 1))
    diff = [p for p in POs if g[p] != f[p]]
    g0 = piv["G0"]
    det_cnt[g0][1] += 1
    if diff:
        det_cnt[g0][0] += 1
        det_po[g0][tuple(diff)] += 1
for g0 in (0,1):
    d, n = det_cnt[g0]
    print(f"G0={g0}: detected {d}/{n}", dict(det_po[g0]) if d else "")

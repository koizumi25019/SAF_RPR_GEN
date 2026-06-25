#!/usr/bin/env python3
# 故障効果が伝搬する経路と、n622gat系がサイド入力として効くゲートを特定する。
import re, sys, random
exec(open("verification/xid_bug/check.py").read().split("random.seed(1)")[0])

# 単一パターン(2値)シミュレータ
def sim_scalar(pv, faulty):
    v = dict(pv)
    for gid, gtype, ins, out in order:
        iv = [v[i] for i in ins]
        if faulty and out == FAULT_GATE_OUT:
            iv = list(iv); iv[FAULT_PIN_IDX] = FAULT_SA
        v[out] = gate_eval(gtype, iv)
    return v

# out -> driving gate
drv = {g[3]: g for g in gates}

random.seed(3)
lines = open(CUBES).read().splitlines()
pi_order = [l.split()[1:] for l in lines if l.startswith("[PIORDER]")][0]
cubes = [l.split()[1] for l in lines if l.startswith("[CUBE]")]

cube = cubes[1]
# 検出する代表パターン: cube固定 + n622gat=0 + 残りX=ランダム
pv = {}
for i, net in enumerate(pi_order):
    if cube[i] == '1': pv[net] = 1
    elif cube[i] == '0': pv[net] = 0
    else: pv[net] = random.randint(0,1)
pv["n622gat"] = 0

g = sim_scalar(pv, False)
f = sim_scalar(pv, True)
diff = {n for n in g if g.get(n) != f.get(n)}
print("検出PO:", [o for o in POs if g[o]!=f[o]])
print("故障効果ネット数(good!=faulty):", len(diff))

# 伝搬経路上で「n622gat系がサイド入力」になっているゲートを探す
# n622gatのfanout錐(論理的影響)を求める
infl = set(["n622gat"])
changed = True
while changed:
    changed = False
    for gid, gtype, ins, out in order:
        if out not in infl and any(i in infl for i in ins):
            infl.add(out); changed = True
print("n622gat 影響錐サイズ:", len(infl))

# 伝搬経路(diff)上のゲートで、サイド入力(diffでない)が n622gat 影響下にあるもの
print("\n--- 伝搬ゲートのうち、サイド入力が n622gat 影響下にあるもの ---")
for gid, gtype, ins, out in order:
    if out in diff:  # このゲート出力は故障効果を持つ
        for i in ins:
            if i not in diff and i in infl:
                # i は故障効果でないサイド入力で、n622gat に依存
                print("gate %s %s out=%s  side=%s (n622依存)  good_side=%d faulty_side=%d  out g/f=%d/%d" %
                      (gid, gtype, out, i, g[i], f[i], g[out], f[out]))

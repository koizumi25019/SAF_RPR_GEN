#!/usr/bin/env python3
# 非健全キューブ(cube1, cube9)について、検出を支配しているX位置のPIを特定する。
# 各X-PIの乱数マスクと検出マスクの相関を測り、「このPIを固定すれば検出100%」のものを探す。
import re, sys, random
exec(open("verification/xid_bug/check.py").read().split("random.seed(1)")[0])  # parse + sim 関数を流用

random.seed(7)
lines = open(CUBES).read().splitlines()
pi_order = [l.split()[1:] for l in lines if l.startswith("[PIORDER]")][0]
cubes = [l.split()[1] for l in lines if l.startswith("[CUBE]")]

def popcount(x): return bin(x).count("1")

for ci in (1, 9):
    cube = cubes[ci]
    pim = {}
    xnets = []
    for i, net in enumerate(pi_order):
        if cube[i] == '1': pim[net] = MASK
        elif cube[i] == '0': pim[net] = 0
        else:
            pim[net] = random.getrandbits(W); xnets.append(net)
    d = detect_mask(pim)
    frac = popcount(d)/W
    print("\n=== cube %d  detect=%.4f  |X|=%d ===" % (ci, frac, len(xnets)))
    # 各X-PIについて: P(detect | net=1) と P(detect | net=0)
    best = []
    for net in xnets:
        m = pim[net]
        n1 = popcount(m); n0 = W - n1
        d1 = popcount(d & m)            # net=1 かつ 検出
        d0 = popcount(d & (~m & MASK))  # net=0 かつ 検出
        p1 = d1/n1 if n1 else 0
        p0 = d0/n0 if n0 else 0
        # 検出を強く分離するPIほど |p1-p0| が大きい
        sep = abs(p1 - p0)
        if sep > 0.5:
            best.append((sep, net, p1, p0))
    best.sort(reverse=True)
    print(" 検出を支配するX-PI (|P(det|x=1)-P(det|x=0)|>0.5):")
    for sep, net, p1, p0 in best[:8]:
        print("   %-12s P(det|=1)=%.3f  P(det|=0)=%.3f  sep=%.3f" % (net, p1, p0, sep))
    if not best:
        print("   (単一PIでは分離されない → 複数条件の組合せ)")

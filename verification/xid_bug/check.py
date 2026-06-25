#!/usr/bin/env python3
# 故障値考慮XID(InlineXID)が生成したキューブの健全性を独立シミュレーションで検査する。
# 対象: s5378_C, 分岐故障 n1006gat_n1175gat_B/sa0
#   = ステム n1006gat -> gate2799 NOR2(n621gat,n1006gat)->n1175gat の B入力(n1006gat側)が sa0。
# 各キューブ(固定PI + X)について、X位置をランダムに埋めて good/faulty を評価し、
# 「全ミンタームが故障を検出するか」を測る。100%未満なら過大評価(非検出点を含む)。
import re, sys, random

NET = "data/circuit/s5378_C.v"
CUBES = "verification/xid_bug/cubes.txt"
FAULT_GATE_OUT = "n1175gat"   # 故障ゲートの出力
FAULT_STEM     = "n1006gat"   # 分岐元ステム
FAULT_PIN_IDX  = 1            # gate2799 = NOR2(.A(n621gat), .B(n1006gat)) → B = index1
FAULT_SA       = 0            # sa0

txt = open(NET).read()

NET_ID = r"[A-Za-z_][A-Za-z0-9_]*"

# --- ポート宣言(input/output) を multiline で取得 ---
def decl_names(kw):
    m = re.search(r"^\s*" + kw + r"\b(.*?);", txt, re.S | re.M)
    return re.findall(NET_ID, m.group(1))
PIs = decl_names("input")
POs = decl_names("output")

# --- ゲート解析: TYPE gateNNN ( .A(..), .B(..), ... .Z(out) ); ---
gates = []  # (gate_id, type, [inputs], out)
for m in re.finditer(r"([A-Z][A-Z0-9]+)\s+(gate\d+)\s*\((.*?)\)\s*;", txt, re.S):
    gtype, gid, body = m.group(1), m.group(2), m.group(3)
    pins = re.findall(r"\.([A-Z])\(\s*(" + NET_ID + r")\s*\)", body)
    ins = [net for p, net in pins if p != "Z"]
    outs = [net for p, net in pins if p == "Z"]
    gates.append((gid, gtype, ins, outs[0]))

# 出力ネット -> ゲート
drv = {g[3]: g for g in gates}

# --- 評価関数(2値) ---
def base_type(t):
    return re.match(r"[A-Z]+", t).group(0)

def gate_eval(gtype, vals):
    bt = base_type(gtype)
    if bt == "INV": return vals[0] ^ 1
    if bt == "BUF": return vals[0]
    if bt == "AND": r=1
    if bt == "NOR":
        return 0 if any(vals) else 1
    if bt == "OR":
        return 1 if any(vals) else 0
    if bt == "NAND":
        return 0 if all(vals) else 1
    if bt == "AND":
        return 1 if all(vals) else 0
    if bt == "XOR":
        r=0
        for v in vals: r^=v
        return r
    if bt == "XNOR":
        r=0
        for v in vals: r^=v
        return r^1
    raise SystemExit("unknown gate "+gtype)

# トポロジカル順(出力ネットを評価できる順序)を構築
order = []
done = set(PIs)
pending = list(gates)
guard = 0
while pending:
    guard += 1
    if guard > 100000: raise SystemExit("topo loop stuck")
    nxt = []
    for g in pending:
        gid, gtype, ins, out = g
        if all(i in done for i in ins):
            order.append(g); done.add(out)
        else:
            nxt.append(g)
    if len(nxt) == len(pending):
        raise SystemExit("topo deadlock; unresolved %d" % len(pending))
    pending = nxt

# --- ビット並列シミュレーション: W個のミンタームを多倍長整数で同時評価 ---
W = 4096
MASK = (1 << W) - 1

def bit_eval(gtype, vals):
    bt = base_type(gtype)
    if bt == "INV": return (~vals[0]) & MASK
    if bt == "BUF": return vals[0]
    if bt == "NOR":
        a = 0
        for x in vals: a |= x
        return (~a) & MASK
    if bt == "OR":
        a = 0
        for x in vals: a |= x
        return a
    if bt == "NAND":
        a = MASK
        for x in vals: a &= x
        return (~a) & MASK
    if bt == "AND":
        a = MASK
        for x in vals: a &= x
        return a
    if bt in ("XOR", "XNOR"):
        a = 0
        for x in vals: a ^= x
        return (~a) & MASK if bt == "XNOR" else a
    raise SystemExit("unknown gate " + gtype)

def simulate_par(pi_mask, faulty):
    v = dict(pi_mask)
    for gid, gtype, ins, out in order:
        invals = [v[i] for i in ins]
        if faulty and out == FAULT_GATE_OUT:
            invals = list(invals)
            invals[FAULT_PIN_IDX] = MASK if FAULT_SA else 0  # 分岐sa値
        v[out] = bit_eval(gtype, invals)
    return v

def detect_mask(pi_mask):
    g = simulate_par(pi_mask, False)
    f = simulate_par(pi_mask, True)
    d = 0
    for o in POs:
        d |= (g[o] ^ f[o])
    return d & MASK

def randmask():
    return random.getrandbits(W)

# --- キューブ読み込み ---
lines = open(CUBES).read().splitlines()
pi_order = None
cubes = []
for ln in lines:
    if ln.startswith("[PIORDER]"):
        pi_order = ln.split()[1:]
    elif ln.startswith("[CUBE]"):
        cubes.append(ln.split()[1])

assert pi_order is not None
assert len(pi_order) == len(PIs), (len(pi_order), len(PIs))
print("PIs=%d POs=%d gates=%d cubes=%d  (W=%d samples/cube)" % (len(PIs), len(POs), len(gates), len(cubes), W))
sys.stdout.flush()

random.seed(1)
unsound = []
for ci, cube in enumerate(cubes):
    assert len(cube) == len(pi_order)
    pim = {}
    nfix = nx = 0
    for i in range(len(cube)):
        net = pi_order[i]
        if cube[i] == '1':
            pim[net] = MASK; nfix += 1
        elif cube[i] == '0':
            pim[net] = 0; nfix += 1
        else:
            pim[net] = randmask(); nx += 1
    # PI以外の未定義ネットは評価で全て駆動されるので初期化不要
    dmask = detect_mask(pim)
    det = bin(dmask).count("1")
    frac = det / W
    flag = "" if frac > 0.99999 else "  <<< UNSOUND (過大評価: 非検出点を含む)"
    print("cube %2d: |fixed|=%2d |X|=%3d  detect=%.5f%s" % (ci, nfix, nx, frac, flag))
    sys.stdout.flush()
    if frac <= 0.99999:
        unsound.append((ci, cube, pim, dmask))

print("\nUNSOUND cubes: %d / %d" % (len(unsound), len(cubes)))
# 健全な参照: 全キューブ和集合の検出率と、cube無関係に全空間の真FDPを比較したいが、
# ここではキューブ単体健全性のみ報告（過大評価の局所原因特定が目的）。
import pickle
pickle.dump(dict(PIs=PIs,POs=POs,order=order,gates=gates,pi_order=pi_order,cubes=cubes,
                 unsound=[(u[0],u[1]) for u in unsound]),
            open("verification/xid_bug/state.pkl","wb"))

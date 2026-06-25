#!/usr/bin/env python3
# 故障値考慮XID(inline) vs 旧・外部XID(external) の比較集計。
# results/<circuit>_<variant>.{fdp.csv,log,stderr,runinfo} を読み、表とCSVを出す。
import csv, re, sys, os

HERE = os.path.dirname(os.path.abspath(__file__))
RES  = os.path.join(HERE, "results")
CIRCUITS = ["s5378_C", "s9234_C"]
VARIANTS = ["inline", "external"]

def parse_fdp(path):
    n_fault = n_enum = total_cubes = n_incomplete = 0
    fdp = {}
    if not os.path.exists(path):
        return None
    with open(path) as f:
        r = csv.reader(f)
        next(r, None)
        for row in r:
            if len(row) < 5:
                continue
            net, ftype, cube, comp, val = row[0], row[1], row[2], row[3], row[4]
            n_fault += 1
            fdp[(net, ftype)] = val
            if cube.strip() != "":
                n_enum += 1
                try: total_cubes += int(cube)
                except ValueError: pass
            if comp.strip() == "0":
                n_incomplete += 1
    return dict(n_fault=n_fault, n_enum=n_enum, total_cubes=total_cubes,
               n_incomplete=n_incomplete, fdp=fdp)

def parse_log(path):
    keys = {"Time":"cpu_wall_log", "CPU Time":"cpu", "CaDiCaL":"cadical",
            "BDD":"bdd", "Don't care":"xid", "Read Fault":"read"}
    out = {}
    if not os.path.exists(path):
        return out
    for line in open(path, errors="ignore"):
        m = re.search(r"//\s*(?:CPU Time|Time)\s*(?:\(([^)]+)\))?\s*:\s*([\d.]+)\s*sec", line)
        if not m:
            continue
        label = m.group(1) or "CPU Time" if "CPU Time" in line else (m.group(1) or "Time")
        val = float(m.group(2))
        if m.group(1) is None:
            out["cpu" if "CPU Time" in line else "wall_log"] = val
        else:
            lab = m.group(1)
            if "CaDiCaL" in lab: out["cadical"] = val
            elif "BDD" in lab:   out["bdd"] = val
            elif "care" in lab:  out["xid"] = val
            elif "Read" in lab:  out["read"] = val
    return out

def parse_xstat(path):
    if not os.path.exists(path):
        return {}
    for line in open(path, errors="ignore"):
        m = re.search(r"\[XSTAT\].*cubes_bits=(\d+) x_bits=(\d+) x_ratio=([\d.]+)", line)
        if m:
            return dict(cubes_bits=int(m.group(1)), x_bits=int(m.group(2)),
                        x_ratio=float(m.group(3)))
    return {}

def parse_runinfo(path):
    if not os.path.exists(path):
        return {}
    parts = open(path).read().split()
    if len(parts) >= 2:
        return dict(rc=int(parts[0]), wall=float(parts[1]))
    return {}

def load(c, v):
    base = os.path.join(RES, f"{c}_{v}")
    d = {}
    fdp = parse_fdp(base + ".fdp.csv")
    if fdp: d.update(fdp)
    d.update(parse_log(base + ".log"))
    d.update(parse_xstat(base + ".stderr"))
    d.update(parse_runinfo(base + ".runinfo"))
    return d

def fdp_mismatch(a, b):
    if not a or not b: return None
    fa, fb = a.get("fdp", {}), b.get("fdp", {})
    keys = set(fa) & set(fb)
    return sum(1 for k in keys if fa[k] != fb[k]), len(keys)

rows = []
for c in CIRCUITS:
    data = {v: load(c, v) for v in VARIANTS}
    mm = fdp_mismatch(data["inline"], data["external"])
    for v in VARIANTS:
        d = data[v]
        rows.append(dict(circuit=c, variant=v,
            n_enum=d.get("n_enum"), total_cubes=d.get("total_cubes"),
            x_ratio=d.get("x_ratio"), incomplete=d.get("n_incomplete"),
            wall=d.get("wall"), cpu=d.get("cpu"), cadical=d.get("cadical"),
            bdd=d.get("bdd"), xid=d.get("xid")))
    # 比較行
    inl, ext = data["inline"], data["external"]
    def pct(a, b):
        if not a or not b or a == 0: return None
        return 100.0 * (b - a) / a
    print(f"\n## {c}")
    print(f"  X率:     inline {inl.get('x_ratio')}  ->  external {ext.get('x_ratio')}  "
          f"(Δ {None if not inl.get('x_ratio') else round(ext.get('x_ratio',0)-inl.get('x_ratio',0),4)})")
    print(f"  cube総数: inline {inl.get('total_cubes')}  ->  external {ext.get('total_cubes')}  "
          f"(Δ% {None if pct(inl.get('total_cubes'),ext.get('total_cubes')) is None else round(pct(inl['total_cubes'],ext['total_cubes']),1)})")
    print(f"  fdp不一致: {mm[0] if mm else '?'} / {mm[1] if mm else '?'} 行  (0なら両XIDで健全一致)")
    print(f"  wall:    inline {inl.get('wall')}s  external {ext.get('wall')}s")
    print(f"  内訳CPU(inline):  cadical={inl.get('cadical')} bdd={inl.get('bdd')} xid={inl.get('xid')}")
    print(f"  内訳CPU(external):cadical={ext.get('cadical')} bdd={ext.get('bdd')} xid={ext.get('xid')}")
    print(f"  incomplete(complete=0): inline {inl.get('n_incomplete')}  external {ext.get('n_incomplete')}")

# CSV
out_csv = os.path.join(HERE, "summary.csv")
with open(out_csv, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
    w.writeheader(); w.writerows(rows)
print(f"\n-> {out_csv}")

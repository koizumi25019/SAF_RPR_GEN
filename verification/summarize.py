#!/usr/bin/env python3
"""verification/results/ を集計して summary.csv を作る。
   各 (回路, 構成) について: 故障数, 総キューブ数, complete=1率, 各種CPU時間, 壁時計, FDP整合。
   FDP整合は baseline を基準に (net,f_type)->fdp が一致するかで判定（理論上 不変であるべき）。"""
import os, csv, re, glob

ROOT = os.path.dirname(os.path.abspath(__file__))
RES  = os.path.join(ROOT, "results")
CONFIGS = ["baseline", "maxdc", "maxham", "maxdc_maxham"]

def parse_fdp(path):
    """return dict: n_faults(全対象故障), n_enum(自前列挙した故障=cube_cnt有り),
       total_cubes, n_incomplete(列挙故障のうちcomplete!=1), fdp{(net,ft):val}
       空欄行=等価故障(他故障のキューブで検出され脱落, fdpのみ継承)。"""
    if not os.path.exists(path): return None
    n=enum=tot=incomp=0; fdp={}
    with open(path) as f:
        r = csv.reader(f)
        next(r, None)  # header
        for row in r:
            if len(row) < 5: continue
            net, ft, cc, cpl, val = row[0], row[1], row[2].strip(), row[3].strip(), row[4].strip()
            n += 1
            fdp[(net,ft)] = val
            if cc != "":               # 自前列挙した故障
                enum += 1
                tot += int(cc)
                if cpl != "1": incomp += 1
    return dict(n_faults=n, n_enum=enum, total_cubes=tot, n_incomplete=incomp, fdp=fdp)

def parse_log(path):
    """return dict of timings parsed from log"""
    d = {"time":None,"cpu":None,"cadical":None,"bdd":None,"xid":None,"read":None,"nfault":None}
    if not os.path.exists(path): return d
    txt = open(path, errors="ignore").read()
    def g(pat):
        m = re.search(pat, txt)
        return float(m.group(1)) if m else None
    d["time"]    = g(r"//\s*Time\s*:\s*([0-9.]+)")
    d["cpu"]     = g(r"//\s*CPU Time\s*:\s*([0-9.]+)")
    d["cadical"] = g(r"CPU Time \(CaDiCaL\)\s*:\s*([0-9.]+)")
    d["bdd"]     = g(r"CPU Time \(BDD\)\s*:\s*([0-9.]+)")
    d["xid"]     = g(r"CPU Time \(Don't care\)\s*:\s*([0-9.]+)")
    d["read"]    = g(r"CPU Time \(Read Fault\)\s*:\s*([0-9.]+)")
    m = re.search(r"Number of Target Faults\s*:\s*([0-9]+)", txt)
    d["nfault"]  = int(m.group(1)) if m else None
    return d

def runinfo(path):
    if not os.path.exists(path): return (None,None)
    parts = open(path).read().split()
    rc = int(parts[0]) if parts else None
    wall = float(parts[1]) if len(parts)>1 else None
    return rc, wall

rows = []
circuits = sorted([d for d in os.listdir(RES) if os.path.isdir(os.path.join(RES,d))])
for c in circuits:
    base = parse_fdp(os.path.join(RES,c,"baseline.fdp.csv"))
    base_fdp = base["fdp"] if base else {}
    base_cubes = base["total_cubes"] if base else None
    for cfg in CONFIGS:
        cdir = os.path.join(RES,c)
        fd = parse_fdp(os.path.join(cdir,f"{cfg}.fdp.csv"))
        log = parse_log(os.path.join(cdir,f"{cfg}.log"))
        rc, wall = runinfo(os.path.join(cdir,f"{cfg}.runinfo"))
        if fd is None:
            rows.append(dict(circuit=c,config=cfg,rc=rc,status="NO_OUTPUT")); continue
        # FDP 整合（baseline基準、共通キーで比較）— 理論上 不変であるべき
        mism = sum(1 for k,v in fd["fdp"].items() if k in base_fdp and base_fdp[k]!=v)
        # cube削減率 vs baseline
        red = None
        if base_cubes and cfg!="baseline":
            red = round(100.0*(base_cubes - fd["total_cubes"])/base_cubes, 1)
        # 案1オーバーヘッド = cpu - (cadical+bdd+xid+read)  (MDC_Expandは未計測のためここに出る)
        parts = [log["cadical"],log["bdd"],log["xid"],log["read"]]
        overhead = None
        if log["cpu"] is not None and all(p is not None for p in parts):
            overhead = round(log["cpu"] - sum(parts), 3)
        rows.append(dict(circuit=c,config=cfg,rc=rc,status=("TIMEOUT" if rc==124 else "ok"),
            n_faults=fd["n_faults"], n_enum=fd["n_enum"], total_cubes=fd["total_cubes"],
            cube_reduction_pct=red, incomplete=fd["n_incomplete"],
            wall=wall, cpu=log["cpu"], t_testgen=log["cadical"], t_bdd=log["bdd"],
            t_xid=log["xid"], t_read=log["read"], overhead=overhead, fdp_mismatch=mism))

cols = ["circuit","config","status","rc","n_faults","n_enum","total_cubes","cube_reduction_pct",
        "incomplete","wall","cpu","t_testgen","t_bdd","t_xid","t_read","overhead","fdp_mismatch"]
out = os.path.join(ROOT,"summary.csv")
with open(out,"w",newline="") as f:
    w = csv.DictWriter(f, fieldnames=cols); w.writeheader()
    for r in rows: w.writerow({k:r.get(k,"") for k in cols})

# コンソールにも見やすく出す
print(f"{'circuit':9} {'config':13} {'enum':>5} {'cubes':>6} {'red%':>6} {'wall':>8} {'tgen':>7} {'bdd':>6} {'xid':>6} {'ovh':>6} {'fdpΔ':>5} {'inc':>4}")
for r in rows:
    if r.get("status")=="NO_OUTPUT":
        print(f"{r['circuit']:9} {r['config']:13} -- NO OUTPUT --"); continue
    f=lambda x: ("" if x is None else x)
    print(f"{r['circuit']:9} {r['config']:13} {f(r['n_enum']):>5} {f(r['total_cubes']):>6} "
          f"{f(r['cube_reduction_pct']):>6} {f(r['wall']):>8} {f(r['t_testgen']):>7} {f(r['t_bdd']):>6} "
          f"{f(r['t_xid']):>6} {f(r['overhead']):>6} {f(r['fdp_mismatch']):>5} {f(r['incomplete']):>4}  {r['status']}")
print(f"\nwrote {out}")

"""Export reproducible profile evidence and invariant comparisons."""
import csv,json,pathlib,re
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
OUT=HERE/'results'
OUT.mkdir(exist_ok=True)
measured={}
for p in sorted((HERE/'runs').glob('*/result.json')):
 row=json.loads(p.read_text())
 stderr=(p.parent/'stderr.log').read_text()
 row['cone']=[dict(zip(['instances','kept_gates','full_gates'],map(int,m))) for m in re.findall(r'\[CONE\] instances=(\d+) kept_gates=(\d+) full_gates=(\d+)',stderr)]
 measured[row['name']]=row
(OUT/'measurements.json').write_text(json.dumps(measured,indent=2))
with open(OUT/'timings.csv','w',newline='') as fp:
 w=csv.writer(fp)
 w.writerow(['run','wall','cpu','solve','bdd','xid','model','good_load','target','xid_values'])
 for name,r in measured.items():
  t=r['timing'];p=r['profile']
  w.writerow([name,t['Time'],t['CPU Time'],t['CPU Time (CaDiCaL)'],t['CPU Time (BDD)'],t["CPU Time (Don't care)"],*(p[k]['seconds'] for k in ['model','good_load','target','xid_model_values'])])
checks={}
for stem in ['s5378','s38584_sample','b18_sample']:
 base=HERE/'runs'/(stem+'_base')/'fdp.csv'
 for variant in ['queue','sim','compact']:
  path=HERE/'runs'/(stem+'_'+variant)/'fdp.csv'
  if base.exists() and path.exists():
   same=base.read_bytes()==path.read_bytes()
   assert same,(stem,variant)
   checks[stem+'_'+variant]=same
if 's38584_target_base' in measured and 's38584_target_queue' in measured:
 assert measured['s38584_target_base']['order']==measured['s38584_target_queue']['order']
 checks['s38584_target_order']=True
(OUT/'unchanged_outputs.json').write_text(json.dumps(checks,indent=2))
rows=[]
for path in sorted((ROOT/'output/limit30/log').glob('*')):
 text=path.read_text()
 t={k.strip():float(v) for k,v in re.findall(r'//\s+(.*?)\s+:\s+([\d.]+) sec',text)}
 if 'CPU Time' not in t: continue
 cpu=t['CPU Time'];other=cpu-sum(t.get(k,0) for k in ['CPU Time (CaDiCaL)','CPU Time (BDD)',"CPU Time (Don't care)",'CPU Time (Read Fault)'])
 faults=re.search(r'Number of Target Faults\s+:\s+(\d+)',text)
 rows.append({'file':str(path.relative_to(ROOT)),'faults':int(faults[1]) if faults else None,**t,'unattributed_cpu_seconds':other,'unattributed_percent':100*other/cpu})
(OUT/'historical_logs.json').write_text(json.dumps(rows,indent=2))
print(json.dumps({'measurements':len(measured),'unchanged_output_checks':len(checks)}))

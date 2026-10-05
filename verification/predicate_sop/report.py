"""Keep compact evidence and a complete exact CSV separate from bulky runs."""
import csv,json,pathlib
HERE=pathlib.Path(__file__).resolve().parent
OUT=HERE/'results';rows=[]
for name in ['pilot','predicates_pilot','learned_pilot','phase_pilot','frontier_pilot']:
 for r in json.loads((OUT/(name+'.json')).read_text()):
  union=r.get('union')
  rows.append({'experiment':name,'fault':r['fault'],'stuck':r['stuck'],'mode':r['mode'],'width':r['width'],
   'phase':r.get('phase','positive'),'pilot_cubes':r.get('pilot_cubes',''),
   'simulation':r.get('simulation_necessary_literals',0)>0,'enumeration_complete':r.get('enumeration_complete',r['complete']),
   'fdp_complete':r['complete'] and union is not None,'regions':r['cubes'],'enum_seconds':r['seconds'],
   'total_seconds':r.get('total_seconds'),'bdd_seconds':union['seconds'] if union else '',
   'fdp_fraction':r.get('detection_probability',union['probability'] if union else '')})
with open(OUT/'comparison.csv','w',newline='') as fp:
 w=csv.DictWriter(fp,fieldnames=rows[0]);w.writeheader();w.writerows(rows)
batch=HERE/'runs/s5378_phase'
phase=json.loads((batch/'summary.json').read_text())
(OUT/'phase_batch.json').write_text(json.dumps(phase,indent=2))
results=list(map(json.loads,(batch/'faults.jsonl').read_text().splitlines()))
with open(OUT/'s5378_phase_exact.csv','w',newline='') as fp:
 w=csv.writer(fp);w.writerow(['net_name','f_type','complete','fdp_fraction'])
 for r in results:w.writerow([r['fault'],f"sa{r['stuck']}",int(r['complete']),r['fdp']])
counts=[json.loads(p.read_text()) for p in sorted((HERE/'runs').glob('*learned512.recount_*.json'))]
(OUT/'bdd_orders.json').write_text(json.dumps(counts,indent=2))
print(json.dumps({'pilot_conditions':len(rows),'phase_faults':len(results),'bdd_orders':len(counts)}))

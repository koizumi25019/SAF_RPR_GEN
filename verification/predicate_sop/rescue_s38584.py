"""Retry only the four incomplete stem faults from the previous full batch."""
import csv,importlib.util,json,pathlib,sys,time
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from learned_groups import blocks,run
spec=importlib.util.spec_from_file_location('predicate_verifier',HERE/'verify.py')
verify=importlib.util.module_from_spec(spec);spec.loader.exec_module(verify)
netpath=blocks.ROOT/'input/circuit/s38584_C.v';start=time.monotonic();net=blocks.FastNet(netpath)
old=list(csv.DictReader(open(HERE.parent/'factored_sop/results/s38584_all_bounded.csv')))
results=[]
for row in old:
 if row['complete']=='1':continue
 fault=row['net_name'];stuck=int(row['f_type'][-1])
 t=time.monotonic()
 ex0,out,rows,_,npar,_=blocks.build_fast(net,fault,stuck,difference=True)
 ex,out,active=blocks.compact(ex0,out);prepared=(ex,out,[rows[i-1] for i in active],[],npar,set())
 prefix=HERE/'runs/rescue'/f'{fault}_{stuck}'
 r=run(net,fault,stuck,prefix,prepared,seconds=5,pilot_limit=512,union_order='large')
 r['retry_wall_seconds']=time.monotonic()-t
 assert r['complete'],r
 r['certificate']=verify.certificate(netpath,prefix)
 row.update({'complete':'1','fdp_fraction':r['union']['probability'],'value_kind':'exact','seconds':str(r['retry_wall_seconds']),'time_kind':'retry_only'})
 results.append(r);print(json.dumps(r),flush=True)
with open(HERE/'results/s38584_all_stems_exact.csv','w',newline='') as fp:
 writer=csv.DictWriter(fp,fieldnames=old[0].keys());writer.writeheader();writer.writerows(old)
summary={'faults':len(old),'complete':sum(r['complete']=='1' for r in old),'retried':len(results),
         'retry_seconds_sum':sum(r['retry_wall_seconds'] for r in results),'wall_including_verification':time.monotonic()-start,'results':results}
(HERE/'results/s38584_rescue.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary),flush=True)

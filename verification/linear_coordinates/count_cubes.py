"""Count the union of an existing cube file. No circuit is read or translated to BDD."""
import argparse
import json
import pathlib
import subprocess

HERE = pathlib.Path(__file__).resolve().parent

def count_cubes(prefix, nvars):
    cubes = json.loads((HERE/'runs'/(prefix+'.cubes.json')).read_text())
    data = f'{nvars} {len(cubes)}\n' + ''.join(' '.join(map(str,c))+' 0\n' for c in cubes)
    run = subprocess.run([str(HERE/'build'/'cube_union')],input=data,text=True,capture_output=True,check=True,timeout=60)
    result = json.loads(run.stdout)
    (HERE/'runs'/(prefix+'.union.json')).write_text(json.dumps(result,indent=2)+'\n')
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('prefix')
    p.add_argument('--nvars',type=int,required=True)
    a=p.parse_args()
    print(json.dumps(count_cubes(a.prefix,a.nvars)))

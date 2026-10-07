"""Build current sources with existing dependencies; CMake is not required.
--capture instruments an isolated copy to dump final covers for validation.
"""
import argparse, os, pathlib, re, subprocess
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parents[1]
def build(capture=False,debug=False):
    directory=HERE/'build';directory.mkdir(exist_ok=True)
    ext=pathlib.Path(os.environ.get('FDP_EXTERNAL_DIR',ROOT/'external')).resolve()
    sources=re.search(r'set\(SOURCES\s+(.*?)\)',(ROOT/'CMakeLists.txt').read_text(),re.S).group(1).split()
    sources=[ROOT/s for s in sources]
    if capture:
        source=ROOT/'src/fdp/fault_detection_prob.c'
        text=source.read_text();old='\t\t\t\tGT_Check(f, &cubes, limit_hit);'
        assert text.count(old)==1
        text=text.replace(old,'                ProfileDumpCubes(count, f, &cubes, !limit_hit);\n'+old)
        # Declaration after includes, before first function.
        at=text.index('static void AddBlockingClauseFromCube')
        text=text[:at]+'void ProfileDumpCubes(int,FNODE*,CubeSet*,int);\n'+text[at:]
        copy=directory/'fault_detection_prob_capture.c';copy.write_text(text)
        sources=[copy if s==source else s for s in sources]
        sources.append(ROOT/'verification/pipeline_profile/cube_dump.c')
    includes=[ROOT/p for p in ['src','src/fdp','src/fdp/cnf','src/lib','src/netlist','src/opt','src/fdp/xid']]+[ext/p for p in ['cadical/src','cudd','cudd/cudd']]
    binary=directory/('main_capture' if capture else 'main_debug' if debug else 'main_release')
    flags=['-g','-O0','-DDEBUG'] if debug else ['-O3','-DNDEBUG']
    cmd=['gcc','-std=gnu11','-fcommon']+flags+['-I'+str(p) for p in includes]+list(map(str,sources))+[str(ext/'cadical/build/libcadical.a'),str(ext/'cudd/cudd/.libs/libcudd.a'),'-lgmp','-lm','-lstdc++','-o',str(binary)]
    with (directory/(binary.name+'.build.log')).open('w') as log:
        subprocess.run(cmd,stdout=log,stderr=log,check=True)
    return binary
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',action='store_true');parser.add_argument('--debug',action='store_true');a=parser.parse_args();print(build(a.capture,a.debug))

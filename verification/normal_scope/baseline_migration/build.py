"""Build baseline binaries and an isolated independent GT cover checker.
Baseline production sources do not receive GT, CORE, TDF, or dump hooks.
"""
import argparse,os,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent
VERIFICATION=HERE.parents[2]
BASELINE=Path(os.environ.get('BASELINE_ROOT',VERIFICATION.parent/'SAF_RPR_GEN')).resolve()
BUILD=HERE/'build'
EXT=Path(os.environ.get('FDP_EXTERNAL_DIR',BASELINE/'external')).resolve()

def compile_program(root,sources,binary,debug=False):
    includes=[root/p for p in ['src','src/fdp','src/fdp/cnf','src/lib','src/netlist','src/opt','src/fdp/xid']]+[EXT/p for p in ['cadical/src','cudd','cudd/cudd']]
    binary.parent.mkdir(parents=True,exist_ok=True)
    flags=['-O0','-g','-DDEBUG'] if debug else ['-O3','-DNDEBUG']
    cmd=['gcc','-std=gnu11','-D_DEFAULT_SOURCE','-fcommon']+flags+['-I'+str(p) for p in includes]+list(map(str,sources))+[str(EXT/'cadical/build/libcadical.a'),str(EXT/'cudd/cudd/.libs/libcudd.a'),'-lgmp','-lm','-lstdc++','-o',str(binary)]
    with binary.with_suffix('.build.log').open('w') as log:subprocess.run(cmd,check=True,stdout=log,stderr=log)
    return binary

def sources(root):return [root/p for p in re.search(r'set\(SOURCES\s+(.*?)\)',(root/'CMakeLists.txt').read_text(),re.S).group(1).split()]

def build_all():
    BUILD.mkdir(parents=True,exist_ok=True)
    original=BUILD/'original';original.mkdir(exist_ok=True)
    # The exact pre-migration baseline commit, not another experiment branch.
    archive=subprocess.run(['git','archive','5da133b','src','CMakeLists.txt'],cwd=BASELINE,check=True,capture_output=True).stdout
    subprocess.run(['tar','-x','-C',str(original)],input=archive,check=True)
    compile_program(original,sources(original),BUILD/'main_original')
    compile_program(BASELINE,sources(BASELINE),BASELINE/'build/main_release')
    compile_program(BASELINE,sources(BASELINE),BASELINE/'build/main_debug',debug=True)
    source=BASELINE/'src/fdp/fault_detection_prob.c';text=source.read_text()
    at=text.index('static void AddBlockingClauseFromCube')
    text=text[:at]+'void BaselineDumpCubes(FNODE*,CubeSet*,int);\n'+text[at:]
    old='                dom_total_cubes  += cubes.n;';assert text.count(old)==1
    text=text.replace(old,'                BaselineDumpCubes(f, &cubes, !limit_hit);\n'+old)
    copy=BUILD/'baseline_capture.c';copy.write_text(text)
    compile_program(BASELINE,[copy if s==source else s for s in sources(BASELINE)]+[HERE/'capture.c'],BUILD/'main_capture')
    source=VERIFICATION/'src/fdp/fault_detection_prob.c';text=source.read_text()
    old='\tif (CreateConsGC() != true) return AFD_ERROR;';assert text.count(old)==1
    text=text.replace(old,old+'''\n    if (getenv("BASELINE_COVER_DIR")) {
        CheckBaselineCovers();
        *out_time_read = time_read;
        return AFD_OKAY;
    }
''')
    at=text.index('static void AddBlockingClauseFromCube')
    text=text[:at]+'void CheckBaselineCovers(void);\n'+text[at:]
    copy=BUILD/'verification_checker.c';copy.write_text(text)
    compile_program(VERIFICATION,[copy if s==source else s for s in sources(VERIFICATION)]+[HERE/'checker.c'],BUILD/'check_covers')
    print('Built actual baseline Debug/Release, exact original, isolated capture and independent GT checker',flush=True)
if __name__=='__main__':build_all()

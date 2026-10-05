"""Build a timed, isolated copy of current sources; do not edit production code."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
TREE = HERE / "build/source"
TREE.mkdir(parents=True, exist_ok=True)
shutil.copytree(ROOT / "src", TREE / "src", dirs_exist_ok=True)
p = TREE / "src/fdp/fault_detection_prob.c"
s = p.read_text()

def replace(old, new):
    global s
    assert s.count(old) == 1, (old, s.count(old))
    s = s.replace(old, new)

replace('#include <stdlib.h>', '#include <stdlib.h>\n#include <time.h>')
replace('bool AnalyzeFaultDensity(', '''static double pf_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

bool AnalyzeFaultDensity(''')
replace('if (CreateConsGC() != true) return AFD_ERROR;', '''if (CreateConsGC() != true) return AFD_ERROR;
    const char *snapshot = getenv("PF_SNAPSHOT");
    if (snapshot) {
        FILE *fp = fopen(snapshot, "w");
        if (!fp) return AFD_ERROR;
        fprintf(fp, "name,type,level,dependencies\\n");
        for (int h = 0; h < MAXSIZE_HASH; h++)
            for (FNODE *f = readdata.fault.list[h]; f; f = f->nextptr) {
                fprintf(fp, "%s,%s,%d,", f->name, FaultTypeName(f->type), f->netptr->level);
                for (int k = 0; k < f->n_subset_faults; k++) {
                    FNODE *d = f->subset_faults[k];
                    fprintf(fp, "%s%s/%s", k ? ";" : "", d->name, FaultTypeName(d->type));
                }
                fprintf(fp, "\\n");
            }
        fclose(fp);
        return AFD_OKAY;
    }''')
replace('// ソルバの初期化\n', 'double pf_start = pf_now();\n\t\t// ソルバの初期化\n')
replace('\n\t\tccadical_release(solver);\n\t}', '''
        ccadical_release(solver);
        if (getenv("PF_TIMING"))
            fprintf(stderr, "[PF_FAULT] %s %s %.9f\\n", f->name, FaultTypeName(f->type), pf_now()-pf_start);
    }''')
p.write_text(s)
cmake = (ROOT / "CMakeLists.txt").read_text().replace('${CMAKE_SOURCE_DIR}/external', str(ROOT / 'external'))
(TREE / 'CMakeLists.txt').write_text(cmake)
manifest = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted((ROOT / 'src').rglob('*')) if p.is_file()}
(HERE / 'build/source_hashes.json').write_text(json.dumps(manifest, indent=2) + '\n')
subprocess.run(['cmake', '-S', str(TREE), '-B', str(HERE / 'build/bin')], check=True)
subprocess.run(['cmake', '--build', str(HERE / 'build/bin'), '--target', 'main_release', '-j4'], check=True)

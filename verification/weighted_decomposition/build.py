"""Build a weighted union of generated regions, reusing the existing reader."""
import pathlib
import subprocess

HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parents[1]
source=(HERE.parent/'linear_scaling/blocks/region_union.cc').read_text()
def change(old,new):
    global source
    assert source.count(old)==1,old
    source=source.replace(old,new)
change('  std::vector<std::vector<int>> groups(ngroups);', '''  int original_nvars;
  if (!(std::cin>>original_nvars) || original_nvars<0) return 2;
  std::vector<mpq_class> weights;
  for (int i=0;i<nvars;i++) {
    std::string text;if (!(std::cin>>text)) return 2;
    mpq_class p(text);p.canonicalize();if(p<0 || p>1)return 2;
    weights.push_back(p);
  }
  std::vector<std::vector<int>> groups(ngroups);''')
change('    mpq_class p = (probability(Cudd_T(node)) + probability(Cudd_E(node))) / 2;',
       '''    const mpq_class &weight=weights[Cudd_NodeReadIndex(node)];
    mpq_class p = weight*probability(Cudd_T(node)) + (1-weight)*probability(Cudd_E(node));''')
change('mpz_class(1) << nvars', 'mpz_class(1) << original_nvars')
change('uniform input distribution', 'independent normalized input distribution')
(HERE/'build').mkdir(exist_ok=True)
path=HERE/'build/weighted_union.cc';path.write_text(source)
subprocess.run(['g++','-std=c++17','-O3','-Wall','-Wextra','-I',str(ROOT/'external/cudd/cudd'),
                str(path),str(ROOT/'external/cudd/cudd/.libs/libcudd.a'),'-lgmpxx','-lgmp','-lm',
                '-o',str(HERE/'build/weighted_union')],check=True)

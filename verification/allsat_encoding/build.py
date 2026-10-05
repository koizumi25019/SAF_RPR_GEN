"""Build an isolated native enumerator with optional CNF witness shrinking.

This is a non-disjoint adaptation of the projected clause-satisfaction shrink
in Masina et al. SAT 2023, not an implementation of TabularAllSAT's CDCL calculus.
"""
import pathlib
import subprocess

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def replace_once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def build():
    source = (HERE.parent/'linear_scaling/native/enumerator.cc').read_text()
    source = replace_once(source, 'struct Sensitivity {', r'''
// Drop PI literals only when the original CNF remains clause-wise satisfied
// under the current auxiliary witness. Blocking/learned clauses are excluded:
// generated cubes may overlap; the detector still blocks every emitted cube.
struct Witness {
  std::vector<Vec> clauses;
  Vec shrink(Solver &det, const Vec &fixed, int declared) {
    std::vector<unsigned char> selected(declared+1);
    for (int l:fixed) selected[std::abs(l)]=1;
    std::vector<Vec> occurrences(declared+1);
    Vec counts(clauses.size());
    for (size_t c=0;c<clauses.size();++c) {
      bool auxiliary=false;
      Vec projected;
      for (int l:clauses[c]) if (ccadical_val(det.s,l)==l) {
        if (!selected[std::abs(l)]) { auxiliary=true; break; }
        projected.push_back(std::abs(l));
      }
      if (auxiliary) continue;
      if (projected.empty()) throw std::runtime_error("CNF witness not satisfied");
      counts[c]=projected.size();
      for (int v:projected) occurrences[v].push_back(c);
    }
    Vec result;
    for (int l:fixed) {
      const auto &cs=occurrences[std::abs(l)];
      bool needed=false;
      for (int c:cs) if (counts[c]==1) {needed=true;break;}
      if (needed) result.push_back(l);
      else for (int c:cs) --counts[c];
    }
    return result;
  }
};

struct Sensitivity {''')
    source = replace_once(source, 'method != "adaptive")', 'method != "adaptive" && method != "witness")')
    source = replace_once(source, '    Solver det, off;', '''    Solver det, off;
    Witness witness;
    const bool seed = std::stoi(get("--witness-seed", "0")) || method == "witness";
    if (seed && negative_only) throw std::runtime_error("Witness seed requires ordinary PI literals");
    long long seed_removed=0;
    double seed_seconds=0;''')
    source = replace_once(source,
        'while (ls >> lit) { ccadical_add(det.s, lit); ccadical_add(off.s, lit); }',
        '''Vec c;
        while (ls >> lit) {
          ccadical_add(det.s, lit);
          if (!options.count("--off-cnf")) ccadical_add(off.s, lit);
          if (lit) c.push_back(lit);
          else {witness.clauses.push_back(c);c.clear();}
        }
        if (!c.empty()) throw std::runtime_error("Expected one complete clause per line");''')
    source = replace_once(source, '    if (npi < 0 || npi > declared', '''    if (options.count("--off-cnf")) {
      std::ifstream input(options.at("--off-cnf"));
      if (!input) throw std::runtime_error("Cannot open negative CNF");
      while (std::getline(input,line)) {
        if (line.empty() || line[0]=='c') continue;
        std::istringstream ls(line);
        if (line[0]=='p') {
          std::string p,format;int n,c;ls>>p>>format>>n>>c;
          if (n!=declared) throw std::runtime_error("CNF variable spaces differ");
        } else {
          int lit;while (ls>>lit) ccadical_add(off.s,lit);
        }
      }
    }
    if (npi < 0 || npi > declared''')
    source = replace_once(source, '    det.add({output});', '    det.add({output});\n    witness.clauses.push_back({output});')
    source = replace_once(source,
        '''        if (!oracle.unsat(fixed)) throw std::runtime_error("Initial cube not sound: support/CNF mismatch");
        fixed = off.core(fixed);''',
        '''        if (seed) {
          const auto seed_start=Clock::now();
          auto before=fixed.size();
          fixed=witness.shrink(det,fixed,declared);
          seed_removed+=before-fixed.size();
          seed_seconds+=std::chrono::duration<double>(Clock::now()-seed_start).count();
        }
        if (method != "witness") {
          if (!oracle.unsat(fixed)) throw std::runtime_error("Initial cube not sound: support/CNF mismatch");
          fixed = off.core(fixed);
        }''')
    source = replace_once(source, '         << ",\\\"care_hist\\\":{";',
        '''         << ",\\\"witness_removed_literals\\\":" << seed_removed
         << ",\\\"witness_seconds\\\":" << seed_seconds
         << ",\\\"care_hist\\\":{";''')
    directory = HERE/'build'
    directory.mkdir(exist_ok=True)
    path = directory/'enumerator.cc'
    path.write_text(source)
    subprocess.run(['g++', '-std=c++17', '-O3', '-Wall', '-Wextra',
                    '-I', str(ROOT/'external/cadical/src'), str(path),
                    str(ROOT/'external/cadical/build/libcadical.a'),
                    '-o', str(directory/'enumerator')], check=True)


if __name__ == '__main__':
    build()

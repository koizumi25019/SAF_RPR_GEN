// Input is only SAT-generated products of small block masks, never a circuit.
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <cstdint>
#include <gmpxx.h>
#include <cudd.h>

int main() {
  int nvars, ngroups, ncubes;
  if (!(std::cin >> nvars >> ngroups >> ncubes) || nvars < 0 || ngroups < 0 || ncubes < 0) return 2;
  std::vector<std::vector<int>> groups(ngroups);
  for (auto &g:groups) {
    int width; if (!(std::cin>>width) || width<1 || width>6) return 2;
    for (int j=0;j<width;j++) { int v; std::cin>>v; if(v<1 || v>nvars)return 2;g.push_back(v); }
  }
  const auto start = std::chrono::steady_clock::now();
  DdManager *mgr = Cudd_Init(nvars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
  if (!mgr) return 3;
  // Match the production cube-union path's variable-order optimization.
  Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);
  auto checked = [](DdNode *node) {
    if (!node) { std::cerr << "BDD allocation failed\n"; std::exit(3); }
    Cudd_Ref(node); return node;
  };
  DdNode *covered = checked(Cudd_ReadLogicZero(mgr));
  std::map<std::pair<int,uint64_t>,DdNode*> local_cache;
  for (int i = 0; i < ncubes; ++i) {
    DdNode *cube = checked(Cudd_ReadOne(mgr));
    for (int j=0;j<ngroups;j++) {
      uint64_t mask; if (!(std::cin>>mask) || mask==0) return 2;
      const auto &g=groups[j]; const int states=1<<g.size();
      const uint64_t full=states==64 ? ~uint64_t(0) : (uint64_t(1)<<states)-1;
      if (mask & ~full) return 2;
      if (mask==full) continue;
      auto key=std::make_pair(j,mask);
      auto it=local_cache.find(key);
      DdNode *allowed;
      if (it!=local_cache.end()) allowed=it->second;
      else {
        allowed=checked(Cudd_ReadLogicZero(mgr));
        for (int state=0;state<states;state++) if(mask>>state&1) {
          DdNode *term=checked(Cudd_ReadOne(mgr));
          for (unsigned k=0;k<g.size();k++) {
            DdNode *lit=Cudd_bddIthVar(mgr,g[k]-1);
            if (!(state>>k&1)) lit=Cudd_Not(lit);
            DdNode *next=checked(Cudd_bddAnd(mgr,term,lit));
            Cudd_RecursiveDeref(mgr,term);term=next;
          }
          DdNode *next=checked(Cudd_bddOr(mgr,allowed,term));
          Cudd_RecursiveDeref(mgr,allowed);Cudd_RecursiveDeref(mgr,term);allowed=next;
        }
        local_cache.emplace(key,allowed);
      }
      DdNode *next = checked(Cudd_bddAnd(mgr, cube, allowed));
      Cudd_RecursiveDeref(mgr, cube); cube = next;
    }
    DdNode *next = checked(Cudd_bddOr(mgr, covered, cube));
    Cudd_RecursiveDeref(mgr, covered); Cudd_RecursiveDeref(mgr, cube);
    covered = next;
  }
  std::unordered_map<DdNode *, mpq_class> memo;
  std::function<mpq_class(DdNode *)> probability = [&](DdNode *node) -> mpq_class {
    if (Cudd_IsComplement(node)) return 1 - probability(Cudd_Regular(node));
    if (Cudd_IsConstant(node)) return 1;
    auto it = memo.find(node);
    if (it != memo.end()) return it->second;
    // A skipped variable integrates to one under the uniform input distribution.
    mpq_class p = (probability(Cudd_T(node)) + probability(Cudd_E(node))) / 2;
    memo.emplace(node, p); return p;
  };
  const mpq_class p = probability(covered);
  const mpz_class space = mpz_class(1) << nvars;
  if (space % p.get_den() != 0) return 4;
  const mpz_class count = (space / p.get_den()) * p.get_num();
  const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  std::cout << "{\"cubes\":" << ncubes << ",\"nvars\":" << nvars
    << ",\"bdd_nodes\":" << Cudd_DagSize(covered)
    << ",\"probability\":\"" << p.get_str() << "\",\"model_count\":\"" << count.get_str()
    << "\",\"fdp\":" << std::setprecision(17) << p.get_d()
    << ",\"seconds\":" << seconds << "}\n";
  for (auto &entry:local_cache) Cudd_RecursiveDeref(mgr,entry.second);
  Cudd_RecursiveDeref(mgr, covered); Cudd_Quit(mgr);
}

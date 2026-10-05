// Input is only a list of SAT-generated cubes. This program cannot read a circuit.
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <gmpxx.h>
#include <cudd.h>

int main() {
  int nvars, ncubes;
  if (!(std::cin >> nvars >> ncubes) || nvars < 0 || ncubes < 0) return 2;
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
  for (int i = 0; i < ncubes; ++i) {
    DdNode *cube = checked(Cudd_ReadOne(mgr));
    int lit;
    do {
      if (!(std::cin >> lit) || std::abs(lit) > nvars) return 2;
      if (!lit) break;
      DdNode *v = Cudd_bddIthVar(mgr, std::abs(lit) - 1);
      if (lit < 0) v = Cudd_Not(v);
      DdNode *next = checked(Cudd_bddAnd(mgr, cube, v));
      Cudd_RecursiveDeref(mgr, cube); cube = next;
    } while (true);
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
  Cudd_RecursiveDeref(mgr, covered); Cudd_Quit(mgr);
}

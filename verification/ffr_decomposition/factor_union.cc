// Input: SAT-generated PI cube covers for factors; no gate netlist is accepted.
// Preserve correlations by ANDing the factor covers on the SAME PI variables.
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <cudd.h>
#include <gmpxx.h>

int main() {
  int n, nf;
  if (!(std::cin >> n >> nf) || n < 0 || nf < 0) return 2;
  auto start = std::chrono::steady_clock::now();
  DdManager *m = Cudd_Init(n, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
  if (!m) return 3;
  Cudd_AutodynEnable(m, CUDD_REORDER_SIFT);
  auto hold = [](DdNode *p) {
    if (!p) std::exit(3);
    Cudd_Ref(p); return p;
  };
  DdNode *result = hold(Cudd_ReadOne(m));
  int cubes = 0;
  for (int f = 0; f < nf; ++f) {
    int nc;
    if (!(std::cin >> nc) || nc < 0) return 2;
    cubes += nc;
    DdNode *factor = hold(Cudd_ReadLogicZero(m));
    for (int c = 0; c < nc; ++c) {
      DdNode *cube = hold(Cudd_ReadOne(m));
      while (true) {
        int lit;
        if (!(std::cin >> lit) || std::abs(lit) > n) return 2;
        if (!lit) break;
        DdNode *v = Cudd_bddIthVar(m, std::abs(lit)-1);
        if (lit < 0) v = Cudd_Not(v);
        DdNode *next = hold(Cudd_bddAnd(m, cube, v));
        Cudd_RecursiveDeref(m, cube); cube = next;
      }
      DdNode *next = hold(Cudd_bddOr(m, factor, cube));
      Cudd_RecursiveDeref(m, cube);
      Cudd_RecursiveDeref(m, factor); factor = next;
    }
    DdNode *next = hold(Cudd_bddAnd(m, result, factor));
    Cudd_RecursiveDeref(m, result);
    Cudd_RecursiveDeref(m, factor); result = next;
  }
  std::unordered_map<DdNode *, mpq_class> memo;
  std::function<mpq_class(DdNode *)> prob = [&](DdNode *p) -> mpq_class {
    if (Cudd_IsComplement(p)) return 1-prob(Cudd_Regular(p));
    if (Cudd_IsConstant(p)) return 1;
    auto it = memo.find(p);
    if (it != memo.end()) return it->second;
    mpq_class value = (prob(Cudd_T(p))+prob(Cudd_E(p)))/2;
    memo.emplace(p, value); return value;
  };
  mpq_class p = prob(result);
  std::cout << "{\"probability\":\"" << p.get_str()
            << "\",\"cubes\":" << cubes << ",\"factors\":" << nf
            << ",\"bdd_nodes\":" << Cudd_DagSize(result)
            << ",\"seconds\":" << std::setprecision(12)
            << std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
            << "}\n";
  Cudd_RecursiveDeref(m, result); Cudd_Quit(m);
}

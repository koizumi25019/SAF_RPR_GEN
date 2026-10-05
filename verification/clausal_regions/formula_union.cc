// Count only the union of SAT-generated clausal regions. Each predicate is a
// bounded truth table over independent input coordinates. The circuit's
// detection function is never read or converted to a BDD by this program.
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <cudd.h>
#include <gmpxx.h>

namespace {

DdNode *hold(DdNode *node) {
  if (!node) {
    std::cerr << "BDD allocation failed\n";
    std::exit(3);
  }
  Cudd_Ref(node);
  return node;
}

DdNode *truth_table(DdManager *mgr, const std::vector<int> &group,
                    uint64_t mask) {
  const int states = 1 << group.size();
  const uint64_t full = states == 64 ? ~uint64_t(0)
                                     : (uint64_t(1) << states) - 1;
  if (mask & ~full) return nullptr;
  DdNode *result = hold(Cudd_ReadLogicZero(mgr));
  for (int state = 0; state < states; ++state) {
    if (!(mask >> state & 1)) continue;
    DdNode *term = hold(Cudd_ReadOne(mgr));
    for (size_t j = 0; j < group.size(); ++j) {
      DdNode *lit = Cudd_bddIthVar(mgr, group[j] - 1);
      if (!(state >> j & 1)) lit = Cudd_Not(lit);
      DdNode *next = hold(Cudd_bddAnd(mgr, term, lit));
      Cudd_RecursiveDeref(mgr, term);
      term = next;
    }
    DdNode *next = hold(Cudd_bddOr(mgr, result, term));
    Cudd_RecursiveDeref(mgr, result);
    Cudd_RecursiveDeref(mgr, term);
    result = next;
  }
  return result;
}

}  // namespace

int main() {
  int nvars, npredicates, nregions;
  if (!(std::cin >> nvars >> npredicates >> nregions) || nvars < 0 ||
      npredicates < 0 || nregions < 0)
    return 2;

  std::vector<std::vector<int>> groups(npredicates);
  std::vector<uint64_t> masks(npredicates);
  for (int i = 0; i < npredicates; ++i) {
    int width;
    if (!(std::cin >> width) || width < 1 || width > 6) return 2;
    groups[i].resize(width);
    for (int &var : groups[i]) {
      if (!(std::cin >> var) || var < 1 || var > nvars) return 2;
    }
    if (!(std::cin >> masks[i])) return 2;
  }

  const auto start = std::chrono::steady_clock::now();
  DdManager *mgr =
      Cudd_Init(nvars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
  if (!mgr) return 3;
  Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);

  std::vector<DdNode *> predicates;
  predicates.reserve(npredicates);
  for (int i = 0; i < npredicates; ++i) {
    DdNode *node = truth_table(mgr, groups[i], masks[i]);
    if (!node) return 2;
    predicates.push_back(node);
  }

  DdNode *covered = hold(Cudd_ReadLogicZero(mgr));
  for (int r = 0; r < nregions; ++r) {
    int nclauses;
    if (!(std::cin >> nclauses) || nclauses < 0) return 2;
    DdNode *region = hold(Cudd_ReadOne(mgr));
    for (int c = 0; c < nclauses; ++c) {
      int size;
      if (!(std::cin >> size) || size < 1) return 2;
      DdNode *clause = hold(Cudd_ReadLogicZero(mgr));
      for (int j = 0; j < size; ++j) {
        int literal;
        if (!(std::cin >> literal) || !literal ||
            std::abs(literal) > npredicates)
          return 2;
        DdNode *atom = predicates[std::abs(literal) - 1];
        if (literal < 0) atom = Cudd_Not(atom);
        DdNode *next = hold(Cudd_bddOr(mgr, clause, atom));
        Cudd_RecursiveDeref(mgr, clause);
        clause = next;
      }
      DdNode *next = hold(Cudd_bddAnd(mgr, region, clause));
      Cudd_RecursiveDeref(mgr, region);
      Cudd_RecursiveDeref(mgr, clause);
      region = next;
    }
    DdNode *next = hold(Cudd_bddOr(mgr, covered, region));
    Cudd_RecursiveDeref(mgr, covered);
    Cudd_RecursiveDeref(mgr, region);
    covered = next;
  }

  std::unordered_map<DdNode *, mpq_class> memo;
  std::function<mpq_class(DdNode *)> probability =
      [&](DdNode *node) -> mpq_class {
    if (Cudd_IsComplement(node)) return 1 - probability(Cudd_Regular(node));
    if (Cudd_IsConstant(node)) return 1;
    const auto found = memo.find(node);
    if (found != memo.end()) return found->second;
    const mpq_class value =
        (probability(Cudd_T(node)) + probability(Cudd_E(node))) / 2;
    memo.emplace(node, value);
    return value;
  };
  const mpq_class p = probability(covered);
  const mpz_class space = mpz_class(1) << nvars;
  if (space % p.get_den() != 0) return 4;
  const mpz_class models = (space / p.get_den()) * p.get_num();
  const double seconds = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - start)
                             .count();
  std::cout << "{\"regions\":" << nregions << ",\"predicates\":"
            << npredicates << ",\"nvars\":" << nvars
            << ",\"bdd_nodes\":" << Cudd_DagSize(covered)
            << ",\"probability\":\"" << p.get_str()
            << "\",\"model_count\":\"" << models.get_str()
            << "\",\"fdp\":" << std::setprecision(17) << p.get_d()
            << ",\"seconds\":" << seconds << "}\n";

  Cudd_RecursiveDeref(mgr, covered);
  for (DdNode *node : predicates) Cudd_RecursiveDeref(mgr, node);
  Cudd_Quit(mgr);
  return 0;
}

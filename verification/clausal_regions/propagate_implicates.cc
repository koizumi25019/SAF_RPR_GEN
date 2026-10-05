// Extract unit and binary implicates visible by unit propagation from D.
// Every emitted (a OR b) follows from propagating b under D AND NOT a.
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "cadical.hpp"

using Clause = std::vector<int>;

int main(int argc, char **argv) {
  if (argc < 4) return 2;
  const std::string path = argv[1];
  const int output = std::stoi(argv[2]);
  std::vector<int> atoms;
  std::map<int, int> atom_index;
  for (int i = 3; i < argc; ++i) {
    const int atom = std::stoi(argv[i]);
    if (atom <= 0 || atom_index.count(atom)) return 2;
    atoms.push_back(atom);
    atom_index[atom] = i - 2;
  }

  CaDiCaL::Solver solver;
  solver.set("quiet", 1);
  std::ifstream input(path);
  if (!input) return 3;
  std::string line;
  int declared = 0;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == 'c') continue;
    if (line[0] == 'p') {
      std::istringstream header(line);
      std::string p, cnf;
      int count;
      header >> p >> cnf >> declared >> count;
      solver.declare_more_variables(declared);
      continue;
    }
    std::istringstream clause(line);
    int literal;
    while (clause >> literal) solver.add(literal);
  }
  if (!output || std::abs(output) > declared) return 2;
  for (int atom : atoms) {
    if (atom > declared) return 2;
    solver.freeze(atom);
  }
  solver.add(output);
  solver.add(0);

  std::set<Clause> result;
  auto local_literal = [&](int expression_literal) {
    const auto found = atom_index.find(std::abs(expression_literal));
    if (found == atom_index.end()) return 0;
    return found->second * (expression_literal > 0 ? 1 : -1);
  };
  auto add_clause = [&](Clause clause) {
    std::sort(clause.begin(), clause.end());
    clause.erase(std::unique(clause.begin(), clause.end()), clause.end());
    for (int literal : clause)
      if (std::binary_search(clause.begin(), clause.end(), -literal)) return;
    result.insert(clause);
  };

  int status = solver.propagate();
  if (status == 20) {
    std::cout << "UNSAT\n";
    return 0;
  }
  std::vector<int> implied;
  solver.implied(implied);
  for (int literal : implied) {
    const int local = local_literal(literal);
    if (local) add_clause({local});
  }
  solver.reset_assumptions();

  for (int first = 1; first <= static_cast<int>(atoms.size()); ++first) {
    for (int sign : {1, -1}) {
      const int local_first = sign * first;
      const int expression_first = sign * atoms[first - 1];
      solver.assume(-expression_first);
      status = solver.propagate();
      if (status == 20) {
        add_clause({local_first});
      } else {
        implied.clear();
        solver.implied(implied);
        for (int expression_second : implied) {
          const int local_second = local_literal(expression_second);
          if (local_second) add_clause({local_first, local_second});
        }
      }
      solver.reset_assumptions();
    }
  }

  std::cout << result.size() << '\n';
  for (const Clause &clause : result) {
    std::cout << clause.size();
    for (int literal : clause) std::cout << ' ' << literal;
    std::cout << '\n';
  }
}

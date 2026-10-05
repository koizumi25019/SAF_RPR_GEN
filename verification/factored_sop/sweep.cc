// Bounded SAT sweeping. Simulation proposes pairs; SAT proves every rewrite.
#include "ccadical.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
using Clock = std::chrono::steady_clock;
struct Pair { int node, replacement, a, b; };
struct Deadline {
  Clock::time_point end;
  static int stop(void *p) { return Clock::now() >= static_cast<Deadline *>(p)->end; }
};
int main(int argc, char **argv) {
  if (argc != 4) return 2;
  const auto start = Clock::now();
  std::ifstream cnf(argv[1]), pairs_file(argv[2]);
  if (!cnf || !pairs_file) return 2;
  std::vector<Pair> pairs;
  Pair item;
  while (pairs_file >> item.node >> item.replacement >> item.a >> item.b) pairs.push_back(item);
  CCaDiCaL *solver = ccadical_init();
  ccadical_set_option(solver, "quiet", 1);
  std::string line;
  while (std::getline(cnf, line)) {
    if (line.empty() || line[0] == 'c') continue;
    std::istringstream in(line);
    if (line[0] == 'p') {
      std::string p, format; int variables, clauses;
      in >> p >> format >> variables >> clauses;
      ccadical_declare_more_variables(solver, variables);
    } else { int lit; while (in >> lit) ccadical_add(solver, lit); }
  }
  for (const auto &p : pairs) {
    ccadical_freeze(solver, std::abs(p.a));
    ccadical_freeze(solver, std::abs(p.b));
  }
  Deadline deadline{Clock::now()+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(std::stod(argv[3])))};
  ccadical_set_terminate(solver, &deadline, Deadline::stop);
  std::vector<Pair> proven;
  int calls = 0, examined = 0;
  bool timed_out = false;
  auto solve = [&](int a, int b) {
    ccadical_assume(solver, a); ccadical_assume(solver, b);
    ++calls; return ccadical_solve(solver);
  };
  for (const auto &p : pairs) {
    if (Deadline::stop(&deadline)) { timed_out = true; break; }
    ++examined;
    int rc = solve(p.a, -p.b);
    if (rc == 0) { timed_out = true; break; }
    if (rc != 20) continue;
    rc = solve(-p.a, p.b);
    if (rc == 0) { timed_out = true; break; }
    if (rc == 20) proven.push_back(p);
  }
  std::cout << "{\"pairs\":" << pairs.size() << ",\"examined\":" << examined
            << ",\"sat_calls\":" << calls << ",\"timed_out\":" << (timed_out ? "true" : "false")
            << ",\"seconds\":" << std::chrono::duration<double>(Clock::now()-start).count() << ",\"proven\":[";
  for (size_t i = 0; i < proven.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << '[' << proven[i].node << ',' << proven[i].replacement << ']';
  }
  std::cout << "]}\n";
  ccadical_release(solver);
}

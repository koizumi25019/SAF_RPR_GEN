#include "ccadical.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

// Standalone SAT/semantic-DC cover enumerator.  This code does not contain BDD
// operations and never counts a circuit.  Generated cubes may overlap.
using Clock = std::chrono::steady_clock;
using Vec = std::vector<int>;
struct Timeout {};
struct Deadline {
  Clock::time_point end;
  static int stop(void *p) { return Clock::now() >= static_cast<Deadline *>(p)->end; }
};
struct Solver {
  CCaDiCaL *s = ccadical_init();
  long long calls = 0, sat = 0, unsat = 0;
  Solver() { ccadical_set_option(s, "quiet", 1); }
  ~Solver() { ccadical_release(s); }
  void add(const Vec &clause) {
    for (int lit : clause) ccadical_add(s, lit);
    ccadical_add(s, 0);
  }
  int solve(const Vec &assumptions = {}) {
    for (int lit : assumptions) ccadical_assume(s, lit);
    ++calls;
    int rc = ccadical_solve(s);
    if (rc == 0) throw Timeout{};
    if (rc == 10) ++sat;
    else if (rc == 20) ++unsat;
    else throw std::runtime_error("Unexpected SAT result");
    return rc;
  }
  Vec core(const Vec &lits) {
    Vec result;
    for (int lit : lits) if (ccadical_failed(s, lit)) result.push_back(lit);
    return result;
  }
};

Vec join(const Vec &a, const Vec &b) {
  Vec result = a; result.insert(result.end(), b.begin(), b.end()); return result;
}
struct Oracle {
  Solver &solver;
  int bad;
  const std::vector<unsigned char> *essential = nullptr;
  bool unsat(const Vec &lits) { return solver.solve(join(Vec{bad}, lits)) == 20; }
  Vec greedy(Vec fixed) {
    // Once a deletion is SAT, it stays SAT when further assumptions are removed;
    // one pass, including core reductions, therefore gives a subset-minimal cube.
    const Vec initial = fixed;
    for (int candidate : initial) {
      if (essential && (*essential)[std::abs(candidate)]) continue;
      auto pos = std::find(fixed.begin(), fixed.end(), candidate);
      if (pos == fixed.end()) continue;
      Vec trial = fixed;
      trial.erase(trial.begin() + (pos - fixed.begin()));
      if (unsat(trial)) fixed = solver.core(trial);
    }
    return fixed;
  }
  Vec quickxplain(const Vec &background, const Vec &candidates) {
    if (candidates.empty() || unsat(background)) return {};
    if (candidates.size() == 1) return Vec{candidates.front()};
    auto middle = candidates.begin() + candidates.size() / 2;
    Vec first(candidates.begin(), middle), second(middle, candidates.end());
    Vec d1 = quickxplain(join(background, second), first);
    Vec d2 = quickxplain(join(background, d1), second);
    return join(d1, d2);
  }
};

struct Sensitivity {
  struct Gate { int id; char kind; Vec args; };
  int npi=0, maxnode=0, output=0;
  std::vector<Gate> gates;
  std::vector<std::vector<std::pair<int,int>>> perturb;
  void read(const std::string &path, int declared) {
    std::ifstream in(path);int ng,np;
    if (!(in>>npi>>maxnode>>output>>ng>>np)) throw std::runtime_error("Invalid simulation file");
    for (int i=0;i<ng;i++) {
      Gate g;int size;in>>g.id>>g.kind>>size;
      for (int j=0;j<size;j++) {int a;in>>a;g.args.push_back(a);}gates.push_back(g);
    }
    perturb.resize(declared+1);
    for (int i=0;i<np;i++) {
      int atom,size;in>>atom>>size;
      if (atom<1 || atom>declared) throw std::runtime_error("Invalid simulation atom");
      for (int j=0;j<size;j++) {int v,b;in>>v>>b;perturb[atom].push_back({v,b});}
    }
    if (!in) throw std::runtime_error("Truncated simulation file");
  }
  long long necessary(Solver &det, const Vec &fixed, std::vector<unsigned char> &essential) {
    std::fill(essential.begin(),essential.end(),0);
    Vec mapped;
    for (int l:fixed) if (!perturb[std::abs(l)].empty()) mapped.push_back(std::abs(l));
    std::vector<uint64_t> values(maxnode+1);long long count=0;
    for (size_t offset=0;offset<mapped.size();offset+=64) {
      const size_t lanes=std::min(size_t(64),mapped.size()-offset);
      values[0]=0;
      for (int i=1;i<=npi;i++) values[i]=ccadical_val(det.s,i)>0 ? ~uint64_t(0) : 0;
      for (size_t lane=0;lane<lanes;lane++) {
        const uint64_t bit=uint64_t(1)<<lane;
        for (auto [v,b]:perturb[mapped[offset+lane]]) {
          if (b==2) values[v]^=bit;
          else if(b)values[v]|=bit;
          else values[v]&=~bit;
        }
      }
      auto value=[&](int l) {return (l&1) ? ~values[l/2] : values[l/2];};
      for (const auto &g:gates) {
        uint64_t acc=g.kind=='a' ? ~uint64_t(0) : 0;
        for (int l:g.args) {
          if(g.kind=='a')acc&=value(l);else acc^=value(l);
        }
        values[g.id]=acc;
      }
      const uint64_t detected=value(output);
      for (size_t lane=0;lane<lanes;lane++) if (!(detected>>lane&1)) {
        essential[mapped[offset+lane]]=1;count++;
      }
    }
    return count;
  }
};

int main(int argc, char **argv) {
  try {
    const auto total_start = Clock::now();
    std::map<std::string, std::string> options;
    for (int i = 1; i < argc; i += 2) {
      if (i + 1 == argc) throw std::runtime_error("Every option requires a value");
      options[argv[i]] = argv[i + 1];
    }
    auto required = [&](const std::string &k) {
      if (!options.count(k)) throw std::runtime_error("Missing " + k);
      return options.at(k);
    };
    auto get = [&](const std::string &k, const std::string &d) {
      return options.count(k) ? options.at(k) : d;
    };
    const std::string cnf = required("--cnf"), prefix = required("--prefix");
    const int npi = std::stoi(required("--npi"));
    const int output = std::stoi(required("--output"));
    // Optional care literal H.  With --care H, enumerate a cover U such that
    // H & U == H & output.  Detector models satisfy H & output, while the
    // semantic-DC oracle checks H & !output & cube for UNSAT.
    const int care = std::stoi(get("--care", "0"));
    const std::string method = get("--method", "core");
    if (method != "core" && method != "greedy" && method != "qx" && method != "adaptive")
      throw std::runtime_error("method must be core, greedy, qx or adaptive");
    const long long limit = std::stoll(get("--limit", "1000000"));
    const double seconds = std::stod(get("--seconds", "60"));
    const int cadence = std::stoi(get("--cadence", "16"));
    const bool certify = std::stoi(get("--certify", "0"));
    const bool negative_only = std::stoi(get("--negative-only", "0"));
    Vec support;
    if (options.count("--support")) {
      auto text = options.at("--support"); std::replace(text.begin(), text.end(), ',', ' ');
      std::istringstream in(text); int var;
      while (in >> var) support.push_back(var);
    } else for (int i = 1; i <= npi; ++i) support.push_back(i);
    if (get("--order", "forward") == "reverse") std::reverse(support.begin(), support.end());
    Solver det, off;
    std::ifstream in(cnf);
    if (!in) throw std::runtime_error("Cannot read CNF: " + cnf);
    std::string line; int declared = 0, clauses = 0;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == 'c') continue;
      if (line[0] == 'p') {
        std::istringstream hdr(line); std::string p, format;
        hdr >> p >> format >> declared >> clauses;
        ccadical_declare_more_variables(det.s, declared);
        ccadical_declare_more_variables(off.s, declared);
      } else {
        std::istringstream ls(line); int lit;
        while (ls >> lit) { ccadical_add(det.s, lit); ccadical_add(off.s, lit); }
      }
    }
    if (npi < 0 || npi > declared || std::abs(output) > declared || !output ||
        std::abs(care) > declared)
      throw std::runtime_error("Invalid PI count or detection literal");
    for (int var : support) if (var < 1 || var > npi)
      throw std::runtime_error("Support must contain PI variable IDs");
    const bool freeze_support_only = std::stoi(get("--freeze-support-only", "0"));
    if (freeze_support_only) {
      for (int var : support) { ccadical_freeze(det.s, var); ccadical_freeze(off.s, var); }
    } else {
      for (int i = 1; i <= npi; ++i) { ccadical_freeze(det.s, i); ccadical_freeze(off.s, i); }
    }
    det.add({output});
    if (care) {
      det.add({care});
      off.add({care});
    }
    if (std::stoi(get("--off-unit", "0"))) off.add({-output});
    Oracle oracle{off, -output};
    Sensitivity sensitivity;
    const bool simulate = options.count("--sim");
    if (simulate) sensitivity.read(options.at("--sim"),declared);
    std::vector<unsigned char> essential(declared+1);
    const auto start = Clock::now();
    Deadline deadline{start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds))};
    ccadical_set_terminate(det.s, &deadline, Deadline::stop);
    ccadical_set_terminate(off.s, &deadline, Deadline::stop);
    std::vector<Vec> cubes;
    std::map<int, long long> hist;
    bool complete = false, timed_out = false;
    long long refined = 0, removed = 0;
    long long sim_essential = 0;
    double simulation_seconds = 0;
    try {
      while (limit <= 0 || static_cast<long long>(cubes.size()) < limit) {
        if (Deadline::stop(&deadline)) throw Timeout{};
        if (det.solve() == 20) { complete = true; break; }
        Vec fixed;
        for (int var : support) {
          const bool value = ccadical_val(det.s, var) > 0;
          if (!negative_only || !value) fixed.push_back(value ? var : -var);
        }
        if (!oracle.unsat(fixed)) throw std::runtime_error("Initial cube not sound: support/CNF mismatch");
        fixed = off.core(fixed);
        bool refine = method == "greedy" || method == "qx";
        // Sample prime shrinking; if recent samples remove no literals, avoid
        // paying for a minimality proof on every generated cube.
        if (method == "adaptive")
          refine = cubes.size() < 8 || cubes.size() % std::max(cadence, 1) == 0 ||
                   (refined > 0 && removed * 4 > refined);
        if (refine) {
          auto before = fixed.size();
          if (simulate) {
            const auto t=Clock::now();
            sim_essential+=sensitivity.necessary(det,fixed,essential);
            simulation_seconds+=std::chrono::duration<double>(Clock::now()-t).count();
            oracle.essential=&essential;
          }
          if (method=="qx" && simulate) {
            Vec base,rest;
            for (int l:fixed) (essential[std::abs(l)] ? base : rest).push_back(l);
            fixed=join(base,oracle.quickxplain(base,rest));
          } else fixed = method == "qx" ? oracle.quickxplain({}, fixed) : oracle.greedy(fixed);
          ++refined; removed += before - fixed.size();
        }
        // Optional comparison mode reproduces the old Python prototype's final
        // re-solve/core projection.  Soundness already follows from UNSAT/core.
        if (certify) {
          if (!oracle.unsat(fixed)) throw std::runtime_error("Shrunk cube is unsound");
          fixed = off.core(fixed);
        }
        ++hist[fixed.size()];
        cubes.push_back(fixed);
        for (int &lit : fixed) lit = -lit;
        det.add(fixed);
      }
      if (!complete && !Deadline::stop(&deadline)) complete = det.solve() == 20;
    } catch (const Timeout &) { timed_out = true; }
    const double elapsed = std::chrono::duration<double>(Clock::now() - start).count();
    const double init = std::chrono::duration<double>(start - total_start).count();
    std::ofstream cube_file(prefix + ".cubes.json");
    if (!cube_file) throw std::runtime_error("Cannot write cubes: " + prefix);
    cube_file << '[';
    for (size_t i = 0; i < cubes.size(); ++i) {
      if (i) cube_file << ',';
      cube_file << '[';
      for (size_t j = 0; j < cubes[i].size(); ++j) {
        if (j) cube_file << ',';
        cube_file << cubes[i][j];
      }
      cube_file << ']';
    }
    cube_file << "]\n"; cube_file.close();
    std::ostringstream json;
    json.precision(10);
    json << "{\"method\":\"" << method << "\",\"cubes\":" << cubes.size()
         << ",\"care_literal\":" << care
         << ",\"complete\":" << (complete ? "true" : "false")
         << ",\"timed_out\":" << (timed_out ? "true" : "false")
         << ",\"seconds\":" << elapsed << ",\"init_seconds\":" << init
         << ",\"sat_calls\":" << det.calls + off.calls
         << ",\"detector_calls\":" << det.calls << ",\"oracle_calls\":" << off.calls
         << ",\"oracle_sat\":" << off.sat << ",\"oracle_unsat\":" << off.unsat
         << ",\"refined_cubes\":" << refined << ",\"refinement_removed_literals\":" << removed
         << ",\"simulation_necessary_literals\":" << sim_essential
         << ",\"simulation_seconds\":" << simulation_seconds
         << ",\"care_hist\":{";
    bool first = true;
    for (auto [care, count] : hist) {
      if (!first) json << ',';
      first = false;
      json << '\"' << care << "\":" << count;
    }
    json << "}}";
    std::ofstream result_file(prefix + ".result.json"); result_file << json.str() << '\n';
    std::cout << json.str() << std::endl;
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}

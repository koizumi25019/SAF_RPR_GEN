#include "clasp_bridge.h"
#include <clasp/clasp_facade.h>
#include <clasp/cli/clasp_options.h>
#include <clasp/enumerator.h>
#include <chrono>
#include <cstdio>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if !CLASP_HAS_THREADS
#error "SB-XID time limits require clasp built with CLASP_BUILD_WITH_THREADS=ON"
#endif

namespace {
/* Only the solve thread writes this buffer; the caller reads it after get().
 * Copy models immediately: a later optimization step invalidates Model data.
 */
class LastModel : public Clasp::EventHandler {
public:
    explicit LastModel(int variables) : values(variables + 1, 0), found(false) {}
    bool onModel(const Clasp::Solver&, const Clasp::Model& model) override {
        for (size_t i = 1; i < values.size(); ++i) {
            if (!model.hasVar(static_cast<Clasp::Var>(i)))
                throw std::runtime_error("clasp model is missing an original OPB variable");
            values[i] = model.isTrue(Clasp::posLit(static_cast<Clasp::Var>(i))) ? 1 : 0;
        }
        found = true;
        return true; /* continue optimization until completion or deadline */
    }
    std::vector<int> values;
    bool found;
};
}

extern "C" SbxidSolveResult sbxid_solve_opb(const char* opb, size_t length,
                                            int variables, int* values,
                                            double limit_seconds)
{
    SbxidSolveResult report = {};
    report.status = SBXID_SOLVE_ERROR;
    try {
        if (!opb || !values || variables < 1 || !(limit_seconds > 0.0))
            throw std::invalid_argument("invalid OPB solver arguments");

        Clasp::Cli::ClaspCliConfig config;
        if (!config.setValue("solver.opt_heuristic", "model") ||
            !config.setValue("solve.opt_mode", "opt") ||
            !config.setValue("solve.parallel_mode", "1"))
            throw std::runtime_error("cannot configure clasp optimization");
        config.solve.numModels = 0;

        std::istringstream input(std::string(opb, length));
        LastModel last(variables); /* outlives facade and its solve handle */
        Clasp::ClaspFacade facade;
        facade.start(config, input);
        facade.enableSolveInterrupts();
        facade.read();
        facade.prepare();

        const auto start = std::chrono::steady_clock::now();
        auto handle = facade.solve(Clasp::SolveMode_t::Async, Clasp::LitVec(), &last);
        for (;;) {
            const double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            if (handle.waitFor(elapsed < limit_seconds ? limit_seconds - elapsed : 0.0))
                break;
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()
                >= limit_seconds) {
                report.timed_out = 1;
                handle.cancel();
                break;
            }
        }
        const Clasp::SolveResult result = handle.get(); /* join before using last */
        report.seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();

        if (result.unsat()) report.status = SBXID_SOLVE_UNSAT;
        else if (last.found) {
            for (int i = 1; i <= variables; ++i) values[i] = last.values[i];
            report.status = result.exhausted() && facade.summary().optimum()
                ? SBXID_SOLVE_OPTIMAL : SBXID_SOLVE_FEASIBLE;
        }
        else if (result.unknown()) report.status = SBXID_SOLVE_UNKNOWN;
        else throw std::runtime_error("clasp returned SAT without a captured model");
    }
    catch (const std::exception& error) {
        report.status = SBXID_SOLVE_ERROR;
        std::snprintf(report.error, sizeof(report.error), "%s", error.what());
    }
    catch (...) {
        report.status = SBXID_SOLVE_ERROR;
        std::snprintf(report.error, sizeof(report.error), "unknown clasp exception");
    }
    return report;
}

extern "C" const char* sbxid_clasp_version(void)
{
    return CLASP_VERSION;
}

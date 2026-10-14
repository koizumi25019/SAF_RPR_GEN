#include <cassert>
#include <cstring>
#include <sstream>
#include <vector>
#include "fdp/sbxid/clasp_bridge.h"

int main()
{
    /* Signed coefficients and inline comments are used by the original SB code. */
    const char* opb = "* #variable= 3 #constraint= 2\n"
        "min: +10 x1 +10 x2 -1 x3;\n"
        "+1 x1 +1 x2 >= 1; * at least one PI\n"
        "-1 x3 >= 0;\n";
    int values[4] = {};
    auto result = sbxid_solve_opb(opb, std::strlen(opb), 3, values, 1.0);
    assert(result.status == SBXID_SOLVE_OPTIMAL && !result.timed_out);
    assert(values[1] + values[2] == 1 && values[3] == 0);

    const char* unsat = "* #variable= 1 #constraint= 2\n"
        "min: +1 x1;\n+1 x1 >= 1;\n-1 x1 >= 0;\n";
    result = sbxid_solve_opb(unsat, std::strlen(unsat), 1, values, 1.0);
    assert(result.status == SBXID_SOLVE_UNSAT);
    const char* invalid = "* #variable= 1 #constraint= 1\nnot an OPB constraint";
    result = sbxid_solve_opb(invalid, std::strlen(invalid), 1, values, 1.0);
    assert(result.status == SBXID_SOLVE_ERROR && result.error[0]);

    /* A deadline on a hard UNSAT instance, followed by a fresh easy solve.
     * Fast machines may prove UNSAT before the deadline; both outcomes are valid.
     */
    const int pigeons = 21, holes = 20, variables = pigeons * holes;
    std::ostringstream hard;
    hard << "* #variable= " << variables << " #constraint= " << pigeons + holes << "\n";
    for (int p = 0; p < pigeons; ++p) {
        for (int h = 0; h < holes; ++h) hard << "+1 x" << p * holes + h + 1 << ' ';
        hard << ">= 1;\n";
    }
    for (int h = 0; h < holes; ++h) {
        for (int p = 0; p < pigeons; ++p) hard << "-1 x" << p * holes + h + 1 << ' ';
        hard << ">= -1;\n";
    }
    std::vector<int> many(variables + 1);
    std::string text = hard.str();
    result = sbxid_solve_opb(text.data(), text.size(), variables, many.data(), 0.01);
    assert(result.status == SBXID_SOLVE_UNSAT ||
        (result.status == SBXID_SOLVE_UNKNOWN && result.timed_out));

    /* An easy feasible solution, followed by a hard optimality proof.
     * Keep the last complete model when the one-second deadline expires.
     * A faster solver may prove optimality; the model must be valid either way.
     */
    const int opt_pigeons = 25, opt_holes = 24;
    const int hole_vars = opt_pigeons * opt_holes, opt_vars = hole_vars + opt_pigeons;
    std::ostringstream optimization;
    optimization << "* #variable= " << opt_vars << " #constraint= "
                 << opt_pigeons + opt_holes << "\nmin: ";
    for (int p = 0; p < opt_pigeons; ++p) optimization << "+1 x" << hole_vars + p + 1 << ' ';
    optimization << ";\n";
    for (int p = 0; p < opt_pigeons; ++p) {
        for (int h = 0; h < opt_holes; ++h) optimization << "+1 x" << p * opt_holes + h + 1 << ' ';
        optimization << "+1 x" << hole_vars + p + 1 << " >= 1;\n";
    }
    for (int h = 0; h < opt_holes; ++h) {
        for (int p = 0; p < opt_pigeons; ++p) optimization << "-1 x" << p * opt_holes + h + 1 << ' ';
        optimization << ">= -1;\n";
    }
    std::vector<int> feasible(opt_vars + 1);
    text = optimization.str();
    result = sbxid_solve_opb(text.data(), text.size(), opt_vars, feasible.data(), 1.0);
    assert(result.status == SBXID_SOLVE_OPTIMAL ||
        (result.status == SBXID_SOLVE_FEASIBLE && result.timed_out));
    for (int p = 0; p < opt_pigeons; ++p) {
        int sum = feasible[hole_vars + p + 1];
        for (int h = 0; h < opt_holes; ++h) sum += feasible[p * opt_holes + h + 1];
        assert(sum >= 1);
    }
    for (int h = 0; h < opt_holes; ++h) {
        int sum = 0;
        for (int p = 0; p < opt_pigeons; ++p) sum += feasible[p * opt_holes + h + 1];
        assert(sum <= 1);
    }
    result = sbxid_solve_opb(opb, std::strlen(opb), 3, values, 1.0);
    assert(result.status == SBXID_SOLVE_OPTIMAL && values[1] + values[2] == 1);
    return 0;
}

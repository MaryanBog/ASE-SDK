#include <cmath>
#include <iostream>
#include "ase/ase.hpp"

static bool is_admissible(const double& S, const double& dS) {
    if (!std::isfinite(S) || !std::isfinite(dS)) return false;
    constexpr double LIMIT = 1.0;
    const double next = S + dS;
    if (!std::isfinite(next)) return false;
    return std::fabs(next) <= LIMIT;
}

static double neutral_step() { return 0.0; }

// “Projection” rule: clamp the resulting next state into [-LIMIT, +LIMIT]
// then convert back to a step.
static bool project_step(const double& S, const double& dS, double& out) {
    if (!std::isfinite(S) || !std::isfinite(dS)) return false;
    constexpr double LIMIT = 1.0;

    double next = S + dS;
    if (!std::isfinite(next)) return false;

    if (next > LIMIT) next = LIMIT;
    if (next < -LIMIT) next = -LIMIT;

    out = next - S;
    return std::isfinite(out);
}

int main() {
    ase::Config cfg;
    cfg.mode = ase::Mode::Project;

    ase::Dependencies<double,double> deps;
    deps.is_admissible = &is_admissible;
    deps.neutral_step  = &neutral_step;
    deps.project_step  = &project_step;

    ase::Engine<double,double> engine(cfg, deps);

    double S = 0.9;
    double dS = 0.5; // inadmissible (would go to 1.4)

    double eff = engine.enforce(S, dS);
    double S_next = S + eff;

    std::cout << "S=" << S
              << " proposed=" << dS
              << " effective=" << eff
              << " next=" << S_next
              << "\n";

    return 0;
}

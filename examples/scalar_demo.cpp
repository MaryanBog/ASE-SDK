#include <cmath>
#include <iostream>
#include "ase/ase.hpp"

// Domain: State = double, Step = double
// apply_step(S, dS) = S + dS  (host responsibility, not ASE)

static bool is_admissible(const double& S, const double& dS) {
    // Fail-closed numeric policy SHOULD treat NaN/Inf as inadmissible (Spec §8.2)
    if (!std::isfinite(S) || !std::isfinite(dS)) return false;

    constexpr double LIMIT = 1.0;
    const double next = S + dS;
    if (!std::isfinite(next)) return false;

    return std::fabs(next) <= LIMIT;
}

static double neutral_step() {
    return 0.0; // no-op for additive model
}

static bool scale_step(const double& in, double k, double& out) {
    if (!std::isfinite(in) || !std::isfinite(k)) return false;
    out = in * k;
    return std::isfinite(out);
}

int main() {
    ase::Config cfg;
    cfg.mode = ase::Mode::Scale;
    cfg.max_scale_attempts = 16;
    cfg.scale_factor = 0.5;

    ase::Dependencies<double, double> deps;
    deps.is_admissible = &is_admissible;
    deps.neutral_step  = &neutral_step;
    deps.scale_step    = &scale_step;

    ase::Engine<double, double> engine(cfg, deps);

    double S = 0.9;
    double dS = 0.5; // inadmissible if applied directly (0.9 + 0.5 = 1.4 > 1.0)

    const double dS_eff = engine.enforce(S, dS);

    // Host applies effective step (ASE does NOT update state)
    const double S_next = S + dS_eff;

    std::cout << "S=" << S
              << " proposed=" << dS
              << " effective=" << dS_eff
              << " next=" << S_next
              << "\n";

    return 0;
}

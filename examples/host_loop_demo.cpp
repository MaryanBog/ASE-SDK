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

static bool scale_step(const double& in, double k, double& out) {
    if (!std::isfinite(in) || !std::isfinite(k)) return false;
    out = in * k;
    return std::isfinite(out);
}

static double apply_step(const double& S, const double& dS_eff) {
    return S + dS_eff; // host-defined ⊕
}

int main() {
    ase::Config cfg;
    cfg.mode = ase::Mode::Scale;
    cfg.max_scale_attempts = 16;
    cfg.scale_factor = 0.5;

    ase::Dependencies<double,double> deps;
    deps.is_admissible = &is_admissible;
    deps.neutral_step  = &neutral_step;
    deps.scale_step    = &scale_step;

    ase::Engine<double,double> engine(cfg, deps);

    double S = 0.9;
    double proposed = 0.5;

    // Canonical integration form:
    // S_{t+1} = S_t ⊕ ASE(S_t, ΔS)
    const double effective = engine.enforce(S, proposed);
    const double S_next = apply_step(S, effective);

    std::cout << "S=" << S
              << " proposed=" << proposed
              << " effective=" << effective
              << " next=" << S_next
              << "\n";
    return 0;
}

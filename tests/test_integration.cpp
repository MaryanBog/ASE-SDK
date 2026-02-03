// tests/test_integration.cpp
#include <cmath>
#include <cstdlib> // std::abort

#include "ase/ase.hpp"

static bool is_finite(double x) { return std::isfinite(x); }

static int g_eval_calls = 0;

// Admissibility predicate with a call counter
static bool admissible_counted(const double& S, const double& dS) {
    ++g_eval_calls;

    if (!is_finite(S) || !is_finite(dS)) return false;
    constexpr double LIMIT = 1.0;
    const double next = S + dS;
    if (!is_finite(next)) return false;
    return std::fabs(next) <= LIMIT;
}

static double neutral_zero() { return 0.0; }

static bool scale_mul(const double& in, double k, double& out) {
    if (!is_finite(in) || !is_finite(k)) return false;
    out = in * k;
    return is_finite(out);
}

// Host-defined state update operator ⊕
static double apply_step(const double& S, const double& dS_eff) {
    return S + dS_eff;
}

// Always-on check (works in Release; unlike assert it is NOT compiled out)
static void REQUIRE(bool cond) {
    if (!cond) std::abort();
}

static void test_host_single_call_and_apply_exact_output() {
    ase::Config cfg;
    cfg.mode = ase::Mode::Scale;
    cfg.max_scale_attempts = 16;
    cfg.scale_factor = 0.5;

    ase::Dependencies<double, double> deps;
    deps.is_admissible = &admissible_counted;
    deps.neutral_step  = &neutral_zero;
    deps.scale_step    = &scale_mul;

    ase::Engine<double, double> engine(cfg, deps);

    g_eval_calls = 0;

    const double S = 0.9;
    const double proposed = 0.5;

    const double eff   = engine.enforce(S, proposed);
    const double S_next = apply_step(S, eff);

    // Must apply exactly returned step (no override)
    REQUIRE(std::fabs(S_next - (S + eff)) < 1e-12);

    // Output must be admissible or neutral (fail-closed)
    // NOTE: this extra call increments g_eval_calls by +1.
    REQUIRE(admissible_counted(S, eff) || (eff == neutral_zero()));

    // Boundedness of admissibility evaluations:
    // enforce() <= (1 + max_scale_attempts) checks,
    // plus +1 from the REQUIRE(admissible_counted...) above.
    const int max_expected = 1 + static_cast<int>(cfg.max_scale_attempts) + 1;
    REQUIRE(g_eval_calls > 0);
    REQUIRE(g_eval_calls <= max_expected);
}

int main() {
    test_host_single_call_and_apply_exact_output();
    return 0;
}

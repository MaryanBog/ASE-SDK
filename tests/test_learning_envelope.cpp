// ============================================================================
// tests/test_learning_envelope.cpp
// Proves (for this toy learning-loop) that ASE enforcement maintains envelope:
// - boundedness: θ stays in ||θ||2 <= R
// - sign-preservation on subset
// - no NaN/Inf in θ
// - repeatability: same seed + same config => identical final state
//
// IMPORTANT: This test is about envelope safety, not "optimizer quality".
// ============================================================================

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>

#include "ase/ase.hpp"

namespace {

constexpr std::size_t N = 128;

using Vec   = std::array<double, N>;
using State = Vec;
using Step  = Vec;

inline bool is_finite(double x) { return std::isfinite(x) != 0; }

inline Step neutral_step() {
    Step z{};
    z.fill(0.0);
    return z;
}

inline Vec add(const Vec& a, const Vec& b) {
    Vec r{};
    for (std::size_t i = 0; i < N; ++i) r[i] = a[i] + b[i];
    return r;
}

inline Vec sub(const Vec& a, const Vec& b) {
    Vec r{};
    for (std::size_t i = 0; i < N; ++i) r[i] = a[i] - b[i];
    return r;
}

inline double l2_norm(const Vec& v) {
    long double s = 0.0L;
    for (double x : v) s += (long double)x * (long double)x;
    return std::sqrt((double)s);
}

inline bool scale_step(const Step& in, double k, Step& out) {
    if (!is_finite(k)) return false;
    for (std::size_t i = 0; i < N; ++i) {
        const double v = in[i] * k;
        if (!is_finite(v)) return false;
        out[i] = v;
    }
    return true;
}

// ----------------------------
// Envelope
// ----------------------------
struct Envelope {
    double R = 5.0;
    std::size_t sign_preserve_k = 8;
    double eps = 1e-12;
};

static Envelope g_env;

inline bool is_state_valid(const State& theta) {
    for (double x : theta) if (!is_finite(x)) return false;

    const double n = l2_norm(theta);
    if (!is_finite(n)) return false;
    if (n > g_env.R + g_env.eps) return false;

    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (theta[i] < -g_env.eps) return false;
    }
    return true;
}

inline void clamp_state_into_envelope(State& theta) {
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (!is_finite(theta[i])) theta[i] = 0.0;
        if (theta[i] < 0.0) theta[i] = 0.0;
    }
    for (double& x : theta) if (!is_finite(x)) x = 0.0;

    const double n = l2_norm(theta);
    if (!is_finite(n)) { theta.fill(0.0); return; }

    if (n > g_env.R && n > 0.0) {
        const double k = g_env.R / n;
        for (double& x : theta) x *= k;
    }
}

bool is_admissible(const State& S, const Step& dS) {
    if (!is_state_valid(S)) return false;
    for (double x : dS) if (!is_finite(x)) return false;
    const State next = add(S, dS);
    return is_state_valid(next);
}

bool project_step(const State& S, const Step& in, Step& out) {
    if (!is_state_valid(S)) return false;
    for (double x : in) if (!is_finite(x)) return false;

    State next = add(S, in);

    // sign clamp
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (!is_finite(next[i])) return false;
        if (next[i] < 0.0) next[i] = 0.0;
    }

    // finiteness check
    for (double x : next) if (!is_finite(x)) return false;

    // L2 projection
    const double n = l2_norm(next);
    if (!is_finite(n)) return false;

    if (n > g_env.R && n > 0.0) {
        const double k = g_env.R / n;
        for (double& x : next) x *= k;
    }

    out = sub(next, S);
    return true;
}

// proposer (same as demo) — deterministic seed => repeatable
Step propose_step(const State& theta, std::mt19937& rng, double eta) {
    std::normal_distribution<double> noise(0.0, 0.02);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    Step d{};
    for (std::size_t i = 0; i < N; ++i) {
        d[i] = (-eta * theta[i]) + noise(rng);
    }

    if (uni(rng) < 0.02) {
        for (double& x : d) x *= 50.0;
    }

    if (uni(rng) < 0.005) d[0] = std::numeric_limits<double>::infinity();
    if (uni(rng) < 0.005) d[1] = std::numeric_limits<double>::quiet_NaN();

    return d;
}

State run_enforced(ase::Mode mode) {
    // envelope
    g_env.R = 5.0;
    g_env.sign_preserve_k = 8;
    g_env.eps = 1e-12;

    // start inside domain
    State theta{};
    for (std::size_t i = 0; i < N; ++i) theta[i] = 0.20;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) theta[i] = 0.15;
    clamp_state_into_envelope(theta);

    ase::Config cfg;
    cfg.mode = mode;
    cfg.max_scale_attempts = 16;
    cfg.scale_factor = 0.5;

    ase::Dependencies<State, Step> deps;
    deps.is_admissible = &is_admissible;
    deps.neutral_step  = &neutral_step;
    deps.scale_step    = &scale_step;
    deps.project_step  = &project_step;

    ase::Engine<State, Step> engine(cfg, deps);

    std::mt19937 rng(12345);
    const std::size_t T = 2000;
    const double eta = 0.02;

    // Safety invariant: θ must remain valid under enforcement
    if (!is_state_valid(theta)) {
        // should not happen
        return State{}; // will fail comparisons later if used
    }

    for (std::size_t t = 0; t < T; ++t) {
        const Step proposed = propose_step(theta, rng, eta);
        const Step eff = engine.enforce(theta, proposed);

        // enforced step must be admissible at current state
        if (!is_admissible(theta, eff)) {
            // return a sentinel invalid state (test will fail deterministically)
            State bad{};
            bad.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }

        theta = add(theta, eff);

        if (!is_state_valid(theta)) {
            State bad{};
            bad.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }
    }

    return theta;
}

} // namespace

int main() {
    // Repeatability (within same build/process): run twice and require exact match
    {
        const State a1 = run_enforced(ase::Mode::Scale);
        const State a2 = run_enforced(ase::Mode::Scale);

        for (std::size_t i = 0; i < N; ++i) {
            if (a1[i] != a2[i]) return 1;
            if (!is_finite(a1[i])) return 2;
        }
        if (!is_state_valid(a1)) return 3;
    }

    {
        const State p1 = run_enforced(ase::Mode::Project);
        const State p2 = run_enforced(ase::Mode::Project);

        for (std::size_t i = 0; i < N; ++i) {
            if (p1[i] != p2[i]) return 4;
            if (!is_finite(p1[i])) return 5;
        }
        if (!is_state_valid(p1)) return 6;
    }

    return 0;
}

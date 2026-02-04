// ============================================================================
// internal/learning_loop_demo.cpp
// INTERNAL ONLY: learning-loop envelope demonstration for ASE-SDK
//
// Demonstrates: "ASE is not an optimizer" — it gates any proposed update ΔS,
// producing ΔS_eff that keeps θ_next in an admissible domain.
// ============================================================================

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>

#include "ase/ase.hpp"

namespace {

constexpr std::size_t N = 128;

using Vec   = std::array<double, N>;
using State = Vec; // training state: θ
using Step  = Vec; // proposed update: Δθ

// ----------------------------
// Utilities
// ----------------------------
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

// Deterministic scaling hook for Scale mode
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
// Envelope definition
// ----------------------------
// Domain constraints on θ_next:
//
// 1) all finite
// 2) ||θ_next||_2 <= R
// 3) sign preservation on first K coordinates: θ_next[i] >= 0
//
struct Envelope {
    double R = 5.0;
    std::size_t sign_preserve_k = 8;
    double eps = 1e-12;
};

static Envelope g_env;

// Validity of a state (θ itself), independent of step
inline bool is_state_valid(const State& theta) {
    for (double x : theta) {
        if (!is_finite(x)) return false;
    }
    const double n = l2_norm(theta);
    if (!is_finite(n)) return false;
    if (n > g_env.R + g_env.eps) return false;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (theta[i] < -g_env.eps) return false;
    }
    return true;
}

// Clamp θ into domain (used for initialization / robustness)
inline void clamp_state_into_envelope(State& theta) {
    // Sign clamp (first K)
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (!is_finite(theta[i])) theta[i] = 0.0;
        if (theta[i] < 0.0) theta[i] = 0.0;
    }

    // If any non-finite elsewhere => zero out
    for (double& x : theta) {
        if (!is_finite(x)) x = 0.0;
    }

    // L2 projection into ball
    const double n = l2_norm(theta);
    if (!is_finite(n)) {
        theta.fill(0.0);
        return;
    }
    if (n > g_env.R && n > 0.0) {
        const double k = g_env.R / n;
        for (double& x : theta) x *= k;
    }
}

// Admissibility predicate: checks θ_next = θ + Δθ
bool is_admissible(const State& S, const Step& dS) {
    // If current state invalid, fail-closed
    if (!is_state_valid(S)) return false;

    // Step must be finite
    for (double x : dS) if (!is_finite(x)) return false;

    const State next = add(S, dS);
    return is_state_valid(next);
}

// Project-mode hook:
// deterministically maps proposed step to a projected θ_next inside domain,
// then returns Δθ_eff = θ_proj - θ.
bool project_step(const State& S, const Step& in, Step& out) {
    // Fail-closed if S invalid or input not finite
    if (!is_state_valid(S)) return false;
    for (double x : in) if (!is_finite(x)) return false;

    State next = add(S, in);

    // enforce sign on first K
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (!is_finite(next[i])) return false;
        if (next[i] < 0.0) next[i] = 0.0;
    }

    // enforce finiteness for all coords
    for (double& x : next) {
        if (!is_finite(x)) return false;
    }

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

// ----------------------------
// "Pseudo-learning" proposer
// ----------------------------
// loss(θ) = 0.5 * ||θ - θ*||^2 with θ* = 0 => grad = θ
inline double loss(const State& theta) {
    const double n = l2_norm(theta);
    return 0.5 * n * n;
}

Step propose_step(const State& theta, std::mt19937& rng, double eta) {
    std::normal_distribution<double> noise(0.0, 0.02);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    Step d{};
    // SGD-like step: -eta * grad + noise
    for (std::size_t i = 0; i < N; ++i) {
        d[i] = (-eta * theta[i]) + noise(rng);
    }

    // rare exploding spike: multiply update by 50
    if (uni(rng) < 0.02) {
        for (double& x : d) x *= 50.0;
    }

    // rare numeric fault injection
    if (uni(rng) < 0.005) d[0] = std::numeric_limits<double>::infinity();
    if (uni(rng) < 0.005) d[1] = std::numeric_limits<double>::quiet_NaN();

    return d;
}

// ----------------------------
// Run + stats
// ----------------------------
struct RunStats {
    std::size_t steps = 0;
    std::size_t nan_inf_proposed = 0;
    std::size_t inadmissible_proposed = 0;
    std::size_t neutral_emitted = 0;

    double final_norm = 0.0;
    double final_loss = 0.0;
};

RunStats run_loop(const std::string& label,
                  State theta0,
                  std::size_t T,
                  double eta,
                  bool use_ase,
                  ase::Mode mode)
{
    // Ensure valid start (critical)
    clamp_state_into_envelope(theta0);

    std::mt19937 rng(12345); // fixed seed => deterministic run
    State theta = theta0;

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

    RunStats st{};
    st.steps = T;

    for (std::size_t t = 0; t < T; ++t) {
        Step proposed = propose_step(theta, rng, eta);

        bool proposed_finite = true;
        for (double x : proposed) {
            if (!is_finite(x)) { proposed_finite = false; break; }
        }
        if (!proposed_finite) st.nan_inf_proposed++;

        if (!is_admissible(theta, proposed)) st.inadmissible_proposed++;

        Step eff = proposed;
        if (use_ase) {
            eff = engine.enforce(theta, proposed);

            // neutral detection
            bool is_neutral = true;
            for (double x : eff) {
                if (x != 0.0) { is_neutral = false; break; }
            }
            if (is_neutral) st.neutral_emitted++;
        }

        theta = add(theta, eff);

        // Keep demo invariant: state remains valid under ASE modes (should always hold)
        // For NO_ASE, we allow it to explode (that’s the point).
        if (use_ase) {
            if (!is_state_valid(theta)) {
                std::cerr << "ERROR: state became invalid under ASE at step " << t << "\n";
                break;
            }
        }
    }

    st.final_norm = l2_norm(theta);
    st.final_loss = loss(theta);

    std::cout << std::left << std::setw(16) << label
              << " nan/inf(proposed)=" << std::setw(5) << st.nan_inf_proposed
              << " inadmiss(proposed)=" << std::setw(5) << st.inadmissible_proposed
              << " neutral=" << std::setw(5) << st.neutral_emitted
              << " final||theta||=" << std::setw(10) << std::fixed << std::setprecision(6) << st.final_norm
              << " final_loss=" << std::setw(10) << std::fixed << std::setprecision(6) << st.final_loss
              << "\n";

    return st;
}

} // namespace

int main() {
    // Envelope configuration
    g_env.R = 5.0;
    g_env.sign_preserve_k = 8;
    g_env.eps = 1e-12;

    // Initialize θ0 safely INSIDE the ball (then clamp again anyway).
    State theta0{};
    theta0.fill(0.0);

    // Make a deterministic non-trivial start:
    // put small positive mass in all dims so ||θ||2 is well under R.
    // Example: 0.20 on 128 dims => norm ≈ 2.262 < 5
    for (std::size_t i = 0; i < N; ++i) theta0[i] = 0.20;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) theta0[i] = 0.15;

    clamp_state_into_envelope(theta0);

    const std::size_t T = 2000;
    const double eta = 0.02;

    std::cout << "ASE learning-loop envelope demo (internal)\n";
    std::cout << "Domain: ||theta_next||_2 <= " << g_env.R
              << ", sign(theta_next[i])>=0 for i<" << g_env.sign_preserve_k << "\n\n";

    run_loop("NO_ASE",      theta0, T, eta, false, ase::Mode::Reject);
    run_loop("ASE_SCALE",   theta0, T, eta, true,  ase::Mode::Scale);
    run_loop("ASE_PROJECT", theta0, T, eta, true,  ase::Mode::Project);

    std::cout << "\nInterpretation:\n"
              << "- NO_ASE: may drift outside bounds or hit NaN/Inf.\n"
              << "- ASE_* : θ stays inside the admissible domain; non-finite proposals -> neutral.\n";
    return 0;
}

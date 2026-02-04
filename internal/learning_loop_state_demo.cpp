// ============================================================================
// internal/learning_loop_state_demo.cpp
// INTERNAL ONLY: learning-loop demo with extended state:
//   State = { theta, ema, step_counter }
//
// Shows: ASE constrains NEXT STATE (theta_next, ema_next, step_next),
// not just "clip grad". One gate call per proposed optimizer step.
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

constexpr std::size_t N = 256;

using Vec = std::array<double, N>;

struct State {
    Vec theta{};
    Vec ema{};
    std::uint64_t step = 0;
};

using Step = Vec; // proposed update is only dtheta (optimizer proposal)

// ----------------------------
// Utilities
// ----------------------------
inline bool is_finite(double x) { return std::isfinite(x) != 0; }

inline Vec vec_zero() { Vec z{}; z.fill(0.0); return z; }

inline Vec vec_add(const Vec& a, const Vec& b) {
    Vec r{};
    for (std::size_t i = 0; i < N; ++i) r[i] = a[i] + b[i];
    return r;
}

inline Vec vec_sub(const Vec& a, const Vec& b) {
    Vec r{};
    for (std::size_t i = 0; i < N; ++i) r[i] = a[i] - b[i];
    return r;
}

inline Vec vec_mul(const Vec& a, double k) {
    Vec r{};
    for (std::size_t i = 0; i < N; ++i) r[i] = a[i] * k;
    return r;
}

inline double l2_norm(const Vec& v) {
    long double s = 0.0L;
    for (double x : v) s += (long double)x * (long double)x;
    return std::sqrt((double)s);
}

inline double l2_norm_diff(const Vec& a, const Vec& b) {
    long double s = 0.0L;
    for (std::size_t i = 0; i < N; ++i) {
        const long double d = (long double)a[i] - (long double)b[i];
        s += d * d;
    }
    return std::sqrt((double)s);
}

inline Step neutral_step() { return vec_zero(); }

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
// Envelope for extended state
// ----------------------------
// Constraints are on *next state*, derived deterministically from (S, dtheta):
//
// theta_next = theta + dtheta
// ema_next   = beta*ema + (1-beta)*theta_next
// step_next  = step + 1
//
// Domain D:
//  1) all finite
//  2) ||theta_next||2 <= R_theta
//  3) ||ema_next||2   <= R_ema
//  4) ||ema_next - theta_next||2 <= R_gap
//  5) sign(theta_next[i]) >= 0 for i < K
//  6) step_next == step + 1 (monotone exact)
//
struct Envelope {
    double R_theta = 5.0;
    double R_ema   = 5.0;
    double R_gap   = 2.0;
    std::size_t sign_preserve_k = 8;
    double beta = 0.98;
    double eps = 1e-12;
};

static Envelope g_env;

// Validate a "materialized" state against envelope (no step transition check here)
inline bool is_state_valid_materialized(const State& s) {
    for (double x : s.theta) if (!is_finite(x)) return false;
    for (double x : s.ema)   if (!is_finite(x)) return false;

    const double nt = l2_norm(s.theta);
    const double ne = l2_norm(s.ema);
    if (!is_finite(nt) || !is_finite(ne)) return false;

    if (nt > g_env.R_theta + g_env.eps) return false;
    if (ne > g_env.R_ema   + g_env.eps) return false;

    const double gap = l2_norm_diff(s.ema, s.theta);
    if (!is_finite(gap)) return false;
    if (gap > g_env.R_gap + g_env.eps) return false;

    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (s.theta[i] < -g_env.eps) return false;
    }
    return true;
}

inline void clamp_state_into_envelope(State& s) {
    // Finite + sign for theta
    for (double& x : s.theta) if (!is_finite(x)) x = 0.0;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (s.theta[i] < 0.0) s.theta[i] = 0.0;
    }

    // Keep theta in ball
    const double nt = l2_norm(s.theta);
    if (!is_finite(nt)) {
        s.theta = vec_zero();
    } else if (nt > g_env.R_theta && nt > 0.0) {
        s.theta = vec_mul(s.theta, g_env.R_theta / nt);
    }

    // Put EMA close to theta and within bounds
    s.ema = s.theta;

    // Ensure ema bound (redundant if ema==theta, but safe)
    const double ne = l2_norm(s.ema);
    if (!is_finite(ne)) s.ema = vec_zero();
    else if (ne > g_env.R_ema && ne > 0.0) s.ema = vec_mul(s.ema, g_env.R_ema / ne);

    // Ensure gap bound (ema==theta => gap=0)
    if (!is_state_valid_materialized(s)) {
        // fail-closed fallback
        s.theta = vec_zero();
        s.ema   = vec_zero();
    }
}

// Deterministically derive next state from (S, dtheta)
inline State derive_next(const State& S, const Step& dtheta) {
    State next = S;

    next.theta = vec_add(S.theta, dtheta);

    // EMA update uses theta_next (not theta)
    const double b = g_env.beta;
    for (std::size_t i = 0; i < N; ++i) {
        next.ema[i] = b * S.ema[i] + (1.0 - b) * next.theta[i];
    }

    next.step = S.step + 1;
    return next;
}

// ASE admissibility: checks next state, and step monotonicity exactness
bool is_admissible(const State& S, const Step& dtheta) {
    // current state must itself be valid
    if (!is_state_valid_materialized(S)) return false;

    // proposed step must be finite
    for (double x : dtheta) if (!is_finite(x)) return false;

    const State next = derive_next(S, dtheta);

    // step transition must be exact
    if (next.step != S.step + 1) return false;

    return is_state_valid_materialized(next);
}

// Project-mode hook: project theta_next into constraints and return dtheta_eff.
// Note: EMA/step are not directly controlled by Step; they are derived.
// So we project theta_next such that derived ema_next also satisfies envelope.
bool project_step(const State& S, const Step& in, Step& out) {
    if (!is_state_valid_materialized(S)) return false;
    for (double x : in) if (!is_finite(x)) return false;

    // Start from candidate theta_next
    Vec theta_next = vec_add(S.theta, in);

    // Enforce sign on theta_next
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (!is_finite(theta_next[i])) return false;
        if (theta_next[i] < 0.0) theta_next[i] = 0.0;
    }
    for (double x : theta_next) if (!is_finite(x)) return false;

    // Project theta_next into theta-ball
    {
        const double nt = l2_norm(theta_next);
        if (!is_finite(nt)) return false;
        if (nt > g_env.R_theta && nt > 0.0) {
            theta_next = vec_mul(theta_next, g_env.R_theta / nt);
        }
    }

    // Now check derived EMA constraints; if violated, pull theta_next toward current EMA
    // (deterministic "gap repair" step)
    //
    // We do a bounded single repair: mix theta_next with S.ema to satisfy gap bound.
    // This keeps deterministic and bounded semantics.
    {
        // compute derived ema_next
        Vec ema_next{};
        const double b = g_env.beta;
        for (std::size_t i = 0; i < N; ++i) {
            ema_next[i] = b * S.ema[i] + (1.0 - b) * theta_next[i];
        }

        double gap = l2_norm_diff(ema_next, theta_next);
        if (!is_finite(gap)) return false;

        if (gap > g_env.R_gap && gap > 0.0) {
            // Pull theta_next toward ema_next (or toward S.ema) by a scalar mix
            // theta_next' = alpha*theta_next + (1-alpha)*S.ema
            // Choose alpha in [0,1] to reduce gap deterministically.
            //
            // Use conservative alpha = R_gap / gap (clamped).
            double alpha = g_env.R_gap / gap;
            if (!is_finite(alpha)) return false;
            if (alpha < 0.0) alpha = 0.0;
            if (alpha > 1.0) alpha = 1.0;

            for (std::size_t i = 0; i < N; ++i) {
                theta_next[i] = alpha * theta_next[i] + (1.0 - alpha) * S.ema[i];
            }

            // Re-enforce sign + theta ball after mix
            for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
                if (theta_next[i] < 0.0) theta_next[i] = 0.0;
            }
            const double nt2 = l2_norm(theta_next);
            if (!is_finite(nt2)) return false;
            if (nt2 > g_env.R_theta && nt2 > 0.0) {
                theta_next = vec_mul(theta_next, g_env.R_theta / nt2);
            }
        }
    }

    out = vec_sub(theta_next, S.theta);

    // Safety: ensure final is admissible (derived ema_next, step_next)
    if (!is_admissible(S, out)) {
        return false;
    }

    return true;
}

// ----------------------------
// Proposed optimizer update (dtheta)
// ----------------------------
// "Pseudo-learning": grad = theta (target=0), step = -eta*grad + noise
// plus rare spikes and rare NaN/Inf injection.
Step propose_dtheta(const State& s, std::mt19937& rng, double eta) {
    std::normal_distribution<double> noise(0.0, 0.02);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    Step d{};
    for (std::size_t i = 0; i < N; ++i) {
        d[i] = (-eta * s.theta[i]) + noise(rng);
    }

    // spike
    if (uni(rng) < 0.02) {
        for (double& x : d) x *= 50.0;
    }

    // numeric fault injection
    if (uni(rng) < 0.005) d[0] = std::numeric_limits<double>::infinity();
    if (uni(rng) < 0.005) d[1] = std::numeric_limits<double>::quiet_NaN();

    return d;
}

inline double loss_theta_only(const State& s) {
    const double n = l2_norm(s.theta);
    return 0.5 * n * n;
}

struct RunStats {
    std::size_t steps = 0;
    std::size_t nan_inf_proposed = 0;
    std::size_t inadmissible_proposed = 0;
    std::size_t neutral_emitted = 0;

    double final_theta_norm = 0.0;
    double final_ema_norm   = 0.0;
    double final_gap_norm   = 0.0;
    double final_loss       = 0.0;
};

RunStats run_loop(const std::string& label,
                  State s0,
                  std::size_t T,
                  double eta,
                  bool use_ase,
                  ase::Mode mode)
{
    clamp_state_into_envelope(s0);

    std::mt19937 rng(12345);
    State s = s0;

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
        const Step proposed = propose_dtheta(s, rng, eta);

        bool finite = true;
        for (double x : proposed) { if (!is_finite(x)) { finite = false; break; } }
        if (!finite) st.nan_inf_proposed++;

        if (!is_admissible(s, proposed)) st.inadmissible_proposed++;

        Step eff = proposed;
        if (use_ase) {
            eff = engine.enforce(s, proposed);
            bool is_neutral = true;
            for (double x : eff) { if (x != 0.0) { is_neutral = false; break; } }
            if (is_neutral) st.neutral_emitted++;
        }

        // Apply: entire state transitions deterministically
        s = derive_next(s, eff);

        if (use_ase && !is_state_valid_materialized(s)) {
            std::cerr << "ERROR: invalid state under ASE at step " << t << "\n";
            break;
        }
    }

    st.final_theta_norm = l2_norm(s.theta);
    st.final_ema_norm   = l2_norm(s.ema);
    st.final_gap_norm   = l2_norm_diff(s.ema, s.theta);
    st.final_loss       = loss_theta_only(s);

    std::cout << std::left << std::setw(18) << label
              << " nan/inf(prop)=" << std::setw(5) << st.nan_inf_proposed
              << " inadmiss(prop)=" << std::setw(5) << st.inadmissible_proposed
              << " neutral=" << std::setw(5) << st.neutral_emitted
              << " ||theta||=" << std::setw(10) << std::fixed << std::setprecision(6) << st.final_theta_norm
              << " ||ema||="   << std::setw(10) << std::fixed << std::setprecision(6) << st.final_ema_norm
              << " gap="       << std::setw(10) << std::fixed << std::setprecision(6) << st.final_gap_norm
              << " loss="      << std::setw(10) << std::fixed << std::setprecision(6) << st.final_loss
              << "\n";

    return st;
}

} // namespace

int main() {
    // Envelope configuration
    g_env.R_theta = 5.0;
    g_env.R_ema   = 5.0;
    g_env.R_gap   = 2.0;
    g_env.sign_preserve_k = 8;
    g_env.beta = 0.98;
    g_env.eps  = 1e-12;

    // Start safely inside theta/ema bounds
    State s0{};
    s0.step = 0;

    // Put small mass in theta so ||theta|| ~ 2-3
    for (std::size_t i = 0; i < N; ++i) s0.theta[i] = 0.18;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) s0.theta[i] = 0.12;

    // EMA starts equal to theta (typical)
    s0.ema = s0.theta;

    clamp_state_into_envelope(s0);

    const std::size_t T = 2000;
    const double eta = 0.02;

    std::cout << "ASE learning-loop STATE envelope demo (internal)\n";
    std::cout << "State = {theta, ema, step}\n";
    std::cout << "Domain:\n"
              << "  ||theta_next||2 <= " << g_env.R_theta << "\n"
              << "  ||ema_next||2   <= " << g_env.R_ema   << "\n"
              << "  ||ema_next - theta_next||2 <= " << g_env.R_gap << "\n"
              << "  sign(theta_next[i])>=0 for i<" << g_env.sign_preserve_k << "\n"
              << "  step_next == step + 1\n\n";

    run_loop("NO_ASE",       s0, T, eta, false, ase::Mode::Reject);
    run_loop("ASE_SCALE",    s0, T, eta, true,  ase::Mode::Scale);
    run_loop("ASE_PROJECT",  s0, T, eta, true,  ase::Mode::Project);

    std::cout << "\nInterpretation:\n"
              << "- NO_ASE: may corrupt state via NaN/Inf in proposed dtheta.\n"
              << "- ASE_* : keeps derived next-state (theta_next, ema_next, step_next) inside envelope.\n";
    return 0;
}

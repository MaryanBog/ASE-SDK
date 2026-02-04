// ============================================================================
// internal/learning_loop_adam_state_demo.cpp
// INTERNAL ONLY: "killer" demo — optimizer STATE is part of the training system,
// but ASE gates a PROPOSED PARAMETER STEP (dtheta), not a raw gradient.
//
//   State = { theta, m, v, ema, step }
//   Proposal = dtheta_proposed  (what any optimizer would propose to add to theta)
//   ASE returns dtheta_eff s.t. NEXT STATE remains admissible.
//
// Transition (derived, deterministic from (S, dtheta_eff)):
//   theta_next = theta + dtheta_eff
//   m_next = b1*m + (1-b1)*dtheta_eff
//   v_next = b2*v + (1-b2)*(dtheta_eff^2)
//   ema_next = beta*ema + (1-beta)*theta_next
//   step_next = step + 1
//
// This makes "neutral_step == 0" a true fail-closed neutral update.
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

using Vec  = std::array<double, N>;
using Step = Vec; // proposed parameter step dtheta

struct State {
    Vec theta{};
    Vec m{};
    Vec v{};
    Vec ema{};
    std::uint64_t step = 0;
};

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

inline bool vec_all_finite(const Vec& v) {
    for (double x : v) if (!is_finite(x)) return false;
    return true;
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
// Envelope + dynamics
// ----------------------------
struct Envelope {
    // bounds on next state
    double R_theta = 5.0;
    double R_m     = 10.0;
    double R_v     = 10.0;
    double R_ema   = 5.0;
    double R_gap   = 2.0;  // ||ema_next - theta_next||2
    double R_dinf  = 0.25; // optional: ||dtheta_eff||_inf <= R_dinf

    std::size_t sign_preserve_k = 8;

    // "optimizer state" filters over the step (diagnostic inertia)
    double b1  = 0.9;
    double b2  = 0.999;

    // EMA
    double beta = 0.98;

    // numeric tolerance
    double tol = 1e-12;
};

static Envelope g_env;

inline bool is_state_valid_materialized(const State& s) {
    if (!vec_all_finite(s.theta)) return false;
    if (!vec_all_finite(s.m))     return false;
    if (!vec_all_finite(s.v))     return false;
    if (!vec_all_finite(s.ema))   return false;

    const double nt = l2_norm(s.theta);
    const double nm = l2_norm(s.m);
    const double nv = l2_norm(s.v);
    const double ne = l2_norm(s.ema);
    if (!is_finite(nt) || !is_finite(nm) || !is_finite(nv) || !is_finite(ne)) return false;

    if (nt > g_env.R_theta + g_env.tol) return false;
    if (nm > g_env.R_m     + g_env.tol) return false;
    if (nv > g_env.R_v     + g_env.tol) return false;
    if (ne > g_env.R_ema   + g_env.tol) return false;

    const double gap = l2_norm_diff(s.ema, s.theta);
    if (!is_finite(gap)) return false;
    if (gap > g_env.R_gap + g_env.tol) return false;

    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (s.theta[i] < -g_env.tol) return false;
    }

    // v should be >= 0 (since we store squared magnitude-like accumulator)
    for (double x : s.v) {
        if (x < -g_env.tol) return false;
    }

    return true;
}

inline void clamp_state_into_envelope(State& s) {
    for (double& x : s.theta) if (!is_finite(x)) x = 0.0;
    for (double& x : s.m)     if (!is_finite(x)) x = 0.0;
    for (double& x : s.v)     if (!is_finite(x)) x = 0.0;
    for (double& x : s.ema)   if (!is_finite(x)) x = 0.0;

    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (s.theta[i] < 0.0) s.theta[i] = 0.0;
    }

    // theta ball
    const double nt = l2_norm(s.theta);
    if (!is_finite(nt)) s.theta = vec_zero();
    else if (nt > g_env.R_theta && nt > 0.0) s.theta = vec_mul(s.theta, g_env.R_theta / nt);

    // set ema = theta to satisfy gap initially
    s.ema = s.theta;

    // v nonnegative
    for (double& x : s.v) if (x < 0.0) x = 0.0;

    // bound m/v norms
    {
        const double nm = l2_norm(s.m);
        if (!is_finite(nm)) s.m = vec_zero();
        else if (nm > g_env.R_m && nm > 0.0) s.m = vec_mul(s.m, g_env.R_m / nm);
    }
    {
        const double nv = l2_norm(s.v);
        if (!is_finite(nv)) s.v = vec_zero();
        else if (nv > g_env.R_v && nv > 0.0) s.v = vec_mul(s.v, g_env.R_v / nv);
        for (double& x : s.v) if (x < 0.0) x = 0.0;
    }

    if (!is_state_valid_materialized(s)) {
        s.theta = vec_zero();
        s.m     = vec_zero();
        s.v     = vec_zero();
        s.ema   = vec_zero();
        s.step  = 0;
    }
}

// Derive next state from (S, dtheta_eff)
inline State derive_next(const State& S, const Step& dtheta) {
    State next = S;

    next.theta = vec_add(S.theta, dtheta);

    // m/v track step statistics (optimizer state as part of system)
    const double b1 = g_env.b1;
    const double b2 = g_env.b2;

    for (std::size_t i = 0; i < N; ++i) {
        const double dt = dtheta[i];
        next.m[i] = b1 * S.m[i] + (1.0 - b1) * dt;
        next.v[i] = b2 * S.v[i] + (1.0 - b2) * (dt * dt);
        if (next.v[i] < 0.0) next.v[i] = 0.0;
    }

    // EMA over theta_next
    const double beta = g_env.beta;
    for (std::size_t i = 0; i < N; ++i) {
        next.ema[i] = beta * S.ema[i] + (1.0 - beta) * next.theta[i];
    }

    next.step = S.step + 1;
    return next;
}

inline bool step_finite_and_bounded(const Step& dtheta) {
    for (double x : dtheta) {
        if (!is_finite(x)) return false;
        if (std::fabs(x) > g_env.R_dinf + g_env.tol) return false;
    }
    return true;
}

bool is_admissible(const State& S, const Step& dtheta) {
    if (!is_state_valid_materialized(S)) return false;
    if (!step_finite_and_bounded(dtheta)) return false;

    const State next = derive_next(S, dtheta);
    if (next.step != S.step + 1) return false;

    return is_state_valid_materialized(next);
}

// Project step: deterministic scalar search for k in [0,1] such that admissible(S, k*in)
bool project_step(const State& S, const Step& in, Step& out) {
    if (!is_state_valid_materialized(S)) return false;
    // input must be finite (bounds handled by scaling too, but keep strict)
    for (double x : in) if (!is_finite(x)) return false;

    // if in violates per-component bound, treat as needing projection
    if (is_admissible(S, in)) { out = in; return true; }

    const Step z = neutral_step();
    if (!is_admissible(S, z)) return false; // neutral must be admissible

    double lo = 0.0, hi = 1.0;
    for (int it = 0; it < 24; ++it) {
        const double mid = 0.5 * (lo + hi);
        Step cand{};
        if (!scale_step(in, mid, cand)) return false;
        if (is_admissible(S, cand)) lo = mid;
        else hi = mid;
    }

    if (!scale_step(in, lo, out)) return false;
    return is_admissible(S, out);
}

// ----------------------------
// Proposed step generator (dtheta)
// ----------------------------
// "Any optimizer could propose this": a drift-to-zero step + noise,
// plus spikes and NaN/Inf injection to simulate failures.
Step propose_dtheta(const State& s, std::mt19937& rng) {
    std::normal_distribution<double> noise(0.0, 0.02);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    // propose to move theta toward 0: dtheta ~ -eta*theta + noise
    const double eta = 0.05;

    Step d{};
    for (std::size_t i = 0; i < N; ++i) {
        d[i] = (-eta * s.theta[i]) + noise(rng);
    }

    // spikes (exploding update)
    if (uni(rng) < 0.02) {
        for (double& x : d) x *= 50.0;
    }

    // NaN/Inf injection
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

    double th = 0.0, m = 0.0, v = 0.0, ema = 0.0, gap = 0.0, loss = 0.0;
};

RunStats run_loop(const std::string& label,
                  State s0,
                  std::size_t T,
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
        const Step prop = propose_dtheta(s, rng);

        bool finite = true;
        for (double x : prop) { if (!is_finite(x)) { finite = false; break; } }
        if (!finite) st.nan_inf_proposed++;

        if (!is_admissible(s, prop)) st.inadmissible_proposed++;

        Step eff = prop;
        if (use_ase) {
            eff = engine.enforce(s, prop);
            bool is_neutral = true;
            for (double x : eff) { if (x != 0.0) { is_neutral = false; break; } }
            if (is_neutral) st.neutral_emitted++;
        }

        s = derive_next(s, eff);

        if (use_ase && !is_state_valid_materialized(s)) {
            std::cerr << "ERROR: invalid state under ASE at step " << t << "\n";
            break;
        }
    }

    st.th   = l2_norm(s.theta);
    st.m    = l2_norm(s.m);
    st.v    = l2_norm(s.v);
    st.ema  = l2_norm(s.ema);
    st.gap  = l2_norm_diff(s.ema, s.theta);
    st.loss = loss_theta_only(s);

    std::cout << std::left << std::setw(18) << label
              << " nan/inf(prop)=" << std::setw(5) << st.nan_inf_proposed
              << " inadmiss(prop)=" << std::setw(5) << st.inadmissible_proposed
              << " neutral=" << std::setw(5) << st.neutral_emitted
              << " ||th||=" << std::setw(10) << std::fixed << std::setprecision(6) << st.th
              << " ||m||="  << std::setw(10) << std::fixed << std::setprecision(6) << st.m
              << " ||v||="  << std::setw(10) << std::fixed << std::setprecision(6) << st.v
              << " ||ema||="<< std::setw(10) << std::fixed << std::setprecision(6) << st.ema
              << " gap="    << std::setw(10) << std::fixed << std::setprecision(6) << st.gap
              << " loss="   << std::setw(10) << std::fixed << std::setprecision(6) << st.loss
              << "\n";

    return st;
}

} // namespace

int main() {
    g_env.R_theta = 5.0;
    g_env.R_ema   = 5.0;
    g_env.R_gap   = 2.0;
    g_env.R_m     = 10.0;
    g_env.R_v     = 10.0;
    g_env.R_dinf  = 0.25;

    g_env.sign_preserve_k = 8;
    g_env.b1   = 0.9;
    g_env.b2   = 0.999;
    g_env.beta = 0.98;
    g_env.tol  = 1e-12;

    State s0{};
    s0.step = 0;
    for (std::size_t i = 0; i < N; ++i) s0.theta[i] = 0.15;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) s0.theta[i] = 0.10;
    s0.m   = vec_zero();
    s0.v   = vec_zero();
    s0.ema = s0.theta;

    clamp_state_into_envelope(s0);

    const std::size_t T = 2000;

    std::cout << "ASE learning-loop ADAM-STATE envelope demo (internal)\n";
    std::cout << "State = {theta, m, v, ema, step}\n";
    std::cout << "Domain (NEXT STATE):\n"
              << "  ||theta_next||2 <= " << g_env.R_theta << "\n"
              << "  ||ema_next||2   <= " << g_env.R_ema   << "\n"
              << "  ||ema_next - theta_next||2 <= " << g_env.R_gap << "\n"
              << "  ||m_next||2 <= " << g_env.R_m << "\n"
              << "  ||v_next||2 <= " << g_env.R_v << " and v_next>=0\n"
              << "  ||dtheta_eff||_inf <= " << g_env.R_dinf << "\n"
              << "  sign(theta_next[i])>=0 for i<" << g_env.sign_preserve_k << "\n"
              << "  step_next == step + 1\n\n";

    run_loop("NO_ASE",      s0, T, false, ase::Mode::Reject);
    run_loop("ASE_SCALE",   s0, T, true,  ase::Mode::Scale);
    run_loop("ASE_PROJECT", s0, T, true,  ase::Mode::Project);

    std::cout << "\nInterpretation:\n"
              << "- NO_ASE: NaN/Inf and spikes in proposed dtheta corrupt the system.\n"
              << "- ASE_* : gates dtheta so the entire next-state (theta,m,v,ema,step) stays admissible.\n";
    return 0;
}

// ============================================================================
// tests/test_learning_adam_state_envelope.cpp
// Test for Adam-like TRAINING STATE under ASE where the proposal is dtheta.
//
// State = {theta, m, v, ema, step}
// Proposal = dtheta
// Neutral step (all zeros) must be admissible.
//
// Asserts:
// - boundedness of theta/m/v/ema/gap
// - v >= 0
// - sign(theta[i]) >= 0 for i<8
// - step monotonicity exact
// - repeatability: two runs produce identical final state
// ============================================================================

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>

#include "ase/ase.hpp"

namespace {

constexpr std::size_t N = 256;

using Vec  = std::array<double, N>;
using Step = Vec;

struct State {
    Vec theta{};
    Vec m{};
    Vec v{};
    Vec ema{};
    std::uint64_t step = 0;
};

struct Envelope {
    double R_theta = 5.0;
    double R_m     = 10.0;
    double R_v     = 10.0;
    double R_ema   = 5.0;
    double R_gap   = 2.0;
    double R_dinf  = 0.25;

    std::size_t sign_preserve_k = 8;

    double b1   = 0.9;
    double b2   = 0.999;
    double beta = 0.98;

    double tol  = 1e-12;
};

static Envelope g_env;

inline bool is_finite(double x) { return std::isfinite(x) != 0; }

inline Vec vec_zero() { Vec z{}; z.fill(0.0); return z; }

inline Vec vec_add(const Vec& a, const Vec& b) {
    Vec r{};
    for (std::size_t i = 0; i < N; ++i) r[i] = a[i] + b[i];
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

    const double nt = l2_norm(s.theta);
    if (!is_finite(nt)) s.theta = vec_zero();
    else if (nt > g_env.R_theta && nt > 0.0) s.theta = vec_mul(s.theta, g_env.R_theta / nt);

    for (double& x : s.v) if (x < 0.0) x = 0.0;

    s.ema = s.theta;
    if (!is_state_valid_materialized(s)) {
        s.theta = vec_zero();
        s.m     = vec_zero();
        s.v     = vec_zero();
        s.ema   = vec_zero();
        s.step  = 0;
    }
}

inline bool step_finite_and_bounded(const Step& dtheta) {
    for (double x : dtheta) {
        if (!is_finite(x)) return false;
        if (std::fabs(x) > g_env.R_dinf + g_env.tol) return false;
    }
    return true;
}

inline State derive_next(const State& S, const Step& dtheta) {
    State next = S;

    next.theta = vec_add(S.theta, dtheta);

    const double b1 = g_env.b1;
    const double b2 = g_env.b2;
    for (std::size_t i = 0; i < N; ++i) {
        const double dt = dtheta[i];
        next.m[i] = b1 * S.m[i] + (1.0 - b1) * dt;
        next.v[i] = b2 * S.v[i] + (1.0 - b2) * (dt * dt);
        if (next.v[i] < 0.0) next.v[i] = 0.0;
    }

    const double beta = g_env.beta;
    for (std::size_t i = 0; i < N; ++i) {
        next.ema[i] = beta * S.ema[i] + (1.0 - beta) * next.theta[i];
    }

    next.step = S.step + 1;
    return next;
}

bool is_admissible(const State& S, const Step& dtheta) {
    if (!is_state_valid_materialized(S)) return false;
    if (!step_finite_and_bounded(dtheta)) return false;

    const State next = derive_next(S, dtheta);
    if (next.step != S.step + 1) return false;
    return is_state_valid_materialized(next);
}

bool project_step(const State& S, const Step& in, Step& out) {
    if (!is_state_valid_materialized(S)) return false;
    for (double x : in) if (!is_finite(x)) return false;

    if (is_admissible(S, in)) { out = in; return true; }

    const Step z = neutral_step();
    if (!is_admissible(S, z)) return false;

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

Step propose_dtheta(const State& s, std::mt19937& rng) {
    std::normal_distribution<double> noise(0.0, 0.02);
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    const double eta = 0.05;

    Step d{};
    for (std::size_t i = 0; i < N; ++i) d[i] = (-eta * s.theta[i]) + noise(rng);

    if (uni(rng) < 0.02) {
        for (double& x : d) x *= 50.0;
    }

    if (uni(rng) < 0.005) d[0] = std::numeric_limits<double>::infinity();
    if (uni(rng) < 0.005) d[1] = std::numeric_limits<double>::quiet_NaN();

    return d;
}

State run_enforced(ase::Mode mode) {
    g_env = Envelope{};

    State s{};
    s.step = 0;
    for (std::size_t i = 0; i < N; ++i) s.theta[i] = 0.15;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) s.theta[i] = 0.10;
    s.m   = vec_zero();
    s.v   = vec_zero();
    s.ema = s.theta;

    clamp_state_into_envelope(s);

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

    if (!is_state_valid_materialized(s)) {
        State bad{};
        bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
        bad.m.fill(std::numeric_limits<double>::quiet_NaN());
        bad.v.fill(std::numeric_limits<double>::quiet_NaN());
        bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
        return bad;
    }

    // neutral must be admissible
    if (!is_admissible(s, neutral_step())) {
        State bad{};
        bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
        bad.m.fill(std::numeric_limits<double>::quiet_NaN());
        bad.v.fill(std::numeric_limits<double>::quiet_NaN());
        bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
        return bad;
    }

    for (std::size_t t = 0; t < T; ++t) {
        const Step prop = propose_dtheta(s, rng);
        const Step eff  = engine.enforce(s, prop);

        if (!is_admissible(s, eff)) {
            State bad{};
            bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
            bad.m.fill(std::numeric_limits<double>::quiet_NaN());
            bad.v.fill(std::numeric_limits<double>::quiet_NaN());
            bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }

        const std::uint64_t prev = s.step;
        s = derive_next(s, eff);
        if (s.step != prev + 1) {
            State bad{};
            bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
            bad.m.fill(std::numeric_limits<double>::quiet_NaN());
            bad.v.fill(std::numeric_limits<double>::quiet_NaN());
            bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }

        if (!is_state_valid_materialized(s)) {
            State bad{};
            bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
            bad.m.fill(std::numeric_limits<double>::quiet_NaN());
            bad.v.fill(std::numeric_limits<double>::quiet_NaN());
            bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }
    }

    return s;
}

} // namespace

int main() {
    // Repeatability: Scale
    {
        const State a1 = run_enforced(ase::Mode::Scale);
        const State a2 = run_enforced(ase::Mode::Scale);

        if (a1.step != a2.step) return 1;
        for (std::size_t i = 0; i < N; ++i) {
            if (a1.theta[i] != a2.theta[i]) return 2;
            if (a1.m[i]     != a2.m[i])     return 3;
            if (a1.v[i]     != a2.v[i])     return 4;
            if (a1.ema[i]   != a2.ema[i])   return 5;
            if (!is_finite(a1.theta[i]) || !is_finite(a1.m[i]) || !is_finite(a1.v[i]) || !is_finite(a1.ema[i])) return 6;
        }
    }

    // Repeatability: Project
    {
        const State p1 = run_enforced(ase::Mode::Project);
        const State p2 = run_enforced(ase::Mode::Project);

        if (p1.step != p2.step) return 7;
        for (std::size_t i = 0; i < N; ++i) {
            if (p1.theta[i] != p2.theta[i]) return 8;
            if (p1.m[i]     != p2.m[i])     return 9;
            if (p1.v[i]     != p2.v[i])     return 10;
            if (p1.ema[i]   != p2.ema[i])   return 11;
            if (!is_finite(p1.theta[i]) || !is_finite(p1.m[i]) || !is_finite(p1.v[i]) || !is_finite(p1.ema[i])) return 12;
        }
    }

    return 0;
}

// ============================================================================
// tests/test_learning_state_envelope.cpp
// Safety test for extended learning state:
//   State = {theta, ema, step}
//
// Asserts:
// - boundedness: theta/ema/gap within envelope for all steps
// - numerical safety: no NaN/Inf
// - sign preservation on theta subset
// - exact step monotonicity: step_next == step + 1
// - repeatability: two runs => identical final state (within same build)
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

using Vec = std::array<double, N>;
using Step = Vec;

struct State {
    Vec theta{};
    Vec ema{};
    std::uint64_t step = 0;
};

inline bool is_finite(double x) { return std::isfinite(x) != 0; }

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

inline Step neutral_step() { Step z{}; z.fill(0.0); return z; }

inline bool scale_step(const Step& in, double k, Step& out) {
    if (!is_finite(k)) return false;
    for (std::size_t i = 0; i < N; ++i) {
        const double v = in[i] * k;
        if (!is_finite(v)) return false;
        out[i] = v;
    }
    return true;
}

// Envelope
struct Envelope {
    double R_theta = 5.0;
    double R_ema   = 5.0;
    double R_gap   = 2.0;
    std::size_t sign_preserve_k = 8;
    double beta = 0.98;
    double eps = 1e-12;
};

static Envelope g_env;

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
    for (double& x : s.theta) if (!is_finite(x)) x = 0.0;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (s.theta[i] < 0.0) s.theta[i] = 0.0;
    }

    const double nt = l2_norm(s.theta);
    if (!is_finite(nt)) s.theta.fill(0.0);
    else if (nt > g_env.R_theta && nt > 0.0) s.theta = vec_mul(s.theta, g_env.R_theta / nt);

    s.ema = s.theta;

    const double ne = l2_norm(s.ema);
    if (!is_finite(ne)) s.ema.fill(0.0);
    else if (ne > g_env.R_ema && ne > 0.0) s.ema = vec_mul(s.ema, g_env.R_ema / ne);
}

inline State derive_next(const State& S, const Step& dtheta) {
    State next = S;
    next.theta = vec_add(S.theta, dtheta);

    const double b = g_env.beta;
    for (std::size_t i = 0; i < N; ++i) {
        next.ema[i] = b * S.ema[i] + (1.0 - b) * next.theta[i];
    }

    next.step = S.step + 1;
    return next;
}

bool is_admissible(const State& S, const Step& dtheta) {
    if (!is_state_valid_materialized(S)) return false;
    for (double x : dtheta) if (!is_finite(x)) return false;

    const State next = derive_next(S, dtheta);
    if (next.step != S.step + 1) return false;

    return is_state_valid_materialized(next);
}

bool project_step(const State& S, const Step& in, Step& out) {
    if (!is_state_valid_materialized(S)) return false;
    for (double x : in) if (!is_finite(x)) return false;

    Vec theta_next = vec_add(S.theta, in);

    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
        if (theta_next[i] < 0.0) theta_next[i] = 0.0;
    }
    for (double x : theta_next) if (!is_finite(x)) return false;

    const double nt = l2_norm(theta_next);
    if (!is_finite(nt)) return false;
    if (nt > g_env.R_theta && nt > 0.0) theta_next = vec_mul(theta_next, g_env.R_theta / nt);

    // gap repair (single deterministic mix if needed)
    Vec ema_next{};
    const double b = g_env.beta;
    for (std::size_t i = 0; i < N; ++i) {
        ema_next[i] = b * S.ema[i] + (1.0 - b) * theta_next[i];
    }

    double gap = l2_norm_diff(ema_next, theta_next);
    if (!is_finite(gap)) return false;

    if (gap > g_env.R_gap && gap > 0.0) {
        double alpha = g_env.R_gap / gap;
        if (!is_finite(alpha)) return false;
        if (alpha < 0.0) alpha = 0.0;
        if (alpha > 1.0) alpha = 1.0;

        for (std::size_t i = 0; i < N; ++i) {
            theta_next[i] = alpha * theta_next[i] + (1.0 - alpha) * S.ema[i];
        }

        for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) {
            if (theta_next[i] < 0.0) theta_next[i] = 0.0;
        }

        const double nt2 = l2_norm(theta_next);
        if (!is_finite(nt2)) return false;
        if (nt2 > g_env.R_theta && nt2 > 0.0) theta_next = vec_mul(theta_next, g_env.R_theta / nt2);
    }

    out = vec_sub(theta_next, S.theta);

    return is_admissible(S, out);
}

Step propose_dtheta(const State& s, std::mt19937& rng, double eta) {
    std::normal_distribution<double> noise(0.0, 0.02);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

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
    for (std::size_t i = 0; i < N; ++i) s.theta[i] = 0.18;
    for (std::size_t i = 0; i < g_env.sign_preserve_k; ++i) s.theta[i] = 0.12;
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
    const double eta = 0.02;

    if (!is_state_valid_materialized(s)) {
        State bad{};
        bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
        bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
        return bad;
    }

    for (std::size_t t = 0; t < T; ++t) {
        const Step prop = propose_dtheta(s, rng, eta);
        const Step eff  = engine.enforce(s, prop);

        if (!is_admissible(s, eff)) {
            State bad{};
            bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
            bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }

        const std::uint64_t prev_step = s.step;
        s = derive_next(s, eff);

        if (s.step != prev_step + 1) {
            State bad{};
            bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
            bad.ema.fill(std::numeric_limits<double>::quiet_NaN());
            return bad;
        }

        if (!is_state_valid_materialized(s)) {
            State bad{};
            bad.theta.fill(std::numeric_limits<double>::quiet_NaN());
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
            if (a1.ema[i]   != a2.ema[i])   return 3;
            if (!is_finite(a1.theta[i]) || !is_finite(a1.ema[i])) return 4;
        }
        if (!is_state_valid_materialized(a1)) return 5;
    }

    // Repeatability: Project
    {
        const State p1 = run_enforced(ase::Mode::Project);
        const State p2 = run_enforced(ase::Mode::Project);

        if (p1.step != p2.step) return 6;
        for (std::size_t i = 0; i < N; ++i) {
            if (p1.theta[i] != p2.theta[i]) return 7;
            if (p1.ema[i]   != p2.ema[i])   return 8;
            if (!is_finite(p1.theta[i]) || !is_finite(p1.ema[i])) return 9;
        }
        if (!is_state_valid_materialized(p1)) return 10;
    }

    return 0;
}

// tests/test_core.cpp
#include <cassert>
#include <cmath>
#include <limits>

#include "ase/ase.hpp"

static bool is_finite(double x) { return std::isfinite(x); }

static bool admissible_limit(const double& S, const double& dS) {
    if (!is_finite(S) || !is_finite(dS)) return false;
    const double next = S + dS;
    if (!is_finite(next)) return false;
    return std::fabs(next) <= 1.0;
}

static double neutral_zero() { return 0.0; }

static bool scale_mul(const double& in, double k, double& out) {
    if (!is_finite(in) || !is_finite(k)) return false;
    out = in * k;
    return is_finite(out);
}

static bool project_clamp(const double& S, const double& dS, double& out) {
    if (!is_finite(S) || !is_finite(dS)) return false;
    constexpr double LIMIT = 1.0;

    double next = S + dS;
    if (!is_finite(next)) return false;

    if (next >  LIMIT) next =  LIMIT;
    if (next < -LIMIT) next = -LIMIT;

    out = next - S;
    return is_finite(out);
}

static void test_pass_through() {
    ase::Config cfg{ase::Mode::Reject};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        &scale_mul,
        nullptr
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double S = 0.0;
    const double dS = 0.2;
    const double eff = eng.enforce(S, dS);

    assert(eff == dS);
    assert(admissible_limit(S, eff));
    (void)eff;
}

static void test_reject_to_neutral() {
    ase::Config cfg{ase::Mode::Reject};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        &scale_mul,
        nullptr
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double eff = eng.enforce(0.9, 0.5);

    assert(eff == 0.0);
    (void)eff;
}

static void test_scale_finds_admissible() {
    ase::Config cfg{ase::Mode::Scale, 16, 0.5};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        &scale_mul,
        nullptr
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double eff = eng.enforce(0.9, 0.5);

    assert(eff > 0.0);
    assert(std::fabs(0.9 + eff) <= 1.0);
    (void)eff;
}

static void test_bounded_termination_scale() {
    ase::Config cfg{ase::Mode::Scale, 4, 1.0};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        &scale_mul,
        nullptr
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double eff = eng.enforce(0.9, 0.5);

    assert(eff == 0.0);
    (void)eff;
}

static void test_nan_inf_fail_closed() {
    ase::Config cfg{ase::Mode::Scale, 16, 0.5};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        &scale_mul,
        nullptr
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double eff_nan = eng.enforce(0.0, std::numeric_limits<double>::quiet_NaN());
    const double eff_inf = eng.enforce(0.0, std::numeric_limits<double>::infinity());

    assert(eff_nan == 0.0);
    assert(eff_inf == 0.0);
    (void)eff_nan;
    (void)eff_inf;
}

static void test_determinism() {
    ase::Config cfg{ase::Mode::Scale, 16, 0.5};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        &scale_mul,
        nullptr
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double a = eng.enforce(0.9, 0.5);
    const double b = eng.enforce(0.9, 0.5);

    assert(a == b);
    (void)a;
    (void)b;
}

static void test_project() {
    ase::Config cfg{ase::Mode::Project};

    ase::Dependencies<double,double> deps{
        &admissible_limit,
        &neutral_zero,
        nullptr,
        &project_clamp
    };

    ase::Engine<double,double> eng(cfg, deps);

    const double eff = eng.enforce(0.9, 0.5);

    assert(std::fabs(0.9 + eff - 1.0) < 1e-12);
    (void)eff;
}

int main() {
    test_pass_through();
    test_reject_to_neutral();
    test_scale_finds_admissible();
    test_bounded_termination_scale();
    test_nan_inf_fail_closed();
    test_determinism();
    test_project();
    return 0;
}

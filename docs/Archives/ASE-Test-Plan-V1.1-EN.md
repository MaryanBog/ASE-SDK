# Admissible Step Engine (ASE)
## Test Plan & Test Skeletons v1.1

---

## 1. Purpose

This document defines the **test plan** and **test skeletons** required to execute
the planned verification items referenced by the **ASE Compliance Matrix v1.1**.

It provides:

- a complete test inventory,
- minimal fixtures and adapters,
- deterministic test scenarios covering modes, failures, and numerical faults.

---

## 2. Test Harness Assumptions

- Language: C++17+
- No mandatory third-party framework.
- Tests may be implemented using any harness (custom, Catch2, GoogleTest),
  but the test logic and determinism constraints remain unchanged.

A minimal test harness should support:

- `ASSERT_TRUE(x)`, `ASSERT_FALSE(x)`
- `ASSERT_EQ(a,b)` for scalars
- `ASSERT_STEP_EQ(step_a, step_b)` for step objects

---

## 3. Minimal Domain Fixture (Reference)

ASE is domain-agnostic, so tests use a minimal synthetic domain:

- `State` = single double `x`
- `Step`  = single double `dx`
- Apply: `x' = x + dx`

Admissibility predicate (deterministic, local):

- admissible if `abs(x + dx) <= X_MAX`
- not admissible otherwise

Neutral step:

- `dx = 0`

Scaling:

- `dx_scaled = dx * k`

Projection:

- clamp to boundary: `dx_proj = clamp(-X_MAX - x, +X_MAX - x)` (one-shot)

All operations are deterministic and free of side effects.

---

## 4. Test Inventory (Mapped to Compliance Matrix Planned Items)

### 4.1 Core Behavior
- `test_admissible_passthrough`              (CM-5.2)
- `test_inadmissible_rejected`               (CM-2.7)
- `test_neutral_fallback`                    (CM-5.4)
- `test_fail_closed`                         (CM-4.7)
- `test_fail_closed_all_failures`            (CM-11.2)

### 4.2 Enforcement Modes
- `test_reject_mode`                         (CM-6.2)
- `test_scale_mode`                          (CM-6.3)
- `test_project_mode`                        (CM-6.4)

### 4.3 Determinism
- `test_deterministic_repeat`                (CM-4.5)
- `test_deterministic_enforcement`           (CM-5.7)
- `test_admissibility_determinism`           (CM-7.2)
- `test_strong_determinism`                  (CM-9.1)
- `test_repeatability`                       (CM-14.4)

### 4.4 Numerical Safety
- `test_nan_inf_rejection`                   (CM-8.2)
- `test_scaling_stability`                   (CM-8.5)

### 4.5 Neutral Step Semantics
- `test_neutral_noop`                        (CM-10.1)
- `test_neutral_usage_conditions`            (CM-10.4)

### 4.6 Termination / Bounds
- `test_termination_fallback`                (CM-9.5)

---

## 5. Test Skeletons (Implementation-Oriented)

NOTE: Code below is intentionally framework-neutral and uses pseudo-macros.

### 5.1 Shared Types and Helpers

    struct State { double x; };
    struct Step  { double dx; };

    static inline State apply_step(State s, Step d) {
        return State{ s.x + d.dx };
    }

    static inline double absd(double v) { return v < 0 ? -v : v; }

    struct DomainOps {
        double X_MAX;

        bool admissible(const State& s, const Step& d) const {
            const double x2 = s.x + d.dx;
            // If x2 is NaN, the comparison is false; treat as inadmissible by returning false.
            return absd(x2) <= X_MAX;
        }

        Step neutral_step() const {
            return Step{0.0};
        }

        Step scale_step(const Step& d, double k) const {
            return Step{ d.dx * k };
        }

        Step project_step(const State& s, const Step& d) const {
            (void)d; // projection ignores original magnitude; clamps to boundary in one shot
            const double lo = -X_MAX - s.x;
            const double hi =  X_MAX - s.x;
            double clamped = 0.0;
            if (d.dx < lo) clamped = lo;
            else if (d.dx > hi) clamped = hi;
            else clamped = d.dx;
            return Step{ clamped };
        }
    };

### 5.2 ASE Configuration Fixture

    enum class Mode { Reject, Scale, Project };

    struct ASE_Config {
        Mode mode;
        int max_iters;          // used by Scale
        double shrink_factor;   // used by Scale (e.g., 0.5)
    };

### 5.3 Test: Admissible Pass-Through (CM-5.2)

    void test_admissible_passthrough() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Reject, .max_iters = 8, .shrink_factor = 0.5 };

        State s{ 1.0 };
        Step  d{ 2.0 }; // x' = 3.0 admissible

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out.dx, d.dx);
        ASSERT_TRUE(ops.admissible(s, out));
    }

### 5.4 Test: Inadmissible Rejected (CM-2.7)

    void test_inadmissible_rejected() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Reject, .max_iters = 8, .shrink_factor = 0.5 };

        State s{ 9.0 };
        Step  d{ 5.0 }; // x' = 14.0 inadmissible

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out.dx, 0.0);
        ASSERT_TRUE(ops.admissible(s, out));
    }

### 5.5 Test: Reject Mode (CM-6.2)

    void test_reject_mode() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Reject, .max_iters = 8, .shrink_factor = 0.5 };

        State s{ 9.0 };
        Step  d{ 2.0 }; // inadmissible

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out.dx, 0.0);
    }

### 5.6 Test: Scale Mode Finds Admissible (CM-6.3)

    void test_scale_mode() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Scale, .max_iters = 10, .shrink_factor = 0.5 };

        State s{ 9.0 };
        Step  d{ 4.0 }; // inadmissible at k=1, admissible at k=0.25 -> dx=1.0 => x'=10.0

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_TRUE(ops.admissible(s, out));
        ASSERT_TRUE(absd(out.dx) <= absd(d.dx)); // non-expansive magnitude check
    }

### 5.7 Test: Project Mode One-Shot Clamp (CM-6.4)

    void test_project_mode() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Project, .max_iters = 8, .shrink_factor = 0.5 };

        State s{ 9.0 };
        Step  d{ 100.0 }; // inadmissible, projection clamps to dx=1.0

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out.dx, 1.0);
        ASSERT_TRUE(ops.admissible(s, out));
    }

### 5.8 Test: Neutral No-Op (CM-10.1)

    void test_neutral_noop() {
        DomainOps ops{ .X_MAX = 10.0 };
        Step n = ops.neutral_step();

        State s{ 3.5 };
        State s2 = apply_step(s, n);

        ASSERT_EQ(s2.x, s.x);
    }

### 5.9 Test: Neutral Usage Conditions (CM-10.4)

    void test_neutral_usage_conditions() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Scale, .max_iters = 0, .shrink_factor = 0.5 }; // forces immediate failure

        State s{ 9.0 };
        Step  d{ 4.0 };

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out.dx, 0.0);
    }

### 5.10 Test: NaN / Inf Rejection (CM-8.2)

    void test_nan_inf_rejection() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Scale, .max_iters = 8, .shrink_factor = 0.5 };

        State s{ 0.0 };

        // NaN step
        Step d_nan{ 0.0 / 0.0 };
        Step out_nan = ase_enforce(cfg, ops, s, d_nan);
        ASSERT_EQ(out_nan.dx, 0.0);

        // +Inf step
        Step d_inf{ 1.0 / 0.0 };
        Step out_inf = ase_enforce(cfg, ops, s, d_inf);
        ASSERT_EQ(out_inf.dx, 0.0);
    }

### 5.11 Test: Deterministic Repeat (CM-4.5)

    void test_deterministic_repeat() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Scale, .max_iters = 10, .shrink_factor = 0.5 };

        State s{ 9.0 };
        Step  d{ 4.0 };

        Step out1 = ase_enforce(cfg, ops, s, d);
        Step out2 = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out1.dx, out2.dx);
    }

### 5.12 Test: Strong Determinism Sweep (CM-9.1, CM-14.4)

    void test_strong_determinism() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Scale, .max_iters = 12, .shrink_factor = 0.5 };

        for (int i = 0; i < 1000; ++i) {
            State s{ (i % 21) - 10.0 };     // [-10..10]
            Step  d{ (i % 41) - 20.0 };     // [-20..20]

            Step a = ase_enforce(cfg, ops, s, d);
            Step b = ase_enforce(cfg, ops, s, d);

            ASSERT_EQ(a.dx, b.dx);
            ASSERT_TRUE(ops.admissible(s, a));
        }
    }

### 5.13 Test: Termination Fallback (CM-9.5)

    void test_termination_fallback() {
        DomainOps ops{ .X_MAX = 10.0 };
        ASE_Config cfg{ .mode = Mode::Scale, .max_iters = 1, .shrink_factor = 0.5 };

        State s{ 9.99 };
        Step  d{ 1000.0 }; // even after one shrink, still inadmissible

        Step out = ase_enforce(cfg, ops, s, d);

        ASSERT_EQ(out.dx, 0.0);
    }

---

## 6. Required Additional Artifacts (for Verification)

The test suite should produce:

- a test list with stable names matching the inventory,
- deterministic execution logs (optional),
- pass/fail summary.

Planned additional report:

- static analysis checklist and report (separate document).

---

## 7. End of Test Plan

This document defines the complete minimum test plan
and provides skeletons for implementing the tests mechanically.

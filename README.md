# ASE-SDK — Admissible Step Enforcement

ASE (Admissible Step Enforcement) is a minimal, deterministic, header-only C++ library
that enforces admissibility constraints on proposed state update steps.

ASE is **not** a controller, **not** an optimizer, and **not** an AI system.
It does not compute actions and never updates state by itself.

ASE evaluates a *proposed* step and returns a *safe effective* step
according to a strictly bounded and deterministic enforcement policy.

---

## Core Idea

Given:
- current state `S`
- proposed update step `ΔS`

ASE computes:

```
ΔS_eff = ASE(S, ΔS)
```

The host system **must** then apply:

```
S_next = S ⊕ ΔS_eff
```

ASE never applies the step itself.
State evolution is always performed by the host system.

---

## What ASE Guarantees

When the integration rule above is followed, ASE guarantees:

- no inadmissible step is ever applied
- bounded execution (finite number of checks)
- deterministic behavior (same inputs → same output)
- fail-closed semantics (neutral step on failure)
- no internal state, no memory, no side effects

---

## Enforcement Modes

ASE supports three enforcement modes:

1. `Reject`  
   If the proposed step is inadmissible, return the neutral step.

2. `Scale`  
   Iteratively scale the proposed step until it becomes admissible
   or a bounded attempt limit is reached.

3. `Project`  
   Apply a single host-defined projection into the admissible set.

---

## Integration Contract (Mandatory)

To preserve ASE guarantees, the host system **must**:

- call `engine.enforce(S, ΔS)` exactly once per integration step
- apply **exactly** the returned value
- never modify or override the enforced step

Correct pattern:

```cpp
ΔS_eff = engine.enforce(S, ΔS);
S_next = apply_step(S, ΔS_eff);
```

Incorrect usage (breaks guarantees):

- calling `enforce()` multiple times per step
- post-processing `ΔS_eff`
- applying `ΔS` instead of `ΔS_eff`

---

## API Overview

```cpp
ase::Config cfg;
cfg.mode = ase::Mode::Scale;
cfg.max_scale_attempts = 16;
cfg.scale_factor = 0.5;

ase::Dependencies<State, Step> deps;
deps.is_admissible = &is_admissible;
deps.neutral_step  = &neutral_step;
deps.scale_step    = &scale_step;   // only for Scale mode
deps.project_step  = &project_step; // only for Project mode

ase::Engine<State, Step> engine(cfg, deps);

Step effective = engine.enforce(S, proposed);
```

ASE is header-only and requires no linking.

---

## Validated Scenarios (Internal)

While executable examples are not part of the public API,
the following scenarios are **fully validated** by tests and internal simulations:

- **Scaling enforcement**  
  Inadmissible steps are progressively reduced until admissible.

- **Boundary projection**  
  Single-step projection lands exactly on admissible boundaries.

- **Fail-closed behavior**  
  NaN, infinity, or invalid inputs always return the neutral step.

- **Determinism**  
  Repeated calls with identical inputs always produce identical outputs.

- **Host integration correctness**  
  Verified that the host applies exactly one enforced step per iteration.

Observed reference outputs (illustrative):

```
S=0.9, proposed=0.5 → effective=0.0625 → next=0.9625
S=0.9, proposed=0.5 → effective=0.1    → next=1.0
S=0.9, proposed=0.5 → effective=0.0625 → next=0.9625
```

These results confirm boundedness, admissibility, and deterministic enforcement.

---

## Requirements

- C++17 compatible compiler
- CMake 3.16+
- Ninja (optional, recommended)

---

## Build & Test (Internal Validation)

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -V
```

---

## License

MIT License.  
See the `LICENSE` file for details.

---

## Status

- Core logic: **stable**
- Tests: **passing (Release, -Werror)**
- API: **frozen**
- Intended use: **safety enforcement layer**

ASE is designed to be embedded inside larger systems
requiring strict admissibility guarantees and deterministic behavior.

# Admissible Step Engine (ASE)
## Implementation Notes v1.0

---

## 1. Purpose of This Document

This document defines the **implementation-level realization**
of the Admissible Step Engine (ASE) as specified by:

- ASE Normative Specification v1.0
- ASE Design Document v1.0

The purpose of this document is to:
- eliminate ambiguity before coding,
- define exact algorithms and control flow,
- specify corner-case handling,
- define constraints for a correct C++ implementation.

This document is **prescriptive**.
Any deviation during coding MUST be justified explicitly.

---

## 2. Implementation Scope

This document applies to:

- C++17 (minimum),
- header-only or small static library,
- no mandatory external dependencies.

This document does NOT define:
- build system details,
- packaging or distribution,
- language bindings.

---

## 3. Core Execution Flow

The canonical ASE execution flow is:

    Input: State S, Step dS
    Output: Step dS'

Execution steps:

1. Validate inputs for numerical validity.
2. Evaluate admissibility of the proposed step.
3. If admissible → return dS.
4. If inadmissible → apply enforcement algorithm.
5. If enforcement fails → return neutral step.

No step outside this flow is permitted.

---

## 4. Input Validation Rules

### 4.1 State Validation

ASE MUST NOT introspect state semantics.

However, ASE MUST allow user-provided validation hooks
for detecting invalid numerical representations.

If the state is detected as invalid by user logic:
- the step MUST be rejected,
- the neutral step MUST be returned.

---

### 4.2 Step Validation

Before admissibility checks, ASE MUST verify:

- the step does not contain NaN values,
- the step does not contain Inf values.

If NaN or Inf is detected:
- the step is immediately treated as inadmissible,
- enforcement proceeds directly to neutral step.

---

## 5. Admissibility Evaluation

### 5.1 Admissibility Predicate Invocation

The admissibility predicate MUST be invoked as:

    admissible(S, dS)

The predicate MUST:
- be called exactly once for the unmodified step,
- never be cached,
- never be short-circuited based on history.

---

### 5.2 Predicate Failure Handling

If the predicate throws (in non-embedded profiles):
- the step MUST be treated as inadmissible,
- no exception MUST escape the ASE boundary.

Exception swallowing MUST be documented if used.

---

## 6. Enforcement Algorithms Implementation

### 6.1 Reject Mode

Implementation logic:

    if admissible(S, dS):
        return dS
    else:
        return neutral_step

No further checks are performed.

---

### 6.2 Scale Mode

Implementation logic:

    k = 1.0
    for i in 0 .. max_iters-1:
        dS_k = scale_step(dS, k)
        if admissible(S, dS_k):
            return dS_k
        k = k * shrink_factor

    return neutral_step

Constraints:
- shrink_factor MUST satisfy 0 < shrink_factor < 1
- k MUST be monotonically decreasing
- scale_step MUST be pure and deterministic

---

### 6.3 Project Mode

Implementation logic:

    if admissible(S, dS):
        return dS
    dS_p = project_step(S, dS)
    if admissible(S, dS_p):
        return dS_p
    return neutral_step

Projection MUST be applied at most once.

---

## 7. Neutral Step Handling

A neutral step MUST:

- be constructible deterministically,
- be admissible by definition,
- leave the state unchanged when applied.

ASE MUST NOT attempt to infer a neutral step.
It MUST be explicitly provided by the integrator.

---

## 8. Termination Guarantees

All loops MUST:
- have explicit iteration bounds,
- avoid floating termination conditions,
- terminate deterministically.

Recursion is forbidden.

---

## 9. Numerical Stability Rules

ASE MUST apply the following rules:

- Any NaN or Inf encountered → treat step as inadmissible.
- Any overflow detected during scaling → treat as inadmissible.
- No normalization or renormalization beyond user-provided logic.

Floating-point comparisons MUST use explicit tolerances
defined by configuration.

---

## 10. Memory Model Constraints

### 10.1 Allocation

In Profile P0:
- dynamic allocation is forbidden in core logic.

In Profile P1:
- allocation MAY occur only in user-provided operations.

ASE core MUST NOT allocate intentionally.

---

### 10.2 Object Lifetime

All temporaries MUST:
- be stack-allocated,
- have bounded lifetime within the enforce call.

No persistent objects are allowed.

---

## 11. Error Signaling

ASE MUST support at least one of:

- silent neutral-step return,
- explicit status code via out-parameter.

Exceptions MUST NOT be used in Profile P0.

---

## 12. Logging and Diagnostics

ASE MUST NOT:
- log internally,
- write to stdout/stderr,
- depend on logging frameworks.

Any diagnostics MUST be external to ASE.

---

## 13. Implementation Checklist

Before writing code, verify:

- Specification is satisfied.
- Design constraints are enforced.
- All corner cases are enumerated.
- No hidden state is introduced.
- Determinism is preserved.

---

## 14. Summary

This document defines the precise implementation behavior
required to produce a conforming ASE library.

Any C++ implementation following these notes
will satisfy both the ASE Specification v1.0
and the ASE Design Document v1.0.
# Admissible Step Engine (ASE)
## Normative Specification v1.0

---

## 1. Purpose and Scope

This document specifies the **Admissible Step Engine (ASE)** — a minimal,
deterministic engineering mechanism that enforces **step admissibility**
in iterative systems.

ASE prevents execution of **inadmissible steps**, defined as steps that may
cause **irreversible or structurally destructive state transitions**.

This specification defines:
- what constitutes a *step*,
- what it means for a step to be *admissible*,
- required invariants and guarantees,
- prohibited behaviors,
- the mandatory input/output contract.

This document is **normative**.
All requirements expressed using **MUST**, **MUST NOT**, **SHOULD**, and
**MAY** are to be interpreted as described in RFC 2119.

---

## 2. Intended Use

ASE is intended to be used as a **low-level enforcement layer**
inside iterative systems, including but not limited to:

- learning systems (online or offline),
- parameter tuning and auto-tuning systems,
- numerical optimization loops,
- configuration and update pipelines,
- adaptive and autonomous software systems,
- control systems (including but not limited to ECT).

ASE is **not** a controller, optimizer, learning algorithm, diagnostic system,
or monitoring tool.

ASE operates exclusively at the level of **individual state transitions**.

---

## 3. Non-Goals

ASE explicitly does **not** attempt to:

- optimize system performance,
- improve convergence speed,
- guarantee optimality,
- model system dynamics,
- predict future system behavior,
- perform diagnosis or monitoring,
- recover from already irreversible states.

ASE only determines whether a **proposed step** is admissible
*at the moment it is proposed*.

---

## 4. Core Concept: Step Admissibility

An **iterative system** is assumed to evolve through a sequence of states:

S₀ → S₁ → S₂ → …

where each transition is produced by application of a **step**.

A **step** is any proposed transformation:

ΔS : S → S'

A step is considered **admissible** if and only if
its application does **not** violate the admissibility constraints
defined in this specification.

ASE enforces the following principle:

> **Not every step is admissible.  
> Steps that may cause irreversible structural degradation MUST be rejected or constrained.**

---

## 5. Definition of State and Step

### 5.1 State

A **state** `S` is an opaque, user-defined representation of the system
at a given iteration.

ASE does **not** interpret the semantics of the state.
ASE treats the state as a structured value
that can be copied, transformed, and evaluated via user-provided operations.

---

### 5.2 Step

A **step** `ΔS` is a user-provided proposed modification of the current state.

A step MUST:
- be representable independently of the state,
- be applicable to a given state via a deterministic operation,
- not depend on hidden or implicit global state.

ASE does not assume any specific representation of a step.

---

## 6. Admissibility Requirement

For a given state `S` and proposed step `ΔS`,
ASE MUST evaluate admissibility by checking a **local admissibility condition**
provided or selected by the implementation.

The admissibility condition MUST satisfy the following properties:

- **Locality**  
  The admissibility decision depends only on `S` and `ΔS`.

- **Determinism**  
  Given identical `S` and `ΔS`, the decision MUST be identical.

- **Monotonic Safety**  
  An admissible step MUST NOT increase structural tension,
  deviation, or any equivalent monotonic safety measure
  defined by the implementation.

ASE MUST NOT use:
- historical information,
- accumulated memory,
- prediction of future states,
- probabilistic criteria.

---

## 7. Enforcement Modes

ASE MUST support at least one of the following enforcement modes:

1. **Reject**  
   The proposed step is discarded and the state remains unchanged.

2. **Scale**  
   The proposed step is reduced in magnitude until admissibility is satisfied.

3. **Project**  
   The proposed step is replaced by a nearest admissible step.

The selected enforcement mode MUST be deterministic.

---

## 8. Mandatory Invariants

An implementation conforming to this specification MUST satisfy all of the following:

- No hidden state is maintained by ASE.
- No dynamic memory allocation is required for core operation.
- No side effects outside the provided state transition.
- No probabilistic behavior.
- No dependence on system time, randomness, or external resources.

---

## 9. Relation to Flexion Framework

ASE is an **engineering enforcement mechanism**
derived from the **admissibility principle** defined in the Flexion Framework.

ASE does not extend, modify, or reinterpret the Framework.
It implements enforcement of admissible transitions
at the level of individual steps.

---

## 10. Summary

Admissible Step Engine (ASE) is a minimal, deterministic mechanism that
ensures iterative systems do not execute structurally inadmissible steps.

ASE provides:
- a strict admissibility contract,
- deterministic enforcement,
- zero reliance on models or prediction,
- applicability across diverse domains.

ASE exists to answer a single engineering question:

> **Is this step allowed?**
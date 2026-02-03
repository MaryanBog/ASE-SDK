# Admissible Step Engine (ASE)
## Normative Specification v1.1

---

## 1. Purpose and Scope

This document defines the **Admissible Step Engine (ASE)** as a **normative,
engineering-level enforcement mechanism** for iterative systems.

ASE exists to **prevent execution of inadmissible steps**—steps that may cause
**irreversible or structurally destructive state transitions**—by enforcing
admissibility at the level of individual iterations.

This specification defines **mandatory requirements** for any implementation
claiming compliance with ASE.

---

## 1.1 Purpose

The purpose of ASE is to ensure that an iterative system:

- never applies a step that violates admissibility constraints,
- never transitions into an irrecoverable state due to a single step,
- enforces admissibility **locally, deterministically, and without prediction**.

ASE answers exactly one engineering question:

> **Is this step allowed to be executed?**

ASE does not attempt to determine whether a step is optimal, correct,
efficient, or desirable. It determines only whether the step is **permissible**.

---

## 1.2 Scope of Application

ASE applies to **any system that evolves through discrete steps**, including,
but not limited to:

- learning and adaptive systems,
- numerical optimization loops,
- parameter tuning and auto-tuning systems,
- configuration and update pipelines,
- autonomous and self-modifying software,
- control systems and actuation pipelines.

ASE operates exclusively at the level of **individual state transitions**.

ASE does not own, manage, or influence:
- the iteration loop,
- the step generation process,
- system scheduling or control flow.

---

## 1.3 Scope of Responsibility

ASE is responsible **only** for enforcing step admissibility.

Specifically, ASE MUST:

- evaluate admissibility of a proposed step with respect to the current state,
- enforce admissibility via a deterministic enforcement mechanism,
- return an admissible effective step or a neutral step.

ASE MUST NOT:

- modify the system state directly,
- generate steps,
- reorder or skip iterations,
- infer system goals or semantics.

---

## 1.4 Non-Goals

ASE explicitly does **not** aim to:

- optimize performance or convergence,
- guarantee global stability,
- improve learning outcomes,
- model system dynamics,
- predict future behavior,
- diagnose failures or anomalies,
- recover from already irreversible states.

ASE is **not**:
- a controller,
- an optimizer,
- a learning algorithm,
- a monitoring or diagnostic system,
- a rollback or recovery mechanism.

---

## 1.5 Normative Language and Compliance

This specification uses the key words **MUST**, **MUST NOT**, **SHOULD**, and
**MAY** as defined in RFC 2119.

Any implementation claiming **ASE compliance** MUST satisfy all requirements
marked as **MUST** and **MUST NOT** in this document.

Where conflicts arise, this Specification **takes precedence** over
all design, implementation, or explanatory documents.

---

## 1.6 Specification Boundary

This document defines **what ASE guarantees** and **what it prohibits**.

It does not define:
- specific algorithms beyond normative requirements,
- programming language bindings,
- performance characteristics beyond boundedness guarantees.

Details of architecture and realization are defined in the
ASE Design Document and ASE Implementation Notes and
MUST NOT weaken or reinterpret this Specification.

---

## 1.7 Section Summary

This section establishes ASE as a **minimal, normative enforcement mechanism**
whose sole purpose is to prevent execution of inadmissible steps in iterative
systems.

ASE enforces admissibility.
Nothing more.
Nothing less.

---

## 2. Fundamental Definitions

This section defines the **fundamental terms and concepts**
used throughout this Specification.

All definitions in this section are **normative**.
Terms defined here MUST NOT be redefined or reinterpreted
in any compliant implementation.

---

## 2.1 Iterative System

An **iterative system** is a system whose evolution can be represented
as a sequence of discrete state transitions:

    S₀ → S₁ → S₂ → …

Each transition is produced by the application of a **step**
to the current state.

An iterative system MAY be synchronous or asynchronous,
but ASE operates exclusively at the level of **individual transitions**.

---

## 2.2 State

A **state** `S` is a complete representation of the system
at a given iteration.

Properties of a state:

- A state is treated as an **opaque value** by ASE.
- ASE does not interpret the semantics of the state.
- A state MUST be representable independently of external mutable context.
- A state MUST be usable as input to admissibility evaluation.

ASE MUST NOT:
- modify the state directly,
- decompose or introspect state semantics,
- maintain copies of state beyond the scope of a single enforcement call.

---

## 2.3 Step

A **step** `ΔS` is a proposed transformation intended to be applied
to a state.

A step represents a **candidate state transition**, not an executed one.

Properties of a step:

- A step MUST be representable independently of the state.
- A step MUST be applicable to a state via a deterministic operation.
- A step MUST NOT depend on hidden or implicit global state.
- A step MAY be rejected, modified, or replaced by ASE.

---

## 2.4 Proposed Step

A **proposed step** is a step produced by an external process,
such as a learning algorithm, optimizer, tuner, update mechanism,
or control logic.

A proposed step:

- is not assumed to be admissible,
- is subject to evaluation by ASE,
- MUST NOT be applied to the state without enforcement.

---

## 2.5 Effective Step

An **effective step** is the result of ASE enforcement.

An effective step:

- is guaranteed to be admissible,
- MAY be identical to the proposed step,
- MAY be a modified version of the proposed step,
- MAY be a neutral step.

Only the effective step MAY be applied to the state.

---

## 2.6 Neutral (No-Op) Step

A **neutral step** is a special admissible step that,
when applied to a state, leaves the state unchanged.

Properties of a neutral step:

- It MUST be admissible by definition.
- It MUST be deterministic.
- It MUST have no side effects.
- It MUST preserve the state exactly.

The definition and construction of a neutral step
is the responsibility of the integrator.

ASE MUST NOT attempt to infer or synthesize a neutral step implicitly.

---

## 2.7 Admissible Step

A **step** is **admissible** if its application to the current state
does not violate the admissibility constraints defined in this Specification.

An admissible step:

- does not cause irreversible structural degradation,
- does not increase structural tension or deviation beyond allowed bounds,
- satisfies all local admissibility requirements.

Admissibility is evaluated **locally** with respect to the current state
and the proposed step.

---

## 2.8 Inadmissible Step

A **step** is **inadmissible** if its application may:

- transition the system into an irrecoverable state,
- violate monotonic safety conditions,
- produce undefined or structurally destructive behavior.

Inadmissible steps MUST NOT be applied.

---

## 2.9 Admissibility Evaluation

**Admissibility evaluation** is the process by which ASE determines
whether a proposed step is admissible.

Admissibility evaluation MUST:

- depend only on the current state and the proposed step,
- be deterministic,
- be free of historical or predictive elements.

Admissibility evaluation MUST NOT:

- use accumulated history,
- perform future trajectory prediction,
- rely on probabilistic or statistical criteria.

---

## 2.10 Section Summary

This section defines the foundational concepts required
to interpret the remainder of this Specification.

All subsequent requirements, algorithms, and guarantees
are expressed in terms of the definitions provided here.

---

## 3. Admissibility Principle (Normative)

This section defines the **admissibility principle**
that underlies the Admissible Step Engine (ASE).

The requirements in this section are **normative**
and apply to all compliant ASE implementations.

---

## 3.1 Existence of Inadmissible Steps

In any non-trivial iterative system, there exist steps that,
if applied, may cause **irreversible or structurally destructive
state transitions**.

Such steps are referred to as **inadmissible steps**.

ASE is founded on the assumption that:

- inadmissible steps exist,
- inadmissible steps cannot always be detected after execution,
- prevention of inadmissible steps must occur **before execution**.

---

## 3.2 Irreversibility as a Step Property

Irreversibility is treated as a property of a **step applied to a state**,
not as a global property of the system.

A step is considered **irreversible** if, once applied:

- no sequence of subsequent admissible steps can restore
  the system to an equivalent or acceptable state,
- recovery would require external intervention outside
  the defined system dynamics.

ASE does not attempt to repair or reverse irreversible transitions.
ASE prevents their execution.

---

## 3.3 Locality of Admissibility

Admissibility MUST be evaluated **locally**.

For a given state `S` and proposed step `ΔS`:

- admissibility depends only on `(S, ΔS)`,
- admissibility MUST NOT depend on past states,
- admissibility MUST NOT depend on predicted future states.

This locality requirement ensures that admissibility enforcement
is applicable in real-time and safety-critical contexts.

---

## 3.4 Monotonic Safety Requirement

Admissible steps MUST satisfy a **monotonic safety condition**.

Specifically, an admissible step MUST NOT:

- increase structural tension,
- increase deviation beyond defined safe bounds,
- move the system closer to an irrecoverable boundary.

The concrete representation of structural tension or deviation
is domain-specific, but the monotonicity requirement is mandatory.

---

## 3.5 Fail-Closed Interpretation

In cases where admissibility cannot be conclusively established,
the step MUST be treated as **inadmissible**.

ASE MUST operate under a **fail-closed policy**:

- uncertainty results in rejection or neutralization,
- absence of proof of admissibility is treated as inadmissibility.

Fail-open behavior is strictly prohibited.

---

## 3.6 Enforcement Obligation

Any step classified as inadmissible according to this principle:

- MUST NOT be applied to the state,
- MUST be rejected, scaled, projected, or replaced
  according to the enforcement semantics defined later
  in this Specification.

ASE MUST ensure that no inadmissible step
is ever executed.

---

## 3.7 Relationship to Underlying Theory (Informative)

The admissibility principle enforced by ASE
is derived from the recognition that
irreversibility emerges from step-level transitions
in complex systems.

ASE implements this principle as an **engineering enforcement rule**,
not as a theoretical model.

This Specification does not require understanding
of any underlying theory for correct implementation.

---

## 3.8 Section Summary

This section establishes admissibility as a **local, monotonic,
and enforceable property of steps**.

ASE exists to ensure that inadmissible steps,
as defined by this principle,
are never executed.

---

## 4. Execution Model

This section defines the **normative execution model**
for the Admissible Step Engine (ASE).

All compliant implementations MUST adhere
to the execution semantics defined herein.

---

## 4.1 Canonical Enforcement Flow

ASE enforces admissibility according to the following canonical flow:

    Input:  current state S, proposed step ΔS
    Output: effective step ΔS'

The execution sequence is:

1. Receive the current state `S`.
2. Receive the proposed step `ΔS`.
3. Evaluate admissibility of `ΔS` with respect to `S`.
4. If admissible, produce `ΔS' = ΔS`.
5. If inadmissible, produce `ΔS'` according to enforcement semantics.
6. Return `ΔS'`.

Only the effective step `ΔS'` MAY be applied to the state.

---

## 4.2 Single-Step Enforcement Semantics

ASE operates strictly on **one step at a time**.

ASE MUST NOT:
- accumulate steps,
- reorder steps,
- merge multiple steps,
- inspect or modify iteration indices.

Each invocation of ASE enforcement
is independent of all previous invocations.

---

## 4.3 Separation of Responsibilities

ASE enforces a strict separation between:

- step generation (external),
- step enforcement (ASE),
- state update (external).

ASE MUST NOT:
- generate steps,
- apply steps to the state,
- modify the iteration loop,
- influence system scheduling.

ASE exists solely as an **intermediate enforcement layer**.

---

## 4.4 State Ownership and Mutation

ASE does not own the system state.

Specifically:
- ASE MUST NOT mutate the state directly,
- ASE MUST NOT persist state between calls,
- ASE MUST NOT assume any structure or semantics of the state.

All state mutation MUST occur outside ASE
via application of the effective step.

---

## 4.5 Stateless Operation

ASE MUST operate in a **stateless manner**.

This means:
- no internal memory across invocations,
- no accumulation of historical data,
- no adaptive internal parameters.

Statelessness is a mandatory property
and a prerequisite for determinism.

---

## 4.6 Deterministic Invocation Semantics

For identical inputs `(S, ΔS)` and identical configuration,
ASE MUST produce identical outputs `ΔS'`.

ASE MUST NOT:
- depend on system time,
- depend on randomness,
- depend on external I/O,
- depend on global mutable state.

---

## 4.7 Bounded Execution

ASE enforcement MUST complete in **bounded time**.

Specifically:
- all loops MUST have explicit bounds,
- recursion is prohibited,
- termination MUST be guaranteed.

Unbounded or data-dependent execution time
is not permitted.

---

## 4.8 Fail-Closed Behavior

If any error, ambiguity, or undefined condition
is encountered during enforcement:

- the proposed step MUST be treated as inadmissible,
- the effective step MUST be produced
  according to enforcement semantics,
- no inadmissible step MAY be applied.

Fail-open behavior is strictly prohibited.

---

## 4.9 Execution Model Summary

The ASE execution model ensures that:

- admissibility is enforced before execution,
- enforcement is local, deterministic, and bounded,
- state mutation is strictly external to ASE.

This execution model is mandatory
for all ASE-compliant implementations.

---

## 5. Enforcement Semantics

This section defines the **normative enforcement semantics**
of the Admissible Step Engine (ASE).

Enforcement semantics specify how ASE transforms a proposed step
into an effective step under admissibility constraints.

All requirements in this section are **mandatory**.

---

## 5.1 Definition of Enforcement

**Enforcement** is the process by which ASE ensures that
only admissible steps may be executed.

Given a current state `S` and a proposed step `ΔS`,
ASE MUST produce an **effective step** `ΔS'` such that:

- `ΔS'` is admissible with respect to `S`,
- `ΔS'` satisfies all constraints of this Specification.

ASE MUST NOT permit execution of a step
that has not passed enforcement.

---

## 5.2 Pass-Through Condition

If a proposed step `ΔS` is admissible with respect to the current state `S`,
ASE MUST:

- accept the step without modification, and
- produce `ΔS'` that is semantically equivalent to `ΔS`.

ASE MUST NOT modify an admissible step
except for representational normalization
that preserves semantics.

---

## 5.3 Modification Condition

If a proposed step `ΔS` is inadmissible,
ASE MUST attempt enforcement according to the selected enforcement mode.

An inadmissible step MUST NOT be applied directly.

Modification MAY include:
- scaling the step,
- projecting the step,
- replacing the step with a neutral step.

The specific modification strategy is defined
by the enforcement mode selected by the integrator.

---

## 5.4 Rejection Condition

If enforcement cannot produce an admissible modified step,
ASE MUST:

- reject the proposed step, and
- return a neutral step.

Under no circumstances may ASE return an inadmissible step.

---

## 5.5 Neutral Step Substitution

When a step is rejected,
ASE MUST substitute the proposed step
with a neutral step.

The neutral step MUST satisfy all requirements
defined in Section 10 of this Specification.

Neutral step substitution MUST be deterministic.

---

## 5.6 Prohibition of Escalation

ASE MUST NOT escalate a proposed step.

Specifically, ASE MUST NOT:
- increase step magnitude,
- increase step aggressiveness,
- move a step closer to an irrecoverable boundary.

All enforcement transformations MUST be
non-expansive with respect to admissibility.

---

## 5.7 Single-Enforcement Guarantee

Each proposed step MUST be enforced exactly once.

ASE MUST NOT:
- re-enforce an already enforced step,
- chain enforcement across multiple steps,
- apply cumulative enforcement effects.

Enforcement is strictly **per-step**.

---

## 5.8 Deterministic Outcome Requirement

Given identical inputs and configuration,
enforcement MUST yield identical effective steps.

Non-deterministic enforcement behavior
is strictly prohibited.

---

## 5.9 Enforcement Semantics Summary

ASE enforcement semantics guarantee that:

- admissible steps pass unchanged,
- inadmissible steps are never executed,
- enforcement is deterministic and fail-closed,
- neutral steps are used when enforcement fails.

These semantics form the core safety guarantee
provided by ASE.

---

## 6. Enforcement Modes (Normative)

This section defines the **normative enforcement modes**
supported by the Admissible Step Engine (ASE).

An ASE-compliant implementation MUST support
at least one enforcement mode defined in this section.
All enforcement modes MUST conform to the general
enforcement semantics defined in Section 5.

---

## 6.1 Enforcement Mode Selection

The enforcement mode determines how ASE transforms
an inadmissible proposed step into an admissible effective step.

The enforcement mode:

- MUST be selected explicitly by the integrator,
- MUST remain fixed for the lifetime of an ASE instance,
- MUST NOT be changed dynamically based on runtime conditions.

Automatic or adaptive mode selection is strictly prohibited.

---

## 6.2 Reject Mode

### Definition

In **Reject mode**, ASE enforces admissibility
by discarding inadmissible steps.

### Normative Behavior

Given a state `S` and proposed step `ΔS`:

- If `ΔS` is admissible, ASE MUST return `ΔS`.
- If `ΔS` is inadmissible, ASE MUST return a neutral step.

### Properties

- Maximum safety
- No partial progress under constraint violation
- Simplest and most conservative enforcement mode

Reject mode MUST be deterministic
and MUST terminate in constant time.

---

## 6.3 Scale Mode

### Definition

In **Scale mode**, ASE enforces admissibility
by reducing the magnitude of a proposed step
until an admissible step is obtained.

### Normative Behavior

Given a state `S` and proposed step `ΔS`:

- ASE MUST attempt to find a scaling factor `k` such that
  the scaled step `ΔS_k` is admissible.
- Scaling MUST begin from `k = 1.0`
  and proceed monotonically toward zero.
- If an admissible scaled step is found,
  ASE MUST return it as the effective step.
- If no admissible scaled step is found within bounded attempts,
  ASE MUST return a neutral step.

### Requirements

- Scaling MUST be deterministic.
- Scaling MUST be monotonic with respect to `k`.
- Scaling MUST NOT change step direction semantics
  beyond domain-defined scaling behavior.
- Termination MUST be guaranteed by explicit bounds.

---

## 6.4 Project Mode

### Definition

In **Project mode**, ASE enforces admissibility
by replacing an inadmissible step
with a nearest admissible step
according to a domain-defined projection rule.

### Normative Behavior

Given a state `S` and proposed step `ΔS`:

- If `ΔS` is admissible, ASE MUST return `ΔS`.
- If `ΔS` is inadmissible, ASE MUST invoke projection logic
  to obtain a projected step `ΔS_p`.
- If the projected step is admissible,
  ASE MUST return `ΔS_p`.
- If the projected step is inadmissible,
  ASE MUST return a neutral step.

### Requirements

- Projection MUST be deterministic.
- Projection MUST depend only on `(S, ΔS)`.
- Projection MUST NOT introduce hidden state or adaptation.
- Projection MUST be applied at most once per step.

---

## 6.5 Prohibited Enforcement Behaviors

In all enforcement modes, ASE MUST NOT:

- increase step magnitude,
- introduce non-deterministic transformations,
- perform multiple enforcement passes,
- escalate inadmissible steps into more aggressive ones.

All enforcement modes MUST preserve the fail-closed policy.

---

## 6.6 Enforcement Modes Summary

The enforcement modes defined in this section
provide controlled mechanisms for handling inadmissible steps.

Regardless of the mode selected:

- inadmissible steps are never executed,
- enforcement is deterministic and bounded,
- admissibility guarantees are preserved.

---

## 7. Admissibility Evaluation Requirements

This section defines the **mandatory requirements**
for admissibility evaluation used by the Admissible Step Engine (ASE).

All admissibility evaluation logic used by ASE
MUST conform to the requirements in this section.

---

## 7.1 Locality Requirement

Admissibility evaluation MUST be **local**.

For a given state `S` and proposed step `ΔS`:

- admissibility MUST depend only on `(S, ΔS)`,
- admissibility MUST NOT depend on past states,
- admissibility MUST NOT depend on future predictions,
- admissibility MUST NOT depend on iteration indices or counters.

Any admissibility logic that violates locality
is non-compliant.

---

## 7.2 Determinism Requirement

Admissibility evaluation MUST be **deterministic**.

Given identical inputs `(S, ΔS)`,
the admissibility decision MUST be identical.

Admissibility evaluation MUST NOT:
- use randomness,
- depend on wall-clock time,
- depend on system load,
- depend on external I/O.

---

## 7.3 Statelessness Requirement

Admissibility evaluation MUST be **stateless**.

Specifically:
- no historical accumulation,
- no internal memory,
- no adaptive thresholds,
- no learning or tuning.

Any form of stateful admissibility evaluation
is strictly prohibited.

---

## 7.4 Prohibited Evaluation Criteria

Admissibility evaluation MUST NOT rely on:

- accumulated error or loss history,
- predicted future trajectories,
- probabilistic or statistical confidence,
- ensemble voting or sampling,
- heuristic thresholds derived from runtime history.

Admissibility evaluation MUST be decidable
in finite time using local information only.

---

## 7.5 Monotonic Safety Condition

Admissibility evaluation MUST enforce
a **monotonic safety condition**.

An admissible step MUST NOT:

- increase structural tension,
- increase deviation beyond defined safe bounds,
- reduce margin to an irrecoverable boundary.

The concrete representation of structural tension
or deviation is domain-specific,
but monotonicity is mandatory.

---

## 7.6 Conservative Interpretation

Admissibility evaluation MUST be **conservative**.

If admissibility cannot be conclusively established,
the step MUST be treated as inadmissible.

Optimistic or speculative admissibility
is strictly prohibited.

---

## 7.7 Consistency Across Enforcement Modes

Admissibility evaluation MUST be consistent
across all enforcement modes.

The same admissibility predicate MUST be used
for:
- initial evaluation of the proposed step,
- evaluation of scaled steps,
- evaluation of projected steps.

Changing admissibility criteria within a single enforcement
cycle is not permitted.

---

## 7.8 Admissibility Evaluation Summary

Admissibility evaluation in ASE is:

- local,
- deterministic,
- stateless,
- conservative,
- monotonic with respect to safety.

These properties ensure that admissibility
can be enforced reliably in safety-critical
and real-time systems.

---

## 8. Numerical Safety Requirements

This section defines the **mandatory numerical safety requirements**
for all Admissible Step Engine (ASE) implementations.

Numerical safety is a **normative requirement** and is essential
for using ASE as a safety-critical enforcement mechanism.

---

## 8.1 Numerical Model Assumptions

ASE implementations MUST operate under a **well-defined numerical model**.

The reference numerical model is:

- deterministic evaluation order,
- IEEE-754 compliant floating-point arithmetic where applicable,
- explicitly documented precision and rounding assumptions.

If a non–IEEE-754 numerical model is used,
it MUST be explicitly documented and justified.

---

## 8.2 NaN and Infinity Handling

ASE MUST treat the presence of NaN or Infinity values
as a **numerical fault**.

Specifically:

- If the proposed step `ΔS` contains NaN or Infinity,
  the step MUST be treated as inadmissible.
- If admissibility evaluation produces NaN or Infinity,
  the step MUST be treated as inadmissible.
- If scaling or projection produces NaN or Infinity,
  the resulting step MUST be treated as inadmissible.

In all such cases, ASE MUST apply fail-closed behavior
and return a neutral step.

---

## 8.3 Overflow and Underflow Handling

ASE implementations MUST guard against:

- arithmetic overflow during enforcement,
- underflow resulting in loss of meaningful resolution,
- catastrophic cancellation near admissibility boundaries.

If overflow or severe numerical instability is detected
during enforcement:

- the step MUST be treated as inadmissible,
- enforcement MUST terminate deterministically.

Silent propagation of numerical faults is prohibited.

---

## 8.4 Comparison Tolerances

Floating-point comparisons used in admissibility evaluation
MUST use explicit tolerances.

Tolerance values:

- MUST be explicitly defined or configurable,
- MUST be deterministic,
- MUST NOT be adapted dynamically.

Implicit or environment-dependent tolerances
are not permitted.

---

## 8.5 Scaling Stability Requirements

In Scale enforcement mode:

- scaling factors MUST be applied monotonically,
- scaling MUST NOT amplify numerical noise,
- scaling MUST NOT introduce oscillatory behavior.

If scaling leads to numerical instability,
the scaled step MUST be treated as inadmissible.

---

## 8.6 Projection Stability Requirements

In Project enforcement mode:

- projection MUST NOT rely on iterative or unstable solvers
  unless explicitly bounded and deterministic,
- projection MUST NOT introduce numerical amplification.

If projection produces numerically unstable results,
the projected step MUST be rejected.

---

## 8.7 Numerical Fault Classification

Numerical faults include, but are not limited to:

- NaN or Infinity values,
- overflow or underflow,
- loss of determinism due to precision issues.

All numerical faults MUST result in
inadmissibility of the affected step.

---

## 8.8 Numerical Safety Summary

ASE numerical safety guarantees ensure that:

- numerical faults cannot lead to execution of inadmissible steps,
- all numerical anomalies are handled conservatively,
- enforcement behavior remains deterministic and bounded.

Numerical safety is an integral part
of ASE admissibility enforcement.

---

## 9. Determinism and Boundedness

This section defines the **mandatory determinism and boundedness guarantees**
for the Admissible Step Engine (ASE).

All ASE-compliant implementations MUST satisfy
the requirements in this section.

---

## 9.1 Determinism Contract

ASE MUST operate under a **strong determinism contract**.

Given:
- identical current state `S`,
- identical proposed step `ΔS`,
- identical configuration parameters,
- identical admissibility and enforcement functions,

ASE MUST produce an identical effective step `ΔS'`.

Determinism MUST hold across:
- repeated invocations,
- different execution environments with equivalent numerical models,
- all supported enforcement modes.

---

## 9.2 Sources of Non-Determinism (Prohibited)

ASE MUST NOT depend on:

- random number generators,
- wall-clock time or timers,
- system load or scheduling,
- thread interleaving effects,
- external I/O or environment variables.

Any dependence on such sources
renders an implementation non-compliant.

---

## 9.3 Bounded Execution Time

ASE enforcement MUST complete in **bounded time**.

Specifically:

- all loops MUST have explicit, finite bounds,
- recursion is strictly prohibited,
- enforcement MUST terminate regardless of input values.

Data-dependent unbounded execution
is not permitted.

---

## 9.4 Bounded Memory Usage

ASE MUST operate with **bounded memory usage**.

Specifically:

- no unbounded memory allocation,
- no accumulation of data across invocations,
- no growth proportional to iteration count.

In safety-critical profiles,
dynamic allocation MUST be avoided entirely.

---

## 9.5 Termination Guarantees

ASE MUST guarantee termination
for all possible inputs `(S, ΔS)`.

If enforcement cannot produce
an admissible effective step within bounds:

- the step MUST be treated as inadmissible,
- a neutral step MUST be returned.

Non-termination is a specification violation.

---

## 9.6 Fail-Closed Enforcement under Uncertainty

If determinism or boundedness
cannot be guaranteed for a given input:

- the proposed step MUST be treated as inadmissible,
- ASE MUST return a neutral step.

Fail-open behavior under uncertainty
is strictly prohibited.

---

## 9.7 Determinism and Boundedness Summary

This section ensures that ASE:

- behaves predictably and reproducibly,
- cannot hang or diverge,
- remains suitable for real-time and safety-critical use.

Determinism and boundedness
are fundamental to ASE compliance.

---

## 10. Neutral Step Requirements

This section defines the **mandatory requirements**
for the neutral (no-op) step used by the Admissible Step Engine (ASE).

The neutral step is a critical safety element
and MUST satisfy all requirements defined herein.

---

## 10.1 Definition of Neutral Step

A **neutral step** is an admissible step that,
when applied to a state, leaves the state unchanged.

Formally, for any state `S`:

    apply_step(S, neutral_step) = S

The neutral step represents the **safe fallback outcome**
of enforcement when no admissible modification of a proposed step exists.

---

## 10.2 Mandatory Properties

A neutral step MUST satisfy all of the following properties:

- **Admissibility**  
  The neutral step MUST be admissible by definition.

- **Determinism**  
  The neutral step MUST be deterministic.

- **Idempotence**  
  Applying the neutral step multiple times MUST have no cumulative effect.

- **Side-Effect Freedom**  
  The neutral step MUST NOT produce side effects of any kind.

- **State Preservation**  
  The neutral step MUST preserve the state exactly,
  including all representational and numerical properties.

---

## 10.3 Responsibility for Definition

The definition and construction of the neutral step
is the responsibility of the **integrator**.

ASE MUST NOT:
- attempt to infer a neutral step,
- synthesize a neutral step automatically,
- assume the existence of a default neutral step.

An ASE implementation MUST require
the integrator to provide a neutral step explicitly.

---

## 10.4 Neutral Step Usage Conditions

ASE MUST use the neutral step in the following cases:

- the proposed step is inadmissible and cannot be modified,
- enforcement fails to produce an admissible step,
- numerical faults are detected during enforcement,
- determinism or boundedness cannot be guaranteed.

Under no circumstances may ASE return an inadmissible step
in place of a neutral step.

---

## 10.5 Prohibited Neutral Step Behaviors

A neutral step MUST NOT:

- modify the state,
- trigger external actions,
- allocate resources,
- perform I/O,
- depend on runtime context.

Any step violating these constraints
is not a valid neutral step.

---

## 10.6 Neutral Step Summary

The neutral step provides the **fail-safe termination**
of the enforcement process.

It ensures that, even under failure or uncertainty,
ASE preserves system integrity by preventing
inadmissible state transitions.

---

## 11. Error and Failure Handling

This section defines the **mandatory error and failure handling rules**
for the Admissible Step Engine (ASE).

All ASE-compliant implementations MUST adhere
to the fail-safe behaviors defined herein.

---

## 11.1 Failure Classification

Failures encountered during ASE operation are classified as follows:

- **Admissibility Failure**  
  The proposed step is determined to be inadmissible.

- **Numerical Failure**  
  Numerical faults occur during evaluation or enforcement
  (including NaN, Infinity, overflow, or severe instability).

- **Evaluation Failure**  
  Admissibility evaluation cannot be completed deterministically
  or within bounded limits.

- **Enforcement Failure**  
  Enforcement mechanisms fail to produce an admissible step
  within the specified bounds.

---

## 11.2 Mandatory Fail-Closed Behavior

ASE MUST operate under a **fail-closed policy**.

For any failure classified in Section 11.1:

- the proposed step MUST be treated as inadmissible,
- ASE MUST return a neutral step,
- no inadmissible step MAY be executed.

Fail-open behavior is strictly prohibited.

---

## 11.3 Exception Handling Policy

ASE implementations MUST NOT allow exceptions
to escape the enforcement boundary.

In environments where exceptions are used internally:

- all exceptions MUST be caught within ASE,
- any exception MUST result in fail-closed behavior,
- the effective step MUST be a neutral step.

In safety-critical profiles,
the use of exceptions is prohibited entirely.

---

## 11.4 Error Signaling

ASE MAY provide error signaling mechanisms such as:

- explicit status codes,
- boolean success/failure indicators.

Any error signaling mechanism MUST:

- be deterministic,
- not alter enforcement semantics,
- not expose internal state.

Error signaling MUST NOT replace
mandatory fail-closed enforcement behavior.

---

## 11.5 Prohibited Failure Responses

ASE MUST NOT respond to failures by:

- aborting program execution,
- terminating the iteration loop,
- performing rollback operations,
- modifying global system state,
- performing logging or I/O as part of enforcement.

ASE failure handling is local to the current step.

---

## 11.6 Failure Handling Summary

ASE failure handling guarantees that:

- failures cannot result in inadmissible steps being executed,
- enforcement remains deterministic and bounded,
- system integrity is preserved under all failure conditions.

Failure handling is an integral part
of ASE admissibility enforcement.

---

## 12. Prohibited Behaviors

This section defines **explicitly prohibited behaviors**
for any Admissible Step Engine (ASE) implementation.

Any implementation exhibiting one or more behaviors
listed in this section is **non-compliant**.

---

## 12.1 Hidden State

ASE MUST NOT maintain hidden or persistent state.

Specifically prohibited:

- internal memory across invocations,
- cached admissibility results,
- accumulated statistics or counters,
- adaptive internal parameters.

All ASE behavior MUST be derived solely
from the current inputs `(S, ΔS)` and fixed configuration.

---

## 12.2 Adaptive or Learning Behavior

ASE MUST NOT exhibit adaptive behavior.

Specifically prohibited:

- internal learning mechanisms,
- parameter tuning based on runtime history,
- modification of admissibility criteria over time,
- feedback-driven adjustment of enforcement behavior.

ASE is a static enforcement mechanism.

---

## 12.3 Side Effects

ASE MUST NOT produce side effects.

Specifically prohibited:

- modification of global or static state,
- interaction with external systems,
- triggering callbacks outside enforcement,
- modifying the system state directly.

All observable effects of ASE
are confined to the returned effective step.

---

## 12.4 Input/Output Operations

ASE MUST NOT perform any form of I/O.

Specifically prohibited:

- logging,
- printing to stdout or stderr,
- file system access,
- network communication,
- interaction with operating system services.

ASE enforcement MUST be computation-only.

---

## 12.5 Time and Environment Dependence

ASE MUST NOT depend on:

- system time or clocks,
- timers or delays,
- environment variables,
- system configuration outside explicit parameters.

Any such dependency violates determinism requirements.

---

## 12.6 Escalation and Amplification

ASE MUST NOT:

- increase step magnitude,
- amplify numerical noise,
- transform a step into a more aggressive one,
- move a step closer to an irrecoverable boundary.

All enforcement transformations MUST be non-expansive.

---

## 12.7 Multi-Step Reasoning

ASE MUST NOT:

- consider sequences of steps,
- reason about cumulative effects across iterations,
- perform multi-step optimization or planning.

ASE enforcement is strictly **single-step**.

---

## 12.8 Prohibited Behaviors Summary

This section establishes a strict negative boundary
around ASE behavior.

Anything not explicitly permitted by this Specification
and not required for enforcement
is prohibited.

---

## 13. Conformance Definition

This section defines what it means for an implementation
to be considered **ASE-compliant**.

Conformance is defined strictly in terms of
**observable behavior**, not internal structure.

---

## 13.1 Definition of ASE Compliance

An implementation is **ASE-compliant** if and only if it:

- satisfies all requirements marked **MUST** and **MUST NOT**
  in this Specification,
- enforces admissibility for every proposed step,
- never permits execution of an inadmissible step,
- adheres to determinism, boundedness, and safety requirements.

Partial compliance is not permitted.

---

## 13.2 Normative Precedence

In case of conflict between documents:

1. This Specification takes precedence.
2. The ASE Design Document MUST conform to this Specification.
3. The ASE Implementation Notes MUST conform to both.

No implementation may claim ASE compliance
by referencing Design or Implementation documents
in contradiction to this Specification.

---

## 13.3 Mandatory Features

An ASE-compliant implementation MUST provide:

- step admissibility enforcement,
- at least one enforcement mode defined in Section 6,
- fail-closed behavior under all failure conditions,
- neutral step substitution,
- deterministic and bounded execution.

Omission of any mandatory feature
constitutes non-compliance.

---

## 13.4 Optional Features

Optional features MAY be implemented provided that:

- they do not alter normative behavior,
- they do not weaken safety guarantees,
- they do not introduce prohibited behaviors.

Optional features MUST be clearly documented
and MUST NOT be required for core operation.

---

## 13.5 Non-Compliant Usage

An implementation MUST NOT claim ASE compliance if it:

- bypasses enforcement under any condition,
- allows inadmissible steps to be executed,
- violates determinism or boundedness,
- introduces hidden state or side effects.

Experimental or research usage
MUST NOT be labeled as ASE-compliant.

---

## 13.6 Conformance Summary

ASE compliance is a **binary property**:
an implementation either complies fully
with this Specification or it does not.

Compliance guarantees that ASE enforces
admissible step execution exactly as defined herein.

---

## 14. Compliance and Verification (Normative)

This section defines the **mandatory verification requirements**
for demonstrating compliance with the Admissible Step Engine (ASE)
Specification v1.1.

Verification is an integral part of ASE compliance.

---

## 14.1 Verification Obligation

Any implementation claiming ASE compliance MUST be verifiable.

Verification MUST demonstrate that:

- all mandatory requirements of this Specification are satisfied,
- inadmissible steps are never executed,
- enforcement behavior is deterministic and bounded,
- fail-closed behavior is correctly implemented.

Verification results MUST be reproducible.

---

## 14.2 Minimal Test Requirements

An ASE-compliant implementation MUST provide
a minimal test suite covering:

- admissible step pass-through,
- inadmissible step rejection,
- neutral step substitution,
- each supported enforcement mode,
- NaN and Infinity handling,
- bounded termination under worst-case inputs,
- determinism across repeated executions.

Tests MUST be deterministic
and MUST NOT depend on external resources.

---

## 14.3 Static Analysis Requirements

An ASE-compliant implementation MUST undergo static analysis
sufficient to demonstrate:

- absence of undefined behavior,
- bounded memory usage,
- absence of unbounded loops or recursion,
- absence of hidden mutable global state.

Static analysis results MUST be documented.

---

## 14.4 Determinism Verification

Verification MUST include explicit checks that:

- identical inputs produce identical outputs,
- enforcement results do not vary across executions,
- numerical behavior is stable within documented tolerances.

Non-deterministic behavior
constitutes non-compliance.

---

## 14.5 Compliance Matrix

An ASE-compliant implementation MUST provide
a **Compliance Matrix** mapping:

- each normative requirement in this Specification
- to a verification method (test, analysis, inspection),
- with a clear compliance status.

The Compliance Matrix MUST be kept in sync
with the implementation version.

---

## 14.6 Verification Summary

Compliance with this Specification is established
only through explicit verification.

Verification ensures that ASE implementations
are not only conceptually correct,
but operationally safe and reliable.

---

## 15. Profiles (Informative but Referenced)

This section defines **implementation profiles**
for the Admissible Step Engine (ASE).

This section is **informative**, but profiles referenced here
MAY be used in compliance declarations and verification documents.

---

## 15.1 Profile P0 — Safety-Critical / Embedded

Profile P0 is intended for safety-critical
and resource-constrained environments.

Characteristics:

- no dynamic memory allocation,
- no exceptions,
- no recursion,
- strict bounded execution,
- strict numerical determinism,
- no standard I/O or system calls.

Implementations claiming Profile P0
MUST explicitly document compliance with these constraints.

---

## 15.2 Profile P1 — General-Purpose Software

Profile P1 is intended for general-purpose software systems.

Characteristics:

- deterministic standard library usage permitted,
- dynamic allocation MAY occur in user-provided logic,
- exceptions MAY be used internally but MUST NOT escape enforcement,
- bounded execution remains mandatory.

Profile P1 MUST NOT weaken
any normative requirements of this Specification.

---

## 15.3 Non-Compliant Experimental Usage

Experimental or research implementations:

- MAY relax certain constraints for exploration,
- MUST NOT claim ASE compliance,
- MUST NOT be presented as production-ready ASE.

Such usage is explicitly non-compliant.

---

## 15.4 Profiles Summary

Profiles provide **implementation guidance**,
not alternative semantics.

All profiles MUST preserve:
- admissibility enforcement,
- determinism,
- fail-closed behavior.

---

## 16. Relationship to Other Documents

This Specification defines the **normative behavior**
of the Admissible Step Engine (ASE).

Related documents:

- ASE Design Document v1.0  
  Defines architecture and structure.
- ASE Implementation Notes v1.0  
  Defines implementation-level realization.

In case of conflict:

1. This Specification takes precedence.
2. Design Document MUST conform to this Specification.
3. Implementation Notes MUST conform to both.

No lower-level document may weaken
or reinterpret requirements defined here.

---

## 17. Specification Summary

This Specification defines the Admissible Step Engine (ASE)
as a **deterministic, stateless, fail-closed enforcement mechanism**
for iterative systems.

ASE guarantees that:

- inadmissible steps are never executed,
- enforcement is local, deterministic, and bounded,
- numerical and operational failures are handled safely,
- system integrity is preserved under all conditions.

ASE does not optimize, predict, diagnose, or recover.
It enforces admissibility.

This concludes the **ASE Specification v1.1**.

# ASE-Integration-Constraints-V1.1-EN
## Admissible Step Engine — Integration Constraints

### Status
Draft

### Scope
This document defines mandatory integration constraints for embedding ASE into any host system.
It specifies boundary rules, forbidden bypasses, and required call semantics.
Implementation details and testing are out of scope.

---

## Section 1 — Purpose and Integration Role

### 1.1 Purpose

ASE integration constraints exist to ensure that ASE remains:
- non-bypassable,
- non-overridable,
- stateless,
- deterministic,
- and free of side effects.

Integration MUST preserve all guarantees defined by:
- ASE Specification,
- ASE Theoretical Validation,
- ASE Static Analysis Requirements.

---

### 1.2 Role in System Evolution

ASE SHALL be positioned as a mandatory gate between:
- step proposal, and
- state update execution.

The only compliant evolution form is:

Sₜ₊₁ = Sₜ ⊕ ASE(Sₜ, ΔS)

Any evolution path not passing through ASE is non-compliant.

---

## Section 2 — Call Semantics and API Boundary

### 2.1 Pure-Function Boundary

ASE MUST be invoked as a pure function of:
- current state S,
- proposed step ΔS.

ASE MUST return only:
- an effective step ΔS′.

ASE MUST NOT return:
- error codes,
- diagnostics,
- status flags,
- reason codes,
- timing markers.

---

### 2.2 Single-Call Enforcement

For each proposed step ΔS, the host system MUST:
- call ASE exactly once,
- apply exactly the returned ΔS′,
- and perform no additional modification to ΔS′.

Double-enforcement, retry loops, or adaptive re-calls are forbidden.

---

### 2.3 Deterministic Input Representation

The host system MUST provide S and ΔS to ASE in a deterministic representation.
Serialization, normalization, and canonicalization MUST be stable.

Non-canonical or lossy representations invalidate determinism.

---

## Section 3 — Non-Bypassability Requirements

### 3.1 No Bypass Paths

The host system MUST NOT provide any code path that applies ΔS
without passing through ASE.

Prohibited bypass patterns include:
- debug modes,
- emergency modes,
- performance modes,
- conditional disabling,
- feature flags,
- user overrides,
- fallback controllers.

If such a path exists, the system is non-compliant.

---

### 3.2 No Override of ASE Output

The host system MUST NOT override ASE output ΔS′.

Forbidden actions include:
- modifying ΔS′ post-return,
- clamping ΔS′ externally to different rules,
- merging ΔS′ with another step,
- partially applying ΔS′.

The applied step MUST equal the returned step.

---

## Section 4 — Isolation and Side-Effect Prohibition

### 4.1 No I/O Through ASE Boundary

Integration MUST ensure that ASE performs no:
- file I/O,
- network I/O,
- logging,
- telemetry emission,
- IPC,
- shared-memory signaling.

Any observability is external to ASE.

---

### 4.2 No Hidden State Injection

The host system MUST NOT introduce hidden state into ASE via:
- global variables,
- static caches,
- singleton references,
- thread-local storage,
- implicit context objects.

All inputs to ASE MUST be explicit arguments only.

---

### 4.3 No Timing Channels

The host system MUST NOT use ASE execution timing as a signal.

In particular:
- do not branch on time(ASE),
- do not measure time for policy decisions,
- do not expose ASE latency externally.

Timing observability is treated as an information leak.

---

## Section 5 — Concurrency and Scheduling Constraints

### 5.1 Single-Thread Determinism

If ASE is invoked concurrently, the host system MUST guarantee:
- deterministic scheduling,
- no shared mutable state,
- deterministic results independent of invocation order.

If determinism cannot be proven under concurrency,
ASE MUST be executed single-threaded.

---

### 5.2 Reentrancy and Thread Safety

ASE MUST be integrated as reentrant and thread-safe only if:
- proven stateless by construction,
- and proven free of shared mutable resources.

Otherwise, reentrancy assumptions are prohibited.

---

## Section 6 — Data Access Constraints

### 6.1 Minimal Visibility Principle

ASE MUST be given the minimal required subset of state S
needed for admissibility evaluation.

The host system MUST NOT provide:
- policy objectives,
- reward signals,
- training data,
- external context,
- user identifiers,
- long history buffers.

ASE input is structural only.

---

### 6.2 No Feedback from Outcomes

The host system MUST NOT feed back:
- success/failure outcomes,
- post-state metrics,
- boundary proximity,
- performance signals

into ASE across invocations.

ASE remains memoryless and outcome-independent.

---

## Section 7 — Integration Failure Handling

### 7.1 Fail-Closed Host Rule

If ASE invocation fails at the host boundary for any reason,
the host system MUST behave fail-closed:

Applied step MUST be the neutral step ΔS₀.

The host MUST NOT:
- apply the original ΔS,
- attempt alternative policies that bypass ASE,
- switch to a non-ASE mode.

---

### 7.2 Deterministic Fallback

Any fallback behavior MUST be:
- deterministic,
- stateless,
- and equivalent to neutral enforcement.

Non-deterministic fallback invalidates compliance.

---

## Section 8 — Integration Compliance Obligations

Integration is compliant only if all are true:
- ASE is invoked on every proposed step,
- no bypass or override exists,
- applied step equals returned step,
- ASE remains isolated and side-effect free,
- host fail behavior is fail-closed,
- determinism is preserved end-to-end.

Violation of any obligation invalidates all ASE guarantees.

---

## Section 9 — Structural Consequence

Correct integration ensures that ASE functions as a non-negotiable
structural boundary law inside the host system.

ASE cannot be weakened by policy, convenience, or runtime conditions.
Structural admissibility becomes mandatory at execution time.